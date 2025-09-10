kernel ParticleRenderKernel : ImageComputationKernel<ePixelWise>
{
    Image<eRead, eAccessRandom,eEdgeClamped> p_position;
    Image<eRead, eAccessRandom> p_id;

    Image<eRead, eAccessRandom> p_color;


    Image<eRead, eAccessRandom> p_size;
    Image<eRead, eAccessRandom> p_life;
    Image<eRead, eAccessRandom> p_startTime;


    Image<eWrite, eAccessRandom, eEdgeClamped> output;

    param:
        float4x4 _worldToScreen;
        float _size;
        int outputMode;
        float _systemTime;

    local:
        int _w, _h;
        int _numParticles;
        int sqSize;



    void define() {
        defineParam(_worldToScreen, "_worldToScreen", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f) );
        defineParam(_size, "paSize", 3.0f);
        defineParam(_systemTime, "_systemTime", 1.0f);
        defineParam(outputMode, "outputMode", 0);
    }

    float4 srcOver( float4 a, float4 b ) {
        return (1.0f-a.w)*b + a;
    }

    float2 transform( float3 p ){
        float4 r = _worldToScreen*float4(p.x, p.y, p.z, 1.0);
        return float2(r.x, r.y)/r.w;
    }

    float smoothstep( float a, float b, float x ) {
        float t = clamp((x - a) / (b - a), 0.0, 1.0);
        return t*t * (3.0f - 2.0f*t);
    }

    void init()
    {
        _w = output.bounds.width();
        _h = output.bounds.height();
        _numParticles = p_position.bounds.height();
        sqSize = ceil( sqrt(_numParticles) );
  }

  void process(int2 pos) {

    //sq num

    int id = p_id(0, pos.y);
    int id_loop = fmod(id, _w*_h);
    int x = fmod(id_loop, _w);
    int y = floor(id_loop / _h);


    // position and ID
    if (outputMode == 0) {
        float written_id = output(x,y).w;
        if (written_id < id) {
        output(x, y)= float4(p_position(0, pos.y), id);            
        }
   }

    // colour
    if (outputMode == 1) {
        output(x, y)= p_color(0, pos.y);            
    }

    // utility: size, age, life
    
    if (outputMode == 2) {
    float size = p_size(0, pos.y)[0];
    float age = _systemTime-p_startTime(0, pos.y);
    float life = p_life(0, pos.y);
    float nage = 1-(age/life);
    output(x, y)= float4(size, age, life, nage);            
    }

    

    //int x_id = x + _w*0.5;
    //output(x_id, y) = float(p_id(0, pos.y));
    //output(1,1) = sqSize;


    // colour
    //int x_col = x + _w*0.5;
    //output(x_col, y) = p_color(0, pos.y);
  }
};
