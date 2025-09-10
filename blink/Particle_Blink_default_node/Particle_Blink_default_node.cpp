
kernel blink_render : ImageComputationKernel<ePixelWise> {
  Image<eRead, eAccessRandom, eEdgeClamped> src;  // randomly accessing and edge clamping
  Image<eWrite, eAccessRandom, eEdgeClamped> dst;

// Parameters are made available to the user as knobs.
param:
    float _size;

    float4x4 camera_matrix;
    float haperture;
    float focal;
    float pixel_aspect;
    float cam_near;
    float cam_far;

local:

    int _w, _h;
    float image_aspect;
    // projection matrix stuff. This is copying the one nuke provides in nukescripts.snap3d.py
    float4x4 view;      // inverted camera matrix.
    float4x4 p;         // projection matrix
    float4x4 t;         // translate projected points into normalsed pixel coords (from 0,0 to -2,2 instead of -1,-1 to 1,1)
    float4x4 s;         // scale normalised screen coords to actual pixel coords
    float4x4 w2s;       // final world to screen matrix. This matches the one provided by this node (ParticleBlinkScriptRender),
                        // and the projection() function in _nukemath py.
                        // with this we can uhh. Do stuff..?



  void define(){

    defineParam(_size, "paSize", 3.0f);

    defineParam(camera_matrix, "camera_matrix", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f) );
    defineParam(haperture, "haperture", 24.576f);
    defineParam(focal, "focal", 24.0f);
    defineParam(pixel_aspect, "pixel_aspect", 1.0f);
    defineParam(cam_near, "cam_near", 0.1f);
    defineParam(cam_far, "cam_far", 1000.0f);
  }

  float4 srcOver( float4 a, float4 b ) {
    return (1.0f-a.w)*b + a;
  }

  float2 transform( float3 p )
  {
    float4 r = w2s*float4(p.x, p.y, p.z, 1.0);
    return float2(r.x, r.y)/r.w;
  }

  float smoothstep( float a, float b, float x ) {
    float t = clamp((x - a) / (b - a), 0.0, 1.0);
    return t*t * (3.0f - 2.0f*t);
  }
float4x4 projectionMatrix(
        const float focalLength,
        const float horizontalAperture,
        const float nearPlane,
        const float farPlane)
{
    float farMinusNear = farPlane - nearPlane;
    return float4x4(
        2 * focalLength / horizontalAperture, 0, 0, 0,
        0, 2 * focalLength / horizontalAperture, 0, 0,
        0, 0, -(farPlane + nearPlane) / farMinusNear, -2 * (farPlane * nearPlane) / farMinusNear,
        0, 0, -1, 0
    );
}

  void init() {
    _w = dst.bounds.width();
    _h = dst.bounds.height();
    image_aspect = float(_h) / float(_w);
    // camera projection matrix
    p = projectionMatrix(focal, haperture, cam_near, cam_far);

    // Matrix to scale normalised pixel coords into actual pixel coords.
    float x_scale = float(_w) * 0.5;
    float y_scale = x_scale *pixel_aspect;
    s.setIdentity();
    s.scale(float4(x_scale, y_scale, 1.0f,1.0f));

    // Matrix to translate the projected points into normalised pixel coords
    t.setIdentity();
    t.translate(float4(1.0, 1.0 - (1.0 - image_aspect / pixel_aspect), 0.0f, 1.0f));

    // view matrix is inverted camera world matrix
    view =  camera_matrix.invert(); 

    // world to screen matrix is the concatenation of above matrices, from right to left.
    // TO DO: maybe add screen scale/roll all that crap that the snap3d projection matrix provides. Could be useful for overscan..?
    w2s = s * t * p * view;
  }

  void renderPoint( float2 xy, float4 pcolor, float pointSize )
  {
    float sizeSquared = pointSize*pointSize;
    float2 f = xy-floor(xy);
    float2 g = float2(1.0f) - f;
    xy = floor(xy);
    int size = ceil(pointSize);
    int minX = max(0, int(xy.x)-size);
    int maxX = min(_w-1, int(xy.x)+size);
    int minY = max(0, int(xy.y)-size);
    int maxY = min(_h-1, int(xy.y)+size);
    for ( int y = minY; y <= maxY; y++ ) {
      for ( int x = minX ; x <= maxX; x++ ) {
        float2 p = float2(x, y);
        float2 d = p-xy;
        float r2 = dot(d, d);
        if ( r2 < sizeSquared ) {
         float t = 1.0f-smoothstep(0, sizeSquared, r2);
          dst(p.x, p.y) += pcolor*t;
        }
      }
    }
  }

  // The process() function runs over all pixel positions of the output image.
  // The int3 pos parameter is used to indicate positions x, y and the component.
  void process(int2 pos) {
    float4 read_src = src(pos.x, pos.y);
    float3 P = float3(read_src.x, read_src.y, read_src.z);
    float2 xy = transform(P);
    float4 col = float4(1.0, 0.0, 0.0, 1.0);
    renderPoint(xy, col, 5.0f);

    //dst(pos.x,pos.y) = src(pos.x, pos.y);
    }
  };