kernel ScaleImageFromCenter : ImageComputationKernel<ePixelWise>
{
  // Input and output images
  Image<eRead, eAccessRandom, eEdgeClamped> src;
  Image<eWrite> dst;

  // Parameters for scaling and center point
  param:
    float scaleFactor; // Scaling factor (e.g., 2.0 for 2x zoom, 0.5 for half size)
    float2 center;


float4 scale(int2 pos, float amount){
        // Calculate the offset from the center point
    float dx = pos.x - center.x;
    float dy = pos.y - center.y;

    // Apply inverse scaling to map the current pixel to the source pixel
    float srcX = center.x + (dx / scaleFactor);
    float srcY = center.y + (dy / scaleFactor);

    // return src(srcX, srcY);
    return bilinear(src, srcX, srcY);
}
  // Process each pixel
  void process(int2 pos) {
    

    float4 scaled_image = scale(pos, scaleFactor);

    float4 result = scaled_image;

    // Write the result to the output image
    dst() = result;
  }
};