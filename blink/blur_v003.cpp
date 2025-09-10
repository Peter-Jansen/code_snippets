kernel blur : ImageComputationKernel<ePixelWise> {

  Image<eRead, eAccessRandom, eEdgeClamped> src;
  Image<eWrite, eAccessRandom, eEdgeClamped> dst;


param:

float size;
int quality;


local:
  void define() {
    defineParam(size, "size", 10.0f);
    defineParam(quality, "quality", 10);
  }
    int _w, _h;
    int int_size;
    int size_half;
    int _filterSize;


  void init() {
    _w = dst.bounds.width();
    _h = dst.bounds.height();
    size = max(1.0, size);
    quality = max(1.0, quality);
    int_size = int(size);
    size_half = int(size/2);
    _filterSize = (size_half * 2) * (size_half * 2); 

  }

float4 box_blur(int sx, int sy){
        int2 xy = int2(sx,sy);
        int iteration_size = int(( size / quality ) / 2.0 );
        int start_x = xy.x - iteration_size;
        int start_y = xy.y - iteration_size;
        int end_x = xy.x + iteration_size;
        int end_y = xy.y + iteration_size;

        float4 sum = float4(0.0);
        int iters = 0;
        for (int x = start_x; x < end_x; x+=quality){
            for (int y = start_y; y < end_y; y+=quality){
                sum += src(x,y);
                iters++;
            }
        }
        return sum/float(iters);
}


  void process(int2 pos) {
    float4 p0, p1, p2, p3;


    if(pos.x % quality == 0 && pos.y % quality==0){
        dst(pos.x,pos.y) = float4(1.0);
        // p0 = src(pos.x, pos.y);
        // p1 = src(pos.x + int_size,  pos.y);
        // p2 = src(pos.x,  pos.y + int_size);
        // p3 = src(pos.x + int_size,  pos.y + int_size);

        p0 = box_blur(pos.x, pos.y);
        p1 = box_blur(pos.x + quality,  pos.y);
        p2 = box_blur(pos.x,  pos.y + quality);
        p3 = box_blur(pos.x + quality,  pos.y + quality);
        int end_x = pos.x + quality;
        int end_y = pos.y + quality;

        for (int x = pos.x; x < end_x; x++){
            for (int y = pos.y; y < end_y; y++){
                float fx = float(x - pos.x) / quality;
                float fy = float(y - pos.y) / quality;
                //fx = 1.0f-fx;
                //fy = 1.0f-fy;

                float4 result = p0 * (1.0f - fx) * (1.0f - fy) +
                    p1 * fx * (1.0f - fy) +
                    p2 * (1.0f - fx) * fy +
                    p3 * fx * fy;



                dst(x,y) = result;                    


            }
        }

    }

        
        // int start_x = pos.x - size_half;
        // int start_y = pos.y - size_half;
        // int end_x = pos.x + size_half;
        // int end_y = pos.y + size_half;
        // float4 sum = float4(0.0);
        // for (int x = start_x; x <= end_x; x++){
        //     for (int y = start_y; y <= end_y; y++){
        //         sum += src(x,y);
        //     }
        // }
        // dst(pos.x, pos.y) = sum/_filterSize;
            

        // dst(pos.x, pos.y) = box_blur(pos.x, pos.y);
                




  }
};
