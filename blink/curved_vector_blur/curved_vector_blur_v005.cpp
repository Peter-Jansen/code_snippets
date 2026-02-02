kernel curved_vector_blur : ImageComputationKernel<ePixelWise> {

  Image<eRead, eAccessRandom, eEdgeClamped> src; 
  Image<eRead, eAccessPoint, eEdgeClamped> vec; 
  Image<eWrite, eAccessPoint, eEdgeClamped> dst;


param:
float _shutter;
float _quality;
int _minSteps;



local:
    float xlerp(float a, float b, float t){
        return a + t * (b-a);
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




  void init() {

  }


  void process(int2 pos) {
    dst() = float4(0.0f);
    // float4 s = src(pos.x,pos.y);
    // if (s.x + s.y + s.z + s.z + s.w == 0.0f) return;
    float2 forward = float2(vec().x, vec().y);
    float2 backward = float2(vec().z, vec().w);
    float blur_length = length(forward) + length(backward);
    int steps =  min(1024, max(_minSteps, ceil(blur_length * _shutter * _quality)));
    float inv_steps = 1.0f/float(steps);

    float shutter_start = 0.5 - (_shutter*0.25);
    float shutter_end = 0.5 + (_shutter*0.25);

    float2 center = float2(pos.x, pos.y);
    float2 start = center + forward;
    float2 end = center + backward;
    float2 fpos = float2(pos.x, pos.y);

    for (int step = 0; step < steps; step++){
        float step_position = float(step)/float(steps);
        float shutter_step = xlerp(shutter_start, shutter_end, step_position);
        float2 xy = quadraticInterpolation(start, center, end, shutter_step);
        float2 diff = xy - fpos;
        xy = fpos - diff;
        // dst(xy.x, xy.y) += src() *inv_steps ;
        dst() += bilinear(src, xy.x, xy.y) *inv_steps ;


    }



  }
};
