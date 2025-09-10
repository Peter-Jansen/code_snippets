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




kernel ParticleRenderKernel : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessRandom> p_position;
  Image<eRead, eAccessRandom> p_color;
  Image<eRead, eAccessRandom> p_size;
  Image<eWrite, eAccessRandom> output;

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

// BLINKSCRIPT DOESN'T LIKE  ' ' ' USE " " " INSTEAD
  void define() {
    defineParam(_worldToScreen, "_worldToScreen", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f) );
    defineParam(_size, "paSize", 3.0f);
    defineParam(camera_matrix, "camera_matrix", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));

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

float4x4 m1 = float4x4( 
 1.0f, 0.0f, 0.0f, 1.0f,
 0.0f, 1.0f, 0.0f, 2.0f,
 0.0f, 0.0f, 1.0f, 3.0f,
 0.0f, 0.0f, 0.0f, 1.0f);

 output(701, 650) = m1[0][0];
 output(702, 650) = m1[0][1];
 output(703, 650) = m1[0][2];
 output(704, 650) = m1[0][3];
 
 output(701, 649) = m1[1][0];
 output(702, 649) = m1[1][1];
 output(703, 649) = m1[1][2];
 output(704, 649) = m1[1][3];
 
 output(701, 648) = m1[2][0];
 output(702, 648) = m1[2][1];
 output(703, 648) = m1[2][2];
 output(704, 648) = m1[2][3];
 
 output(701, 647) = m1[3][0];
 output(702, 647) = m1[3][1];
 output(703, 647) = m1[3][2];
 output(704, 647) = m1[3][3];
 
 
float4x4 m2 = float4x4( 
 1.0f, 0.0f, 0.0f, 0.0f,
 0.0f, 2.0f, 0.0f, 0.0f,
 0.0f, 0.0f, 3.0f, 0.0f,
 0.0f, 0.0f, 0.0f, 1.0f);




 output(711, 650) = m2[0][0];
 output(712, 650) = m2[0][1];
 output(713, 650) = m2[0][2];
 output(714, 650) = m2[0][3];
 
 output(711, 649) = m2[1][0];
 output(712, 649) = m2[1][1];
 output(713, 649) = m2[1][2];
 output(714, 649) = m2[1][3];
 
 output(711, 648) = m2[2][0];
 output(712, 648) = m2[2][1];
 output(713, 648) = m2[2][2];
 output(714, 648) = m2[2][3];
 
 output(711, 647) = m2[3][0];
 output(712, 647) = m2[3][1];
 output(713, 647) = m2[3][2];
 output(714, 647) = m2[3][3];


float4x4 m3 = m1 * m2;

 output(721, 650) = m3[0][0];
 output(722, 650) = m3[0][1];
 output(723, 650) = m3[0][2];
 output(724, 650) = m3[0][3];
 
 output(721, 649) = m3[1][0];
 output(722, 649) = m3[1][1];
 output(723, 649) = m3[1][2];
 output(724, 649) = m3[1][3];
 
 output(721, 648) = m3[2][0];
 output(722, 648) = m3[2][1];
 output(723, 648) = m3[2][2];
 output(724, 648) = m3[2][3];
 
 output(721, 647) = m3[3][0];
 output(722, 647) = m3[3][1];
 output(723, 647) = m3[3][2];
 output(724, 647) = m3[3][3];

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

float4x4 m5 = camera_matrix;

 output(711, 640) = m5[0][0];
 output(712, 640) = m5[0][1];
 output(713, 640) = m5[0][2];
 output(714, 640) = m5[0][3];
      
 output(711, 639) = m5[1][0];
 output(712, 639) = m5[1][1];
 output(713, 639) = m5[1][2];
 output(714, 639) = m5[1][3];
      
 output(711, 638) = m5[2][0];
 output(712, 638) = m5[2][1];
 output(713, 638) = m5[2][2];
 output(714, 638) = m5[2][3];
      
 output(711, 637) = m5[3][0];
 output(712, 637) = m5[3][1];
 output(713, 637) = m5[3][2];
 output(714, 637) = m5[3][3];




float4x4 p = projectionMatrix(focal, haperture, near, far);

 output(711, 630) = p[0][0];
 output(712, 630) = p[0][1];
 output(713, 630) = p[0][2];
 output(714, 630) = p[0][3];
      
 output(711, 629) = p[1][0];
 output(712, 629) = p[1][1];
 output(713, 629) = p[1][2];
 output(714, 629) = p[1][3];
      
 output(711, 628) = p[2][0];
 output(712, 628) = p[2][1];
 output(713, 628) = p[2][2];
 output(714, 628) = p[2][3];
      
 output(711, 627) = p[3][0];
 output(712, 627) = p[3][1];
 output(713, 627) = p[3][2];
 output(714, 627) = p[3][3];



// Matrix to scale normalised pixel coords into actual pixel coords.
float pixel_aspect = 1.0;
float x_scale = float(_w) * 0.5;
float y_scale = x_scale *pixel_aspect;
float4x4 s;
s.setIdentity();
s.scale(float4(x_scale, y_scale, 1.0f,1.0f));

 output(721, 630) = s[0][0];
 output(722, 630) = s[0][1];
 output(723, 630) = s[0][2];
 output(724, 630) = s[0][3];
      
 output(721, 629) = s[1][0];
 output(722, 629) = s[1][1];
 output(723, 629) = s[1][2];
 output(724, 629) = s[1][3];
      
 output(721, 628) = s[2][0];
 output(722, 628) = s[2][1];
 output(723, 628) = s[2][2];
 output(724, 628) = s[2][3];
      
 output(721, 627) = s[3][0];
 output(722, 627) = s[3][1];
 output(723, 627) = s[3][2];
 output(724, 627) = s[3][3];


// Matrix to translate the projected points into normalised pixel coords
float4x4 t;
float aspect = float(_h) / float(_w);
t.setIdentity();
t.translate(float4(1.0, 1.0 - (1.0 - aspect / pixel_aspect), 0.0f, 1.0f));

 output(731, 630) = t[0][0];
 output(732, 630) = t[0][1];
 output(733, 630) = t[0][2];
 output(734, 630) = t[0][3];
      
 output(731, 629) = t[1][0];
 output(732, 629) = t[1][1];
 output(733, 629) = t[1][2];
 output(734, 629) = t[1][3];
      
 output(731, 628) = t[2][0];
 output(732, 628) = t[2][1];
 output(733, 628) = t[2][2];
 output(734, 628) = t[2][3];
      
 output(731, 627) = t[3][0];
 output(732, 627) = t[3][1];
 output(733, 627) = t[3][2];
 output(734, 627) = t[3][3];


 //final matrix
 float4x4 cm = camera_matrix.invert();
 float4x4 m9 = s * t * p * cm;
//float4x4 m9 = p;

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