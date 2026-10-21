@prism(type='hlsl', name='Fast3D HLSL Shader', version='1.0.0', description='Ported shader to prism', author='Emill & Prism Team')

@if(o_root_signature)
    @if(o_textures[0])
        #define RS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_VERTEX_SHADER_ROOT_ACCESS), CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), DescriptorTable(Sampler(s0), visibility = SHADER_VISIBILITY_PIXEL)"
    @end
    @if(o_textures[1])
        #define RS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_VERTEX_SHADER_ROOT_ACCESS), CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), DescriptorTable(SRV(t1), visibility = SHADER_VISIBILITY_PIXEL), DescriptorTable(Sampler(s1), visibility = SHADER_VISIBILITY_PIXEL)"
    @end
@end

struct PSInput {
    float4 position : SV_POSITION;
    float3 viewPos : VIEWPOS;
    float3 worldPos : WORLDPOS;
    float4 worldNormal : NORMALWORLD;
    @{update_floats(7)}
@for(i in 0..2)
    @if(o_textures[i])
        float2 uv@{i} : TEXCOORD@{i};
        @{update_floats(2)}
        @for(j in 0..2)
            @if(o_clamp[i][j])
                @if(j == 0)
                    float texClampS@{i} : TEXCLAMPS@{i};
                @else
                    float texClampT@{i} : TEXCLAMPT@{i};
                @end
                @{update_floats(1)}
            @end
        @end
    @end
@end

@if(o_fog)
float4 fog : FOG;
@{update_floats(4)}
@end
@if(o_grayscale)
float4 grayscale : GRAYSCALE;
@{update_floats(4)}
@end

@for(i in 0..o_inputs)
    @if(o_alpha)
        float4 input@{i + 1} : INPUT@{i};
        @{update_floats(4)}
    @else
        float3 input@{i + 1} : INPUT@{i};
        @{update_floats(3)}
    @end
@end
};

@if(o_textures[0]) 
    Texture2D g_texture0 : register(t0);
    SamplerState g_sampler0 : register(s0);
@end
@if(o_textures[1]) 
    Texture2D g_texture1 : register(t1);
    SamplerState g_sampler1 : register(s1);
@end

@if(o_masks[0]) Texture2D g_textureMask0 : register(t2);
@if(o_masks[1]) Texture2D g_textureMask1 : register(t3);

@if(o_blend[0]) Texture2D g_textureBlend0 : register(t4);
@if(o_blend[1]) Texture2D g_textureBlend1 : register(t5);

cbuffer PerFrameCB : register(b0) {
    uint noise_frame;
    float noise_scale;
}

float random(in float3 value) {
    float random = dot(value, float3(12.9898, 78.233, 37.719));
    return frac(sin(random) * 143758.5453);
}

// Packed tangent normals/height derived from each active diffuse texture.
Texture2D reliefTexture0 : register(t6);
Texture2D reliefTexture1 : register(t7);
SamplerState reliefSampler0 : register(s6);
SamplerState reliefSampler1 : register(s7);
cbuffer ReliefCB : register(b3) {
    float4 reliefControl; // normal strength, UV depth, slot0 active, slot1 active
    float4 reliefLight;   // view-space dominant light + diffuse fraction
    float4 reliefSlots;   // slot0 normal/parallax multipliers, slot1 multipliers
    row_major float4x4 reliefView;
}

void ReliefBasis(float3 p,float2 uv,out float3 T,out float3 B,out float3 N,out float valid) {
    float3 px=ddx(p),py=ddy(p);
    float2 ux=ddx(uv),uy=ddy(uv);
    float det=ux.x*uy.y-ux.y*uy.x;
    valid=abs(det)>1e-10 && dot(cross(px,py),cross(px,py))>1e-12;
    float inv=valid>0.5 ? 1.0/det : 0.0;
    T=(px*uy.y-py*ux.y)*inv;
    B=(py*ux.x-px*uy.x)*inv;
    T=T*rsqrt(max(dot(T,T),1e-12));
    B=B*rsqrt(max(dot(B,B),1e-12));
    N=cross(T,B); N=N*rsqrt(max(dot(N,N),1e-12));
    if (dot(N,-p)<0) N=-N;
}

float2 ReliefUV(Texture2D map,SamplerState sampleState,float2 uv,float3 p,float depth) {
    float3 T,B,N; float valid;
    ReliefBasis(p,uv,T,B,N,valid);
    if (depth<=0 || valid<0.5) return uv;
    float3 view=normalize(-p);
    float3 v=float3(dot(view,T),dot(view,B),dot(view,N));
    float grazing=saturate((abs(v.z)-0.08)/0.25);
    float2 stepUV=v.xy/max(abs(v.z),0.3)*depth*grazing/16.0;
    float2 current=uv;
    float layer=0;
    float sampled=1-map.SampleLevel(sampleState,current,0).a;
    [loop] for (int step=0;step<16 && layer<sampled;step++) {
        current-=stepUV;
        layer+=1.0/16.0;
        sampled=1-map.SampleLevel(sampleState,current,0).a;
    }
    float2 before=current+stepUV;
    float a=sampled-layer;
    float b=1-map.SampleLevel(sampleState,before,0).a-layer+1.0/16.0;
    float weight=saturate(a/(abs(a-b)>1e-5 ? a-b : 1e-5));
    return lerp(current,before,weight);
}

float ReliefLighting(Texture2D map,SamplerState sampleState,float2 uv,float3 p,float strength) {
    float3 T,B,N; float valid;
    ReliefBasis(p,uv,T,B,N,valid);
    if (strength<=0 || valid<0.5) return 1.0;
    float3 detail=map.Sample(sampleState,uv).rgb*2-1;
    detail=normalize(float3(detail.xy*strength,max(detail.z,0.2)));
    float3 bumped=normalize(T*detail.x+B*detail.y+N*detail.z);
    float3 light=normalize(reliefLight.xyz);
    float diffuse=saturate(reliefLight.w);
    float flatLight=(1-diffuse)+diffuse*saturate(dot(N,light));
    float bumpLight=(1-diffuse)+diffuse*saturate(dot(bumped,light));
    return clamp(bumpLight/max(flatLight,0.2),0.55,1.5);
}

// 3 point texture filtering
// Original author: ArthurCarvalho
// Based on GLSL implementation by twinaphex, mupen64plus-libretro project.

@if(o_three_point_filtering && (o_textures[0] || o_textures[1]))
cbuffer PerDrawCB : register(b1) {
    struct {
        uint width;
        uint height;
        bool linear_filtering;
    } textures[2];
}

#define TEX_OFFSET(tex, tSampler, texCoord, off, texSize) tex.Sample(tSampler, texCoord - off / texSize)

float4 tex2D3PointFilter(in Texture2D tex, in SamplerState tSampler, in float2 texCoord, in float2 texSize) {
    float2 offset = frac(texCoord * texSize - float2(0.5, 0.5));
    offset -= step(1.0, offset.x + offset.y);
    float4 c0 = TEX_OFFSET(tex, tSampler, texCoord, offset, texSize);
    float4 c1 = TEX_OFFSET(tex, tSampler, texCoord, float2(offset.x - sign(offset.x), offset.y), texSize);
    float4 c2 = TEX_OFFSET(tex, tSampler, texCoord, float2(offset.x, offset.y - sign(offset.y)), texSize);
    return c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0);
}
@end

@if(o_prim_depth)
cbuffer PerPrimDepthCB : register(b2) {
    float prim_depth;
    float3 _pad; // pad to 16-byte cbuffer alignment
}
@end

PSInput VSMain(
    float4 position : POSITION
@for(i in 0..2)
    @if(o_textures[i])
        , float2 uv@{i} : TEXCOORD@{i}
    @end
    @for(j in 0..2)
        @if(o_clamp[i][j])
            @if(j == 0)
                , float texClampS@{i} : TEXCLAMPS@{i}
            @else
                , float texClampT@{i} : TEXCLAMPT@{i}
            @end
        @end
    @end
@end
@if(o_fog)
    , float4 fog : FOG
@end
@if(o_grayscale)
    , float4 grayscale : GRAYSCALE
@end
@for(i in 0..o_inputs)
    @if(o_alpha)
        , float4 input@{i + 1} : INPUT@{i}
    @else
        , float3 input@{i + 1} : INPUT@{i}
    @end
@end
, float3 viewPos : VIEWPOS, float4 normalWorld : NORMALWORLD
) {
    PSInput result;
    result.position = position;
    result.viewPos=mul(float4(viewPos,1.0),reliefView).xyz;
    result.worldPos=viewPos;result.worldNormal=normalWorld;
    @for(i in 0..2)
        @if(o_textures[i])
            result.uv@{i} = uv@{i};
            @for(j in 0..2)
                @if(o_clamp[i][j])
                    @if(j == 0)
                        result.texClampS@{i} = texClampS@{i};
                    @else
                        result.texClampT@{i} = texClampT@{i};
                    @end
                @end
            @end
        @end
    @end

    @if(o_fog)
        result.fog = fog;
    @end

    @if(o_grayscale)
        result.grayscale = grayscale;
    @end

    @for(i in 0..o_inputs)
        @if(o_alpha)
            result.input@{i + 1} = input@{i + 1};
        @else
            result.input@{i + 1} = float4(input@{i + 1}, 1.0);
        @end
    @end

    return result;
}

@if(o_root_signature)
    [RootSignature(RS)]
@end

@if(srgb_mode)
    float4 fromLinear(float4 linearRGB){
        bool3 cutoff = linearRGB.rgb < float3(0.0031308, 0.0031308, 0.0031308);
        float3 higher = 1.055 * pow(linearRGB.rgb, float3(1.0 / 2.4, 1.0 / 2.4, 1.0 / 2.4)) - float3(0.055, 0.055, 0.055);
        float3 lower = linearRGB.rgb * float3(12.92, 12.92, 12.92);
        return float4(lerp(higher, lower, cutoff), linearRGB.a);
    }
@end

#define MOD(x, y) ((x) - (y) * floor((x)/(y)))
#define WRAP(x, low, high) MOD((x)-(low), (high)-(low)) + (low)

struct PSOutput {
    float4 color : SV_TARGET;
    @if(o_prim_depth)
    float depth : SV_Depth;
    @end
};

TextureCube<float> naviShadowMap : register(t14);
Texture2DArray<float> localShadowMap : register(t15);
Texture2D<float4> iblEnvironment : register(t8);
Texture2D<float4> iblEnvironmentNext : register(t13);
Texture2D<float> sunShadowMap : register(t9);
Texture2D<float4> jellyBackground : register(t10);
Texture2D<float2> iblBRDF : register(t11);
Texture2D<float> jellyDepth : register(t12);
SamplerState environmentSampler : register(s8);
SamplerState shadowSampler : register(s9);
SamplerState backgroundSampler : register(s10);
cbuffer LightingCB : register(b4) {
    float4 labToggles; // HDRI, PBR, sun shadows, this draw is jelly
    float4 labMaterial; // roughness, specular, HDRI energy, direct sun energy
    float4 labEyeTime;
    float4 labSun;
    float4 labSunColor;
    float4 labGrade;
    float4 labEffects;
    float4 labPearl;
    float4 labSH[9];
    row_major float4x4 labShadowMatrix;
    float4 labShadowMeta; // inverse map size, bias, legacy direct fraction, background ready
    float4 labDepth; // near, far, viewport width, viewport height
    float4 labJellyShape; // world-space origin and body height
    float4 labEnvironment; // rotation cosine/sine, blend
    float4 labFairyMeta; // active source count
    float4 labFairyPosition[32]; // world position, range
    float4 labFairyColor[32]; // blue-white linear energy
    float4 labNaviOrigin; // shadow-map source position, radius
    float4 labNaviMeta; // Navi source index, ready, inverse size, native debug
    float4 labFairyCone[32];
}
float3 LabSafe(float3 v);
// Match the six LookToLH views used for radial shadow depth. Project each PCF
// direction separately so taps cross cube seams instead of clamping to one face.
float LabLocalDepth(float3 d,float slot) {
    float3 a=abs(d);float2 uv;float face;
    if(a.x>=a.y&&a.x>=a.z) {face=d.x>=0?0:1;uv=float2(d.x>=0?-d.z:d.z,-d.y)/max(a.x,1e-6);}
    else if(a.y>=a.z) {face=d.y>=0?2:3;uv=float2(d.x,d.y>=0?d.z:-d.z)/max(a.y,1e-6);}
    else {face=d.z>=0?4:5;uv=float2(d.z>=0?d.x:-d.x,-d.y)/max(a.z,1e-6);}
    return localShadowMap.SampleLevel(shadowSampler,float3(uv*.5+.5,slot*6+face),0);
}
float LabLocalVisibility(int i,float3 p,float3 n) {
    float slot=labFairyColor[i].w-1;
    if(slot<0)return 1;
    float3 delta=p-labFairyPosition[i].xyz;float distance=length(delta);
    float3 direction=LabSafe(delta);
    float bias=(.7+1.5*(1-saturate(dot(n,-direction))))/labFairyPosition[i].w;
    float depth=distance/labFairyPosition[i].w-bias;
    float3 t=LabSafe(cross(direction,abs(direction.y)<.95?float3(0,1,0):float3(1,0,0)));
    float3 b=cross(direction,t);float visible=0;
    [unroll]for(int y=-1;y<=1;y++)[unroll]for(int x=-1;x<=1;x++)
        visible+=depth<=LabLocalDepth(direction+(t*x+b*y)*(2.0/512),slot);
    return visible/9;
}
float3 LabSafe(float3 v) {return v*rsqrt(max(dot(v,v),1e-12));}
float2 LabPanorama(float3 d) {d=LabSafe(d);return float2(atan2(d.z,d.x)/6.28318530718+.5,acos(clamp(d.y,-1,1))/3.14159265359);}
float3 LabEnvironmentDirection(float3 d){return float3(d.x*labEnvironment.x+d.z*labEnvironment.y,d.y,-d.x*labEnvironment.y+d.z*labEnvironment.x);}
float3 LabReflection(float3 d,float mip){float2 uv=LabPanorama(LabEnvironmentDirection(d));return lerp(iblEnvironment.SampleLevel(environmentSampler,uv,mip).rgb,iblEnvironmentNext.SampleLevel(environmentSampler,uv,mip).rgb,labEnvironment.z);}
float3 LabIrradiance(float3 n) {
    n=LabEnvironmentDirection(n);
    float x=n.x,y=n.y,z=n.z;
    return max(0,labSH[0].rgb*.282095+labSH[1].rgb*(.488603*y)+labSH[2].rgb*(.488603*z)+labSH[3].rgb*(.488603*x)
        +labSH[4].rgb*(1.092548*x*y)+labSH[5].rgb*(1.092548*y*z)+labSH[6].rgb*(.315392*(3*z*z-1))
        +labSH[7].rgb*(1.092548*x*z)+labSH[8].rgb*(.546274*(x*x-y*y)));
}
float LabVisibility(float3 p,float3 n) {
    if(labToggles.z<.5||labSunColor.w<.5)return 1;
    float4 projected=mul(float4(p,1),labShadowMatrix);
    float3 coord=projected.xyz/projected.w;
    float2 uv=coord.xy*float2(.5,-.5)+.5;
    if(any(uv<.002)||any(uv>.998)||coord.z<=0||coord.z>=1)return 1;
    float bias=labShadowMeta.y*(1+3*(1-saturate(dot(n,labSun.xyz))));
    float visible=0;
    [unroll]for(int y=-1;y<=1;y++)[unroll]for(int x=-1;x<=1;x++)
        visible+=(coord.z-bias<=sunShadowMap.SampleLevel(shadowSampler,uv+float2(x,y)*labShadowMeta.x,0));
    return lerp(1,visible/9,saturate(labSun.w));
}
float3 LabNormal(float3 p,float4 supplied,float3 eye) {
    float3 face=LabSafe(cross(ddx(p),ddy(p)));
    face=dot(face,eye-p)<0?-face:face;
    float3 smooth=LabSafe(supplied.xyz);
    // Match the actual surface: eye-facing smooth normals can flip during camera orbit.
    return supplied.w>.5&&dot(smooth,face)>.1?smooth:face;
}
float3 LabSurfaceNormal(float3 p,float2 uv,float3 normal,Texture2D map,SamplerState sampleState,float strength) {
    float3 px=ddx(p),py=ddy(p);float2 ux=ddx(uv),uy=ddy(uv);float det=ux.x*uy.y-ux.y*uy.x;
    if(abs(det)<1e-10||strength<=0)return normal;
    float3 t=LabSafe((px*uy.y-py*ux.y)/det),b=LabSafe((py*ux.x-px*uy.x)/det);
    float3 detail=map.Sample(sampleState,uv).rgb*2-1;
    detail=LabSafe(float3(detail.xy*strength,max(detail.z,.2)));
    return LabSafe(t*detail.x+b*detail.y+normal*detail.z);
}
float3 LabSpecular(float3 n,float3 v,float3 l,float rough,float3 f0) {
    float3 h=LabSafe(v+l);float nv=max(dot(n,v),.001),nl=saturate(dot(n,l)),nh=saturate(dot(n,h)),vh=saturate(dot(v,h));
    float a=rough*rough,a2=a*a,den=nh*nh*(a2-1)+1;
    float d=a2/max(3.14159265*den*den,1e-5),k=(rough+1)*(rough+1)/8;
    float g=nv/(nv*(1-k)+k)*nl/max(nl*(1-k)+k,1e-5);
    float3 f=f0+(1-f0)*pow(1-vh,5);
    return min(d*g*f/max(4*nv*nl,.001),10)*nl;
}
float LabLinearDepth(float z){return labDepth.x*labDepth.y/max(labDepth.y-z*(labDepth.y-labDepth.x),.0001);}
float LabNaviVisibility(float3 p,float3 n) {
    if(labNaviMeta.y<.5)return 1;
    float3 delta=p+n*.6-labNaviOrigin.xyz;
    float distanceFromLight=length(delta);
    float3 direction=LabSafe(delta);
    float3 tangent=LabSafe(cross(direction,abs(direction.y)>.95?float3(1,0,0):float3(0,1,0)));
    float3 bitangent=cross(direction,tangent);
    float compare=(distanceFromLight-.7)/max(labNaviOrigin.w,1);
    float visibility=0;
    [unroll]for(int y=-1;y<=1;y++)[unroll]for(int x=-1;x<=1;x++) {
        float3 tap=direction+(tangent*x+bitangent*y)*labNaviMeta.z*2;
        visibility+=(compare<=naviShadowMap.SampleLevel(shadowSampler,tap,0));
    }
    return visibility/9;
}
float4 LabShade(float4 base,float3 p,float4 normal,float2 uv0,float2 uv1,float4 screen) {
    if(labEffects.w<.5)return base;
    float3 v=LabSafe(labEyeTime.xyz-p),n=LabNormal(p,normal,labEyeTime.xyz);
    if(reliefControl.z>.5)n=LabSurfaceNormal(p,uv0,n,reliefTexture0,reliefSampler0,reliefControl.x*reliefSlots.x);
    else if(reliefControl.w>.5)n=LabSurfaceNormal(p,uv1,n,reliefTexture1,reliefSampler1,reliefControl.x*reliefSlots.z);
    float rough=clamp(labMaterial.x,.05,1),spec=saturate(labMaterial.y);
    bool jelly=labToggles.w>.5;
    float3 unbumpedNormal=n;
    if(jelly){rough=.06;spec=.6;n=LabSurfaceNormal(p,uv0*3,n,reliefTexture0,reliefSampler0,.2);}
    float visibility=LabVisibility(p,n),nl=saturate(dot(n,labSun.xyz)),nv=saturate(dot(n,v));
    float3 surfaceSpecular=0;
    float3 color=base.rgb*(1-labShadowMeta.z*(1-visibility));
    if(labToggles.x>.5)color*=.35+LabIrradiance(n)*labMaterial.z;
    if(labToggles.y>.5) {
        float3 f0=spec*.08;
        float3 directSpec=LabSpecular(n,v,labSun.xyz,rough,f0);
        surfaceSpecular=directSpec*labSunColor.rgb*labMaterial.w*visibility;
        float3 direct=base.rgb*nl*.35+directSpec;
        color+=direct*labSunColor.rgb*labMaterial.w*visibility;
        if(labToggles.x>.5) {
            float3 reflected=LabReflection(reflect(-v,n),rough*labEffects.z);
            float2 brdf=iblBRDF.SampleLevel(backgroundSampler,float2(max(nv,.005),rough),0);
            float3 envSpec=reflected*(f0*brdf.x+brdf.y)*labMaterial.z;
            color+=envSpec;surfaceSpecular+=envSpec;
        }
    }
    if(jelly) {
        float3 core=float3(.05,.10,.46),edge=float3(.10,.22,.66);
        float jellyNV=saturate(dot(unbumpedNormal,v));
        float thickness=jellyNV;
        float2 screenUV=screen.xy/labDepth.zw;
        float surfaceDepth=LabLinearDepth(screen.z);
        if(labShadowMeta.w>.5)thickness=min(thickness,saturate((LabLinearDepth(jellyDepth.SampleLevel(backgroundSampler,screenUV,0))-surfaceDepth)/18));
        float absorb=1-exp(-thickness*2.5),opacity=labPearl.y*absorb;
        float3 jellyColor=lerp(edge,core,absorb);
        float3 viewNormal=mul(float4(unbumpedNormal,0),reliefView).xyz;
        float3 detail=reliefTexture0.Sample(reliefSampler0,uv0*3).rgb;
        float2 refUV=clamp(screenUV-(viewNormal.xy+(detail.xy*2-1)*.2)*labPearl.x,.001,.999);
        if(labShadowMeta.w>.5&&jellyDepth.SampleLevel(backgroundSampler,refUV,0)<screen.z-.00001)refUV=screenUV;
        float3 behind=labShadowMeta.w>.5?jellyBackground.SampleLevel(backgroundSampler,refUV,.6).rgb:base.rgb;
        float3 h=LabSafe(v+labSun.xyz);float nh=saturate(dot(unbumpedNormal,h));
        float angle=lerp(1-jellyNV,1-nh,.5)+(detail.r-.5)*.25;
        float3 pearl=lerp(float3(.24,.04,.52),float3(.56,.28,.78),smoothstep(0,1,saturate(angle*1.8)));
        float band=sin((1-jellyNV)*3.14159265),lobe=lerp(1,.35+pow(nh,4)*1.3,.5);
        jellyColor=lerp(jellyColor,pearl,saturate(band*lobe*labPearl.z*.45));
        color=behind*lerp(1,jellyColor,.45)*(1-opacity)+jellyColor*opacity*(.5+nl*.5)*visibility
            +float3(.3,.42,.86)*pow(1-jellyNV,3)*.25+pearl*band*lobe*labPearl.z;
        color+=core*(1-smoothstep(labJellyShape.y,labJellyShape.y+max(labJellyShape.w,1),p.y))*absorb*.4;
        color+=surfaceSpecular+LabSpecular(n,v,labSun.xyz,.05,float3(.04,.04,.04))*labPearl.w*labSunColor.rgb*labMaterial.w*visibility;
        if(labToggles.x>.5)color+=LabReflection(reflect(-v,n),.4)*.04*labPearl.w*labMaterial.z;
        base.a=1; // Refraction/absorption already composited with the captured scene.
    }
    // Local fairy illumination uses a smooth finite falloff, independently of HDRI/PBR.
    // No extra glow on the fairy's own translucent sprite or the HUD/sky.
    if(normal.w>.5 || base.a>.99) {
        [loop] for(int i=0;i<(int)labFairyMeta.x;i++) {
            float3 delta=labFairyPosition[i].xyz-p;
            float distanceToFairy=length(delta);
            float falloff=saturate(1-distanceToFairy/max(labFairyPosition[i].w,1));
            // Outside this finite light volume the original contribution is exactly
            // zero. Avoid the BRDF and nine cube-map taps, retaining the debug mask.
            [branch] if(labFairyMeta.y>.5 && falloff<=0) {
                if(labNaviMeta.w>.5&&i==(int)labNaviMeta.x)return float4(.5,.5,.5,base.a);
                continue;
            }
            float3 lightDir=LabSafe(delta);
            float diffuse=saturate(dot(n,lightDir));
            if(labFairyCone[i].w>0)falloff*=smoothstep(labFairyCone[i].w,min(.999,labFairyCone[i].w+.12),
                dot(-lightDir,labFairyCone[i].xyz));
            float localVisibility=i==(int)labNaviMeta.x?LabNaviVisibility(p,n):LabLocalVisibility(i,p,n);
            // A local light uses the same bumped normal and material BRDF as
            // directional lighting. No angle-independent fill/albedo floor.
            float3 fairySpecular=labToggles.y>.5?
                LabSpecular(n,v,lightDir,rough,float3(spec*.08,spec*.08,spec*.08)):float3(0,0,0);
            color+=(base.rgb*(diffuse/3.14159265)+fairySpecular)*labFairyColor[i].rgb*falloff*falloff*localVisibility;
            if(labNaviMeta.w>.5&&i==(int)labNaviMeta.x) {
                float mask=falloff>0?localVisibility:.5;
                return float4(mask,mask,mask,base.a);
            }
        }
    }
    return float4(max(color,0),base.a);
}

PSOutput PSMain(PSInput input, float4 screenSpace : SV_Position) {
    float reliefDepth=reliefControl.y;
    @if(o_alpha && !o_texture_edge && !o_alpha_threshold)
        // Scrolling translucent water and blended effects keep their original UVs.
        reliefDepth=0.0;
    @end
    @for(i in 0..2)
        @if(o_textures[i])
            float2 tc@{i} = input.uv@{i};
            @if(i == 0)
                if (reliefControl.z>0.5) tc0=ReliefUV(reliefTexture0,reliefSampler0,tc0,input.viewPos,reliefDepth*reliefSlots.y);
            @else
                if (reliefControl.w>0.5) tc1=ReliefUV(reliefTexture1,reliefSampler1,tc1,input.viewPos,reliefDepth*reliefSlots.w);
            @end
            @{s = o_clamp[i][0]}
            @{t = o_clamp[i][1]}
            @if(s || t)
                int2 texSize@{i};
                g_texture@{i}.GetDimensions(texSize@{i}.x, texSize@{i}.y);
                @if(s && t)
                    tc@{i} = clamp(tc@{i}, 0.5 / texSize@{i}, float2(input.texClampS@{i}, input.texClampT@{i}));
                @elseif(s)
                    tc@{i} = float2(clamp(tc@{i}.x, 0.5 / texSize@{i}.x, input.texClampS@{i}), tc@{i}.y);
                @else
                    tc@{i} = float2(tc@{i}.x, clamp(tc@{i}.y, 0.5 / texSize@{i}.y, input.texClampT@{i}));
                @end
            @end

            @if(o_three_point_filtering)
                float4 texVal@{i};
                if (textures[@{i}].linear_filtering) {
                    @if(o_masks[i])
                        texVal@{i} = tex2D3PointFilter(g_texture@{i}, g_sampler@{i}, tc@{i}, float2(textures[@{i}].width, textures[@{i}].height));
                        float2 maskSize@{i};
                        g_textureMask@{i}.GetDimensions(maskSize@{i}.x, maskSize@{i}.y);
                        float4 maskVal@{i} = tex2D3PointFilter(g_textureMask@{i}, g_sampler@{i}, tc@{i}, maskSize@{i});
                        @if(o_blend[i])
                            float4 blendVal@{i} = tex2D3PointFilter(g_textureBlend@{i}, g_sampler@{i}, tc@{i}, float2(textures[@{i}].width, textures[@{i}].height));
                        @else
                            float4 blendVal@{i} = float4(0, 0, 0, 0);
                        @end

                        texVal@{i} = lerp(texVal@{i}, blendVal@{i}, maskVal@{i}.a);
                    @else
                        texVal@{i} = tex2D3PointFilter(g_texture@{i}, g_sampler@{i}, tc@{i}, float2(textures[@{i}].width, textures[@{i}].height));
                    @end
                } else {
                    texVal@{i} = g_texture@{i}.Sample(g_sampler@{i}, tc@{i});
                    @if(o_masks[i])
                        @if(o_blend[i])
                            float4 blendVal@{i} = g_textureBlend@{i}.Sample(g_sampler@{i}, tc@{i});
                        @else
                            float4 blendVal@{i} = float4(0, 0, 0, 0);
                        @end
                        texVal@{i} = lerp(texVal@{i}, blendVal@{i}, g_textureMask@{i}.Sample(g_sampler@{i}, tc@{i}).a);
                    @end
                }
            @else
                float4 texVal@{i} = g_texture@{i}.Sample(g_sampler@{i}, tc@{i});
                @if(o_masks[i])
                    @if(o_blend[i])
                        float4 blendVal@{i} = g_textureBlend@{i}.Sample(g_sampler@{i}, tc@{i});
                    @else
                        float4 blendVal@{i} = float4(0, 0, 0, 0);
                    @end
                    texVal@{i} = lerp(texVal@{i}, blendVal@{i}, g_textureMask@{i}.Sample(g_sampler@{i}, tc@{i}).a);
                @end
            @end
        @end
    @end

    @if(o_alpha) 
        float4 texel;
    @else
        float3 texel;
    @end

    @if(o_2cyc)
        @{f_range = 2}
    @else
        @{f_range = 1}
    @end

    @for(c in 0..f_range)
        @if(c == 1)
            @if(o_alpha)
                @if(o_c[c][1][2] == SHADER_COMBINED)
                    texel.a = WRAP(texel.a, -1.01, 1.01);
                @else
                    texel.a = WRAP(texel.a, -0.51, 1.51);
                @end
            @end

            @if(o_c[c][0][2] == SHADER_COMBINED)
                texel.rgb = WRAP(texel.rgb, -1.01, 1.01);
            @else
                texel.rgb = WRAP(texel.rgb, -0.51, 1.51);
            @end
        @end

        @if(!o_color_alpha_same[c] && o_alpha)
            texel = float4(@{
            append_formula(o_c[c], o_do_single[c][0],
                           o_do_multiply[c][0], o_do_mix[c][0], false, false, true, c == 0)
            }, @{append_formula(o_c[c], o_do_single[c][1],
                           o_do_multiply[c][1], o_do_mix[c][1], true, true, true, c == 0)
            });
        @else
            texel = @{append_formula(o_c[c], o_do_single[c][0],
                           o_do_multiply[c][0], o_do_mix[c][0], o_alpha, false,
                           o_alpha, c == 0)};
        @end
    @end

    @if(o_texture_edge && o_alpha)
        if (texel.a > 0.19) texel.a = 1.0; else discard;
    @end

    texel = WRAP(texel, -0.51, 1.51);
    texel = clamp(texel, 0.0, 1.0);
    float reliefShade=1.0;
    @if(o_textures[0])
        if (reliefControl.z>0.5) reliefShade*=ReliefLighting(reliefTexture0,reliefSampler0,tc0,input.viewPos,reliefControl.x*reliefSlots.x);
    @end
    @if(o_textures[1])
        if (reliefControl.w>0.5 && reliefControl.z<0.5) reliefShade*=ReliefLighting(reliefTexture1,reliefSampler1,tc1,input.viewPos,reliefControl.x*reliefSlots.z);
    @end
    texel.rgb=saturate(texel.rgb*reliefShade);
    float2 labUV0=float2(0,0),labUV1=float2(0,0);
    @if(o_textures[0])
        labUV0=tc0;
    @end
    @if(o_textures[1])
        labUV1=tc1;
    @end
    @if(o_alpha)
        texel=LabShade(texel,input.worldPos,input.worldNormal,labUV0,labUV1,screenSpace);
    @else
        texel=LabShade(float4(texel,1),input.worldPos,input.worldNormal,labUV0,labUV1,screenSpace).rgb;
    @end
    // TODO discard if alpha is 0?
    @if(o_fog)
        @if(o_alpha)
            texel = float4(lerp(texel.rgb, input.fog.rgb, input.fog.a), texel.a);
        @else
            texel = lerp(texel, input.fog.rgb, input.fog.a);
        @end
    @end

    @if(o_grayscale)
        float intensity = (texel.r + texel.g + texel.b) / 3.0;
        float3 new_texel = input.grayscale.rgb * intensity;
        texel.rgb = lerp(texel.rgb, new_texel, input.grayscale.a);
    @end

    @if(o_alpha && o_noise)
        float2 coords = screenSpace.xy * noise_scale;
        texel.a *= round(saturate(random(float3(floor(coords), noise_frame)) + texel.a - 0.5));
    @end

    @if(o_alpha)
        @if(o_alpha_threshold)
            if (texel.a < 8.0 / 256.0) discard;
        @end
        @if(o_invisible)
            texel.a = 0.0;
        @end
    @end

    PSOutput output;
    @if(o_alpha)
        @if(srgb_mode)
            output.color = fromLinear(texel);
        @else
            output.color = texel;
        @end
    @else
        @if(srgb_mode)
            output.color = fromLinear(float4(texel, 1.0));
        @else
            output.color = float4(texel, 1.0);
        @end
    @end
    @if(o_prim_depth)
        output.depth = prim_depth;
    @end
    return output;
}
