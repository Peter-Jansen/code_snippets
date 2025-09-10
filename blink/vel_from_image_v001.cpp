kernel ParticleExampleKernel : ImageComputationKernel<ePixelWise>
{
  // Declare the particle attributes as Images:
  Image<eReadWrite> p_velocity;
  Image<eReadWrite> p_color;
  Image<eReadWrite> p_position;
  Image<eReadWrite> p_conditions;
  Image<eRead, eAccessRandom, eEdgeClamped> image_vel;

  // Declare our parameter storage
    param:  
        float _amount;
        float _dt;


        float4x4 camera_matrix;
        float focalLength;
        float horizontalAperture;
        float nearPlane;
        float farPlane;
        float pixel_aspect;

    local:
        // float4x4 p_matrix;
        // float4x4 t;         
        // float4x4 s;         
        float4x4 worldToScreen;
        int _w, _h;
        // float4x4 clip_matrix;       

    void define() {
        defineParam(_amount, "paAmount", 0.0f);
        defineParam(_dt, "_dt", 1.0f);
        defineParam(camera_matrix, "camera_matrix", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f) );
    }

  float3 transform( float3 p )
  {
    float4 r = worldToScreen*float4(p.x, p.y, p.z, 1.0);
    float2 xy = float2(r.x, r.y)/r.w;
    float depth = r.w;
    return float3(xy, depth);
  }

    void init(){
        _w = image_vel.bounds.width();
        _h = image_vel.bounds.height();
        float image_aspect = float(_h) / float(_w);
        float4x4 p, s, t;
        float farMinusNear = farPlane - nearPlane;

        // projection matrix
        p = float4x4(
            2 * focalLength / horizontalAperture, 0, 0, 0,
            0, 2 * focalLength / horizontalAperture, 0, 0,
            0, 0, -(farPlane + nearPlane) / farMinusNear, -2 * (farPlane * nearPlane) / farMinusNear,
            0, 0, -1, 0
        );

        // Matrix to scale normalised pixel coords into actual pixel coords.
        float x_scale = float(_w) * 0.5;
        float y_scale = x_scale *pixel_aspect;
        s.setIdentity();
        s.scale(float4(x_scale, y_scale, 1.0f,1.0f));

        // Matrix to translate the projected points into normalised pixel coords
        t.setIdentity();
        t.translate(float4(1.0, 1.0 - (1.0 - image_aspect / pixel_aspect), 0.0f, 1.0f));

        float4x4 view =  camera_matrix.invert();

        worldToScreen = s * t * p * view;

    }




  
    void process() {
        // p_velocity() *= (1.0f-_amount*_dt);
        float3 P = p_position();
        float3 w2s = transform(P);
        float2 samplepos = float2(w2s.x, w2s.y);
        float depth = w2s.z;
        // p_color() = float4(w2s, 1.0);

        float4 sample_vel = image_vel(samplepos.x, samplepos.y) * depth;
        // p_color() = sample_vel;
        p_velocity() += float3(sample_vel.x, sample_vel.y, 0.0f);

    }
};
