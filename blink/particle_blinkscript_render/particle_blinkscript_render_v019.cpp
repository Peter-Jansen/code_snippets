inline float fract (float x) {return x-floor(x);}
inline float random(float co) { return fract(sin(co*(91.3458f)) * 47453.5453f); }

inline float3 randomv(float3 seed){
  float scramble = random(seed.x + seed.y * seed.z);
  float3 rand;
  rand.x = random(seed.x + scramble + 0.14557f + 0.47917f * seed.z)*2-1;
  rand.y = random(seed.y * 0.214447f + scramble * 47.241f * seed.x)*2-1;
  rand.z = random(seed.z * scramble + 3.147855f + 0.2114f * seed.y)*2-1;
  return normalize(rand);
}

inline float3 randomv2(float seed){
    //float scramble = random(seed + seed * seed);
    float3 rand;
    rand.x = random(seed + 0.215568f)*2.0-1.0;
    rand.y = random(seed + 2.112408f)*2.0-1.0;
    rand.z = random(seed + 68.13384f)*2.0-1.0;
    return rand;
}


int fastfloor( const float x ) { return x > 0 ? (int) x : (int) x - 1; }
inline float raw_noise_4d( const float x, const float y, const float z, const float w ) {
int simplex[64][4] = {
    {0,1,2,3},{0,1,3,2},{0,0,0,0},{0,2,3,1},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,2,3,0},
    {0,2,1,3},{0,0,0,0},{0,3,1,2},{0,3,2,1},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,3,2,0},
    {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
    {1,2,0,3},{0,0,0,0},{1,3,0,2},{0,0,0,0},{0,0,0,0},{0,0,0,0},{2,3,0,1},{2,3,1,0},
    {1,0,2,3},{1,0,3,2},{0,0,0,0},{0,0,0,0},{0,0,0,0},{2,0,3,1},{0,0,0,0},{2,1,3,0},
    {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
    {2,0,1,3},{0,0,0,0},{0,0,0,0},{0,0,0,0},{3,0,1,2},{3,0,2,1},{0,0,0,0},{3,1,2,0},
    {2,1,0,3},{0,0,0,0},{0,0,0,0},{0,0,0,0},{3,1,0,2},{0,0,0,0},{3,2,0,1},{3,2,1,0}
};
    int perm[512] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,

        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };
   int grad4[32][4]= {
    {0,1,1,1},  {0,1,1,-1},  {0,1,-1,1},  {0,1,-1,-1},
    {0,-1,1,1}, {0,-1,1,-1}, {0,-1,-1,1}, {0,-1,-1,-1},
    {1,0,1,1},  {1,0,1,-1},  {1,0,-1,1},  {1,0,-1,-1},
    {-1,0,1,1}, {-1,0,1,-1}, {-1,0,-1,1}, {-1,0,-1,-1},
    {1,1,0,1},  {1,1,0,-1},  {1,-1,0,1},  {1,-1,0,-1},
    {-1,1,0,1}, {-1,1,0,-1}, {-1,-1,0,1}, {-1,-1,0,-1},
    {1,1,1,0},  {1,1,-1,0},  {1,-1,1,0},  {1,-1,-1,0},
    {-1,1,1,0}, {-1,1,-1,0}, {-1,-1,1,0}, {-1,-1,-1,0}
   };
    float F4 = (sqrt(5.0f)-1.0)/4.0;
    float G4 = (5.0-sqrt(5.0f))/20.0;
    float n0, n1, n2, n3, n4;
    float s = (x + y + z + w) * F4;
    int i = fastfloor(x + s);
    int j = fastfloor(y + s);
    int k = fastfloor(z + s);
    int l = fastfloor(w + s);
    float t = (i + j + k + l) * G4;
    float X0 = i - t;
    float Y0 = j - t;
    float Z0 = k - t;
    float W0 = l - t;

    float x0 = x - X0;
    float y0 = y - Y0;
    float z0 = z - Z0;
    float w0 = w - W0;
    int c1 = (x0 > y0) ? 32 : 0;
    int c2 = (x0 > z0) ? 16 : 0;
    int c3 = (y0 > z0) ? 8 : 0;
    int c4 = (x0 > w0) ? 4 : 0;
    int c5 = (y0 > w0) ? 2 : 0;
    int c6 = (z0 > w0) ? 1 : 0;
    int c = c1 + c2 + c3 + c4 + c5 + c6;

    int i1, j1, k1, l1;
    int i2, j2, k2, l2;
    int i3, j3, k3, l3;
    i1 = simplex[c][0]>=3 ? 1 : 0;
    j1 = simplex[c][1]>=3 ? 1 : 0;
    k1 = simplex[c][2]>=3 ? 1 : 0;
    l1 = simplex[c][3]>=3 ? 1 : 0;
    i2 = simplex[c][0]>=2 ? 1 : 0;
    j2 = simplex[c][1]>=2 ? 1 : 0;
    k2 = simplex[c][2]>=2 ? 1 : 0;
    l2 = simplex[c][3]>=2 ? 1 : 0;
    i3 = simplex[c][0]>=1 ? 1 : 0;
    j3 = simplex[c][1]>=1 ? 1 : 0;
    k3 = simplex[c][2]>=1 ? 1 : 0;
    l3 = simplex[c][3]>=1 ? 1 : 0;
    float x1 = x0 - i1 + G4;
    float y1 = y0 - j1 + G4;
    float z1 = z0 - k1 + G4;
    float w1 = w0 - l1 + G4;
    float x2 = x0 - i2 + 2.0*G4;
    float y2 = y0 - j2 + 2.0*G4;
    float z2 = z0 - k2 + 2.0*G4;
    float w2 = w0 - l2 + 2.0*G4;
    float x3 = x0 - i3 + 3.0*G4;
    float y3 = y0 - j3 + 3.0*G4;
    float z3 = z0 - k3 + 3.0*G4;
    float w3 = w0 - l3 + 3.0*G4;
    float x4 = x0 - 1.0 + 4.0*G4;
    float y4 = y0 - 1.0 + 4.0*G4;
    float z4 = z0 - 1.0 + 4.0*G4;
    float w4 = w0 - 1.0 + 4.0*G4;
    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;
    int ll = l & 255;
    int gi0 = perm[ii+perm[jj+perm[kk+perm[ll]]]] % 32;
    int gi1 = perm[ii+i1+perm[jj+j1+perm[kk+k1+perm[ll+l1]]]] % 32;
    int gi2 = perm[ii+i2+perm[jj+j2+perm[kk+k2+perm[ll+l2]]]] % 32;
    int gi3 = perm[ii+i3+perm[jj+j3+perm[kk+k3+perm[ll+l3]]]] % 32;
    int gi4 = perm[ii+1+perm[jj+1+perm[kk+1+perm[ll+1]]]] % 32;
    float t0 = 0.6 - x0*x0 - y0*y0 - z0*z0 - w0*w0;
    if(t0<0) n0 = 0.0;
    else {
        t0 *= t0;
        n0 = t0 * t0 * dot(float4(grad4[gi0][0],grad4[gi0][2],grad4[gi0][3],grad4[gi0][3]), float4(x0, y0, z0, w0));
    }
    float t1 = 0.6 - x1*x1 - y1*y1 - z1*z1 - w1*w1;
    if(t1<0) n1 = 0.0;
    else {
        t1 *= t1;
        n1 = t1 * t1 * dot(float4(grad4[gi1][0],grad4[gi1][2],grad4[gi1][3],grad4[gi1][3]), float4(x1, y1, z1, w1));
    }
    float t2 = 0.6 - x2*x2 - y2*y2 - z2*z2 - w2*w2;
    if(t2<0) n2 = 0.0;
    else {
        t2 *= t2;
        n2 = t2 * t2 * dot(float4(grad4[gi2][0],grad4[gi2][2],grad4[gi2][3],grad4[gi2][3]), float4(x2, y2, z2, w2));
    }
    float t3 = 0.6 - x3*x3 - y3*y3 - z3*z3 - w3*w3;
    if(t3<0) n3 = 0.0;
    else {
        t3 *= t3;
        n3 = t3 * t3 * dot(float4(grad4[gi3][0],grad4[gi3][2],grad4[gi3][3],grad4[gi3][3]), float4(x3, y3, z3, w3));
    }
    float t4 = 0.6 - x4*x4 - y4*y4 - z4*z4 - w4*w4;
    if(t4<0) n4 = 0.0;
    else {
        t4 *= t4;
        n4 = t4 * t4 * dot(float4(grad4[gi4][0],grad4[gi4][2],grad4[gi4][3],grad4[gi4][3]), float4(x4, y4, z4, w4));
    }
    return 27.0 * (n0 + n1 + n2 + n3 + n4);
}

inline float3 Noise3D(float3 P, float w){
    float x = raw_noise_4d(P.x, P.y, P.z, w);
    float y = raw_noise_4d(P.x, P.y, P.z, w+20.0);
    float z = raw_noise_4d(P.x, P.y, P.z, w+40.0);
    return float3(x,y,z);
}


kernel ParticleRenderKernel : ImageComputationKernel<ePixelWise>{
  Image<eRead, eAccessPoint> p_position;
  Image<eRead, eAccessPoint> p_color;
  Image<eRead, eAccessPoint> p_size;
  Image<eRead, eAccessPoint> p_velocity;
  Image<eRead, eAccessPoint> p_id;
  Image<eRead, eAccessPoint> p_lastPosition;
//   Image<eRead, eAccessRandom, eEdgeConstant> texture;
  Image<eRead, eAccessRandom, eEdgeClamped> cam_v;
  Image<eWrite, eAccessRandom> output;

  param:
    float4x4 _worldToScreen;
    float4x4 camera_matrix;
    


    float _size;
    float _minSize;
    int _maxSteps;
    float spacing;
    float _shutter;
    float _shutter_offset;
    int _merge_operation;
    int _screen_space_scale;
    int _shutter_type;
    int _duplicates_num;
    float _duplicate_radius;

    float haperture;
    float focal;
    float pixel_aspect;
    float cam_near;
    float cam_far;

    float near;
    float near_falloff;
    float far;
    float far_falloff;
    float falloff_gamma;

  local:
    int _w, _h;
    float image_aspect;
    float2 out_res;

    // int tex_w, tex_h;
    float pixelsPerWorldUnit;

    // projection matrix stuff. This is copying the one nuke provides in nukescripts.snap3d.py
    float4x4 view;      // inverted camera matrix.
    float4x4 p;         // projection matrix
    float4x4 t;         // translate projected points into normalsed pixel coords (from 0,0 to -2,2 instead of -1,-1 to 1,1)
    float4x4 s;         // scale normalised screen coords to actual pixel coords
    float4x4 w2s;       // final world to screen matrix. This matches the one provided by this node (ParticleBlinkScriptRender),
                        // and the projection() function in _nukemath py.
                        // with this we can uhh. Do stuff..?





  void define() {
    defineParam(_worldToScreen, "_worldToScreen", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f) );


    defineParam(_size, "paSize", 3.0f);
    defineParam(_minSize, "minSize", 1.0f);
    defineParam(_maxSteps, "Steps", 10);
    defineParam(spacing, "spacing", 1.0f);
    defineParam(_shutter, "shutter", 0.5f);
    defineParam(_shutter_offset, "shutter_offset", 0.0f);
    defineParam(_shutter_type, "shutter_type", 0);
    defineParam(_duplicates_num, "duplicates_num", 0);
    defineParam(_duplicate_radius, "duplicate_radius", 1.0f);
    defineParam(_merge_operation, "merge_operation", 0);
    defineParam(_screen_space_scale, "screen_space_scale", 0);

    // camera params
    defineParam(camera_matrix, "camera_matrix", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f) );
    defineParam(haperture, "haperture", 24.576f);
    defineParam(focal, "focal", 24.0f);
    defineParam(pixel_aspect, "pixel_aspect", 1.0f);
    defineParam(cam_near, "cam_near", 0.1f);
    defineParam(cam_far, "cam_far", 1000.0f);

    //fade params
    defineParam(near, "near", 0.0f);
    defineParam(near_falloff, "near_falloff", 0.0f);
    defineParam(far, "far", 1000.0f);
    defineParam(far_falloff, "far_falloff", 0.0f);
    defineParam(falloff_gamma, "falloff_gamma", 1.0f);

  }
  float4 srcOver( float4 a, float4 b ) {
    return (1.0f-a.w)*b + a;
  }
/*
  float2 transform( float3 p ){
    float4 r = _worldToScreen*float4(p.x, p.y, p.z, 1.0);
    float2 xy = float2(r.x, r.y)/r.w;
    return xy;
  }
  float3 transformz( float3 p ){
    float4 r = _worldToScreen*float4(p.x, p.y, p.z, 1.0);
    float2 xy = float2(r.x, r.y)/r.w;
    float depth = r.w;
    return float3(xy, depth);
  }  */

 
    float2 transform( float3 p ){
    float4 r = w2s*float4(p.x, p.y, p.z, 1.0);
    float2 xy = float2(r.x, r.y)/r.w;
    return xy;
  }
  float3 transformz( float3 p ){
    float4 r = w2s*float4(p.x, p.y, p.z, 1.0);
    float2 xy = float2(r.x, r.y)/r.w;
    float depth = r.w;
    return float3(xy, depth);
  }  

  float smoothstep( float a, float b, float x ) {
    float t = clamp((x - a) / (b - a), 0.0, 1.0);
    return t*t * (3.0f - 2.0f*t);
  }

  float lerp(float a, float b, float t){
    return a + t * (b-a);
  }
  float2 lerp(float2 a, float2 b, float t) {
     return a + t * (b - a);
  }

float4x4 projectionMatrix(
        const float focalLength,
        const float horizontalAperture,
        const float nearPlane,
        const float farPlane)
{
    float farMinusNear = farPlane - nearPlane;
    return float4x4(
        2 * focalLength / horizontalAperture, 0, 0, 0,
        0, 2 * focalLength / horizontalAperture, 0, 0,
        0, 0, -(farPlane + nearPlane) / farMinusNear, -2 * (farPlane * nearPlane) / farMinusNear,
        0, 0, -1, 0
    );
}

  void init()
  {
    _w = output.bounds.width();
    _h = output.bounds.height();
    out_res = float2(float(_w), float(_h));




    // tex_w = texture.bounds.width();
    // tex_h = texture.bounds.height();
    pixelsPerWorldUnit = (_w*0.5) / haperture;

    image_aspect = float(_h) / float(_w);

    // camera projection matrix
    p = projectionMatrix(focal, haperture, cam_near, cam_far);

    // Matrix to scale normalised pixel coords into actual pixel coords.
    float x_scale = float(_w) * 0.5;
    float y_scale = x_scale *pixel_aspect;
    s.setIdentity();
    s.scale(float4(x_scale, y_scale, 1.0f,1.0f));

    // Matrix to translate the projected points into normalised pixel coords
    t.setIdentity();
    t.translate(float4(1.0, 1.0 - (1.0 - image_aspect / pixel_aspect), 0.0f, 1.0f));

    // view matrix is inverted camera world matrix
    view =  camera_matrix.invert(); 

    // world to screen matrix is the concatenation of above matrices, from right to left.
    // TO DO: maybe add screen scale/roll all that crap that the snap3d projection matrix provides. Could be useful for overscan..?
    w2s = s * t * p * view;



  }
  void renderPoint( float2 xy, float4 pcolor, float pointSize, float falloff ){

    float sizeSquared = pointSize*pointSize;
    float2 f = xy-floor(xy);                            //fractional part of screen coordinate?
    float2 g = float2(1.0f) - f;                        // complement/invert of fractional part
    //xy = floor(xy);
    int size = ceil(pointSize);
    int minX = max(0, int(xy.x)-size);                  // min x coord is clamped to image x=0
    int maxX = min(_w-1, int(xy.x)+size);               // max x coord is clamped to max image x coord
    int minY = max(0, int(xy.y)-size);                  // etc for y coord
    int maxY = min(_h-1, int(xy.y)+size);
    // for loop that runs over the expected size of the point to be rendered.
    for ( int y = minY; y <= maxY; y++ ) {              // starting at min y until max y
      for ( int x = minX ; x <= maxX; x++ ) {           // starting at min x until max x
        float2 p = float2(x, y);                        // current position in the iteration space
        float2 d = p-xy;                                // vector from center point to the current pixel
        float r2 = dot(d, d);                           // squared distance to from p to xy
        // work with squares, ie squared distance and squared radius because it's faster to calculate.
        if ( r2 < sizeSquared ) {                       // if radius squared is smaller than size squared
         float t = 1.0f-smoothstep(0, sizeSquared, r2);
        t *= falloff;
          if (_merge_operation == 0) {
            output(p.x, p.y) += pcolor*t;
          }
          if (_merge_operation == 1){
          float4 colourToAdd = pcolor * t;
          float4 existingColour = output(p.x,p.y);
          output(p.x, p.y) = srcOver(colourToAdd, existingColour);
          }
                           // after we calculate the pixels distance, and smooth it with smoothstep
                                                        // we write it to the output buffer with a simple plus.
        }
      }
    }
  }
//   void renderTex( float2 xy, float4 pcolor, float pointSize, float falloff ){

//     float sizeSquared = pointSize*pointSize;
//     float2 f = xy-floor(xy);                            //fractional part of screen coordinate?
//     float2 g = float2(1.0f) - f;                        // complement/invert of fractional part
//     //xy = floor(xy);
//     int size = ceil(pointSize);
//     int minX = max(0, int(xy.x)-size);                  // min x coord is clamped to image x=0
//     int maxX = min(_w-1, int(xy.x)+size);               // max x coord is clamped to max image x coord
//     int minY = max(0, int(xy.y)-size);                  // etc for y coord
//     int maxY = min(_h-1, int(xy.y)+size);
//     // for loop that runs over the expected size of the point to be rendered.
//     for ( int y = minY; y <= maxY; y++ ) {              // starting at min y until max y
//       for ( int x = minX ; x <= maxX; x++ ) {           // starting at min x until max x
//         float2 p = float2(x, y);                        // current position in the iteration space
//         //float2 uv = float2( (p.x/float(_w))*float(tex_w), ( p.y/float(_h))*float(tex_h));
//         float u = (float(x - minX) / size) * float(tex_w);
//         float v = (float(y - minY) / size) * float(tex_h);
//         //float2 uv = float2(u,v);
//         //float2 uv = float2((float(size)/p.x) * float(tex_w), (float(size)/p.y) * float(tex_h));
        
//         float4 texLookup = bilinear(texture, u, v);
//           if (_merge_operation == 0) {
//             output(p.x, p.y) += texLookup;
//           }
//           if (_merge_operation == 1){
//           float4 colourToAdd = texLookup;
//           float4 existingColour = output(p.x,p.y);
//           output(p.x, p.y) = srcOver(colourToAdd, existingColour);
//           }
//                            // after we calculate the pixels distance, and smooth it with smoothstep
//                                                         // we write it to the output buffer with a simple plus.
//         }
//       }
//     }

  float minMaxSmooth(float n,float ns,float f,float fs,float v){
      // input variables are near, near soft, far, far soft and value
      float output = 0.0;
      float near = min(n, f);
      float near_soft = max(ns, 0.0);
      float far = max(n, f);
      float far_soft = max(fs, 0.0);

      float near_soft_absolute = near-near_soft;
      float far_soft_absolute = far+far_soft;

      if (v <= near){
        output = smoothstep(near_soft_absolute,near,v);
        }
      else{
        output = 1-smoothstep(far,far_soft_absolute,v);
        }
      return output;
  }



    float screenSpaceSize(float focal, float pixelsPerWorldUnit, float p_size, float distance){
        float particleSizeOnImagePlane = (focal * p_size) / distance;
        float particleSizeInPixels = (particleSizeOnImagePlane * pixelsPerWorldUnit);
    return particleSizeInPixels;
  }  




  void process() {

    float3 P = p_position();
    float3 P_prev = p_lastPosition();
    float3 v = p_velocity();
    float3 xyz = transformz(P);


    float2 pos_2d = transform(P);
    float2 cam_vel = float2(cam_v(pos_2d.x, pos_2d.y).x, cam_v(pos_2d.x, pos_2d.y).y);

    float dist = xyz.z;

    float3 parent_P = P;

    // optimisation conditions, render only in frustum, and positive depth (in front of camera)
    if (dist>0.0f){



        for (int pt = 0; pt <= _duplicates_num; pt++){            // duplicate points

        if (pt !=0){
        P += randomv2(float(pt) + float(p_id())*0.02)*_duplicate_radius;
        }
        //P += Noise3D(P*.1, float(pt)) * 0.5;
        //v = P - P_prev;
        float3 shutter_open, shutter_close;
        if (_shutter_type == 0) {                   // start
            shutter_open = P;
            shutter_close = P+(v *_shutter);
        }
        else if (_shutter_type == 1) {              // centered
            shutter_open = (P - (v *_shutter*.5) ) ;
            shutter_close = P + (v *_shutter*.5);
        }
        else if (_shutter_type == 2) {              // end
            shutter_open = P;
            shutter_close = P-(v *_shutter);
        }

        float2 shutter_open_2d, shutter_close_2d;
        float size_start, size_end;
        float3 shutter_open_ndc = transformz(shutter_open);
        float3 shutter_close_ndc = transformz(shutter_close);

        shutter_open_2d = float2(shutter_open_ndc.x, shutter_open_ndc.y);
        shutter_close_2d = float2(shutter_close_ndc.x, shutter_close_ndc.y)  + cam_vel;
        size_start = max(_minSize, min(100.0f, screenSpaceSize(focal, pixelsPerWorldUnit, _size * p_size().x, shutter_open_ndc.z)));
        size_end = max(_minSize, min(100.0f, screenSpaceSize(focal, pixelsPerWorldUnit, _size * p_size().x, shutter_close_ndc.z)));


        float2 blur_vector = (shutter_close_2d-shutter_open_2d);
        float blur_length = length(blur_vector);

        // adaptive steps
        int steps = max(0 , min(_maxSteps, ceil(blur_length/min(size_start, size_end)*2.0/spacing)));
        float2 step = blur_vector/float(steps);


        // colour stuff
        float depth_fade = minMaxSmooth(near, near_falloff, far, far_falloff, dist);
        depth_fade = pow(depth_fade, falloff_gamma);
        float4 pcolor = p_color()/ float(steps);

        for (int i = 0; i <= steps; i++){
            float2 draw_xy = shutter_open_2d + (step * float(i));
            float size = lerp(size_start, size_end, float(i)/float(steps));
            if (draw_xy.x >-size && draw_xy.y>-size && draw_xy.x < out_res.x+size && draw_xy.y < out_res.y+size){
            renderPoint(draw_xy, pcolor, size, depth_fade);
            }
        }
        }
    }
  }
};