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

  inline float4 taylorInvSqrt(float4 r) {
    return float4(1.79284291400159f) - 0.447213595499958f * r; 
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
  // 4D Noise (Cellular/Value hybrid for stability in Blink)
  float noise4D(float4 p) {
    float4 Pi0 = floor(p);
    float4 Pi1 = Pi0 + float4(1.0f);
    Pi0 = mod(Pi0, 289.0f);
    Pi1 = mod(Pi1, 289.0f);
    
    float4 Pf0 = p - floor(p);
    float4 Pf1 = Pf0 - float4(1.0f);
    
    // Manual swizzling/Permutations
    float4 ix = float4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
    float4 iy = float4(Pi0.y, Pi0.y, Pi1.y, Pi1.y);
    float4 iz = float4(Pi0.z, Pi0.z, Pi0.z, Pi0.z);
    float4 iw = float4(Pi0.w, Pi0.w, Pi0.w, Pi0.w);

    float4 ixy = permute(permute(ix) + iy);
    float4 ixyz = permute(permute(ixy) + iz);
    float4 ixyzw = permute(permute(ixyz) + iw);

    // Quintic interpolation curve
    float4 fade4 = Pf0 * Pf0 * Pf0 * (Pf0 * (Pf0 * 6.0f - 15.0f) + 10.0f);
    
    // Mix the results
    float res = dot(ixyzw, float4(0.125f)); // Simplified gradient mix
    return res * 2.0f - 1.0f;
  }



  float noise4DALT(float4 p) {
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
    float ix000 = lerp(n0000, n1000, f.x);
    float ix100 = lerp(n0100, n1100, f.x);
    float ix010 = lerp(n0010, n1010, f.x);
    float ix110 = lerp(n0110, n1110, f.x);
    float iy00 = lerp(ix000, ix100, f.y);
    float iy10 = lerp(ix010, ix110, f.y);
    float iz0 = lerp(iy00, iy10, f.z);

    float ix001 = lerp(n0001, n1001, f.x);
    float ix101 = lerp(n0101, n1101, f.x);
    float ix011 = lerp(n0011, n1011, f.x);
    float ix111 = lerp(n0111, n1111, f.x);
    float iy01 = lerp(ix001, ix101, f.y);
    float iy11 = lerp(ix011, ix111, f.y);
    float iz1 = lerp(iy01, iy11, f.z);

    return lerp(iz0, iz1, f.w);
  }

  // --- THE MULTI-OCTAVE (fBm) FUNCTION ---
  // This is the function you'll call in your process/init loops
  float fBM(float4 p, int octaves, float lacunarity, float gain) {
    float amp = 1.0f;
    float freq = 1.0f;
    float total = 0.0f;
    float maxValue = 0.0f; 
    
    for (int i = 0; i < octaves; ++i) {
      total += noise4DALT(p * freq) * amp;
      maxValue += amp;
      amp *= gain;
      freq *= lacunarity;
    }
    // Normalize to 0-1 or -1 to 1 range
    return total / maxValue;
  }

/// SwirlomaticKernel: Does a nice swirl. Amount is in degrees.
kernel Swirlomatic : ImageComputationKernel<ePixelWise> {
  // Input and output images
  Image<eRead, eAccessPoint, eEdgeClamped> src;  // randomly accessing and edge clamping
  Image<eWrite, eAccessPoint> dst;

// Parameters are made available to the user as knobs.
param:
int octaves;
float lacunarity;
float gain;
// Local variables can be initialised once in init() and used from all pixel positions.
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
