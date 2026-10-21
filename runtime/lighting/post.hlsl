Texture2D<float4> scene : register(t0);
Texture2D<float4> bloom : register(t1);
SamplerState linearClamp : register(s0);
cbuffer PostCB : register(b0){float4 controls;float4 effects;float4 sizes;float4 uvTransform;}
struct VOut{float4 pos:SV_POSITION;float2 uv:TEXCOORD0;};
VOut VS(uint id:SV_VertexID){VOut o;o.uv=float2((id<<1)&2,id&2);o.pos=float4(o.uv*float2(2,-2)+float2(-1,1),0,1);return o;}
float3 aces(float3 x){return saturate(x*(2.51*x+.03)/(x*(2.43*x+.59)+.14));}
float4 PS(VOut i):SV_TARGET{
    float3 c=scene.SampleLevel(linearClamp,i.uv*uvTransform.xy+uvTransform.zw,0).rgb;
    if(sizes.z>.5&&sizes.z<1.5){
        float brightness=max(c.r,max(c.g,c.b));return float4(c*saturate((brightness-effects.y)/max(brightness,.0001)),1);
    }
    if(sizes.z>1.5){
        float2 stepUV=sizes.xy;
        c=scene.SampleLevel(linearClamp,i.uv,0).rgb*.227027;
        c+=(scene.SampleLevel(linearClamp,i.uv+stepUV*1.384615,0).rgb+scene.SampleLevel(linearClamp,i.uv-stepUV*1.384615,0).rgb)*.316216;
        c+=(scene.SampleLevel(linearClamp,i.uv+stepUV*3.230769,0).rgb+scene.SampleLevel(linearClamp,i.uv-stepUV*3.230769,0).rgb)*.070270;
        return float4(c,1);
    }
    if(effects.w>.5)c+=bloom.SampleLevel(linearClamp,i.uv,0).rgb*effects.x;
    if(effects.z>.5){
        c*=exp2(controls.x);
        float luma=dot(c,float3(.2126,.7152,.0722));c=lerp(luma.xxx,c,controls.z);
        c=max((c-.18)*controls.y+.18,0);
        // Neutral exposure=0/contrast=1/gamma=1 is an exact bypass of grading.
        c=pow(max(c,0),1/max(controls.w,.1));
    }
    if(sizes.w>.5)c=aces(c); // Optional filmic tonemapping for HDR radiance.
    return float4(c,1);
}
