// 4D Simplex Noise Implementation for BlinkScript
// Improved hash function for better quality

// Better permutation hash function
inline int perm(int i) {
  i = i & 255;
  // Improved hash with better distribution
  i = (i * 1619) & 255;
  i = ((i >> 3) ^ i) * 0xFFFFF;
  i = ((i >> 5) ^ i) & 255;
  return i;
}

// Alternative even better hash - uses prime number mixing
inline int hash_int(int n) {
  n = (n << 13) ^ n;
  return (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
}

// Hash function for 4D with better mixing
inline int hash4d(int i, int j, int k, int l) {
  // Use prime number offsets for better distribution
  return hash_int(i + hash_int(j + hash_int(k + hash_int(l)))) & 255;
}

// 4D gradient function - generates well-distributed gradients
inline void grad4(int hash, float *gx, float *gy, float *gz, float *gw) {
  int h = hash & 31;
  
  // Generate gradient vectors matching the standard simplex gradient set
  int bit0 = h & 1;
  int bit1 = (h >> 1) & 1;
  int bit2 = (h >> 2) & 1;
  int bit3 = (h >> 3) & 1;
  int bit4 = (h >> 4) & 1;
  
  // Create proper 4D gradients
  if(bit4 == 0) {
    // First 16 gradients
    *gx = (bit3) ? -1.0f : 1.0f;
    *gy = (bit2) ? -1.0f : 1.0f;
    *gz = (bit1) ? -1.0f : 1.0f;
    *gw = (bit0) ? -1.0f : 1.0f;
  } else {
    // Second 16 gradients - some components are zero
    int selector = (h >> 3) & 1;
    if(selector == 0) {
      *gx = 0.0f;
      *gy = (bit2) ? -1.0f : 1.0f;
      *gz = (bit1) ? -1.0f : 1.0f;
      *gw = (bit0) ? -1.0f : 1.0f;
    } else {
      *gx = (bit2) ? -1.0f : 1.0f;
      *gy = 0.0f;
      *gz = (bit1) ? -1.0f : 1.0f;
      *gw = (bit0) ? -1.0f : 1.0f;
    }
  }
}

// 4D Simplex Noise
float noise4d(float x, float y, float z, float w) {
  // Skew factors for 4D simplex
  const float F4 = 0.309016994f; // (sqrt(5) - 1) / 4
  const float G4 = 0.138196601f; // (5 - sqrt(5)) / 20
  
  // Skew the input space to determine which simplex cell we're in
  float s = (x + y + z + w) * F4;
  int i = (int)floor(x + s);
  int j = (int)floor(y + s);
  int k = (int)floor(z + s);
  int l = (int)floor(w + s);
  
  float t = (i + j + k + l) * G4;
  float X0 = i - t;
  float Y0 = j - t;
  float Z0 = k - t;
  float W0 = l - t;
  
  float x0 = x - X0;
  float y0 = y - Y0;
  float z0 = z - Z0;
  float w0 = w - W0;
  
  // For the 4D case, the simplex is a 4D shape with 5 corners
  // Rank the magnitudes to determine traversal order
  int rankx = 0;
  int ranky = 0;
  int rankz = 0;
  int rankw = 0;
  
  if(x0 > y0) rankx++; else ranky++;
  if(x0 > z0) rankx++; else rankz++;
  if(x0 > w0) rankx++; else rankw++;
  if(y0 > z0) ranky++; else rankz++;
  if(y0 > w0) ranky++; else rankw++;
  if(z0 > w0) rankz++; else rankw++;
  
  // The integer offsets for the simplex corners
  int i1 = (rankx >= 3) ? 1 : 0;
  int j1 = (ranky >= 3) ? 1 : 0;
  int k1 = (rankz >= 3) ? 1 : 0;
  int l1 = (rankw >= 3) ? 1 : 0;
  
  int i2 = (rankx >= 2) ? 1 : 0;
  int j2 = (ranky >= 2) ? 1 : 0;
  int k2 = (rankz >= 2) ? 1 : 0;
  int l2 = (rankw >= 2) ? 1 : 0;
  
  int i3 = (rankx >= 1) ? 1 : 0;
  int j3 = (ranky >= 1) ? 1 : 0;
  int k3 = (rankz >= 1) ? 1 : 0;
  int l3 = (rankw >= 1) ? 1 : 0;
  
  // Offsets for second corner
  float x1 = x0 - i1 + G4;
  float y1 = y0 - j1 + G4;
  float z1 = z0 - k1 + G4;
  float w1 = w0 - l1 + G4;
  
  // Offsets for third corner
  float x2 = x0 - i2 + 2.0f * G4;
  float y2 = y0 - j2 + 2.0f * G4;
  float z2 = z0 - k2 + 2.0f * G4;
  float w2 = w0 - l2 + 2.0f * G4;
  
  // Offsets for fourth corner
  float x3 = x0 - i3 + 3.0f * G4;
  float y3 = y0 - j3 + 3.0f * G4;
  float z3 = z0 - k3 + 3.0f * G4;
  float w3 = w0 - l3 + 3.0f * G4;
  
  // Offsets for fifth corner
  float x4 = x0 - 1.0f + 4.0f * G4;
  float y4 = y0 - 1.0f + 4.0f * G4;
  float z4 = z0 - 1.0f + 4.0f * G4;
  float w4 = w0 - 1.0f + 4.0f * G4;
  
  // Calculate contributions from each corner
  float n0, n1, n2, n3, n4;
  
  // Corner 0
  float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0 - w0*w0;
  if(t0 < 0.0f) {
    n0 = 0.0f;
  } else {
    t0 *= t0;
    float gx0, gy0, gz0, gw0;
    grad4(hash4d(i, j, k, l), &gx0, &gy0, &gz0, &gw0);
    n0 = t0 * t0 * (gx0 * x0 + gy0 * y0 + gz0 * z0 + gw0 * w0);
  }
  
  // Corner 1
  float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1 - w1*w1;
  if(t1 < 0.0f) {
    n1 = 0.0f;
  } else {
    t1 *= t1;
    float gx1, gy1, gz1, gw1;
    grad4(hash4d(i+i1, j+j1, k+k1, l+l1), &gx1, &gy1, &gz1, &gw1);
    n1 = t1 * t1 * (gx1 * x1 + gy1 * y1 + gz1 * z1 + gw1 * w1);
  }
  
  // Corner 2
  float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2 - w2*w2;
  if(t2 < 0.0f) {
    n2 = 0.0f;
  } else {
    t2 *= t2;
    float gx2, gy2, gz2, gw2;
    grad4(hash4d(i+i2, j+j2, k+k2, l+l2), &gx2, &gy2, &gz2, &gw2);
    n2 = t2 * t2 * (gx2 * x2 + gy2 * y2 + gz2 * z2 + gw2 * w2);
  }
  
  // Corner 3
  float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3 - w3*w3;
  if(t3 < 0.0f) {
    n3 = 0.0f;
  } else {
    t3 *= t3;
    float gx3, gy3, gz3, gw3;
    grad4(hash4d(i+i3, j+j3, k+k3, l+l3), &gx3, &gy3, &gz3, &gw3);
    n3 = t3 * t3 * (gx3 * x3 + gy3 * y3 + gz3 * z3 + gw3 * w3);
  }
  
  // Corner 4
  float t4 = 0.6f - x4*x4 - y4*y4 - z4*z4 - w4*w4;
  if(t4 < 0.0f) {
    n4 = 0.0f;
  } else {
    t4 *= t4;
    float gx4, gy4, gz4, gw4;
    grad4(hash4d(i+1, j+1, k+1, l+1), &gx4, &gy4, &gz4, &gw4);
    n4 = t4 * t4 * (gx4 * x4 + gy4 * y4 + gz4 * z4 + gw4 * w4);
  }
  
  // Sum up and scale the result to [-1, 1]
  return 27.0f * (n0 + n1 + n2 + n3 + n4);
}

// Fractional Brownian Motion (fBm) - Multi-octave 4D noise
float fbm4d(float x, float y, float z, float w, int octaves, float lacunarity, float gain) {
  float sum = 0.0f;
  float amplitude = 1.0f;
  float frequency = 1.0f;
  float maxValue = 0.0f;
  
  for(int i = 0; i < octaves; i++) {
    sum += amplitude * noise4d(x * frequency, y * frequency, z * frequency, w * frequency);
    maxValue += amplitude;
    amplitude *= gain;
    frequency *= lacunarity;
  }
  
  return sum / maxValue;
}

// Turbulence variant
float turbulence4d(float x, float y, float z, float w, int octaves, float lacunarity, float gain) {
  float sum = 0.0f;
  float amplitude = 1.0f;
  float frequency = 1.0f;
  float maxValue = 0.0f;
  
  for(int i = 0; i < octaves; i++) {
    sum += amplitude * fabs(noise4d(x * frequency, y * frequency, z * frequency, w * frequency));
    maxValue += amplitude;
    amplitude *= gain;
    frequency *= lacunarity;
  }
  
  return sum / maxValue;
}

// Ridged multifractal
float ridged4d(float x, float y, float z, float w, int octaves, float lacunarity, float gain) {
  float sum = 0.0f;
  float amplitude = 1.0f;
  float frequency = 1.0f;
  float maxValue = 0.0f;
  
  for(int i = 0; i < octaves; i++) {
    float n = noise4d(x * frequency, y * frequency, z * frequency, w * frequency);
    n = 1.0f - fabs(n);
    n = n * n;
    sum += amplitude * n;
    maxValue += amplitude;
    amplitude *= gain;
    frequency *= lacunarity;
  }
  
  return sum / maxValue;
}
kernel Claude4D : ImageComputationKernel<ePixelWise> {
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
  //dst() = float4(fBM(in_pos, octaves, lacunarity, gain));
  dst() = float4 ( fbm4d(in_pos.x,in_pos.y,in_pos.z, in_pos.w, octaves, lacunarity, gain) );
    }

};
