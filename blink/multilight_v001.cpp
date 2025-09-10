kernel multilight : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessPoint, eEdgeClamped> p;
  Image<eRead, eAccessRandom, eEdgeClamped> l;
  Image<eWrite> dst;

  param:

    float radius;
    int use_alpha;
    int falloff; //0=linear,1=smooth,2=square,3=inv.square
	float lightscale;
    float hardness;


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

void init(){
	l_w = l.bounds.width();
	l_h = l.bounds.height();
	
	l_w_div = 1.0f/float(l_w);
	l_h_div = 1.0f/float(l_h);
}

  void process() {
	float4 accum = float4(0.0, 0.0, 0.0, 0.0);
	float3 picked = float3(0.0, 0.0, 0.0); 
	float3 l_colour = float3(0.0, 0.0, 0.0);
	float result;
	for (int x = 0; x < l_w; x++){
		for (int y = 0; y < l_h; y++){
			
			if (int(l(x,y).w) == 1) break; // optimisation that doesn't seem to optimise anything...
			l_colour = float3( l(x,y).x, l(x,y).y, l(x,y).z );
			picked  = float3( 
							((float(x)/float(l_w)) - 0.5f),
							((float(y)/float(l_h)) - 0.5f),
							1.0); 
							
			// picked  = float3( 
							// ((float(x)*l_w_div) - 0.5f),
							// ((float(y)*l_h_div) - 0.5f),
							// 1.0); 
							
			picked *= lightscale;
			l_colour = float3( l(x,y).x, l(x,y).y, l(x,y).z );
			
			
			float3 color = float3(p(0)-picked.x,p(1)-picked.y,p(2)-picked.z);
			int black = 0;



			//2. SHAPES
			float dist;


			  dist = sqrt(pow(color[0],2) + pow(color[1],2) + pow(color[2],2));
			  result = radius==0.0f? 0:cl(1-dist/radius);


			//3. FALLOFF
			if(falloff==0){//Linear
			  result = hardness>=1? float(result>0) : cl(result/(1-hardness));
			}else if(falloff==1){//Smooth
			  result = hardness>=1? float(result>0) : smoothstep(0,1-hardness,result);
			}else if(falloff==2){//Quadratic
			  result = hardness>=1? float(result>0) : cl(pow(float(result/(1-hardness)),2));
			}else if(falloff==3){//Cubic
			  result = hardness>=1? float(result>0) : cl(pow(float(result/(1-hardness)),3));
			}else if(falloff==4){//Inv. Cubic
			  result = hardness>=1? float(result>0) : 1-cl(pow(float(cl(1-(result/(1-hardness)))),3));
			}

		accum += float4(l_colour, 1.0) * result;	
		}
	} // loop ends
    dst() = accum;
  }
};