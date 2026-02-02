kernel TaupoDistort2D : ImageComputationKernel<ePixelWise>
{

    Image<eRead, eAccessPoint, eEdgeClamped> src;
    Image<eRead, eAccessRandom, eEdgeClamped> uv;
    Image<eWrite, eAccessPoint, eEdgeClamped> dst;


    param:  
        float _amount;
        float4x4 camera_matrix;
        float focalLength;
        float horizontalAperture;
        float nearPlane;
        float farPlane;
        float pixel_aspect;
        int mode;

    local:
       
        float4x4 worldToScreen;
        float4x4 view;
        int _w, _h;     

    void define() {
        defineParam(_amount, "paAmount", 0.0f);
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
        _w = uv.bounds.width();
        _h = uv.bounds.height();
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

        view =  camera_matrix;
        view.invert();

        worldToScreen = s * t * p * view;

    }




  
    void process() {

        float3 P = float3(src().x, src().y, src().z);
        // float3 P = p_position();
        float3 w2s = transform(P);
        float2 samplepos = float2(w2s.x, w2s.y);
        float depth = w2s.z;

        // direct values from image
        float4 sample_vel = uv(samplepos.x, samplepos.y);


        // if screen space mode, multiply by depth and transform by camera matrix.
        if (mode==0){       
            sample_vel *=  depth;
            sample_vel.w = 0.0;
            sample_vel = camera_matrix * sample_vel;
        }

        sample_vel *= _amount;
        dst() = src() + float4(sample_vel.x, sample_vel.y, sample_vel.z, 0.0);

    }
};
