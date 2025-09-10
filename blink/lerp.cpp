  // overloaded lerp with float, float2 and float3
    float lerp(float a, float b, float t){
        return a + t * (b-a);
  }
    float2 lerp(float2 a, float2 b, float t) {
        return a + t * (b - a);
  }
    float3 lerp(float3 a, float3 b, float t) {
        return a + t * (b - a);
    }