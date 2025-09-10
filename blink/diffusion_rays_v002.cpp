kernel DiffusionRays : ImageComputationKernel<ePixelWise> {

  Image<eRead, eAccessRandom, eEdgeClamped> src;
  Image<eWrite, eAccessRandom, eEdgeClamped> dst;


param:

float size;
int global_quality;
float2 center;
float scaleFactor;



local:
  void define() {
    defineParam(size, "size", 10.0f);
    // defineParam(quality, "quality", 10);
    // defineParam(blur_undersampling, "blur_undersampling", 1);
  }
    int _w, _h;
    int _filterSize;
    // int quality;
    int blur_undersampling;

  void init() {
    _w = dst.bounds.width();
    _h = dst.bounds.height();
    size = max(1.0, size);
    //quality = max(1.0, size/global_quality);
    blur_undersampling = max(1.0, size/global_quality);
  }

float4 box_blur(int sx, int sy, float size){

        int i_size = int(size *0.5 / blur_undersampling);
        float4 sum = float4(0.0f);
        int count = 0;
        for (int x = -i_size; x < i_size; x++){
            for (int y = -i_size; y < i_size; y++){
                sum += src(x*blur_undersampling + sx ,y*blur_undersampling + sy);
                // sum += bilinear(src,x*blur_undersampling + sx, y*blur_undersampling + sy  );
                count++;
            }
        }
        return sum/float(count);
}
float2 scale(int2 pos, float amount){
        // Calculate the offset from the center point
    float dx = pos.x - center.x;
    float dy = pos.y - center.y;

    // Apply inverse scaling to map the current pixel to the source pixel
    float srcX = center.x + (dx / amount);
    float srcY = center.y + (dy / amount);

    return float2(srcX, srcY);
}

float4 gridBlur(int2 pos ){
    float4 p0, p1, p2, p3;
    float2 s = scale(pos, 1.0/scaleFactor);
    float scaled_size = size * scaleFactor;
    int quality = max(1.0, size/global_quality);
    int2 newpos = int2(int(s.x), int(s.y));
    if(pos.x % quality == 0 && pos.y % quality==0){
        p0 = box_blur(newpos.x, newpos.y, scaled_size);
        p1 = box_blur(newpos.x + quality,  newpos.y, scaled_size);
        p2 = box_blur(newpos.x,  newpos.y + quality, scaled_size);
        p3 = box_blur(newpos.x + quality,  newpos.y + quality, scaled_size);
        int end_x = pos.x + quality;
        int end_y = pos.y + quality;

        for (int x = pos.x; x < end_x; x++){
            for (int y = pos.y; y < end_y; y++){
                float fx = float(x - pos.x) / quality;
                float fy = float(y - pos.y) / quality;

                float4 result = p0 * (1.0f - fx) * (1.0f - fy) +
                    p1 * fx * (1.0f - fy) +
                    p2 * (1.0f - fx) * fy +
                    p3 * fx * fy;

                dst(x,y) = result;   
            }
        }
    }
}

  void process(int2 pos) {
    
    // float2 s = scale(pos, scaleFactor);
    // int2 newpos = int2(int(s.x), int(s.y));
    gridBlur(pos); 


    }
 };

    