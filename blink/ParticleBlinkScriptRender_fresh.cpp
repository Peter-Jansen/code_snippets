kernel ParticleRenderKernel : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessPoint> p_position;
  Image<eRead, eAccessPoint> p_color;
  Image<eRead, eAccessPoint> p_size;
  Image<eWrite, eAccessRandom> output;

  param:
    float4x4 _worldToScreen;
    float _size;

  local:
    int _w, _h;

  void define() {
    defineParam(_worldToScreen, "_worldToScreen", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f) );
    defineParam(_size, "paSize", 3.0f);
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

  void process() {
    float2 xy = transform(p_position());
    float4 pcolor = p_color();
    renderPoint(xy, pcolor, _size*p_size().x);
  }
};
