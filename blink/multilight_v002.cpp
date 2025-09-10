kernel multilight : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessPoint, eEdgeClamped> normal;
  Image<eRead, eAccessRandom, eEdgeClamped> l;
  Image<eWrite> dst;

  param:

    float radius;
    int use_alpha;
    int falloff; //0=linear,1=smooth,2=square,3=inv.square
	float lightscale;
    float hardness;
    float light_height;


  void define() {
    defineParam(falloff, "falloff", 1);
  }

  inline float rad(float deg){ 
    return deg*PI/180;
  }

  inline float smoothstep(float edge0, float edge1, float x){
    x = clamp((x - edge0)/(edge1 - edge0), 0.0f, 1.0f);
    return x*x*x*(x*(x*6-15)+10);
  }

  inline float cl(float val){
  return clamp(val,0.0f,1.0f); 
  }
  
local:
int l_w;
int l_h;
float l_w_div;
float l_h_div;
int _w;
int _h;
float _w_div;
float _h_div;

float3 l_dir;
float3 view_dir;

void init(){

	l_w = l.bounds.width();
	l_h = l.bounds.height();
	l_w_div = 1.0f/float(l_w);
	l_h_div = 1.0f/float(l_h);

    _w = normal.bounds.width();
	_h = normal.bounds.height();
	_w_div = 1.0f/float(_w);
	_h_div = 1.0f/float(_h);
}

  void process(int2 pos) {

	float4 accum = float4(0.0, 0.0, 0.0, 0.0);

	float3 l_colour = float3(0.0, 0.0, 0.0);
    float3 l_pos = float3(0.0, 0.0, 0.0);
    float3 N = float3(normal().x, normal().y, normal().z);

 //normalised screen pos, from -0.5 - 0.5.
    float3 P = float3( 
        (float(pos.x) * _w_div) - 0.5f,
        (float(pos.y) * _h_div) - 0.5f,
        0.0f);

    view_dir = float3(0.0, 0.0, -1.0) - P;
    view_dir = normalize(view_dir);

	// float result;
	for (int x = 0; x < l_w; x++){
		for (int y = 0; y < l_h; y++){
			l_colour = float3( l(x,y).x, l(x,y).y, l(x,y).z );
            l_pos  = float3( 
                ((float(x)/float(l_w)) - 0.5f),
                ((float(y)/float(l_h)) - 0.5f),
                light_height); 
            
            // direction of light to pixel
            l_dir = l_pos - P;
            float distance = length(l_dir);
            l_dir = normalize(l_dir);

            //diffuse
            float NdotL = dot(N, l_dir);
            NdotL = min(1.0f, max(0.0f, NdotL));
            float diffuse = NdotL / (distance*distance);
            

            // colour
            l_colour *= diffuse;

		accum += float4(l_colour, 1.0);	
		}
	} // loop ends
    dst() = accum;
    // dst() = float4(float3(NdotL), 1.0);
  }
};