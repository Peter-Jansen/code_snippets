kernel ScaleImageFromCenter : ImageComputationKernel<ePixelWise>
{
  // Input and output images
  Image<eRead, eAccessRandom, eEdgeClamped> src;
  Image<eWrite> dst;

  // Parameters for scaling and center point
  param:
    float scaleFactor; // Scaling factor (e.g., 2.0 for 2x zoom, 0.5 for half size)
    // float centerX;     // X-coordinate of the center point
    // float centerY;     // Y-coordinate of the center point
    float2 center;

  // Process each pixel
  void process(int2 pos) {
    
    // Calculate the offset from the center point
    float dx = pos.x - center.x;
    float dy = pos.y - center.y;

    // Apply inverse scaling to map the current pixel to the source pixel
    float srcX = center.x + (dx / scaleFactor);
    float srcY = center.y + (dy / scaleFactor);

    // Sample the source image at the computed coordinates
    // Using bilinear interpolation for smoother results
    float4 result = src(srcX, srcY);

    // Write the result to the output image
    dst() = result;
  }
};