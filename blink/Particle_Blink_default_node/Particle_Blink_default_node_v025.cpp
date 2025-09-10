

kernel blink_render : ImageComputationKernel<ePixelWise> {

    Image<eRead, eAccessRandom, eEdgeConstant> src; 
    Image<eRead, eAccessRandom, eEdgeConstant> src_prev;  
    Image<eRead, eAccessRandom, eEdgeConstant> src_next;

    Image<eRead, eAccessRandom, eEdgeConstant> col; 
    Image<eRead, eAccessRandom, eEdgeConstant> util; 

    Image<eRead, eAccessRandom, eEdgeConstant> holdout; 

    

    // Image<eRead, eAccessRandom, eEdgeConstant> col_prev;  
    // Image<eRead, eAccessRandom, eEdgeConstant> col_next;    

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
    float focal, focal_prev, focal_next;
    float fStop;
    float focus_distance, focus_distance_prev, focus_distance_next;
    float pixel_aspect;
    float cam_near;
    float cam_far;
    int scene_units;
    int enable_dof;

    float cateye_amount;

    int _duplicates_num;
    float _duplicate_radius;

    int _maxSteps;
    int _minSteps;
    float _spacing;
    float _shutter;
    float clip_padding;

    float near;
    float near_falloff;
    float far;
    float far_falloff;
    float falloff_gamma;

    int _diagnosticMode;
    int _holdout_toggle;

    float _screenclip;

local:
    int _w, _h;
    int src_w, src_h;
    int sqSize;
    float image_aspect;
    float pixelsPerWorldUnit;
    float scene_multiplier;
    float scene_focus_distance;
    float scene_focus_distance_prev;
    float scene_focus_distance_next;
    float2 img_center;
    float img_radius;

    int batch_size_x;
    int batch_size_y;

    bool enable_cateye;

    // projection matrix stuff. This is copying the one nuke provides in nukescripts.snap3d.py
    float4x4 p_matrix, p_matrix_prev, p_matrix_next;         // projection matrix
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

    defineParam(cateye_amount, "cateye_amount", 0.0f);

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
    defineParam(_holdout_toggle, "holdout_toggle", 0);
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
    img_center = float2(float(_w) * 0.5, float(_h) * 0.5);
    img_radius = length(img_center);

    enable_cateye = enable_dof && cateye_amount != 0.0;

    // How many times does the cache fit into the dst image? Round to nearest integer.
    batch_size_x = ceil( float(src_w) / float(_w)); 
    batch_size_y = ceil( float(src_h) / float(_h)); 
    image_aspect = float(_h) / float(_w);
    pixelsPerWorldUnit = (_w*0.5) / haperture;

    // camera projection matrix
    p_matrix = projectionMatrix(focal, haperture, cam_near, cam_far);
    p_matrix_prev = projectionMatrix(focal_prev, haperture, cam_near, cam_far);
    p_matrix_next = projectionMatrix(focal_next, haperture, cam_near, cam_far);

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
    
    clip_matrix = p_matrix * view;
    clip_matrix_prev = p_matrix_prev * view_prev;
    clip_matrix_next = p_matrix_next * view_next;

    // world to screen matrix is the concatenation of above matrices, from right to left.
    // TO DO: maybe add screen scale/roll all that crap that the snap3d projection matrix provides. Could be useful for overscan..?
    w2s = s * t * p_matrix * view;
    w2s_prev = s * t * p_matrix_prev * view_prev;
    w2s_next = s * t * p_matrix_next * view_next;


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
    scene_focus_distance_prev = focus_distance_prev * scene_multiplier;
    scene_focus_distance_next = focus_distance_next * scene_multiplier;
    
  }

// Thanks Jed Smith for OpticalZDefocus - circle of confusion math taken from there.
float CoC(float depth, float f, float fd){
    float scene_depth = depth * scene_multiplier;
    float coc = (fabs(fd - scene_depth) * pow(f, 2) / (fStop * scene_depth * (fd - f)));
    float coc_pixels = fabs(coc / haperture * float(_w) );
    return coc_pixels;
}


// variation of the renderPoint function from the default code inside the ParticleBlinkScriptRender node. 
// Modified to do CoC calculations and take in a falloff
void renderPoint( float2 xy, float4 pcolor, float pointSize, float coc, float falloff, float depth )
{
    float effectiveRadius;
    float effectiveRadius2;
    float energyScale;
    float2 cat_vec;
    float cateye_limit;
    float2 cateye_xy;

    
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
    if (enable_cateye){
        cat_vec = float2(xy - img_center);

        cateye_limit = cateye_amount * effectiveRadius *2.0;

        cateye_xy = xy + (((xy - img_center) / img_radius) * cateye_limit);
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
            float2 cat;
            float cat2;
            bool render_mask; 


            if (enable_cateye){
                cat = p - cateye_xy;
                cat2 = dot(cat, cat);
                render_mask = r2 < effectiveRadius2 && cat2 < effectiveRadius2;
            }
            else{
                render_mask = r2 < effectiveRadius2;
            }

            if (render_mask) {

                float w = 1.0 - smoothstep(soft_thresh, effectiveRadius2, r2);
                if (enable_cateye){
                    float c = 1.0 - smoothstep( soft_thresh, effectiveRadius2, cat2);
                    w=w*c;
                }

                if (_holdout_toggle){
                float4 read_holdout = holdout(x,y);
                float holdout_depth = read_holdout.x;
                float holdout_alpha = read_holdout.w;
                if ( depth > holdout_depth ){
                    if (holdout_alpha >=.99999) break;
                    else w*= (1-holdout_alpha);
                }}

                w *= energyScale * falloff ;

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


//     float screenSpaceSize(float focal, float p_size, float distance){
//         float particleSizeOnImagePlane = (focal * p_size) / distance;
//         float particleSizeInPixels = (particleSizeOnImagePlane * pixelsPerWorldUnit);
//     return particleSizeInPixels;
//   }  

    float screenSpaceSize(float focal_length, float p_size, float distance){
        float particleSizeOnImagePlane = (focal_length * p_size) / distance;
        float particleSizeInPixels = (particleSizeOnImagePlane * pixelsPerWorldUnit);
    return particleSizeInPixels;
  }  

    float maxCoord(float2 xy){
        return max (fabs(xy.x-(_w*0.5)), fabs(xy.y-(_h*0.5)));
    }

float getOutOfBounds(float2 pos) {
    float outX = 0.0;
    float outY = 0.0;
    
    // Check left boundary (negative x)
    if (pos.x < 0.0) {
        outX = -pos.x / _w;
    }
    // Check right boundary (x exceeds width)
    else if (pos.x > _w) {
        outX = (pos.x - _w) / _w;
    }
    
    // Check bottom boundary (negative y)
    if (pos.y < 0.0) {
        outY = -pos.y / _h;
    }
    // Check top boundary (y exceeds height)
    else if (pos.y > _h) {
        outY = (pos.y - _h) / _h;
    }
    
    // Return the maximum normalized distance outside bounds
    return max(outX, outY);
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
    int2 batch_start;
    int2 batch_end;
    // if the batch size is 1, then no use "batching", just use the current pos in iteration space.
    if (batch_size_x==1 && batch_size_y==1){
        batch_start = int2(pos.x, pos.y);
        batch_end = batch_start;
    }
    // but if the batch size is larger, then establish where the batch coordinates are on the cache.
    else{
    batch_start = int2 ( pos.x * batch_size_x, pos.y * batch_size_y);
    batch_end = int2( batch_start.x + (batch_size_x-1), batch_start.y + (batch_size_y-1));
    }
    // if the batch start point is outside of the cache image, then we're guaranteed to just be sampling nothing, so we return.
    if (batch_start.x > src_w || batch_start.y > src_h) return; // should this be and or or... hmm. I think or??

    for (int cache_x = batch_start.x; cache_x <= batch_end.x; cache_x++){
        if (cache_x > src_w) break;
	    for (int cache_y = batch_start.y; cache_y <= batch_end.y; cache_y++){
            if (cache_y > src_h) break; 
    
    // Read position and ID for current frame
    float4 read_pos_id = src(cache_x, cache_y);
    int id = read_pos_id.w;
    // Return if the id=0, ie, there's no particle
    if(id == 0) return; 
    //Read position and ID for the prev and next frames
    float4 read_pos_id_prev = src_prev(cache_x, cache_y);    
    float4 read_pos_id_next = src_next(cache_x, cache_y);   
    int id_prev = read_pos_id_prev.w;
    int id_next = read_pos_id_next.w;
    
    // Read colour and utility data for just this frame
    float4 read_col = col(cache_x, cache_y);
    float4 read_util = util(cache_x, cache_y);

    // Read other utility pass attributes
    float world_size = read_util.x * _size;


    // Create P attributes (split from ID)
    float3 P = float3(read_pos_id.x, read_pos_id.y, read_pos_id.z);
    float3 P_prev = float3(read_pos_id_prev.x, read_pos_id_prev.y, read_pos_id_prev.z);
    float3 P_next = float3(read_pos_id_next.x, read_pos_id_next.y, read_pos_id_next.z);


    // Handle new and dying particles
    if (id_prev != id){                                      // if the particle is new, ie, it didn't exist last frame.
        float3 vel = P_next - P;
        P_prev = P - vel;
    }
    if (id_next != id){                                     // if the particle is about to die, ie, won't exist next frame
        float3 vel =  P - P_prev;
        P_next = P+vel;
    }
    if (id_next == 0 && id_prev == 0){                      // if the particle only exists on 1 frame
        P_prev = P;
        P_next = P;
    }

    // if the particle is behind the camera, kill it.
    float dist = transformz(P, w2s).z;
    // float dist_prev = transformz(P_prev, w2s).z;
    // float dist_next = transformz(P_next, w2s).z;
    if (dist<0.0f){ return;}
    // if (dist<0.0f || dist_next<0.0f || dist_prev<0.0f){ return;}
    



    // Duplicate points
    for (int pt = 0; pt <= _duplicates_num; pt++){

    
    // Apply a jitter to the duplicated points.
    if (pt !=0){
        float3 jitter = randomv2(float(pt) + float(id)*0.02)*_duplicate_radius;
        P += jitter;
        P_prev += jitter;
        P_next += jitter;
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

    // float4 clip_to_screen = s * t * clip;
    // float z_divide = clip.z / clip_to_screen.w;

    // 2d position on screen of particle
    float2 xy = float2(ndc.x, ndc.y);
    float2 xy_prev = float2(ndc_prev.x, ndc_prev.y);
    float2 xy_next = float2(ndc_next.x, ndc_next.y);

    // float max_xy = getOutOfBounds(xy);
    // float max_xy_prev = getOutOfBounds(xy_prev);
    // float max_xy_next = getOutOfBounds(xy_next);
    // float max_coord_all = max(max(max_xy, max_xy_prev), max_xy_next);
    // if (max_coord_all > _screenclip) return;
    float blur_length = (length(xy - xy_prev) + length(xy - xy_next) * _shutter );
    if (blur_length > (_screenclip*_w)) return;
    
    // Calculate 2d sizes
    float size =      max(_minSize, min(100.0f, screenSpaceSize(focal, world_size, ndc.z)));
    float size_prev = max(_minSize, min(100.0f, screenSpaceSize(focal_prev, world_size, ndc_prev.z)));
    float size_next = max(_minSize, min(100.0f, screenSpaceSize(focal_next, world_size, ndc_next.z)));



    float coc, coc_prev, coc_next;
    coc = 0.0;
    coc_prev = 0.0;
    coc_next = 0.0; 

    if (enable_dof){
    coc = max(0.0,min(200.0f, CoC(ndc.z, focal, scene_focus_distance)));
    coc_prev = max(0.0,min(200.0f, CoC(ndc_prev.z, focal_prev, scene_focus_distance_prev)));
    coc_next = max(0.0,min(200.0f, CoC(ndc_next.z, focal_next, scene_focus_distance_next)));
    }


    float shutter_start = 0.5 - (_shutter*0.25);
    float shutter_end = 0.5 + (_shutter*0.25);

    float depth_fade = minMaxSmooth(near, near_falloff, far, far_falloff, ndc.z);
    depth_fade = pow(depth_fade, falloff_gamma);
    float4 render_col = read_col;

    int adaptive_from_size;

    int steps;
    if (_maxSteps <= 1){
        renderPoint(xy,render_col , size, coc, depth_fade, ndc.z );
    }
    else{
        // float blur_length = (length(xy - xy_prev) + length(xy - xy_next) * _shutter );
        steps = max(2, min(_maxSteps, int(ceil(blur_length/max(2.0f, min(size_prev, size_next)+coc)*2.0/_spacing))));
        adaptive_from_size = min(32, int(ceil(fabs(coc_prev-coc_next)*2.0)));
        steps = max(_minSteps, steps);
        steps += adaptive_from_size;
        steps -= 1;

        //steps = 32;

        float shutter_start = 0.5 - (_shutter*0.25);
        float shutter_end = 0.5 + (_shutter*0.25);


        render_col /= float(steps)+1;


        for (int step = 0; step <= steps; step++){
            float step_position = float(step)/float(steps);
            float shutter_step = lerp(shutter_start, shutter_end, step_position);
            float2 xy_render = quadraticInterpolation(xy_prev, xy, xy_next, shutter_step);
            float coc_render = lerp(coc_prev, coc_next, shutter_step); 
            // float coc_render = coc_next;
            float size_render = lerp(size_prev, size_next, shutter_step);
            renderPoint(xy_render,render_col , size_render, coc_render, depth_fade, ndc.z ); // using hero dist, not duplicated particle dist!!
        }
    }

    if (_diagnosticMode){
        
    float4 col = float4(1.0, 1.0, 1.0, 1.0);
    float4 red = float4(1.f, 0.f, 0.f, 1.f);
    float4 green = float4(0.f, 1.f, 0.f, 1.f);
    float4 blue = float4(0.f, 0.f, 1.f, 1.f);
    // renderPoint(xy, green, size, coc, 1.0);
    // renderPoint(xy_prev, red, size, coc, 1.0);
    // renderPoint(xy_next, blue, size, coc, 1.0);
    }

    
    } // end of point replication loop
  }} // end of batch loops
  }  // end of process()
  }; // end of kernel