

kernel blink_render : ImageComputationKernel<ePixelWise> {

    Image<eRead, eAccessRandom, eEdgeConstant> src; 
    Image<eRead, eAccessRandom, eEdgeConstant> src_prev;  
    Image<eRead, eAccessRandom, eEdgeConstant> src_next;  
    Image<eWrite, eAccessRandom, eEdgeClamped> dst; 

param:
    float _size;
    float _minSize;
    float softness;
    int _merge_operation;

    float4x4 camera_matrix;
    float4x4 camera_matrix_prev;
    float4x4 camera_matrix_next;

    float haperture;
    float focal;
    float fStop;
    float focus_distance;
    float pixel_aspect;
    float cam_near;
    float cam_far;
    int scene_units;
    int enable_dof;


    int _duplicates_num;
    float _duplicate_radius;

    int _maxSteps;
    float _spacing;
    float _shutter;
    float clip_padding;

    float near;
    float near_falloff;
    float far;
    float far_falloff;
    float falloff_gamma;

    int _diagnosticMode;
local:

    int _w, _h;
    int src_w, src_h;
    int sqSize;
    float image_aspect;
    float pixelsPerWorldUnit;
    float scene_multiplier;
    float scene_focus_distance;

    // projection matrix stuff. This is copying the one nuke provides in nukescripts.snap3d.py
    float4x4 p;         // projection matrix
    float4x4 t;         // translate projected points into normalsed pixel coords (from 0,0 to -2,2 instead of -1,-1 to 1,1)
    float4x4 s;         // scale normalised screen coords to actual pixel coords
    float4x4 w2s;       // final world to screen matrix. This matches the one provided by this node (ParticleBlinkScriptRender),
    float4x4 w2s_prev;  // world to screen matrix of previous frame
    float4x4 w2s_next;  // world to screen matrix of next frame

    float4x4 clip_matrix;
    float4x4 clip_matrix_prev;
    float4x4 clip_matrix_next;



  void define(){

    defineParam(_size, "paSize", 3.0f);
    defineParam(_minSize, "minSize", 1.0f);
    defineParam(softness, "softness", 1.0f);
    defineParam(_merge_operation, "merge_operation", 0);

    defineParam(camera_matrix, "camera_matrix", float4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f) );
    defineParam(haperture, "haperture", 24.576f);
    defineParam(focal, "focal", 24.0f);
    defineParam(fStop, "fStop", 5.6f);
    defineParam(focus_distance, "focus_distance", 100.0f);
    defineParam(pixel_aspect, "pixel_aspect", 1.0f);
    defineParam(cam_near, "cam_near", 0.1f);
    defineParam(cam_far, "cam_far", 1000.0f);
    defineParam(scene_units, "scene_units", 2);
    defineParam(enable_dof, "enable_dof", 0);

    defineParam(_duplicates_num, "_duplicates_num", 0);
    defineParam(_duplicate_radius, "duplicate_radius", 0.5f);

    defineParam(_maxSteps, "Steps", 10);
    defineParam(_spacing, "spacing", 1.0f);
    defineParam(_shutter, "_shutter", 0.5f);
    
    defineParam(clip_padding, "clip_padding", 0.0f);

    //fade params
    defineParam(near, "near", 0.0f);
    defineParam(near_falloff, "near_falloff", 0.0f);
    defineParam(far, "far", 1000.0f);
    defineParam(far_falloff, "far_falloff", 0.0f);
    defineParam(falloff_gamma, "falloff_gamma", 1.0f);

    defineParam(_diagnosticMode, "_diagnosticMode", 0);
  }

  float4 srcOver( float4 a, float4 b ) {
    return (1.0f-a.w)*b + a;
  }

  float2 transform( float3 p,float4x4 world_to_screen )
  {
    float4 r = world_to_screen*float4(p.x, p.y, p.z, 1.0);
    return float2(r.x, r.y)/r.w;
  }
float3 transformz( float3 p, float4x4 world_to_screen ){
    float4 r = world_to_screen*float4(p.x, p.y, p.z, 1.0);
    float2 xy = float2(r.x, r.y)/r.w;
    float depth = r.w;
    return float3(xy, depth);
  }  

float4 toClipSpace(float3 P, float4x4 clip_matrix){
    float4 homogeneous_P = float4(P.x, P.y, P.z, 1.0);
    return clip_matrix*homogeneous_P;
}

float3 clipToNDC(float4 clip){
    float4 clip_to_screen = s * t * clip;
    return (float3(clip_to_screen.x/clip_to_screen.w, clip_to_screen.y/clip_to_screen.w, clip_to_screen.w));      // but keep w coordinate for depth. I like it more.. lol.
}

// Function to check if a 4D position is within the frustum with padding
bool isInsideFrustum(float4 position, float padding) {

    // Clip space coordinates
    float x = position.x;
    float y = position.y;
    float z = position.z;
    float w = position.w;

    // Frustum boundaries with padding
    bool insideX = (x >= -w - padding) && (x <= w + padding);
    bool insideY = (y >= -w - padding) && (y <= w + padding);
    bool insideZ = (z >= -w) && (z <= w);
    // bool insideZ = (z >= 0) && (z <= w);
    return insideX && insideY && insideZ; 
}

bool isInsideFrustumWithFlip(float4 position, float padding) {
    // Clip space coordinates
    float x = position.x;
    float y = position.y;
    float z = position.z;
    float w = position.w;

    // Correct flipped coordinates if behind the camera
    if (w < 0) {
        z = -z;
        w = -w;
    }

    // Frustum boundaries with padding
    bool insideX = (x >= -w - padding) && (x <= w + padding);
    bool insideY = (y >= -w - padding) && (y <= w + padding);
    bool insideZ = (z >= -w) && (z <= w);

    return insideX && insideY && insideZ;
}



  float smoothstep( float a, float b, float x ) {
    float t = clamp((x - a) / (b - a), 0.0, 1.0);
    return t*t * (3.0f - 2.0f*t);
  }

  // overloaded lerp with float, float2 and float3
    float lerp(float a, float b, float t){
        return a + t * (b-a);
  }
    float2 lerp(float2 a, float2 b, float t) {
        return a + t * (b - a);
  }
    float3 lerp(float3 a, float3 b, float t) {
        return a + t * (b - a);
    }
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

// thanks Erwan Leroy - https://erwanleroy.com/making-3d-lightning-in-nuke-using-blinkscript/
inline float fract (float x) {return x-floor(x);}
inline float random(float co) { return fract(sin(co*(91.3458f)) * 47453.5453f); }

inline float3 randomv(float3 seed){
  float scramble = random(seed.x + seed.y * seed.z);
  float3 rand;
  rand.x = random(seed.x + scramble + 0.14557f + 0.47917f * seed.z)*2-1;
  rand.y = random(seed.y * 0.214447f + scramble * 47.241f * seed.x)*2-1;
  rand.z = random(seed.z * scramble + 3.147855f + 0.2114f * seed.y)*2-1;
  return normalize(rand);
}

inline float3 randomv2(float seed){
    //float scramble = random(seed + seed * seed);
    float3 rand;
    rand.x = random(seed + 0.215568f)*2.0-1.0;
    rand.y = random(seed + 2.112408f)*2.0-1.0;
    rand.z = random(seed + 68.13384f)*2.0-1.0;
    return rand;
}




  void init() {
    _w = dst.bounds.width();
    _h = dst.bounds.height();
    src_w = src.bounds.width();
    src_h = src.bounds.height();
    sqSize = src_w / 2;

    image_aspect = float(_h) / float(_w);
    pixelsPerWorldUnit = (_w*0.5) / haperture;

    // camera projection matrix
    p = projectionMatrix(focal, haperture, cam_near, cam_far);
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
    float4x4 view_prev =  camera_matrix_prev.invert();
    float4x4 view_next =  camera_matrix_next.invert();
    
    clip_matrix = p * view;
    clip_matrix_prev = p * view_prev;
    clip_matrix_next = p * view_next;

    // world to screen matrix is the concatenation of above matrices, from right to left.
    // TO DO: maybe add screen scale/roll all that crap that the snap3d projection matrix provides. Could be useful for overscan..?
    w2s = s * t * p * view;
    w2s_prev = s * t * p * view_prev;
    w2s_next = s * t * p * view_next;


    float cm = 10.0f;
    float dm = 100.0f;
    float m = 1000.0f;
    float inch = 25.4f;
    float feet = 304.8f;
    
    scene_multiplier = (scene_units == 1) ? cm :
                   (scene_units == 2) ? dm :
                   (scene_units == 3) ? m :
                   (scene_units == 4) ? inch :
                   (scene_units == 5) ? feet : 1;
    scene_focus_distance = focus_distance * scene_multiplier;
    
  }

// Thanks Jed Smith for OpticalZDefocus - circle of confusion math taken from there.
float CoC(float depth){
    float scene_depth = depth * scene_multiplier;
    float coc = (fabs(scene_focus_distance - scene_depth) * pow(focal, 2) / (fStop * scene_depth * (scene_focus_distance - focal)));
    float coc_pixels = fabs(coc / haperture * float(_w) );
    return coc_pixels;
}

// variation of the renderPoint function from the default code inside the ParticleBlinkScriptRender node. 
// Modified to do CoC calculations and take in a falloff
void renderPoint( float2 xy, float4 pcolor, float pointSize, float coc, float falloff )
{
    float effectiveRadius;
    float effectiveRadius2;
    float energyScale;
    // Effective radius (original particle size plus defocus)
    if (enable_dof){
        effectiveRadius = pointSize + coc;
        effectiveRadius2 = effectiveRadius * effectiveRadius;
        energyScale = (pointSize * pointSize) / (effectiveRadius2);
    }
    else{
        effectiveRadius = pointSize;
        effectiveRadius2 = pointSize*pointSize;
        energyScale = 1.0f;
    }


    int intRadius = int(ceil(effectiveRadius));
    int minX = max(0, int(xy.x) - intRadius);
    int maxX = min(_w - 1, int(xy.x) + intRadius);
    int minY = max(0, int(xy.y) - intRadius);
    int maxY = min(_h - 1, int(xy.y) + intRadius);

    float soft_thresh = effectiveRadius2 * (1.0 - softness);

    // Loop over all pixels in the bounding box.
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float2 p = float2(x, y);
            float2 d = p - xy;
            float r2 = dot(d, d);

            if (r2 < effectiveRadius2) {
                float w = 1.0 - smoothstep(soft_thresh, effectiveRadius2, r2);
                w *= energyScale * falloff;

                // additive  
                if (_merge_operation == 0) { 
                    dst(p.x, p.y) += pcolor * w;
                }

                // over
                if (_merge_operation == 1) {
                    float4 colourToAdd = pcolor * w;
                    float4 existingColour = dst(p.x,p.y);
                    dst(p.x, p.y) = srcOver(colourToAdd, existingColour);
                }
            }
        }
    }
}

// Copilot "think deeper" finally gave me a function that will make a curve between 3 points
// and ensure that the curve goes through all three. Thanks little AI!
float2 quadraticInterpolation(float2 p0, float2 p1, float2 p2, float t) {
    // Lagrange basis polynomials
    float l0 = (t - 0.5f) * (t - 1.0f) / ((0.0f - 0.5f) * (0.0f - 1.0f));
    float l1 = (t - 0.0f) * (t - 1.0f) / ((0.5f - 0.0f) * (0.5f - 1.0f));
    float l2 = (t - 0.0f) * (t - 0.5f) / ((1.0f - 0.0f) * (1.0f - 0.5f));
    
    // Interpolated point
    return l0 * p0 + l1 * p1 + l2 * p2;
}

  float minMaxSmooth(float n,float ns,float f,float fs,float v){
      // input variables are near, near soft, far, far soft and value
      float output = 0.0;
      float near = min(n, f);
      float near_soft = max(ns, 0.0);
      float far = max(n, f);
      float far_soft = max(fs, 0.0);

      float near_soft_absolute = near-near_soft;
      float far_soft_absolute = far+far_soft;

      if (v <= near){
        output = smoothstep(near_soft_absolute,near,v);
        }
      else{
        output = 1-smoothstep(far,far_soft_absolute,v);
        }
      return output;
  }


    float screenSpaceSize(float focal, float pixelsPerWorldUnit, float p_size, float distance){
        float particleSizeOnImagePlane = (focal * p_size) / distance;
        float particleSizeInPixels = (particleSizeOnImagePlane * pixelsPerWorldUnit);
    return particleSizeInPixels;
  }  


  void process(int2 pos) {

    
    // blink runs on each pixel of the dst image. Every pixel, we'll sample the corresponding pixel in the src image (cache).
    // But in the unlikely but not impossible scenario that the cache is larger than the destination image, then we won't hit
    // those samples in the cache (say the cache is 2048x2048 and dst image is 1280x720. The cache is larger in both dimensions.
    /*
                _______________________              _______________________       
                |                     |              |\\\\\\\\\\\\\\\\\\\\\|       
                |         DST         |              |\\\\\\\ CACHE \\\\\\\|       
                |__________           |              |\\\\\\\\\\\\\\\\\\\\\|       
    Normally:   |         |           |   Oversized  |               |\\\\\|       
                |  CACHE  |           |     Cache:   |      DST      |\\\\\|       
                |         |           |              |               |\\\\\|       
                |_________|___________|              |_______________|_____|       
    */
    // The pixels in the shaded area of the cache will never be sampled.
    // So we set up some batching, so that multiple pixels of the cache image will be sampled per dst image.
    // We lose a little bit of concurrency, but there is an added benefit...
    // There are fewer GPU scheduling conflicts causing artifacts when samples are overlapping in the 2d image.
    // For this reason, we can also expose a batch size override, higher values = slower but less artifacts.
    // Still, CPU gives best, artifact free results.

    int batch_size_x = ceil( float(src_w) / float(_w)); 
    int batch_size_y = ceil( float(src_h) / float(_h)); 
    // batch_size_x = 3;
    // batch_size_y = 3;
    int2 batch_start = int2 ( pos.x * batch_size_x, pos.y * batch_size_y);
    int2 batch_end = int2( batch_start.x + (batch_size_x-1), batch_start.y + (batch_size_y-1));

    if (batch_start.x < src_w && batch_start.y < src_h){

    for (int cache_x = batch_start.x; cache_x <= batch_end.x; cache_x++){
        if (cache_x > src_w) {break;}
	    for (int cache_y = batch_start.y; cache_y <= batch_end.y; cache_y++){
            if (cache_y > src_h) {break;} 
    

    int id = int(src(cache_x, cache_y).w);
    if(id != 0){                                             // also don't bother if the id=0, ie, there's no particle
    int id_prev = int(src_prev(cache_x, cache_y).w);
    int id_next = int(src_next(cache_x, cache_y).w);
    

    float4 read_pos_id = src(cache_x, cache_y);
    float4 read_pos_id_prev = src_prev(cache_x, cache_y);    
    float4 read_pos_id_next = src_next(cache_x, cache_y);   

    //float4 read_col = col(pos.x, pos.y);
    //float4 read_col_prev = col_prev(pos.x, pos.y);
    //float4 read_col_next = col_next(pos.x, pos.y);
    
    //int colour_pixel_x_offset = src_w * 0.5;
    //float4 read_col = src(colour_pixel_x_offset + pos.x, pos.y);

    float3 P = float3(read_pos_id.x, read_pos_id.y, read_pos_id.z);
    float3 P_prev = float3(read_pos_id_prev.x, read_pos_id_prev.y, read_pos_id_prev.z);
    float3 P_next = float3(read_pos_id_next.x, read_pos_id_next.y, read_pos_id_next.z);

    // if (id_prev != id){                                      // if the particle is new, ie, it didn't exist last frame.
    //     float3 forward_v = P - P_next;
    //     P_prev = P;
    //     P = lerp(P, P_next, 0.5);
    // }
    // if (id_next != id){                                     // if the particle is about to die, ie, won't exist next frame
    //     P_next = P;
    //     P = lerp(P, P_prev, 0.5);
    // }
    if (id_prev != id){                                      // if the particle is new, ie, it didn't exist last frame.
        float3 vel = P_next - P;
        P_prev = P - vel;
    }
    if (id_next != id){                                     // if the particle is about to die, ie, won't exist next frame
        float3 vel =  P - P_prev;
        P_next = P+vel;
    }



    float dist = transformz(P, w2s).z;

    if (dist>0.0f){

        for (int pt = 0; pt <= _duplicates_num; pt++){            // duplicate points

        if (pt !=0){
            P += randomv2(float(pt) + float(id)*0.02)*_duplicate_radius;
            P_prev += randomv2(float(pt) + float(id)*0.02)*_duplicate_radius;
            P_next += randomv2(float(pt) + float(id)*0.02)*_duplicate_radius;
        }

        // this part is  a mess. 

        // Move P to clip space
        float4 clip = toClipSpace(P, clip_matrix);
        float4 clip_prev = toClipSpace(P_prev, clip_matrix_prev);
        float4 clip_next = toClipSpace(P_next, clip_matrix_next);

        // very simple clip check
        
        // if (!isInsideFrustum(clip,clip_padding) && !isInsideFrustum(clip_prev,clip_padding) && !isInsideFrustum(clip_next,clip_padding)){
        //     break;
        // }

        // if (!isInsideFrustumWithFlip(clip,clip_padding) && !isInsideFrustumWithFlip(clip_prev,clip_padding) && !isInsideFrustumWithFlip(clip_next,clip_padding)){
        //     break;
        // }

        //if (!isInsideFrustum(clip,clip_padding)) break;
        // if (!isInsideFrustum(clip_next,clip_padding)) break;
        //if (!isInsideFrustum(clip_prev,clip_padding)) break;



        // move to NDC space
        float3 ndc = clipToNDC(clip);
        float3 ndc_prev = clipToNDC(clip_prev);
        float3 ndc_next =  clipToNDC(clip_next);

        float4 clip_to_screen = s * t * clip;
        float z_divide = clip.z / clip_to_screen.w;

        float2 xy = float2(ndc.x, ndc.y);
        float2 xy_prev = float2(ndc_prev.x, ndc_prev.y);
        float2 xy_next = float2(ndc_next.x, ndc_next.y);


        float size = max(_minSize,min(100.0f, screenSpaceSize(focal, pixelsPerWorldUnit, _size, ndc.z)));
        float size_prev = min(100.0f, screenSpaceSize(focal, pixelsPerWorldUnit, _size, ndc_prev.z));
        float size_next = min(100.0f, screenSpaceSize(focal, pixelsPerWorldUnit, _size, ndc_next.z));



        float coc = 0.0;
        if (enable_dof){
        coc = max(0.0,min(200.0f, CoC(ndc.z)));
        }

        float blur_length = (length(xy - xy_prev) + length(xy - xy_next) * _shutter ); // approximate
        int steps = max(1 , min(_maxSteps, int(ceil(blur_length/max(2.0f, min(size_prev, size_next)+coc)*2.0/_spacing))));

        float shutter_start = 0.5 - (_shutter*0.25);
        float shutter_end = 0.5 + (_shutter*0.25);

        float depth_fade = minMaxSmooth(near, near_falloff, far, far_falloff, ndc.z);
        depth_fade = pow(depth_fade, falloff_gamma);
        float4 render_col = float4(float3(1.0), 1.0);// /float(steps);

        //render_col = clip_prev;
        //render_col = float4(clip_next.x, clip_next.y, clip_next.z, id) ;
        //render_col = float4(0.0, 1.0, 0.0, 1.0);


        if (_shutter==0.0 || steps <= 1){
            renderPoint(xy,render_col , size, coc, depth_fade );
        }
        else{
            for (int step = 0; step <= steps; step++){
                float step_position = float(step)/float(steps);
                float shutter_step = lerp(shutter_start, shutter_end, step_position);
                float2 xy_render = quadraticInterpolation(xy_prev, xy, xy_next, shutter_step);
                //float size_render = lerp(size_prev, size_next, step_position);
                renderPoint(xy_render,render_col , size, coc, depth_fade );

            }
        }

        if (_diagnosticMode){
            
        float4 col = float4(1.0, 1.0, 1.0, 1.0);
        float4 red = float4(1.f, 0.f, 0.f, 1.f);
        float4 green = float4(0.f, 1.f, 0.f, 1.f);
        float4 blue = float4(0.f, 0.f, 1.f, 1.f);
        renderPoint(xy, green, size, coc, 1.0);
        renderPoint(xy_prev, red, size, coc, 1.0);
        renderPoint(xy_next, blue, size, coc, 1.0);

        //dst(10,1) = float4(coc);



        //renderPoint(xy_next, blue, size, 1.0);
        //renderPoint(xy_next, blue, size, 1.0);
        }

        
        }
    }
  }


  }} // end of batch loops




    } // end of outside of the src bounds condition
  }  // end of process()
  }; // end of kernel