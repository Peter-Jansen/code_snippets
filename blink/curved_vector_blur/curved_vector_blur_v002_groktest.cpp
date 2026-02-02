kernel curved_vector_blur : ImageComputationKernel<ePixelWise> {

  Image<eRead, eAccessPoint, eEdgeClamped> src; 
  Image<eRead, eAccessPoint, eEdgeClamped> vec; 
  Image<eWrite, eAccessRandom, eEdgeClamped> dst;


param:
float _shutter;
float _quality;


local:

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

  void init() {

  }


  void process(int2 pos) {

    float2 forward = float2(vec().x, vec().y);
    float2 backward = float2(vec().z, vec().w);
    float blur_length = length(forward) + length(backward);
    int steps = ceil(blur_length * _quality);
    float steps_div = 1.0f/float(steps);

    float shutter_start = 0.5 - (_shutter*0.25);
    float shutter_end = 0.5 + (_shutter*0.25);

    float2 center = float2(pos.x, pos.y);
    float2 start = center + forward;
    float2 end = center + backward;

    float x = pos.x;
    float y = pos.y;
    for (int step = 0; step <= steps; step++){
        float step_position = float(step)/float(steps);
        float shutter_step = lerp(shutter_start, shutter_end, step_position);
        float2 xy = quadraticInterpolation(start, center, end, shutter_step);
        dst(xy.x, xy.y) += src() / float(steps);
    }

    
    //dst(pos.x, pos.y) = blur_length;

  }
};
