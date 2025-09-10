// Taupo Combine

kernel Taupo_Combine_Cache : ImageComputationKernel<ePixelWise> {
    Image<eRead, eAccessRandom, eEdgeClamped> format_img; 
    Image<eRead, eAccessRandom, eEdgeClamped> Cache_A; 
    Image<eRead, eAccessRandom, eEdgeClamped> Cache_B; 
    Image<eWrite, eAccessPoint> dst; 

    local:
    int format_w, format_h;
    int Cache_A_w, Cache_A_h;
    int Cache_B_w, Cache_B_h;
    int dst_w, dst_h;

    int Cache_A_count;
    int Cache_B_count;


    void init(){
        format_w = format_img.bounds.width();
        format_h = format_img.bounds.height();
        Cache_A_w = Cache_A.bounds.width();
        Cache_A_h = Cache_A.bounds.height();
        Cache_B_w = Cache_B.bounds.width();
        Cache_B_h = Cache_B.bounds.height();
        dst_w = dst.bounds.width();
        dst_h = dst.bounds.height();    
        
        Cache_B_count = Cache_B_w * Cache_B_h;
    }


    void process(int2 pos){
        int id = pos.x + (pos.y * format_w);
        int2 Cache_B_sample;
        int2 Cache_A_sample;
        
        if(id < Cache_B_count) {
            Cache_B_sample.y = floor( id / Cache_B_w  );
            Cache_B_sample.x = id - (Cache_B_sample.y *  Cache_B_w);
            dst() = Cache_B(Cache_B_sample.x, Cache_B_sample.y);
            // Copy from Cache_B
        }
        else{
            id = id - Cache_B_count;
            Cache_A_sample.y = floor( id / Cache_A_w  );
            Cache_A_sample.x = id - (Cache_A_sample.y *  Cache_A_w);
            dst() = Cache_A(Cache_A_sample.x, Cache_A_sample.y);
        }

        // dst() = float4((float)Cache_B_sample.x, (float)Cache_B_sample.y, 0.0f, 0.0f);
    }








}; // kernel