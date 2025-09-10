
kernel blink_render : ImageComputationKernel<ePixelWise> {
  Image<eRead, eAccessRandom, eEdgeClamped> src;  // randomly accessing and edge clamping
  Image<eRead, eAccessRandom, eEdgeClamped> src_prev;  // randomly accessing and edge clamping
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
    int src_w, src_h;
    int sqSize;
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
    float lerp(float a, float b, float t){
    return a + t * (b-a);
  }
  float2 lerp(float2 a, float2 b, float t) {
     return a + t * (b - a);
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
    src_w = src.bounds.width();
    src_h = src.bounds.height();
    sqSize = src_w / 2;

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

  void process(int2 pos) {
    if (pos.x == 0 && pos.y == 0){
        for (int x = 0; x < sqSize; x++){
            for (int y= 0; y < sqSize; y++){
                float4 read_pos_size = src(x, y);
                float4 read_pos_size_prev = src_prev(x, y);    
                //int colour_pixel_x_offset = src_w * 0.5;
                //float4 read_col = src(colour_pixel_x_offset + pos.x, pos.y);

                float3 P = float3(read_pos_size.x, read_pos_size.y, read_pos_size.z);
                float3 P_prev = float3(read_pos_size_prev.x, read_pos_size_prev.y, read_pos_size_prev.z);
                float size = read_pos_size.w;
                float size_prev = read_pos_size_prev.w;
                //float3 colour = 


                float2 xy = transform(P);

                float2 xy_prev = transform(P_prev);
                float4 col = float4(1.0, 0.0, 0.0, 1.0);
                int steps = 256;
                for (int step = 0; step < steps; step++){
                    float step_position = float(step)/float(steps);
                    float2 xy_render = lerp(xy_prev, xy, step_position);
                    renderPoint(xy_render, col, size);
                }
                

            }
        }


        }
    }
  };