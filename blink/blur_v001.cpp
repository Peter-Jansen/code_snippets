kernel blur : ImageComputationKernel<ePixelWise> {

  Image<eRead, eAccessRandom, eEdgeClamped> src;
  Image<eWrite, eAccessRandom, eEdgeClamped> dst;


param:

float size;
float quality;


local:
  void define() {
    defineParam(size, "size", 10.0f);
    defineParam(quality, "quality", 10.0f);
  }
    int _w, _h;

  void init() {
    _w = dst.bounds.width();
    _h = dst.bounds.height();
    size = max(1.0, size);
  }


  void process(int2 pos) {
    float4 p0, p1, p2, p3;
    int int_size = int(size);

    if(pos.x % int_size == 0 && pos.y % int_size==0){
        dst(pos.x,pos.y) = float4(1.0);
        p0 = src(pos.x, pos.y);
        p1 = src(pos.x + int_size,  pos.y);
        p2 = src(pos.x,  pos.y + int_size);
        p3 = src(pos.x + int_size,  pos.y + int_size);
        int end_x = pos.x + int_size;
        int end_y = pos.y + int_size;

        for (int x = pos.x; x <= end_x; x++){
            for (int y = pos.y; y <= end_y; y++){
                float fx = float(x - pos.x) / int_size;
                float fy = float(y - pos.y) / int_size;
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

    //dst() = bilinear(src, pos.x, pos.y);
  }
};
