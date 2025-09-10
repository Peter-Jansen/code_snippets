kernel BoxBlurDownrezUprez : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessRandom, eEdgeClamped> src;  // Input image
  Image<eWrite, eAccessRandom, eEdgeClamped> dst; // Output image

  param:
    int radius;                                  // Blur radius
    float downsampleFactor;                      // Downsample factor (e.g., 2.0 for half resolution)

  void define() {
    defineParam(radius, "Radius", 2);
    defineParam(downsampleFactor, "Downsample Factor", 2.0f);
  }


  void process(int2 pos) {
    // Step 1: Compute downsampled coordinates
    float downX = pos.x / downsampleFactor;
    float downY = pos.y / downsampleFactor;

    // Step 2: Simulate downsampled pixel (average a 2x2 block for simplicity)
    float sampleX = floor(downX) * downsampleFactor;
    float sampleY = floor(downY) * downsampleFactor;
    float4 downsampledPixel = float4(0.0f);
    int sampleCount = 0;
    
    // Average pixels in a block to simulate downsampling
    for (int dy = 0; dy < ceil(downsampleFactor); dy++) {
      for (int dx = 0; dx < ceil(downsampleFactor); dx++) {
        int sx = sampleX + dx;
        int sy = sampleY + dy;
        if (sx < src.bounds.width() && sy < src.bounds.height()) {
          downsampledPixel += src(sx, sy);
          sampleCount++;
        }
      }
    }
    downsampledPixel /= sampleCount;

    // Step 3: Apply separable box blur at downsampled resolution
    // Horizontal pass (on-the-fly, using downsampled coordinates)
    float horizontalSum = 0.0f;
    int horizontalCount = 0;
    for (int k = -radius; k <= radius; k++) {
      float nx = floor(downX) + k;
      if (nx >= 0 && nx < src.bounds.width() / downsampleFactor) {
        // Sample at downsampled resolution
        float px = nx * downsampleFactor;
        float4 pixel = float4(0.0f);
        int blockCount = 0;
        for (int dx = 0; dx < ceil(downsampleFactor); dx++) {
          int sx = px + dx;
          if (sx < src.bounds.width()) {
            pixel += src(sx, sampleY);
            blockCount++;
          }
        }
        horizontalSum += pixel / blockCount;
        horizontalCount++;
      }
    }
    float horizontalBlur = horizontalSum / horizontalCount;

    // Vertical pass (on-the-fly)
    float verticalSum = 0.0f;
    int verticalCount = 0;
    for (int k = -radius; k <= radius; k++) {
      float ny = floor(downY) + k;
      if (ny >= 0 && ny < src.bounds.height() / downsampleFactor) {
        float py = ny * downsampleFactor;
        float pixel = 0.0f;
        int blockCount = 0;
        for (int dy = 0; dy < ceil(downsampleFactor); dy++) {
          int sy = py + dy;
          if (sy < src.bounds.height()) {
            pixel += src(sampleX, sy);
            blockCount++;
          }
        }
        verticalSum += pixel / blockCount;
        verticalCount++;
      }
    }
    float blurredPixel = verticalSum / verticalCount;

    // Step 4: Upsample using bilinear interpolation
    float fx = downX - floor(downX);
    float fy = downY - floor(downY);
    float x0 = floor(downX) * downsampleFactor;
    float x1 = min(x0 + downsampleFactor, src.width() - 1.0f);
    float y0 = floor(downY) * downsampleFactor;
    float y1 = min(y0 + downsampleFactor, src.height() - 1.0f);

    // Sample blurred values at downsampled corners (approximated)
    float p00 = blurredPixel; // Simplified: using the computed blurred pixel
    float p10 = p00; // In a single pass, we approximate neighbors
    float p01 = p00;
    float p11 = p00;

    // Bilinear interpolation
    float top = p00 + fx * (p10 - p00);
    float bottom = p01 + fx * (p11 - p01);
    float finalPixel = top + fy * (bottom - top);

    dst() = finalPixel;
  }
};