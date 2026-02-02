kernel curved_vector_blur : ImageComputationKernel<ePixelWise> {

  Image<eRead, eAccessPoint, eEdgeClamped> src; 
  Image<eRead, eAccessPoint, eEdgeClamped> vec; 
  Image<eWrite, eAccessRandom, eEdgeClamped> dst;


param:
float _shutter;
float _quality;
int _minSteps;
float _splatSize;


local:
    int _w, _h;
    float xlerp(float a, float b, float t){
        return a + t * (b-a);
  }
    float2 xlerp(float2 a, float2 b, float t) {
        return a + t * (b - a);
  }
    float3 xlerp(float3 a, float3 b, float t) {
        return a + t * (b - a);
    }

    float2 quadraticInterpolation(float2 p0, float2 p1, float2 p2, float t) {
    // Lagrange basis polynomials
    float l0 = (t - 0.5f) * (t - 1.0f) / ((0.0f - 0.5f) * (0.0f - 1.0f));
    float l1 = (t - 0.0f) * (t - 1.0f) / ((0.5f - 0.0f) * (0.5f - 1.0f));
    float l2 = (t - 0.0f) * (t - 0.5f) / ((1.0f - 0.0f) * (1.0f - 0.5f));

    // Interpolated point
    return l0 * p0 + l1 * p1 + l2 * p2;
    }

   float smoothstep( float a, float b, float x ) {
    float t = clamp((x - a) / (b - a), 0.0, 1.0);
    return t*t * (3.0f - 2.0f*t);
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


  void init() {
    _w = src.bounds.width();
    _h = src.bounds.height();
  }


  void process(int2 pos) {

    float2 forward = float2(vec().x, vec().y);
    float2 backward = float2(vec().z, vec().w);
    float blur_length = length(forward) + length(backward);
    int steps =  min(1024, max(_minSteps, ceil(blur_length * _shutter * _quality)));
    // steps = 6;
    float inv_steps = 1.0f/float(steps);

    float shutter_start = 0.5 - (_shutter*0.25);
    float shutter_end = 0.5 + (_shutter*0.25);

    float2 center = float2(pos.x, pos.y);
    float2 start = center + forward;
    float2 end = center + backward;

    float x = pos.x;
    float y = pos.y;
    
    for (int step = 0; step < steps; step++){
        float step_position = float(step)/float(steps);
        float shutter_step = xlerp(shutter_start, shutter_end, step_position);
        float2 xy = quadraticInterpolation(start, center, end, shutter_step);
        // renderPoint(xy, src()* inv_steps, _splatSize);
        // float2 remainder = float2(fmod(xy.x, 1), fmod(xy.y, 1));
        // float remainder = fmod(length(xy),1);
        dst(xy.x, xy.y) += src() *inv_steps ;
        // float3 col = float3(src().x, src().y, src().z) ;
        // dst(xy.x, xy.y) += float4(src().x, src().y, src().z, steps);

    }

    
    //dst(pos.x, pos.y) = blur_length;

  }
};
