kernel DiffusionRays : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessRandom, eEdgeClamped> src;
  Image<eWrite> dst;


  param:
    float scaleFactor;
    int _samples;
    float2 center;

local:
    int samples;

void init(){
    samples = max(1, _samples);
}    

float4 scale(int2 pos, float amount){
        // Calculate the offset from the center point
    float dx = pos.x - center.x;
    float dy = pos.y - center.y;

    // Apply inverse scaling to map the current pixel to the source pixel
    float srcX = center.x + (dx / amount);
    float srcY = center.y + (dy / amount);

    return bilinear(src, srcX, srcY);
}

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

  void process(int2 pos) {
    
    float4 accumulation = float4(0,0,0,0);
    for (int i = 0; i < samples; i++){
        float progression = float(i)/float(samples);
        float scale_step = lerp(1.0, scaleFactor, progression);
        accumulation += scale(pos, scale_step);
    }
    accumulation /= float(samples);
    // float4 scaled_image = scale(pos, scaleFactor);


    dst() = accumulation;

  }
};