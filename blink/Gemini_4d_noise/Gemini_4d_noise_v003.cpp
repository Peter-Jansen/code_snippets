// Helper: Blink requires manual mod for floats
  inline float mod(float x, float y) { 
    return x - y * floor(x / y); 
  }
  inline float4 mod(float4 x, float y) {
    return float4(mod(x.x, y), mod(x.y, y), mod(x.z, y), mod(x.w, y));
  }

  // Basic hash/permutation
  inline float4 permute(float4 x) {
    float4 res;
    for(int i=0; i<4; ++i) {
      res[i] = mod(((x[i] * 34.0f) + 1.0f) * x[i], 289.0f);
    }
    return res;
  }

inline float hash4D(float4 p) {
    float4 i = mod(p, 289.0f);
    float h = mod(((i.x * 34.0f + 1.0f) * i.x + i.y), 289.0f);
    h = mod(((h * 34.0f + 1.0f) * h + i.z), 289.0f);
    h = mod(((h * 34.0f + 1.0f) * h + i.w), 289.0f);
    return (h / 289.0f) * 2.0f - 1.0f;
  }

// 2. Linear Interpolation helper (Blink sometimes prefers manual lerp)
  inline float xlerp(float a, float b, float t) {
    return a + t * (b - a);
  }


  float noise4D(float4 p) {
    float4 Pi0 = floor(p);
    float4 Pi1 = Pi0 + float4(1.0f);
    
    // Smooth quintic interpolation curve
    float4 Pf0 = p - Pi0;
    float4 f = Pf0 * Pf0 * Pf0 * (Pf0 * (Pf0 * 6.0f - 15.0f) + 10.0f);

    // Front slice (w = Pi0.w)
    float n0000 = hash4D(float4(Pi0.x, Pi0.y, Pi0.z, Pi0.w));
    float n1000 = hash4D(float4(Pi1.x, Pi0.y, Pi0.z, Pi0.w));
    float n0100 = hash4D(float4(Pi0.x, Pi1.y, Pi0.z, Pi0.w));
    float n1100 = hash4D(float4(Pi1.x, Pi1.y, Pi0.z, Pi0.w));
    float n0010 = hash4D(float4(Pi0.x, Pi0.y, Pi1.z, Pi0.w));
    float n1010 = hash4D(float4(Pi1.x, Pi0.y, Pi1.z, Pi0.w));
    float n0110 = hash4D(float4(Pi0.x, Pi1.y, Pi1.z, Pi0.w));
    float n1110 = hash4D(float4(Pi1.x, Pi1.y, Pi1.z, Pi0.w));

    // Back slice (w = Pi1.w)
    float n0001 = hash4D(float4(Pi0.x, Pi0.y, Pi0.z, Pi1.w));
    float n1001 = hash4D(float4(Pi1.x, Pi0.y, Pi0.z, Pi1.w));
    float n0101 = hash4D(float4(Pi0.x, Pi1.y, Pi0.z, Pi1.w));
    float n1101 = hash4D(float4(Pi1.x, Pi1.y, Pi0.z, Pi1.w));
    float n0011 = hash4D(float4(Pi0.x, Pi0.y, Pi1.z, Pi1.w));
    float n1011 = hash4D(float4(Pi1.x, Pi0.y, Pi1.z, Pi1.w));
    float n0111 = hash4D(float4(Pi0.x, Pi1.y, Pi1.z, Pi1.w));
    float n1111 = hash4D(float4(Pi1.x, Pi1.y, Pi1.z, Pi1.w));

    // Quadrilinear Interpolation
    float ix000 = xlerp(n0000, n1000, f.x);
    float ix100 = xlerp(n0100, n1100, f.x);
    float ix010 = xlerp(n0010, n1010, f.x);
    float ix110 = xlerp(n0110, n1110, f.x);
    float iy00 = xlerp(ix000, ix100, f.y);
    float iy10 = xlerp(ix010, ix110, f.y);
    float iz0 = xlerp(iy00, iy10, f.z);

    float ix001 = xlerp(n0001, n1001, f.x);
    float ix101 = xlerp(n0101, n1101, f.x);
    float ix011 = xlerp(n0011, n1011, f.x);
    float ix111 = xlerp(n0111, n1111, f.x);
    float iy01 = xlerp(ix001, ix101, f.y);
    float iy11 = xlerp(ix011, ix111, f.y);
    float iz1 = xlerp(iy01, iy11, f.z);

    return xlerp(iz0, iz1, f.w);
  }


  float fBM(float4 p, int octaves, float lacunarity, float gain) {
    float amp = 1.0f;
    float freq = 1.0f;
    float total = 0.0f;
    float maxValue = 0.0f; 
    
    for (int i = 0; i < octaves; ++i) {
      total += noise4D(p * freq) * amp;
      maxValue += amp;
      amp *= gain;
      freq *= lacunarity;
    }
    // Normalize to 0-1 or -1 to 1 range
    return total / maxValue;
  }


kernel Gemini4D : ImageComputationKernel<ePixelWise> {
  // Input and output images
  Image<eRead, eAccessPoint, eEdgeClamped> src;  // randomly accessing and edge clamping
  Image<eWrite, eAccessPoint> dst;

// Parameters are made available to the user as knobs.
param:
int octaves;
float lacunarity;
float gain;

local:

  void define() {

  }

  void init() {

  }

  void process() {
    float4 in_pos = src();
  dst() = float4(fBM(in_pos, octaves, lacunarity, gain));
    }

};
