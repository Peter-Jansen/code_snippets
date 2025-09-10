




kernel ParticleRenderKernel : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessRandom> p_position;
  Image<eRead, eAccessRandom> p_color;
  Image<eRead, eAccessRandom> p_size;
  Image<eWrite, eAccessRandom, eEdgeClamped> output;

  param:
    float4x4 _worldToScreen;
    float4x4 camera_matrix;
    float _size;

    float focal;
    float haperture;
    float near;
    float far;
    float pixel_aspect;

  local:
    int _w, _h;
    int _numParticles;
    int _boundX;
    float4x4 w2s;
    float4x4 p;
    float4x4 s;
    float4x4 t;
    float4x4 view;
    float image_aspect;


// BLINKSCRIPT DOESN'T LIKE  ' ' ' USE " " " INSTEAD
  void define() {
    defineParam(_worldToScreen, "_worldToScreen", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f) );
    defineParam(_size, "paSize", 3.0f);
    defineParam(camera_matrix, "camera_matrix", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));

    //camera stuff
    defineParam(focal, "focal", 24.0f);
    defineParam(haperture, "haperture", 24.0f);
    defineParam(near, "near", 0.1f);
    defineParam(far, "far", 1000.0f);
    defineParam(pixel_aspect, "pixel_aspect", 1.0f);
  }

  float4 srcOver( float4 a, float4 b ) {
    return (1.0f-a.w)*b + a;
  }

  float2 transform( float3 p )
  {
    float4 r = _worldToScreen*float4(p.x, p.y, p.z, 1.0);
    return float2(r.x, r.y)/r.w;
  }

  float smoothstep( float a, float b, float x ) {
    float t = clamp((x - a) / (b - a), 0.0, 1.0);
    return t*t * (3.0f - 2.0f*t);
  }

  void init()
  {
    _numParticles = p_position.bounds.height();
    _boundX = p_position.bounds.width();
    _w = output.bounds.width();
    _h = output.bounds.height();
    image_aspect = float(_h) / float(_w);

    p = projectionMatrix(focal, haperture, near, far);
    float x_scale = float(_w) * 0.5;
    float y_scale = x_scale *pixel_aspect;

    // Matrix to scale normalised pixel coords into actual pixel coords.
    s.setIdentity();
    s.scale(float4(x_scale, y_scale, 1.0f,1.0f));


    // Matrix to translate the projected points into normalised pixel coords
    t.setIdentity();
    t.translate(float4(1.0, 1.0 - (1.0 - image_aspect / pixel_aspect), 0.0f, 1.0f));

    view =  camera_matrix.invert();
    w2s = s * t * p * view;

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
          output(p.x, p.y) += pcolor*t;
        }
      }
    }
  }


  void process(int2 pos) {


      output(pos.x, pos.y) = float4(p_position(pos.x, pos.y),0.0f);


    if ( pos.y == _numParticles-1 ) {



      for(int x= 800; x < 900; x++){
         for(int y= 800; y < 900; y++){
          float3 var = float3(_numParticles);
          output(x,y) = float4(var,0.0f);
          }
        }





float4x4 m4 = _worldToScreen;

 output(701, 640) = m4[0][0];
 output(702, 640) = m4[0][1];
 output(703, 640) = m4[0][2];
 output(704, 640) = m4[0][3];
 
 output(701, 639) = m4[1][0];
 output(702, 639) = m4[1][1];
 output(703, 639) = m4[1][2];
 output(704, 639) = m4[1][3];
 
 output(701, 638) = m4[2][0];
 output(702, 638) = m4[2][1];
 output(703, 638) = m4[2][2];
 output(704, 638) = m4[2][3];
     
 output(701, 637) = m4[3][0];
 output(702, 637) = m4[3][1];
 output(703, 637) = m4[3][2];
 output(704, 637) = m4[3][3];


 float4x4 m9 = w2s;

 output(741, 630) = m9[0][0];
 output(742, 630) = m9[0][1];
 output(743, 630) = m9[0][2];
 output(744, 630) = m9[0][3];
      
 output(741, 629) = m9[1][0];
 output(742, 629) = m9[1][1];
 output(743, 629) = m9[1][2];
 output(744, 629) = m9[1][3];
      
 output(741, 628) = m9[2][0];
 output(742, 628) = m9[2][1];
 output(743, 628) = m9[2][2];
 output(744, 628) = m9[2][3];
      
 output(741, 627) = m9[3][0];
 output(742, 627) = m9[3][1];
 output(743, 627) = m9[3][2];
 output(744, 627) = m9[3][3];

}


  }
};