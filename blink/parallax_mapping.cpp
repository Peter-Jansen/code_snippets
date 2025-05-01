// rudimentary parallax mapping by Peter Jansen.
// can displace pixels in 3d using normals, world position, a displacement value and a camera.
kernel ParallaxMapping:
ImageComputationKernel<ePixelWise>{

Image<eRead, eAccessPoint> P;
Image<eRead, eAccessPoint> N;
Image<eRead, eAccessPoint> displacement;
Image<eRead, eAccessRandom, eEdgeClamped> col;
Image<eWrite, eAccessRandom, eEdgeClamped> dst;

param:
    float4x4 camera_matrix;
    float haperture;
    float focal;
    float pixel_aspect;
    float cam_near;
    float cam_far;
local:
    int _w, _h;
    int src_w, src_h;
    float image_aspect;
    // projection matrix stuff. This is copying the one nuke provides in nukescripts.snap3d.py
    float4x4 p_matrix, p_matrix_prev, p_matrix_next;         // projection matrix
    float4x4 t;         // translate projected points into normalsed pixel coords (from 0,0 to -2,2 instead of -1,-1 to 1,1)
    float4x4 s;         // scale normalised screen coords to actual pixel coords
    float4x4 w2s;       // final world to screen matrix. This matches the one provided by this node (ParticleBlinkScriptRender),


float4x4 projectionMatrix(
        const float focalLength,
        const float horizontalAperture,
        const float nearPlane,
        const float farPlane)
{
    float farMinusNear = farPlane - nearPlane;
    return float4x4(
        2 * focalLength / horizontalAperture, 0, 0, 0,
        0, 2 * focalLength / horizontalAperture, 0, 0,
        0, 0, -(farPlane + nearPlane) / farMinusNear, -2 * (farPlane * nearPlane) / farMinusNear,
        0, 0, -1, 0
    );
}

float2 transform( float3 p){
float4 r = w2s*float4(p.x, p.y, p.z, 1.0);
return float2(r.x, r.y)/r.w;
}



void init(){
	_w = dst.bounds.width();
    _h = dst.bounds.height();
	image_aspect = float(_h) / float(_w);
    // camera projection matrix	
	float4x4 p_matrix = projectionMatrix(focal, haperture, cam_near, cam_far);
	
	// Matrix to scale normalised pixel coords into actual pixel coords.
	float x_scale = float(_w) * 0.5;
    float y_scale = x_scale *pixel_aspect;
    s.setIdentity();
    s.scale(float4(x_scale, y_scale, 1.0f,1.0f));
	
    // Matrix to translate the projected points into normalised pixel coords
    t.setIdentity();
    t.translate(float4(1.0, 1.0 - (1.0 - image_aspect / pixel_aspect), 0.0f, 1.0f));	
	
    // view matrix is inverted camera world matrix
    float4x4 view =  camera_matrix.invert();

	w2s = s * t * p_matrix * view;
}

void process(int2 pos){
	
	float3 P2 = float3( P().x, P().y, P().z);
	float3 norm = float3( N().x, N().y, N().z);
	float disp = displacement().x;
	
	float3 disp_p = P2 + (norm * disp);
	
	float2 pos2d = transform(disp_p);
	
	
	dst(pos2d.x,pos2d.y) = col(pos.x, pos.y);
}
};
