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
    float2 forward = float2(vec().x, vec().y);
    float2 backward = float2(vec().z, vec().w);
    float blur_length = length(forward) + length(backward);

    float target = blur_length * _shutter * _quality;
    target = max((float)_minSteps, target);

    int steps = (int)floor(target);
    float frac = target - steps;  // 0.0 to 1.0
    int steps_next = min(1024, steps + 1);
    float weight_curr = 1.0f - frac;
    float weight_next = frac;

    // Avoid division by zero
    if (steps == 0) {
        dst() = src(pos.x, pos.y);
        return;
    }

    float inv_steps_curr = weight_curr / steps;
    float inv_steps_next = weight_next / steps_next;

    float shutter_start = 0.5f - _shutter * 0.5f;
    float shutter_end = 0.5f + _shutter * 0.5f;
    float shutter_range = shutter_end - shutter_start;

    float2 center = float2(pos.x, pos.y);
    float2 start = center + forward;
    float2 end = center + backward;

    // === Sample with 'steps' ===
    for (int s = 0; s < steps; ++s) {
        float t = (steps == 1) ? 0.5f : (float(s) + 0.5f) / steps;
        float shutter_step = xlerp(shutter_start, shutter_end, t);
        float2 xy = quadraticInterpolation(start, center, end, shutter_step);
        dst() += bilinear(src, xy.x, xy.y) * inv_steps_curr;
    }

    // === Sample with 'steps_next' (only if weight > 0) ===
    if (weight_next > 0.0f && steps_next <= 1024) {
        for (int s = 0; s < steps_next; ++s) {
            float t = (steps_next == 1) ? 0.5f : (float(s) + 0.5f) / steps_next;
            float shutter_step = xlerp(shutter_start, shutter_end, t);
            float2 xy = quadraticInterpolation(start, center, end, shutter_step);
            dst() += bilinear(src, xy.x, xy.y) * inv_steps_next;
        }
    }
}
};
