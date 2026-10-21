cbuffer ShadowCB:register(b0){row_major float4x4 lightMatrix;float4 flags;}
Texture2D<float4> albedo:register(t0);SamplerState alphaSampler:register(s0);
struct VIn{float3 position:POSITION;float2 uv:TEXCOORD0;};
struct VOut{float4 position:SV_POSITION;float2 uv:TEXCOORD0;};
VOut VS(VIn v){VOut o;o.position=mul(float4(v.position,1),lightMatrix);o.uv=v.uv;return o;}
void PS(VOut p){if(flags.x>.5)clip(albedo.Sample(alphaSampler,p.uv).a-.2);}
