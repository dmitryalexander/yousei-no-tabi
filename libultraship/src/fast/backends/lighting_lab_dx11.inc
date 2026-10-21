#include <chrono>
#include <tuple>
namespace Fast {
namespace {
double LabMilliseconds() {
    return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
// Additional passes restore all state they touch, including references cached by F3D.
struct LabPipelineGuard {
    ID3D11DeviceContext* c;
    ComPtr<ID3D11RenderTargetView> rt;ComPtr<ID3D11DepthStencilView> ds;
    ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11InputLayout> layout;ComPtr<ID3D11Buffer> vb;
    ComPtr<ID3D11RasterizerState> raster;ComPtr<ID3D11DepthStencilState> depth;
    ComPtr<ID3D11BlendState> blend;
    ID3D11Buffer* vcb[5]={};ID3D11Buffer* pcb[5]={};
    ID3D11ShaderResourceView* srv[16]={};ID3D11SamplerState* samplers[11]={};
    D3D11_VIEWPORT viewport[16];D3D11_RECT scissor[16];UINT nv=16,ns=16,stride=0,offset=0,stencil=0,mask=0;
    FLOAT factors[4]={};D3D11_PRIMITIVE_TOPOLOGY topology;
    explicit LabPipelineGuard(ID3D11DeviceContext* context):c(context) {
        c->OMGetRenderTargets(1,rt.GetAddressOf(),ds.GetAddressOf());
        c->VSGetShader(vs.GetAddressOf(),nullptr,nullptr);c->PSGetShader(ps.GetAddressOf(),nullptr,nullptr);
        c->IAGetInputLayout(layout.GetAddressOf());c->IAGetVertexBuffers(0,1,vb.GetAddressOf(),&stride,&offset);
        c->IAGetPrimitiveTopology(&topology);c->RSGetState(raster.GetAddressOf());
        c->RSGetViewports(&nv,viewport);c->RSGetScissorRects(&ns,scissor);
        c->OMGetBlendState(blend.GetAddressOf(),factors,&mask);c->OMGetDepthStencilState(depth.GetAddressOf(),&stencil);
        c->VSGetConstantBuffers(0,5,vcb);c->PSGetConstantBuffers(0,5,pcb);
        c->PSGetShaderResources(0,16,srv);c->PSGetSamplers(0,11,samplers);
    }
    ~LabPipelineGuard() {
        ID3D11ShaderResourceView* empty[16]={};c->PSSetShaderResources(0,16,empty);
        c->OMSetRenderTargets(1,rt.GetAddressOf(),ds.Get());
        c->VSSetShader(vs.Get(),nullptr,0);c->PSSetShader(ps.Get(),nullptr,0);
        c->IASetInputLayout(layout.Get());c->IASetVertexBuffers(0,1,vb.GetAddressOf(),&stride,&offset);
        c->IASetPrimitiveTopology(topology);c->RSSetState(raster.Get());
        c->RSSetViewports(nv,viewport);c->RSSetScissorRects(ns,scissor);
        c->OMSetBlendState(blend.Get(),factors,mask);c->OMSetDepthStencilState(depth.Get(),stencil);
        c->VSSetConstantBuffers(0,5,vcb);c->PSSetConstantBuffers(0,5,pcb);
        c->PSSetShaderResources(0,16,srv);c->PSSetSamplers(0,11,samplers);
        for(auto* p:vcb)if(p)p->Release();for(auto* p:pcb)if(p)p->Release();
        for(auto* p:srv)if(p)p->Release();for(auto* p:samplers)if(p)p->Release();
    }
};
std::string LabRead(const char* path) {
    std::ifstream stream(path,std::ios::binary);
    if(!stream)throw std::runtime_error(std::string("Lighting asset missing: ")+path);
    return std::string(std::istreambuf_iterator<char>(stream),{});
}
void LabUpdate(ID3D11DeviceContext* context,ID3D11Buffer* buffer,const void* data,size_t size) {
    D3D11_MAPPED_SUBRESOURCE map={};ThrowIfFailed(context->Map(buffer,0,D3D11_MAP_WRITE_DISCARD,0,&map));
    memcpy(map.pData,data,size);context->Unmap(buffer,0);
}
}
void GfxRenderingAPIDX11::InitLightingLab() {
    if(mLabInitialized)return;
    auto makeBuffer=[&](UINT bytes,ComPtr<ID3D11Buffer>& buffer) {
        D3D11_BUFFER_DESC desc={};desc.ByteWidth=bytes;desc.Usage=D3D11_USAGE_DYNAMIC;
        desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        ThrowIfFailed(mDevice->CreateBuffer(&desc,nullptr,buffer.GetAddressOf()));
    };
    makeBuffer(1984,mLabCb);makeBuffer(96,mLabShadowCb);makeBuffer(64,mLabPostCb);
    auto compile=[&](const char* path,const char* entry,const char* profile) {
        auto source=LabRead(path);ComPtr<ID3DBlob> shader,error;
        HRESULT result=mD3dCompile(source.data(),source.size(),path,nullptr,nullptr,entry,profile,
                                  D3DCOMPILE_OPTIMIZATION_LEVEL3,0,shader.GetAddressOf(),error.GetAddressOf());
        if(FAILED(result)) {
            std::string message=error?std::string((const char*)error->GetBufferPointer(),error->GetBufferSize()):"Unknown shader error";
            std::ofstream("lighting-shader-error.txt")<<message;throw std::runtime_error(message);
        }
        return shader;
    };
    auto vs=compile("lighting/shadow.hlsl","VS","vs_4_0"),ps=compile("lighting/shadow.hlsl","PS","ps_4_0");
    ThrowIfFailed(mDevice->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,mLabShadowVs.GetAddressOf()));
    ThrowIfFailed(mDevice->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,mLabShadowPs.GetAddressOf()));
    auto naviVs=compile("lighting/navi-shadow.hlsl","VS","vs_4_0"),naviPs=compile("lighting/navi-shadow.hlsl","PS","ps_4_0");
    ThrowIfFailed(mDevice->CreateVertexShader(naviVs->GetBufferPointer(),naviVs->GetBufferSize(),nullptr,mLabNaviVs.GetAddressOf()));
    ThrowIfFailed(mDevice->CreatePixelShader(naviPs->GetBufferPointer(),naviPs->GetBufferSize(),nullptr,mLabNaviPs.GetAddressOf()));
    D3D11_TEXTURE2D_DESC pointTex={};pointTex.Width=pointTex.Height=1024;pointTex.MipLevels=1;pointTex.ArraySize=6;
    pointTex.Format=DXGI_FORMAT_R32_TYPELESS;pointTex.SampleDesc.Count=1;pointTex.Usage=D3D11_USAGE_DEFAULT;
    pointTex.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;pointTex.MiscFlags=D3D11_RESOURCE_MISC_TEXTURECUBE;
    ThrowIfFailed(mDevice->CreateTexture2D(&pointTex,nullptr,mLabNaviTexture.GetAddressOf()));
    D3D11_SHADER_RESOURCE_VIEW_DESC pointSrv={};pointSrv.Format=DXGI_FORMAT_R32_FLOAT;pointSrv.ViewDimension=D3D11_SRV_DIMENSION_TEXTURECUBE;pointSrv.TextureCube.MipLevels=1;
    ThrowIfFailed(mDevice->CreateShaderResourceView(mLabNaviTexture.Get(),&pointSrv,mLabNaviSrv.GetAddressOf()));
    for(int face=0;face<6;face++) {
        D3D11_DEPTH_STENCIL_VIEW_DESC pointDsv={};pointDsv.Format=DXGI_FORMAT_D32_FLOAT;pointDsv.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        pointDsv.Texture2DArray.FirstArraySlice=face;pointDsv.Texture2DArray.ArraySize=1;
        ThrowIfFailed(mDevice->CreateDepthStencilView(mLabNaviTexture.Get(),&pointDsv,mLabNaviDsv[face].GetAddressOf()));
    }
    D3D11_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
                                    {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
    ThrowIfFailed(mDevice->CreateInputLayout(input,2,vs->GetBufferPointer(),vs->GetBufferSize(),mLabShadowLayout.GetAddressOf()));
    vs=compile("lighting/post.hlsl","VS","vs_4_0");ps=compile("lighting/post.hlsl","PS","ps_4_0");
    ThrowIfFailed(mDevice->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,mLabPostVs.GetAddressOf()));
    ThrowIfFailed(mDevice->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,mLabPostPs.GetAddressOf()));
    D3D11_SAMPLER_DESC sampler={};sampler.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU=D3D11_TEXTURE_ADDRESS_WRAP;sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.MaxLOD=D3D11_FLOAT32_MAX;
    ThrowIfFailed(mDevice->CreateSamplerState(&sampler,mLabEnvironmentSampler.GetAddressOf()));
    sampler.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;
    ThrowIfFailed(mDevice->CreateSamplerState(&sampler,mLabClampSampler.GetAddressOf()));
    sampler.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;
    ThrowIfFailed(mDevice->CreateSamplerState(&sampler,mLabShadowSampler.GetAddressOf()));
    D3D11_RASTERIZER_DESC raster={};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=true;
    ThrowIfFailed(mDevice->CreateRasterizerState(&raster,mLabRaster.GetAddressOf()));
    raster.DepthBias=100;raster.SlopeScaledDepthBias=1.5f;
    ThrowIfFailed(mDevice->CreateRasterizerState(&raster,mLabShadowRaster.GetAddressOf()));
    D3D11_DEPTH_STENCIL_DESC depth={};depth.DepthEnable=true;depth.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;depth.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
    ThrowIfFailed(mDevice->CreateDepthStencilState(&depth,mLabDepthOn.GetAddressOf()));
    depth.DepthEnable=false;depth.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;
    ThrowIfFailed(mDevice->CreateDepthStencilState(&depth,mLabDepthOff.GetAddressOf()));
    auto noise=LabRead("lighting/jelly-detail.bin");
    if(noise.size()!=8+256*256*4)throw std::runtime_error("Invalid slime detail normal map");
    D3D11_TEXTURE2D_DESC noiseDesc={};noiseDesc.Width=noiseDesc.Height=256;noiseDesc.ArraySize=noiseDesc.MipLevels=1;
    noiseDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;noiseDesc.SampleDesc.Count=1;noiseDesc.Usage=D3D11_USAGE_IMMUTABLE;noiseDesc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA noiseData={noise.data()+8,256*4,0};ComPtr<ID3D11Texture2D> noiseTexture;
    ThrowIfFailed(mDevice->CreateTexture2D(&noiseDesc,&noiseData,noiseTexture.GetAddressOf()));
    ThrowIfFailed(mDevice->CreateShaderResourceView(noiseTexture.Get(),nullptr,mLabJellyNoise.GetAddressOf()));
    sampler.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sampler.AddressU=sampler.AddressV=D3D11_TEXTURE_ADDRESS_WRAP;
    ThrowIfFailed(mDevice->CreateSamplerState(&sampler,mLabNoiseSampler.GetAddressOf()));
    const char* environments[]={"environment","nishita-day","nishita-low","nishita-dusk","forest","interior"};
    for(int index=0;index<6;index++) {
    auto ibl=LabRead((std::string("lighting/")+environments[index]+".ibl").c_str());
    if(ibl.size()<164||ibl.substr(0,8)!="YNTIBL01")throw std::runtime_error("Invalid HDR IBL asset");
    uint32_t width,height,mips;memcpy(&width,ibl.data()+8,4);memcpy(&height,ibl.data()+12,4);memcpy(&mips,ibl.data()+16,4);
    if(width!=512||height!=256||mips!=10)throw std::runtime_error("Unexpected IBL layout");
    memcpy(mLabEnvironmentSH[index],ibl.data()+20,sizeof(mLabEnvironmentSH[index]));
    D3D11_TEXTURE2D_DESC tex={};tex.Width=width;tex.Height=height;tex.MipLevels=mips;tex.ArraySize=1;
    tex.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;tex.SampleDesc.Count=1;tex.Usage=D3D11_USAGE_IMMUTABLE;tex.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    std::vector<D3D11_SUBRESOURCE_DATA> data;size_t offset=164;
    for(uint32_t level=0;level<mips;level++) {
        uint32_t w=std::max(1u,width>>level),h=std::max(1u,height>>level);
        if(offset+size_t(w)*h*16>ibl.size())throw std::runtime_error("Truncated HDR IBL asset");
        data.push_back({ibl.data()+offset,w*16,0});offset+=size_t(w)*h*16;
    }
    ThrowIfFailed(mDevice->CreateTexture2D(&tex,data.data(),mLabEnvironmentTextures[index].GetAddressOf()));
    ThrowIfFailed(mDevice->CreateShaderResourceView(mLabEnvironmentTextures[index].Get(),nullptr,mLabEnvironments[index].GetAddressOf()));
    }
    D3D11_TEXTURE2D_DESC tex={};tex.ArraySize=1;tex.SampleDesc.Count=1;tex.Usage=D3D11_USAGE_IMMUTABLE;tex.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    auto brdf=LabRead("lighting/brdf.bin");uint32_t bw,bh;memcpy(&bw,brdf.data(),4);memcpy(&bh,brdf.data()+4,4);
    if(bw!=64||bh!=64||brdf.size()!=8+bw*bh*8)throw std::runtime_error("Invalid BRDF LUT");
    tex.Width=bw;tex.Height=bh;tex.MipLevels=1;tex.Format=DXGI_FORMAT_R32G32_FLOAT;
    D3D11_SUBRESOURCE_DATA brdfData={brdf.data()+8,bw*8,0};ComPtr<ID3D11Texture2D> brdfTexture;
    ThrowIfFailed(mDevice->CreateTexture2D(&tex,&brdfData,brdfTexture.GetAddressOf()));
    ThrowIfFailed(mDevice->CreateShaderResourceView(brdfTexture.Get(),nullptr,mLabBrdf.GetAddressOf()));
    tex.Width=tex.Height=2048;tex.Format=DXGI_FORMAT_R32_TYPELESS;tex.Usage=D3D11_USAGE_DEFAULT;
    tex.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
    ThrowIfFailed(mDevice->CreateTexture2D(&tex,nullptr,mLabShadowTexture.GetAddressOf()));
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv={};dsv.Format=DXGI_FORMAT_D32_FLOAT;dsv.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2D;
    ThrowIfFailed(mDevice->CreateDepthStencilView(mLabShadowTexture.Get(),&dsv,mLabShadowDsv.GetAddressOf()));
    D3D11_SHADER_RESOURCE_VIEW_DESC srv={};srv.Format=DXGI_FORMAT_R32_FLOAT;srv.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;srv.Texture2D.MipLevels=1;
    ThrowIfFailed(mDevice->CreateShaderResourceView(mLabShadowTexture.Get(),&srv,mLabShadowSrv.GetAddressOf()));
    mLabInitialized=true;SPDLOG_INFO("Lighting Lab: floating HDRI + GGX mips/BRDF + shadow/post resources ready");
}
void GfxRenderingAPIDX11::BindLightingLab() {
    auto& lab=gLightingLab;
    if(!LightingLabActive()&&!mLabInitialized)return;
    InitLightingLab();
    bool active=LightingLabActive()&&mCurrentDepthTest&&mCurrentFramebuffer==mLabGameTarget;
    bool jelly=active&&lab.jelly&&((mShaderProgram->usedTextures[0]&&mLabJellyTextures[mCurrentTextureIds[0]])||(mShaderProgram->usedTextures[1]&&mLabJellyTextures[mCurrentTextureIds[1]]));
    if(jelly) {
        CaptureJellyScene();
        mContext->PSSetShaderResources(6,1,mLabJellyNoise.GetAddressOf());
        mContext->PSSetSamplers(6,1,mLabNoiseSampler.GetAddressOf());
    }
    struct Constants {float toggle[4],material[4],eye[4],sun[4],sunColor[4],grade[4],effects[4],pearl[4],sh[36],matrix[16],shadow[4],depth[4],jellyShape[4],environment[4],fairyMeta[4],fairyPosition[32][4],fairyColor[32][4],naviOrigin[4],naviMeta[4],fairyCone[32][4];} data={};
    static_assert(sizeof(data)==1984);
    data.toggle[0]=active&&lab.hdri;data.toggle[1]=active&&lab.pbr;data.toggle[2]=active&&lab.shadows;data.toggle[3]=jelly;
    float material[]={lab.roughness,lab.specular,lab.hdriStrength,lab.sunStrength};std::copy(material,material+4,data.material);
    std::copy(lab.eye,lab.eye+3,data.eye);data.eye[3]=lab.time;
    std::copy(lab.sun,lab.sun+3,data.sun);data.sun[3]=lab.shadowStrength;
    std::copy(lab.sunColor,lab.sunColor+3,data.sunColor);data.sunColor[3]=mLabShadowReady&&mLabShadowScene==lab.scene;
    float grade[]={lab.exposure,lab.contrast,lab.saturation,lab.gamma};std::copy(grade,grade+4,data.grade);
    data.effects[0]=lab.glowStrength;data.effects[1]=lab.glowThreshold;data.effects[2]=9;data.effects[3]=active;
    float pearl[]={lab.refraction,lab.bodyOpacity,lab.pearlStrength,lab.clearcoat};std::copy(pearl,pearl+4,data.pearl);
    for(int i=0;i<36;i++)data.sh[i]=mLabEnvironmentSH[lab.environment][i]*(1-lab.environmentBlend)+mLabEnvironmentSH[lab.environmentNext][i]*lab.environmentBlend;std::copy(mLabShadowMatrix,mLabShadowMatrix+16,data.matrix);
    data.shadow[0]=1.0f/2048;data.shadow[1]=lab.shadowBias;data.shadow[2]=.65f;data.shadow[3]=mLabBackgroundReady;
    data.depth[0]=std::max(lab.nearPlane,.1f);data.depth[1]=std::max(lab.farPlane,lab.nearPlane+1);
    auto& tex=mTextures[mFrameBuffers[mCurrentFramebuffer].texture_id];data.depth[2]=float(tex.width);data.depth[3]=float(tex.height);
    std::copy(lab.chuchuOrigin,lab.chuchuOrigin+4,data.jellyShape);
    data.environment[0]=std::cos(lab.environmentRotation);data.environment[1]=std::sin(lab.environmentRotation);data.environment[2]=lab.environmentBlend;
    data.fairyMeta[0]=active?float(lab.fairyCount):0;
    data.fairyMeta[1]=lab.optimize?1.0f:0; // native reference keeps the original pixel work
    memcpy(data.fairyPosition,lab.fairyPosition,sizeof(data.fairyPosition));
    memcpy(data.fairyColor,lab.fairyColor,sizeof(data.fairyColor));
    bool naviReady=active&&lab.naviShadows&&lab.naviIndex>=0&&mLabNaviReady&&mLabNaviScene==lab.scene;
    if(naviReady) {
        float movement=0;for(int axis=0;axis<3;axis++){float d=lab.fairyPosition[lab.naviIndex][axis]-mLabNaviOrigin[axis];movement+=d*d;}
        naviReady=movement<10000;
    }
    std::copy(mLabNaviOrigin,mLabNaviOrigin+4,data.naviOrigin);
    data.naviMeta[0]=float(lab.naviIndex);data.naviMeta[1]=naviReady?1.0f:0;data.naviMeta[2]=1.0f/1024;
    data.naviMeta[3]=active&&lab.naviShadowDebug;
    // Use the matching source position for the previously completed shadow map.
    if(naviReady)std::copy(mLabNaviOrigin,mLabNaviOrigin+4,data.fairyPosition[lab.naviIndex]);
    mContext->PSSetShaderResources(14,1,mLabNaviSrv.GetAddressOf());
    memcpy(data.fairyCone,lab.fairyCone,sizeof(data.fairyCone));
    if(active&&lab.naviShadows&&mLabLocalScene==lab.scene) {
        for(int i=0;i<lab.fairyCount;i++)for(int slot=0;slot<mLabLocalCount;slot++) {
            if(i==lab.naviIndex||lab.fairyKey[i]!=mLabLocalKeys[slot])continue;
            float movement=0;for(int axis=0;axis<3;axis++) {
                float d=lab.fairyPosition[i][axis]-mLabLocalOrigin[slot][axis];movement+=d*d;
            }
            if(movement>=10000)continue;
            std::copy(mLabLocalOrigin[slot],mLabLocalOrigin[slot]+4,data.fairyPosition[i]);
            data.fairyColor[i][3]=float(slot+1);
        }
    }
    mContext->PSSetShaderResources(15,1,mLabLocalSrv.GetAddressOf());
    // The buffer is private to this pass. Keep bindings explicit, but avoid renaming
    // and uploading identical contents for every small display-list draw.
    if(!lab.optimize||!mLabConstantsValid||memcmp(mLabLastConstants,&data,sizeof(data))) {
        LabUpdate(mContext.Get(),mLabCb.Get(),&data,sizeof(data));lab.constantUploads++;
        memcpy(mLabLastConstants,&data,sizeof(data));mLabConstantsValid=true;
    }
    mContext->PSSetConstantBuffers(4,1,mLabCb.GetAddressOf());
    ID3D11ShaderResourceView* views[]={mLabEnvironments[lab.environment].Get(),mLabShadowSrv.Get(),mLabBackgroundSrv.Get(),mLabBrdf.Get(),mLabDepthSrv.Get(),mLabEnvironments[lab.environmentNext].Get()};
    mContext->PSSetShaderResources(8,6,views);
    ID3D11SamplerState* samplers[]={mLabEnvironmentSampler.Get(),mLabShadowSampler.Get(),mLabClampSampler.Get()};
    mContext->PSSetSamplers(8,3,samplers);
    if(active){lab.materialDraws++;lab.phaseDraws[std::clamp(lab.phase,0,7)]++;}
    if(jelly){lab.jellyDraws++;lab.phaseJelly[std::clamp(lab.phase,0,7)]++;}
}
void GfxRenderingAPIDX11::RecordSunCaster(const float* vertices,size_t count) {
    if(!LightingLabActive()||(!gLightingLab.shadows&&!(gLightingLab.naviShadows&&gLightingLab.fairyCount>0))||!mCurrentDepthTest||!mCurrentDepthMask||mCurrentFramebuffer!=mLabGameTarget)return;
    if(mShaderProgram->numFloats<7)return;
    if(mLabVertices.size()+count>1500000){gLightingLab.shadowOverflow+=count/3;return;}
    LabShadowBatch batch={};batch.first=uint32_t(mLabVertices.size());batch.count=uint32_t(count);batch.offCamera=gLightingLab.shadowOnly;
    if(batch.offCamera)gLightingLab.offCameraTriangles+=count/3;
    if(batch.offCamera&1)gLightingLab.frustumTriangles+=count/3;
    if(batch.offCamera&4)gLightingLab.explicitTriangles+=count/3;
    int tile=mShaderProgram->usedTextures[0]?0:(mShaderProgram->usedTextures[1]?1:-1);
    if(tile>=0){batch.texture=mTextures[mCurrentTextureIds[tile]].resource_view;batch.sampler=mTextures[mCurrentTextureIds[tile]].sampler_state;}
    for(size_t i=0;i<count;i++) {
        const float* v=vertices+i*mShaderProgram->numFloats;LabShadowVertex shadow={};
        std::copy(v+mShaderProgram->numFloats-7,v+mShaderProgram->numFloats-4,shadow.position);
        if(tile>=0){shadow.uv[0]=v[4];shadow.uv[1]=v[5];}
        for(int axis=0;axis<3;axis++){batch.low[axis]=std::min(batch.low[axis],shadow.position[axis]);batch.high[axis]=std::max(batch.high[axis],shadow.position[axis]);}
        mLabVertices.push_back(shadow);
    }
    if(gLightingLab.nativeTest&&gMaterialRelief.capture>=11) {
        static int lastRequest=-1;
        int request=gMaterialRelief.capture;
        std::ofstream trace("native-casters-"+std::to_string(request)+".csv",lastRequest==request?std::ios::app:std::ios::out);lastRequest=request;
        float lo[3]={1e20f,1e20f,1e20f},hi[3]={-1e20f,-1e20f,-1e20f};
        for(size_t i=batch.first;i<mLabVertices.size();i++)for(int axis=0;axis<3;axis++){lo[axis]=std::min(lo[axis],mLabVertices[i].position[axis]);hi[axis]=std::max(hi[axis],mLabVertices[i].position[axis]);}
        trace<<int(batch.offCamera)<<","<<count<<","<<(tile>=0?sLabTextureNames[mCurrentTextureIds[tile]]:"untextured")<<","<<mShaderProgram->shader_id0;
        for(int axis=0;axis<3;axis++)trace<<","<<lo[axis]<<","<<hi[axis];trace<<"\n";
    }
    if(gLightingLab.nativeTest&&gMaterialRelief.capture>=11) {
        static int request=-1;static float frame=-1;
        bool fresh=request!=gMaterialRelief.capture;
        if(fresh){request=gMaterialRelief.capture;frame=gLightingLab.time;}
        if(frame==gLightingLab.time) {
            std::ofstream dump("native-caster-triangles-"+std::to_string(request)+".csv",fresh?std::ios::out:std::ios::app);
            for(size_t i=batch.first;i<mLabVertices.size();i+=3) {
                dump<<int(batch.offCamera)<<","<<(tile>=0?sLabTextureNames[mCurrentTextureIds[tile]]:"untextured");
                for(int j=0;j<3;j++)for(float value:mLabVertices[i+j].position)dump<<","<<value;
                dump<<"\n";
            }
        }
    }
    mLabBatches.push_back(std::move(batch));
}
void GfxRenderingAPIDX11::PrepareLabShadowBatches() {
    if(mLabPackedReady)return;
    mLabPackedVertices.clear();mLabSunBatches.clear();
    for(auto& batches:mLabNaviBatches)batches.clear();
    for(auto& source:mLabLocalBatches)for(auto& face:source)face.clear();
    // These passes write depth only. Identical texture/sampler groups can share a
    // draw without changing alpha testing or minimum depth. Filter original small
    // bounds BEFORE grouping so aggregation never adds distant geometry to Navi.
    auto pack=[&](int light,int face,std::vector<LabShadowBatch>& output) {
        using Key=std::tuple<uintptr_t,uintptr_t,uint8_t>;
        std::map<Key,std::vector<const LabShadowBatch*>> groups;
        for(const auto& batch:mLabBatches) {
            if(face>=0) {
                const float* origin=gLightingLab.fairyPosition[light];
                float radius=std::max(origin[3],1.0f),distanceSquared=0;
                for(int axis=0;axis<3;axis++) {
                    float d=origin[axis]-std::clamp(origin[axis],batch.low[axis],batch.high[axis]);
                    distanceSquared+=d*d;
                }
                if(distanceSquared>radius*radius)continue;
                const int axis=face/2;
                const float forward=(face&1)?origin[axis]-batch.low[axis]:batch.high[axis]-origin[axis];
                bool outside=forward<-.1f;
                for(int side=0;side<3;side++)if(side!=axis)
                    outside|=batch.low[side]-origin[side]>forward+.1f||origin[side]-batch.high[side]>forward+.1f;
                if(outside)continue;
            }
            groups[{reinterpret_cast<uintptr_t>(batch.texture.Get()),
                    reinterpret_cast<uintptr_t>(batch.sampler.Get()),batch.offCamera}].push_back(&batch);
        }
        for(const auto& entry:groups) {
            LabShadowBatch merged=*entry.second.front();
            merged.first=uint32_t(mLabPackedVertices.size());merged.count=0;
            for(const auto* batch:entry.second) {
                mLabPackedVertices.insert(mLabPackedVertices.end(),mLabVertices.begin()+batch->first,
                                          mLabVertices.begin()+batch->first+batch->count);
                merged.count+=batch->count;
                for(int axis=0;axis<3;axis++) {
                    merged.low[axis]=std::min(merged.low[axis],batch->low[axis]);
                    merged.high[axis]=std::max(merged.high[axis],batch->high[axis]);
                }
            }
            output.push_back(std::move(merged));
        }
    };
    if(gLightingLab.shadows)pack(-1,-1,mLabSunBatches);
    if(gLightingLab.naviShadows&&gLightingLab.naviIndex>=0)
        for(int face=0;face<6;face++)pack(gLightingLab.naviIndex,face,mLabNaviBatches[face]);
    if(gLightingLab.naviShadows)for(int i=0;i<gLightingLab.fairyCount;i++)if(i!=gLightingLab.naviIndex)
        for(int face=0;face<6;face++)pack(i,face,mLabLocalBatches[i][face]);
    mLabPackedReady=true;
}
void GfxRenderingAPIDX11::RenderSunShadows() {
    if(!LightingLabActive()||!gLightingLab.shadows||mLabVertices.empty()){mLabShadowReady=false;return;}
    InitLightingLab();LabPipelineGuard guard(mContext.Get());
    using namespace DirectX;
    auto& lab=gLightingLab;XMVECTOR center=XMVectorSet(lab.center[0],lab.center[1],lab.center[2],1);
    XMVECTOR light=XMVector3Normalize(XMVectorSet(lab.sun[0],lab.sun[1],lab.sun[2],0));
    XMVECTOR eye=XMVectorAdd(center,XMVectorScale(light,6500));
    XMVECTOR up=std::abs(lab.sun[1])>.95f?XMVectorSet(0,0,1,0):XMVectorSet(0,1,0,0);
    XMMATRIX matrix=XMMatrixLookAtLH(eye,center,up)*XMMatrixOrthographicLH(lab.shadowDistance*2,lab.shadowDistance*2,1,16000);
    XMFLOAT4X4 stored;XMStoreFloat4x4(&stored,matrix);memcpy(mLabShadowMatrix,&stored,64);
    if(lab.optimize)PrepareLabShadowBatches();
    const auto& vertices=lab.optimize?mLabPackedVertices:mLabVertices;
    const auto& batches=lab.optimize?mLabSunBatches:mLabBatches;
    size_t bytes=vertices.size()*sizeof(LabShadowVertex);
    if(bytes>mLabShadowCapacity||!mLabShadowVb) {
        mLabShadowVb.Reset();mLabShadowCapacity=UINT((std::max(bytes,size_t(1))+65535)&~size_t(65535));
        D3D11_BUFFER_DESC desc={};desc.ByteWidth=mLabShadowCapacity;desc.Usage=D3D11_USAGE_DYNAMIC;
        desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        ThrowIfFailed(mDevice->CreateBuffer(&desc,nullptr,mLabShadowVb.GetAddressOf()));
    }
    LabUpdate(mContext.Get(),mLabShadowVb.Get(),vertices.data(),bytes);
    mLabShadowVbOptimized=lab.optimize;
    mLabShadowVerticesUploaded=true;lab.shadowVertexUploads++;
    ID3D11ShaderResourceView* empty[16]={};mContext->PSSetShaderResources(0,16,empty);
    mContext->OMSetRenderTargets(0,nullptr,mLabShadowDsv.Get());mContext->ClearDepthStencilView(mLabShadowDsv.Get(),D3D11_CLEAR_DEPTH,1,0);
    D3D11_VIEWPORT viewport={0,0,2048,2048,0,1};mContext->RSSetViewports(1,&viewport);mContext->RSSetState(mLabShadowRaster.Get());
    mContext->OMSetDepthStencilState(mLabDepthOn.Get(),0);mContext->OMSetBlendState(nullptr,nullptr,~0u);
    mContext->VSSetShader(mLabShadowVs.Get(),nullptr,0);mContext->PSSetShader(mLabShadowPs.Get(),nullptr,0);
    mContext->IASetInputLayout(mLabShadowLayout.Get());UINT stride=sizeof(LabShadowVertex),offset=0;
    mContext->IASetVertexBuffers(0,1,mLabShadowVb.GetAddressOf(),&stride,&offset);mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mContext->VSSetConstantBuffers(0,1,mLabShadowCb.GetAddressOf());mContext->PSSetConstantBuffers(0,1,mLabShadowCb.GetAddressOf());
    // A native diagnostic compares the same light matrix with/without off-camera casters.
    // Normal rendering uses both sets; the camera color framebuffer is never redrawn.
    const bool probe=lab.nativeTest&&gMaterialRelief.capture>=11;
    ComPtr<ID3D11Texture2D> probeStage;std::vector<float> visibleDepth;
    auto readDepth=[&]() {
        if(!probeStage) {
            D3D11_TEXTURE2D_DESC desc={};mLabShadowTexture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;
            desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
            ThrowIfFailed(mDevice->CreateTexture2D(&desc,nullptr,probeStage.GetAddressOf()));
        }
        mContext->OMSetRenderTargets(0,nullptr,nullptr);mContext->CopyResource(probeStage.Get(),mLabShadowTexture.Get());
        D3D11_MAPPED_SUBRESOURCE map={};ThrowIfFailed(mContext->Map(probeStage.Get(),0,D3D11_MAP_READ,0,&map));
        std::vector<float> pixels(2048*2048);
        for(UINT y=0;y<2048;y++)memcpy(pixels.data()+y*2048,static_cast<const uint8_t*>(map.pData)+y*map.RowPitch,2048*sizeof(float));
        mContext->Unmap(probeStage.Get(),0);mContext->OMSetRenderTargets(0,nullptr,mLabShadowDsv.Get());return pixels;
    };
    int lastAlpha=-1;
    for(int pass=0;pass<2;pass++) {
        for(const auto& batch:batches) {
            if(bool(batch.offCamera)!=(pass==1))continue;
            float data[20]={};memcpy(data,mLabShadowMatrix,64);data[16]=batch.texture?1.0f:0;
            if(!lab.optimize||lastAlpha!=int(bool(batch.texture))) {
                LabUpdate(mContext.Get(),mLabShadowCb.Get(),data,sizeof(data));lab.constantUploads++;
                lastAlpha=int(bool(batch.texture));
            }
            mContext->PSSetShaderResources(0,1,batch.texture.GetAddressOf());mContext->PSSetSamplers(0,1,batch.sampler.GetAddressOf());
            mContext->Draw(batch.count,batch.first);lab.shadowDraws++;
        }
        if(probe&&pass==0)visibleDepth=readDepth();
    }
    if(probe) {
        auto allDepth=readDepth();uint64_t changed=0;
        std::vector<uint8_t> delta(512*512);
        for(size_t i=0;i<allDepth.size();i++)if(allDepth[i]+.0000001f<visibleDepth[i])changed++;
        for(int y=0;y<512;y++)for(int x=0;x<512;x++){size_t i=size_t(y*4)*2048+x*4;delta[y*512+x]=allDepth[i]+.0000001f<visibleDepth[i]?255:0;}
        lab.offCameraDepthChanges=std::max(lab.offCameraDepthChanges,changed);
        std::ofstream file("offcamera-shadow-"+std::to_string(lab.scene)+"-"+std::to_string(lab.phase)+".pgm",std::ios::binary);
        file<<"P5\n512 512\n255\n";file.write(reinterpret_cast<const char*>(delta.data()),delta.size());
    }
    if(lab.nativeTest&&gMaterialRelief.capture>=11) {
        mContext->OMSetRenderTargets(0,nullptr,nullptr);
        D3D11_TEXTURE2D_DESC desc={};mLabShadowTexture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;
        desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> stage;
        ThrowIfFailed(mDevice->CreateTexture2D(&desc,nullptr,stage.GetAddressOf()));
        mContext->CopyResource(stage.Get(),mLabShadowTexture.Get());D3D11_MAPPED_SUBRESOURCE map={};
        ThrowIfFailed(mContext->Map(stage.Get(),0,D3D11_MAP_READ,0,&map));
        uint64_t nonclear=0;std::vector<uint8_t> image(512*512);
        for(UINT y=0;y<2048;y++) {
            const float* row=reinterpret_cast<const float*>(static_cast<const uint8_t*>(map.pData)+y*map.RowPitch);
            for(UINT x=0;x<2048;x++)if(row[x]>=0&&row[x]<.999999f)nonclear++;
            if(y%4==0)for(UINT x=0;x<512;x++)image[(y/4)*512+x]=uint8_t(std::clamp(1-row[x*4],0.0f,1.0f)*255);
        }
        mContext->Unmap(stage.Get(),0);lab.shadowNonclear=nonclear;
        std::ofstream file("sun-shadow-"+std::to_string(lab.scene)+"-"+std::to_string(lab.phase)+".pgm",std::ios::binary);
        file<<"P5\n512 512\n255\n";file.write(reinterpret_cast<const char*>(image.data()),image.size());
    }
    lab.shadowTriangles+=mLabVertices.size()/3;lab.shadowPasses++;mLabShadowReady=true;mLabShadowScene=lab.scene;
}
void GfxRenderingAPIDX11::RenderNaviShadows() {
    auto& lab=gLightingLab;
    if(!LightingLabActive()||!lab.naviShadows||lab.naviIndex<0||mLabVertices.empty()) {
        mLabNaviReady=false;return;
    }
    InitLightingLab();LabPipelineGuard guard(mContext.Get());
    using namespace DirectX;
    const float* origin=lab.fairyPosition[lab.naviIndex];
    std::copy(origin,origin+4,mLabNaviOrigin);
    const float radius=std::max(origin[3],1.0f);
    if(lab.optimize)PrepareLabShadowBatches();
    const auto& vertices=lab.optimize?mLabPackedVertices:mLabVertices;
    size_t bytes=vertices.size()*sizeof(LabShadowVertex);
    if(bytes>mLabShadowCapacity||!mLabShadowVb) {
        mLabShadowVb.Reset();mLabShadowCapacity=UINT((std::max(bytes,size_t(1))+65535)&~size_t(65535));
        D3D11_BUFFER_DESC desc={};desc.ByteWidth=mLabShadowCapacity;desc.Usage=D3D11_USAGE_DYNAMIC;
        desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        ThrowIfFailed(mDevice->CreateBuffer(&desc,nullptr,mLabShadowVb.GetAddressOf()));
    }
    if(!lab.optimize||!mLabShadowVerticesUploaded||mLabShadowVbOptimized!=lab.optimize) {
        if(bytes) {LabUpdate(mContext.Get(),mLabShadowVb.Get(),vertices.data(),bytes);lab.shadowVertexUploads++;}
        mLabShadowVbOptimized=lab.optimize;
        mLabShadowVerticesUploaded=true;
    }
    ID3D11ShaderResourceView* empty[16]={};mContext->PSSetShaderResources(0,16,empty);
    D3D11_VIEWPORT viewport={0,0,1024,1024,0,1};mContext->RSSetViewports(1,&viewport);
    // Receiver bias is in world units; no directional-shadow raster bias here.
    mContext->RSSetState(mLabRaster.Get());mContext->OMSetDepthStencilState(mLabDepthOn.Get(),0);
    mContext->OMSetBlendState(nullptr,nullptr,~0u);
    mContext->VSSetShader(mLabNaviVs.Get(),nullptr,0);mContext->PSSetShader(mLabNaviPs.Get(),nullptr,0);
    mContext->IASetInputLayout(mLabShadowLayout.Get());UINT stride=sizeof(LabShadowVertex),offset=0;
    mContext->IASetVertexBuffers(0,1,mLabShadowVb.GetAddressOf(),&stride,&offset);
    mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mContext->VSSetConstantBuffers(0,1,mLabShadowCb.GetAddressOf());mContext->PSSetConstantBuffers(0,1,mLabShadowCb.GetAddressOf());
    static const float direction[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    static const float up[6][3]={{0,1,0},{0,1,0},{0,0,-1},{0,0,1},{0,1,0},{0,1,0}};
    XMVECTOR eye=XMVectorSet(origin[0],origin[1],origin[2],1);
    for(int face=0;face<6;face++) {
        mContext->OMSetRenderTargets(0,nullptr,mLabNaviDsv[face].Get());
        mContext->ClearDepthStencilView(mLabNaviDsv[face].Get(),D3D11_CLEAR_DEPTH,1,0);
        XMVECTOR look=XMVectorSet(direction[face][0],direction[face][1],direction[face][2],0);
        XMVECTOR vertical=XMVectorSet(up[face][0],up[face][1],up[face][2],0);
        XMMATRIX matrix=XMMatrixLookToLH(eye,look,vertical)*XMMatrixPerspectiveFovLH(XM_PIDIV2,1,.5f,radius);
        XMFLOAT4X4 stored;XMStoreFloat4x4(&stored,matrix);
        int lastAlpha=-1;
        const auto& batches=lab.optimize?mLabNaviBatches[face]:mLabBatches;
        for(const auto& batch:batches) {
            float distanceSquared=0;
            for(int axis=0;axis<3;axis++) {
                float closest=std::clamp(origin[axis],batch.low[axis],batch.high[axis]);
                float d=origin[axis]-closest;distanceSquared+=d*d;
            }
            if(distanceSquared>radius*radius)continue;
            float data[24]={};memcpy(data,&stored,64);data[16]=batch.texture?1.0f:0;
            std::copy(origin,origin+4,data+20);
            if(!lab.optimize||lastAlpha!=int(bool(batch.texture))) {
                LabUpdate(mContext.Get(),mLabShadowCb.Get(),data,sizeof(data));lab.constantUploads++;
                lastAlpha=int(bool(batch.texture));
            }
            mContext->PSSetShaderResources(0,1,batch.texture.GetAddressOf());mContext->PSSetSamplers(0,1,batch.sampler.GetAddressOf());
            mContext->Draw(batch.count,batch.first);lab.shadowDraws++;lab.naviShadowTriangles+=batch.count/3;
        }
        lab.naviShadowPasses++;
    }
    mContext->OMSetRenderTargets(0,nullptr,nullptr);
    if(lab.nativeTest&&lab.capture>=4200&&lab.capture<4300) {
        D3D11_TEXTURE2D_DESC desc={};mLabNaviTexture->GetDesc(&desc);
        desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
        ComPtr<ID3D11Texture2D> stage;ThrowIfFailed(mDevice->CreateTexture2D(&desc,nullptr,stage.GetAddressOf()));
        mContext->CopyResource(stage.Get(),mLabNaviTexture.Get());lab.naviDepthPixels=0;
        for(int face=0;face<6;face++) {
            D3D11_MAPPED_SUBRESOURCE map={};ThrowIfFailed(mContext->Map(stage.Get(),face,D3D11_MAP_READ,0,&map));
            std::vector<uint8_t> image(256*256);
            for(int y=0;y<1024;y++) {
                const float* row=reinterpret_cast<const float*>(static_cast<const uint8_t*>(map.pData)+y*map.RowPitch);
                for(int x=0;x<1024;x++)if(row[x]>=0&&row[x]<.99999f)lab.naviDepthPixels++;
                if(y%4==0)for(int x=0;x<256;x++)image[(y/4)*256+x]=uint8_t(std::clamp(1-row[x*4],0.0f,1.0f)*255);
            }
            mContext->Unmap(stage.Get(),face);
            std::ofstream file("navi-depth-"+std::to_string(lab.phase)+"-"+std::to_string(face)+".pgm",std::ios::binary);
            file<<"P5\n256 256\n255\n";file.write(reinterpret_cast<const char*>(image.data()),image.size());
        }
    }
    mLabNaviReady=true;mLabNaviScene=lab.scene;
}

void GfxRenderingAPIDX11::RenderLocalShadows() {
    auto& lab=gLightingLab;
    if(!LightingLabActive()||!lab.naviShadows||lab.fairyCount==0||mLabVertices.empty()) {
        mLabLocalCount=0;return;
    }
    InitLightingLab();LabPipelineGuard guard(mContext.Get());
    using namespace DirectX;

    if(lab.optimize)PrepareLabShadowBatches();
    const auto& vertices=lab.optimize?mLabPackedVertices:mLabVertices;
    size_t bytes=vertices.size()*sizeof(LabShadowVertex);
    if(bytes>mLabShadowCapacity||!mLabShadowVb) {
        mLabShadowVb.Reset();mLabShadowCapacity=UINT((std::max(bytes,size_t(1))+65535)&~size_t(65535));
        D3D11_BUFFER_DESC desc={};desc.ByteWidth=mLabShadowCapacity;desc.Usage=D3D11_USAGE_DYNAMIC;
        desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        ThrowIfFailed(mDevice->CreateBuffer(&desc,nullptr,mLabShadowVb.GetAddressOf()));
    }
    if(!lab.optimize||!mLabShadowVerticesUploaded||mLabShadowVbOptimized!=lab.optimize) {
        if(bytes) {LabUpdate(mContext.Get(),mLabShadowVb.Get(),vertices.data(),bytes);lab.shadowVertexUploads++;}
        mLabShadowVbOptimized=lab.optimize;
        mLabShadowVerticesUploaded=true;
    }
    ID3D11ShaderResourceView* empty[16]={};mContext->PSSetShaderResources(0,16,empty);
    D3D11_VIEWPORT viewport={0,0,512,512,0,1};mContext->RSSetViewports(1,&viewport);
    // Receiver bias is in world units; no directional-shadow raster bias here.
    mContext->RSSetState(mLabRaster.Get());mContext->OMSetDepthStencilState(mLabDepthOn.Get(),0);
    mContext->OMSetBlendState(nullptr,nullptr,~0u);
    mContext->VSSetShader(mLabNaviVs.Get(),nullptr,0);mContext->PSSetShader(mLabNaviPs.Get(),nullptr,0);
    mContext->IASetInputLayout(mLabShadowLayout.Get());UINT stride=sizeof(LabShadowVertex),offset=0;
    mContext->IASetVertexBuffers(0,1,mLabShadowVb.GetAddressOf(),&stride,&offset);
    mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mContext->VSSetConstantBuffers(0,1,mLabShadowCb.GetAddressOf());mContext->PSSetConstantBuffers(0,1,mLabShadowCb.GetAddressOf());
    static const float direction[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    static const float up[6][3]={{0,1,0},{0,1,0},{0,0,-1},{0,0,1},{0,1,0},{0,1,0}};
    int count=lab.fairyCount-(lab.naviIndex>=0?1:0);
    if(count>mLabLocalCapacity) {
        mLabLocalSrv.Reset();mLabLocalTexture.Reset();mLabLocalDsv.clear();
        D3D11_TEXTURE2D_DESC desc={};desc.Width=desc.Height=512;desc.MipLevels=1;desc.ArraySize=count*6;
        desc.Format=DXGI_FORMAT_R32_TYPELESS;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;
        desc.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
        ThrowIfFailed(mDevice->CreateTexture2D(&desc,nullptr,mLabLocalTexture.GetAddressOf()));
        D3D11_SHADER_RESOURCE_VIEW_DESC srv={};srv.Format=DXGI_FORMAT_R32_FLOAT;
        srv.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2DARRAY;srv.Texture2DArray.MipLevels=1;srv.Texture2DArray.ArraySize=count*6;
        ThrowIfFailed(mDevice->CreateShaderResourceView(mLabLocalTexture.Get(),&srv,mLabLocalSrv.GetAddressOf()));
        mLabLocalDsv.resize(count*6);
        for(int slice=0;slice<count*6;slice++) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsv={};dsv.Format=DXGI_FORMAT_D32_FLOAT;
            dsv.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsv.Texture2DArray.FirstArraySlice=slice;dsv.Texture2DArray.ArraySize=1;
            ThrowIfFailed(mDevice->CreateDepthStencilView(mLabLocalTexture.Get(),&dsv,mLabLocalDsv[slice].GetAddressOf()));
        }
        mLabLocalCapacity=count;
    }
    int slot=0;
    for(int source=0;source<lab.fairyCount;source++) {
    if(source==lab.naviIndex)continue;
    const float* origin=lab.fairyPosition[source];
    const float radius=std::max(origin[3],1.0f);
    mLabLocalKeys[slot]=lab.fairyKey[source];
    std::copy(origin,origin+4,mLabLocalOrigin[slot]);
    XMVECTOR eye=XMVectorSet(origin[0],origin[1],origin[2],1);
    for(int face=0;face<6;face++) {
        mContext->OMSetRenderTargets(0,nullptr,mLabLocalDsv[slot*6+face].Get());
        mContext->ClearDepthStencilView(mLabLocalDsv[slot*6+face].Get(),D3D11_CLEAR_DEPTH,1,0);
        XMVECTOR look=XMVectorSet(direction[face][0],direction[face][1],direction[face][2],0);
        XMVECTOR vertical=XMVectorSet(up[face][0],up[face][1],up[face][2],0);
        XMMATRIX matrix=XMMatrixLookToLH(eye,look,vertical)*XMMatrixPerspectiveFovLH(XM_PIDIV2,1,.5f,radius);
        XMFLOAT4X4 stored;XMStoreFloat4x4(&stored,matrix);
        int lastAlpha=-1;
        const auto& batches=lab.optimize?mLabLocalBatches[source][face]:mLabBatches;
        for(const auto& batch:batches) {
            float distanceSquared=0;
            for(int axis=0;axis<3;axis++) {
                float closest=std::clamp(origin[axis],batch.low[axis],batch.high[axis]);
                float d=origin[axis]-closest;distanceSquared+=d*d;
            }
            if(distanceSquared>radius*radius)continue;
            float data[24]={};memcpy(data,&stored,64);data[16]=batch.texture?1.0f:0;
            std::copy(origin,origin+4,data+20);
            if(!lab.optimize||lastAlpha!=int(bool(batch.texture))) {
                LabUpdate(mContext.Get(),mLabShadowCb.Get(),data,sizeof(data));lab.constantUploads++;
                lastAlpha=int(bool(batch.texture));
            }
            mContext->PSSetShaderResources(0,1,batch.texture.GetAddressOf());mContext->PSSetSamplers(0,1,batch.sampler.GetAddressOf());
            mContext->Draw(batch.count,batch.first);lab.shadowDraws++;lab.naviShadowTriangles+=batch.count/3;
        }
        lab.naviShadowPasses++;
    }
    mContext->OMSetRenderTargets(0,nullptr,nullptr);
    slot++;
    }
    mLabLocalCount=slot;mLabLocalScene=lab.scene;
}

void GfxRenderingAPIDX11::ValidateLabShadows() {
    // Render the *same* geometry and light matrices both ways, then compare every
    // depth texel, including all six 1024-square Navi faces. Never timed as gameplay.
    auto read=[&](ID3D11Texture2D* source) {
        std::vector<float> result;
        if(!source)return result;
        D3D11_TEXTURE2D_DESC desc={};source->GetDesc(&desc);
        desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
        ComPtr<ID3D11Texture2D> stage;ThrowIfFailed(mDevice->CreateTexture2D(&desc,nullptr,stage.GetAddressOf()));
        mContext->CopyResource(stage.Get(),source);
        for(UINT slice=0;slice<desc.ArraySize;slice++) {
            D3D11_MAPPED_SUBRESOURCE map={};ThrowIfFailed(mContext->Map(stage.Get(),slice,D3D11_MAP_READ,0,&map));
            size_t start=result.size();result.resize(start+size_t(desc.Width)*desc.Height);
            for(UINT y=0;y<desc.Height;y++)memcpy(result.data()+start+size_t(y)*desc.Width,
                static_cast<const uint8_t*>(map.pData)+y*map.RowPitch,desc.Width*sizeof(float));
            mContext->Unmap(stage.Get(),slice);
        }
        return result;
    };
    auto& lab=gLightingLab;
    const bool saved=lab.optimize;
    lab.optimize=false;RenderSunShadows();RenderNaviShadows();RenderLocalShadows();
    auto sunReference=read(mLabShadowReady?mLabShadowTexture.Get():nullptr);
    auto naviReference=read(mLabNaviReady?mLabNaviTexture.Get():nullptr);
    auto localReference=read(mLabLocalCount?mLabLocalTexture.Get():nullptr);
    lab.optimize=true;RenderSunShadows();RenderNaviShadows();RenderLocalShadows();
    auto sun=read(mLabShadowReady?mLabShadowTexture.Get():nullptr);
    auto navi=read(mLabNaviReady?mLabNaviTexture.Get():nullptr);
    auto local=read(mLabLocalCount?mLabLocalTexture.Get():nullptr);
    auto compare=[](const std::vector<float>& a,const std::vector<float>& b) {
        size_t changed=0;
        for(size_t i=0;i<a.size()&&i<b.size();i++)if(a[i]!=b[i])changed++;
        return changed+(a.size()>b.size()?a.size()-b.size():b.size()-a.size());
    };
    std::ofstream file("performance-shadow-validation.csv",std::ios::app);
    file<<lab.performanceFrame<<","<<sun.size()<<","<<compare(sunReference,sun)<<","<<navi.size()<<","<<compare(naviReference,navi)<<","<<local.size()<<","<<compare(localReference,local)<<"\n";
    std::ofstream localStats("performance-local-depth.csv",std::ios::app);
    localStats<<lab.performanceFrame<<","<<mLabLocalCount<<","<<local.size()<<","<<
        std::count_if(local.begin(),local.end(),[](float depth){return depth>=0&&depth<.99999f;})<<"\n";
    lab.optimize=saved;lab.validateShadows=false;
}

void GfxRenderingAPIDX11::CaptureJellyScene() {
    if(mLabBackgroundReady||mCurrentFramebuffer<0)return;
    auto& fb=mFrameBuffers[mCurrentFramebuffer];auto& color=mTextures[fb.texture_id];
    D3D11_TEXTURE2D_DESC desc={};color.texture->GetDesc(&desc);if(desc.SampleDesc.Count!=1)return;
    D3D11_TEXTURE2D_DESC existing={};if(mLabBackground)mLabBackground->GetDesc(&existing);
    if(!mLabBackground||existing.Width!=desc.Width||existing.Height!=desc.Height||existing.Format!=desc.Format) {
        mLabBackground.Reset();mLabBackgroundSrv.Reset();mLabDepthCopy.Reset();mLabDepthSrv.Reset();
        desc.MipLevels=0;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;desc.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;
        ThrowIfFailed(mDevice->CreateTexture2D(&desc,nullptr,mLabBackground.GetAddressOf()));
        ThrowIfFailed(mDevice->CreateShaderResourceView(mLabBackground.Get(),nullptr,mLabBackgroundSrv.GetAddressOf()));
        if(fb.depth_stencil_view) {
            ComPtr<ID3D11Resource> resource;fb.depth_stencil_view->GetResource(resource.GetAddressOf());
            ComPtr<ID3D11Texture2D> depth;ThrowIfFailed(resource.As(&depth));depth->GetDesc(&desc);
            desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.MiscFlags=0;
            ThrowIfFailed(mDevice->CreateTexture2D(&desc,nullptr,mLabDepthCopy.GetAddressOf()));
            D3D11_SHADER_RESOURCE_VIEW_DESC view={};view.Format=DXGI_FORMAT_R32_FLOAT;view.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;view.Texture2D.MipLevels=1;
            ThrowIfFailed(mDevice->CreateShaderResourceView(mLabDepthCopy.Get(),&view,mLabDepthSrv.GetAddressOf()));
        }
    }
    ID3D11ShaderResourceView* empty[3]={};mContext->PSSetShaderResources(10,3,empty);
    mContext->CopySubresourceRegion(mLabBackground.Get(),0,0,0,0,color.texture.Get(),0,nullptr);
    mContext->GenerateMips(mLabBackgroundSrv.Get());
    if(mLabDepthCopy&&fb.depth_stencil_view) {
        ComPtr<ID3D11Resource> resource;fb.depth_stencil_view->GetResource(resource.GetAddressOf());
        mContext->CopyResource(mLabDepthCopy.Get(),resource.Get());mLabBackgroundReady=true;
    }
}
void GfxRenderingAPIDX11::ProcessLightingPost(int framebuffer) {
    if(!LightingLabActive()||framebuffer!=mLabGameTarget)return;
    InitLightingLab();LabPipelineGuard guard(mContext.Get());auto& source=mTextures[mFrameBuffers[framebuffer].texture_id];
    if(!source.resource_view)return;
    if(source.width!=mLabPostWidth||source.height!=mLabPostHeight||!mLabPostTextures[3]) {
        mLabPostWidth=source.width;mLabPostHeight=source.height;
        for(int i=0;i<4;i++) {
            mLabPostTextures[i].Reset();mLabPostTargets[i].Reset();mLabPostViews[i].Reset();
            D3D11_TEXTURE2D_DESC desc={};desc.Width=i==3?source.width:std::max(1u,source.width/4);
            desc.Height=i==3?source.height:std::max(1u,source.height/4);desc.MipLevels=desc.ArraySize=1;
            desc.Format=i==3?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_R16G16B16A16_FLOAT;desc.SampleDesc.Count=1;
            desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
            ThrowIfFailed(mDevice->CreateTexture2D(&desc,nullptr,mLabPostTextures[i].GetAddressOf()));
            ThrowIfFailed(mDevice->CreateRenderTargetView(mLabPostTextures[i].Get(),nullptr,mLabPostTargets[i].GetAddressOf()));
            ThrowIfFailed(mDevice->CreateShaderResourceView(mLabPostTextures[i].Get(),nullptr,mLabPostViews[i].GetAddressOf()));
        }
    }
    auto& lab=gLightingLab;
    auto pass=[&](int target,ID3D11ShaderResourceView* texture,float mode,float dx,float dy) {
        ID3D11ShaderResourceView* empty[16]={};mContext->PSSetShaderResources(0,16,empty);
        mContext->OMSetRenderTargets(1,mLabPostTargets[target].GetAddressOf(),nullptr);
        D3D11_VIEWPORT viewport={0,0,float(target==3?source.width:std::max(1u,source.width/4)),float(target==3?source.height:std::max(1u,source.height/4)),0,1};
        mContext->RSSetViewports(1,&viewport);mContext->RSSetState(mLabRaster.Get());mContext->OMSetDepthStencilState(mLabDepthOff.Get(),0);
        mContext->OMSetBlendState(nullptr,nullptr,~0u);mContext->IASetInputLayout(nullptr);mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mContext->VSSetShader(mLabPostVs.Get(),nullptr,0);mContext->PSSetShader(mLabPostPs.Get(),nullptr,0);
        float data[16]={lab.exposure,lab.contrast,lab.saturation,lab.gamma,lab.glowStrength,lab.glowThreshold,lab.grading?1.0f:0,lab.glow?1.0f:0,
                        dx,dy,mode,lab.grading?1.0f:0,1,1,0,0};
        LabUpdate(mContext.Get(),mLabPostCb.Get(),data,sizeof(data));mContext->PSSetConstantBuffers(0,1,mLabPostCb.GetAddressOf());
        ID3D11ShaderResourceView* resources[]={texture,target==3?mLabPostViews[2].Get():nullptr};mContext->PSSetShaderResources(0,2,resources);
        mContext->PSSetSamplers(0,1,mLabClampSampler.GetAddressOf());mContext->Draw(3,0);
    };
    if(lab.glow){pass(0,source.resource_view.Get(),1,0,0);pass(1,mLabPostViews[0].Get(),2,4.0f/source.width,0);pass(2,mLabPostViews[1].Get(),2,0,4.0f/source.height);}
    pass(3,source.resource_view.Get(),0,0,0);mLabPostReady=true;lab.postPasses++;
}
void GfxRenderingAPIDX11::CopyLightingFramebuffer(int dst,int src,int sx0,int sy0,int sx1,int sy1,int dx0,int dy0,int dx1,int dy1) {
    InitLightingLab();LabPipelineGuard guard(mContext.Get());auto& source=mTextures[mFrameBuffers[src].texture_id];
    if(!mFrameBuffers[dst].render_target_view)return;
    ComPtr<ID3D11Texture2D> readableCopy;ComPtr<ID3D11ShaderResourceView> readableView;
    ID3D11ShaderResourceView* sourceView=source.resource_view.Get();
    if(!sourceView) {
        D3D11_TEXTURE2D_DESC desc={};source.texture->GetDesc(&desc);UINT samples=desc.SampleDesc.Count;
        desc.SampleDesc.Count=1;desc.SampleDesc.Quality=0;desc.MipLevels=1;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        desc.Usage=D3D11_USAGE_DEFAULT;desc.CPUAccessFlags=0;desc.MiscFlags=0;
        ThrowIfFailed(mDevice->CreateTexture2D(&desc,nullptr,readableCopy.GetAddressOf()));
        if(samples>1)mContext->ResolveSubresource(readableCopy.Get(),0,source.texture.Get(),0,desc.Format);
        else mContext->CopyResource(readableCopy.Get(),source.texture.Get());
        ThrowIfFailed(mDevice->CreateShaderResourceView(readableCopy.Get(),nullptr,readableView.GetAddressOf()));sourceView=readableView.Get();
    }
    ID3D11ShaderResourceView* empty[16]={};mContext->PSSetShaderResources(0,16,empty);
    mContext->OMSetRenderTargets(1,mFrameBuffers[dst].render_target_view.GetAddressOf(),nullptr);
    D3D11_VIEWPORT viewport={float(dx0),float(dy0),float(std::max(1,dx1-dx0)),float(std::max(1,dy1-dy0)),0,1};
    mContext->RSSetViewports(1,&viewport);mContext->RSSetState(mLabRaster.Get());mContext->OMSetDepthStencilState(mLabDepthOff.Get(),0);
    mContext->OMSetBlendState(nullptr,nullptr,~0u);mContext->IASetInputLayout(nullptr);mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mContext->VSSetShader(mLabPostVs.Get(),nullptr,0);mContext->PSSetShader(mLabPostPs.Get(),nullptr,0);
    float data[16]={0,1,1,1,0,0,0,0,0,0,0,0,float(sx1-sx0)/source.width,float(sy1-sy0)/source.height,float(sx0)/source.width,float(sy0)/source.height};
    LabUpdate(mContext.Get(),mLabPostCb.Get(),data,sizeof(data));mContext->PSSetConstantBuffers(0,1,mLabPostCb.GetAddressOf());
    mContext->PSSetShaderResources(0,1,&sourceView);mContext->PSSetSamplers(0,1,mLabClampSampler.GetAddressOf());mContext->Draw(3,0);
}
} // namespace Fast
