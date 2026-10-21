// Radial depth for Navi's six directional shadow views.
cbuffer ShadowCB:register(b0) {
    row_major float4x4 lightMatrix;
    float4 flags;
    float4 lightOrigin; // xyz, radius
}
Texture2D<float4> albedo:register(t0);
SamplerState alphaSampler:register(s0);
struct VIn {float3 position:POSITION;float2 uv:TEXCOORD0;};
struct VOut {float4 position:SV_POSITION;float2 uv:TEXCOORD0;float3 world:TEXCOORD1;};
VOut VS(VIn v) {
    VOut o;o.position=mul(float4(v.position,1),lightMatrix);o.uv=v.uv;o.world=v.position;return o;
}
float PS(VOut p):SV_Depth {
    if(flags.x>.5)clip(albedo.Sample(alphaSampler,p.uv).a-.2);
    return saturate(length(p.world-lightOrigin.xyz)/lightOrigin.w);
}
