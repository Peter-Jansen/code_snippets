/*
Nuke blinkscript implementation of a simple image Transform operator.
Demonstrates pixel filter interpolation algorithms.

The following pixel filters are implemented:
0 - Blackman-Harris : Similar to cubic, but better performance in high frequencies
1 - Lanczos4 : 2x2 lanczos windowed sinc function
2 - Lanczos6 : 3x3 lanczos windowed sinc function
3 - Cubic : (Bicubic interpolation) - a=0.0, b=0.0
4 - Mitchell : (Bicubic interpolation) - a=1/3, b=1/3
5 - Keys : Catmull-Rom (Bicubic interpolation) - a=0.5, b=0.0
6 - Simon : (Bicubic interpolation) - a=0.75, b=0.0
7 - Rifman : (Bicubic interpolation) - a=1.0, b=0.0
8 - Parzen : (Bicubic interpolation) - a=0.0, b=1.0
9 - Guassian : gaussian interpolation width=6
10 - Sharp Guassian : gaussian interpolation width=4.5
11- Bilinear : bilinear interpolation between 4 nearest pixels
12 - Impulse : Round to the nearest pixel. 


---------------------
References
Bernd Jähne - Digital Image Processing_ Concepts, Algorithms, and Scientific Applications (1997, Springer) - p284
Wilhelm Burger, Mark James Burge - Principles of Digital Image Processing: Core Algorithms - (2009, Springer-Verlag London) - p220-224

interpolation
Fletcher Dunn, Ian Parberry - 3D Math Primer for Graphics and Game Development, 2nd Edition (2011, A K Peters_CRC Press) - p660

image reconstruction and filtering
http://www.pbr-book.org/3ed-2018/Sampling_and_Reconstruction/Image_Reconstruction.html

Further reading on sampling theory and aliasing
http://www.pbr-book.org/3ed-2018/Sampling_and_Reconstruction/Further_Reading.html


Digital Image Processing by Rafael C. Gonzalez, Richard E. Woods
interpolation introduction p65
bilinear: 4 nearest neighbors
bicubic: 16 nearest neighbors

p71 - distance measures: euclidean, city block (d4) distance, chessboard distance (d8)
D(p, q) = ((x - s)^2 + (y - t)^2)^0.5
d4(p,q) = |x-s|+|y-t|
d8(p,q) = max(|x-s|, |y-t|)

p87 - geometric spatial transformations - matrixes
affine transformation represented by a 3x3 matrix: scale, rotation, translation, shear

categories of image processing:
intensity transformations
spatial filtering
frequency domain

p145 - fundamentals of spatial filtering


https://archive.org/embed/Lectures_on_Image_Processing


J. R. Parker - Algorithms for image processing and computer vision (2010, Wiley)
p253 - fourier transform

Bernd Jähne - Digital Image Processing_ Concepts, Algorithms, and Scientific Applications (1997, Springer)
p277
p212 homogenous coordinates

http://alumni.media.mit.edu/~maov/classes/comp_photo_vision08f/lect/08_image_warps.pdf
https://www.nibcode.com/en/blog/1137/linear-algebra-and-digital-image-processing-part-III-affine-transformations

OpenImageIO Filter Algorithms
https://github.com/OpenImageIO/oiio/blob/master/src/libutil/filter.cpp

overview of sampling interpolation methods with discussion of ringing in constant values with lanczos
https://www.ipol.im/pub/art/2011/g_lmii/article.pdf
*/

kernel Transform : ImageComputationKernel<ePixelWise> {
  Image<eRead, eAccessRandom, eEdgeConstant> src;
  Image<eWrite, eAccessPoint> dst;

param:
  float2 translate;
  float rotate;
  float2 scale;
  float2 skew;
  float2 center;
  bool invert;
  int filter;
  bool range_compress;


  local:
    float3x3 M;
    float pi;
    bool rcomp;
    float weights[512]; // Array to hold lookup table of interpolation weights
    int wlut_size;
    int n; // filter size

  // Multiply a float3 by a matrix3x3
  float3 mult_f3_f33( float3 src, float3x3 mtx) {
    return float3(mtx[0][0] * src.x + mtx[0][1] * src.y + 
    mtx[0][2] * src.z, mtx[1][0] * src.x + mtx[1][1] * src.y + 
    mtx[1][2] * src.z, mtx[2][0] * src.x + mtx[2][1] * src.y + 
    mtx[2][2] * src.z);
  }

  void init() {
    pi = 3.1415926535f;
    
    float t = rotate*pi/180.0f; // rotation angle in radians

    // Assemble affine transformation matrix
    float3x3 R = float3x3( // rotation matrix
      cos(t), -sin(t), 0.0f,
      sin(t), cos(t), 0.0f, 
      0.0f, 0.0f, 1.0f);
    float3x3 S = float3x3( // scale matrix
      scale.x, 0.0f, 0.0f,
      0.0f, scale.y, 0.0f,
      0.0f, 0.0f, 1.0f);
    float3x3 T = float3x3( // translation matrix
      1.0f, 0.0f, translate.x,
      0.0f, 1.0f, translate.y,
      0.0f, 0.0f, 1.0f);
    float3x3 SHX = float3x3( // skew x matrix
      1.0f, skew.x, 0.0f,
      0.0f, 1.0f, 0.0f, 
      0.0f, 0.0f, 1.0f);
    float3x3 SHY = float3x3( // skew y matrix
      1.0f, 0.0f, 0.0f,
      skew.y, 1.0f, 0.0f, 
      0.0f, 0.0f, 1.0f);

    // Combine all transforms into transformation matrix M
    M = T*R*SHX*SHY*S; 

    // Only enable range compression if filter has negative lobes
    rcomp = 0;
    if (range_compress) {
      if (filter == 1 || filter == 2 || filter == 5 || filter == 6 || filter == 7) {
        rcomp = 1;
      }
    }


    // Set filter size based on filter parameter
    // size of pixel neighborhood = n*2+1 by n*2+1
    n = 2; // default size is 3x3 except the following...
    if (filter == 0) n = 1; // Blackman-Harris = 2x2
    if (filter == 3) n = 1; // Cubic = 2x2
    if (filter == 2) n = 3; // Lanczos6 = 7x7
    if (filter == 9) n = 1; // Gaussian4


    // Pre-calculate interpolation weights and store them in a lookup table
    wlut_size = 512; // Needs to match size of weights[] array
    // Loop over all indices of weights 
    for (int i=0; i < wlut_size; i++) {
      // Calc weight for position at weight
      float x = float(i) * float(n) / float(wlut_size);
      weights[i] = weight(x);
    }
  }


  // Logarithmically compress values above 0.18. Matches OpenImageIO oiiotool rangecompress
  // Using log2shaper as this function can still cause overshoots 
  float rangecompress(float x, bool inverse) {
    float x1 = 0.18f;
    float a = -0.54576885700225830078f;
    float b = 0.18351669609546661377f;
    float c = 284.3577880859375f;
    float absx = fabs(x);
    if (!inverse) {
      return absx <= x1 ? x : (a + b * log(fabs(c * absx + 1.0f))) * sign(x);
    } else {
      if (absx <= x1) return x;
      float e = exp((absx - a) / b);
      float _x = (e - 1.0f) / c;
      if (_x < x1) _x = (-e - 1.0f) / c;
      return _x * sign(x);
    }
  }

  // ACEScct log transform
  float acescct(float x, bool inverse) {
    if (inverse) {
      if (x > 0.155251141552511f) {
        return pow( 2.0f, x*17.52f - 9.72f);
      } else {
        return (x - 0.0729055341958355f) / 10.5402377416545f;
      }
    } else {
      if (x <= 0.0078125f) {
        return 10.5402377416545f * x + 0.0729055341958355f;
      } else {
        return (log2(x) + 9.72f) / 17.52f;
      }
    }
  }

  // Generic Log2 shaper 
  float log2shaper(float x, bool inverse) {
    float max_exp = 2.0f;
    float min_exp = -2.0f;
    float mid_grey = 0.18f;
    float cut = 0.008;
    float slope = 1.0f/(log(2)*cut*(max_exp-min_exp));
    float offset = (log(cut/mid_grey)/log(2)-min_exp)/(max_exp-min_exp);
    if (inverse) {
      return x >= offset ? pow(2.0f, x*(max_exp-min_exp)+min_exp)*mid_grey : (x-offset)/slope+cut;
    } else {
      return x >= cut ? (log(x/mid_grey)/log(2)-min_exp)/(max_exp-min_exp) : slope*(x-cut)+offset;
    }
  }

  // Bilinear Interpolation
  float bilinear_filter(float x, float y, int k) {
    int u = floor(x);
    int v = floor(y);
    float a = x-u;
    float b = y-v;
    float p1 = src(u, v, k);
    float p2 = src(u+1, v, k);
    float p3 = src(u, v+1, k);
    float p4 = src(u+1, v+1, k);
    float r1 = p1 + a*(p2-p1);
    float r2 = p3 + a*(p4-p3);
    float r3 = r1 + b*(r2-r1);
    return r3;
  }


  // Gaussian interpolation https://renderman.pixar.com/resources/RenderMan_20/risOptions.html
  float gaussian(float x, float w) {
    x = fabs(x);
    float xw = 6*x/w;
    return x < w ? exp(-2*xw*xw) : 0.0f;
  }

  // Parameterized Cubic spline interpolation - https://www.desmos.com/calculator/il0lu3cnxr
  float bicubic(float x, float a, float b, float n) {
    x = fabs(x);
    if (x > n) return 0.0f;
    float x2 = x*x;
    float x3 = x*x*x;
    return x < 1.0f ? ((-6*a-9*b+12)*x3+(6*a+12*b-18)*x2-2*b+6)/6 : x < 2.0f ? ((-6*a-b)*x3+(30*a+6*b)*x2+(-48*a-12*b)*x+24*a+8*b)/6 : 0.0f;
  }

  // Catmull-Rom interpolation. Same as Keys (not used but included for posterity)
  float catrom(float x) { 
    x        = fabs(x);
    float x2 = x * x;
    float x3 = x * x2;
    return (x >= 2.0f) ? 0.0f
                       : ((x < 1.0f) ? (3.0f * x3 - 5.0f * x2 + 2.0f)/2.0f
                       : (-x3 + 5.0f * x2 - 8.0f * x + 4.0f)/2.0f);
  }

  // Lanczos windowed sinc interpolation. 
  // Lanczos4 a=2, Lanczos6 a=3
  float lanczos(float x, float a) {
    x = fabs(x);
    if (x > a) return 0.0f;
    if (x < 0.0001f) return 1.0f;
    float pi_x = pi*x;
    return a*(sin(pi_x)*(sin((pi_x)/a))/(pi_x*pi_x));
  }

  // Blackman-Harris interpolation
  float blackman_harris(float x) {
    if (x < -1.0f || x > 1.0f) return 0.0f;
    x /= 1.5f;
    x = (x + 1.0f) * 0.5f;
    const float a0   = 0.35875f;
    const float a1   = -0.48829f;
    const float a2   = 0.14128f;
    const float a3   = 0.01168f; 
    float cos2pix = cos(2.0f * pi * x);
    float cos4pix = 2.0f * cos2pix * cos2pix - 1.0f;
    float cos6pix = cos2pix * (2.0f * cos4pix - 1.0f);
    return a0 + a1 * cos2pix + a2 * cos4pix - a3 * cos6pix;
  }

  float lerp(float a, float b, float f) { // Linear interpolation between a and b given position f
    return a + f * (b - a);
  }

  float get_weight(float x) {
    // Calculate linear interpolation at float position x in lookup
    float d = fabs(x)/n*wlut_size;
    int d0 = floor(d);
    int d1 = ceil(d);
    return lerp(weights[d0], weights[d1], d-d0);
  }

  // Choose filter interpolation to weight value x
  float weight(float x) {
    if (filter == 0) { // Blackman-Harris
      return blackman_harris(x);
    } else if (filter == 1 || filter == 2) { // Lanczos
      return lanczos(x, n); 
    } else if (filter == 3) { // Cubic
      return bicubic(x, 0.0f, 0.0f, n);
    } else if (filter == 4) { // Mitchell
      return bicubic(x, 0.33333333f, 0.33333333f, n);
    } else if (filter == 5) { // Keys or Catmull-Rom
      return catrom(x);
    } else if (filter == 6) { // Simon
      return bicubic(x, 0.75f, 0.0f, n);
    } else if (filter == 7) { // Rifman
      return bicubic(x, 1.0f, 0.0f, n);
    } else if (filter == 8) { // Parzen
      return bicubic(x, 0.0f, 1.0f, n);
    } else if (filter == 9) { // Gaussian
      return gaussian(x, 6);
    } else if (filter == 10) { // Sharp Gaussian
      return gaussian(x, 4.25f);
    } else { // Default
      return blackman_harris(x);
    }
  }

  // Sample pixel at continuous float position (x, y) in channel k
  float sample(float x, float y, int k) {
    int u0 = round(x); // nearest x
    int v0 = round(y); // nearest y

    // Normalization factor for lanczos & blackman-harris & gaussian filter windows
    // These weighting functions do not sum to 1 and cause variations in constant input values
    // We sum the weights and then normalize the value after to counteract this
    bool normalize = (filter < 3 || filter == 9 || filter == 10);
    float norm = 0.0f;

    // Loop over neighboring pixels and return weighted sum of pixel values
    float q = 0.0f;
    for (int j = -n; j <= n; j++) {
      int v = v0 + j;
      float p = 0.0f;
      float row_norm = 0.0f;
      for (int i = -n; i <= n; i++) {
        int u = u0 + i;
        float c = src(u, v, k);
        // float w = weight(u-x);
        float w = get_weight(u-x);
        if (rcomp) c = log2shaper(c, 0);
        p += c * w;
        if (normalize) row_norm += w;
      }
      // float w = weight(v-y);
      float w = get_weight(v-y);
      q += p * w;
      if (normalize) norm += row_norm * w;
    }
    if (normalize) q /= norm;
    if (rcomp) return log2shaper(q, 1);
    return q;
  }

  void process(int2 pos) {
    /*
      For all pixels of destination:
        - Find continuous float source pixel location given transformation matrix
        - Interpolate pixel value given pixel filter weighting function
        - Write interpolated value to destination
    */

    float x, y, dx, dy;

    // Current pixel location
    x = float(pos.x+0.5f);
    y = float(pos.y+0.5f);

    // Translate to center of transform
    dx = x - center.x;
    dy = y - center.y;

    // Multiply by transformation matrix
    float3 result;
    if (invert) {
      result = mult_f3_f33(float3(dx, dy, 1.0f), M);
    } else {
      result = mult_f3_f33(float3(dx, dy, 1.0f), M.invert());
    }

    // Translate back to origin
    dx = result.x + center.x-0.5f;
    dy = result.y + center.y-0.5f;

    // Write interpolated pixel value from source location (dx, dy)
    for (int k = 0; k <=3; k++) {
      if (filter < 11) {
        dst(k) = sample(dx, dy, k);
      } else if (filter == 11) { // Bilinear interpolation
        dst(k) = bilinear_filter(dx, dy, k);
      } else if (filter == 12) { // Impulse
        dst(k) = src(round(dx), round(dy), k);
      }
    }
  }
};