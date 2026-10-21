#pragma once

#if defined(ENABLE_DX11) || defined(ENABLE_DX12)

#ifdef __cplusplus
#include "../interpreter.h"
#include <cstdint>
#include <string>
#include "gfx_rendering_api.h"
#include "d3d11.h"
#include "d3dcompiler.h"

namespace Fast {

struct PerFrameCB {
    uint32_t noise_frame;
    float noise_scale;
    uint32_t padding[2]; // constant buffers must be multiples of 16 bytes in size
};

struct PerDrawCB {
    struct Texture {
        uint32_t width;
        uint32_t height;
        uint32_t linear_filtering;
        uint32_t padding;
    } mTextures[SHADER_MAX_TEXTURES];
};

struct PerPrimDepthCB {
    float prim_depth;
    float _pad[3]; // 16-byte CB alignment
};

struct Coord {
    int x, y;
};

struct TextureData {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> resource_view;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_state;
    uint32_t width;
    uint32_t height;
    bool linear_filtering;
};

struct FramebufferDX11 {
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depth_stencil_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depth_stencil_srv;
    uint32_t texture_id;
    bool has_depth_buffer;
    uint32_t msaa_level;
};

struct ShaderProgramD3D11 {
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state;

    uint64_t shader_id0;
    uint64_t shader_id1;
    uint8_t numInputs;
    uint8_t numFloats;
    bool usedTextures[SHADER_MAX_TEXTURES];
};

class GfxWindowBackendDXGI;

class GfxRenderingAPIDX11 final : public GfxRenderingAPI {
  public:
    GfxRenderingAPIDX11() = default;
    ~GfxRenderingAPIDX11() override;
    GfxRenderingAPIDX11(GfxWindowBackendDXGI* backend);
    const char* GetName() override;
    bool WantsViewPosition() const override { return true; }
    void SetReliefSource(const char* path) override { mReliefSource=path ? path : ""; }
    int GetMaxTextureSize() override;
    GfxClipParameters GetClipParameters() override;
    void UnloadShader(struct ShaderProgram* oldPrg) override;
    void LoadShader(struct ShaderProgram* newPrg) override;
    struct ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) override;
    struct ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1) override;
    void ShaderGetInfo(struct ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;
    void ClearShaderCache() override;
    uint32_t NewTexture() override;
    void SelectTexture(int tile, uint32_t textureId) override;
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) override;
    void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) override;
    void SetDepthTestAndMask(bool depth_test, bool z_upd) override;
    void SetCurrentPrimDepth(float depth) override;
    void SetZmodeDecal(bool decal) override;
    void SetViewport(int x, int y, int width, int height) override;
    void SetScissor(int x, int y, int width, int height) override;
    void SetUseAlpha(bool useAlpha) override;
    void DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) override;
    void Init() override;
    void OnResize() override;
    void StartFrame() override;
    void EndFrame() override;
    void FinishRender() override;
    int CreateFramebuffer() override;
    void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                     bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                     bool can_extract_depth) override;
    void StartDrawToFramebuffer(int fbId, float noiseScale) override;
    void CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0,
                         int dstX1, int dstY1) override;
    void ClearFramebuffer(bool color, bool depth) override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) override;
    void ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) override;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) override;
    void* GetFramebufferTextureId(int fbId) override;
    void SelectTextureFb(int fbId) override;
    void DeleteTexture(uint32_t texId) override;
    void SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter() override;
    void SetSrgbMode() override;
    ImTextureID GetTextureById(int id) override;

    PFN_D3D11_CREATE_DEVICE mDX11CreateDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mContext;
    Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
    GfxWindowBackendDXGI* mWindowBackend = nullptr;
    D3D_FEATURE_LEVEL mFeatureLevel;

  private:
    void CreateDepthStencilObjects(uint32_t width, uint32_t height, uint32_t msaa_count, ID3D11DepthStencilView** view,
                                   ID3D11ShaderResourceView** srv);

    struct LabShadowVertex {float position[3],uv[2];};
    struct LabShadowBatch {
        uint32_t first,count;
        uint8_t offCamera=0;
        float low[3]={1e20f,1e20f,1e20f},high[3]={-1e20f,-1e20f,-1e20f};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
    };
    std::vector<LabShadowVertex> mLabVertices;
    std::vector<LabShadowBatch> mLabBatches;
    std::vector<LabShadowVertex> mLabPackedVertices;
    std::vector<LabShadowBatch> mLabSunBatches,mLabNaviBatches[6];
    bool mLabPackedReady=false,mLabShadowVbOptimized=false;
    void PrepareLabShadowBatches();
    bool mLabConstantsValid=false,mLabShadowVerticesUploaded=false;
    uint8_t mLabLastConstants[1984]={};
    double mLabFrameStart=0,mLabPreviousFrameStart=0;
    void ValidateLabShadows();
    std::unordered_map<uint32_t,bool> mLabJellyTextures;
    bool mLabInitialized=false,mLabShadowReady=false,mLabPostReady=false,mLabBackgroundReady=false;
    int mLabGameTarget=-1,mLabShadowScene=-1;
    uint32_t mLabPostWidth=0,mLabPostHeight=0,mLabShadowCapacity=0;
    float mLabEnvironmentSH[6][36]={},mLabShadowMatrix[16]={};
    Microsoft::WRL::ComPtr<ID3D11Buffer> mLabCb,mLabShadowCb,mLabPostCb,mLabShadowVb;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mLabEnvironmentTexture,mLabShadowTexture,mLabBackground,mLabDepthCopy;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mLabEnvironment,mLabBrdf,mLabShadowSrv,mLabBackgroundSrv,mLabDepthSrv,mLabJellyNoise;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mLabShadowDsv;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mLabEnvironmentSampler,mLabClampSampler,mLabShadowSampler,mLabNoiseSampler;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> mLabShadowVs,mLabPostVs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mLabShadowPs,mLabPostPs;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mLabShadowLayout;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mLabRaster,mLabShadowRaster;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mLabDepthOn,mLabDepthOff;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mLabPostTextures[4];
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mLabPostTargets[4];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mLabPostViews[4];
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mLabEnvironmentTextures[6];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mLabEnvironments[6];
    void InitLightingLab();
    void BindLightingLab();
    void RecordSunCaster(const float* vertices,size_t count);
    void RenderSunShadows();
    void RenderNaviShadows();
    void RenderLocalShadows();
    std::vector<LabShadowBatch> mLabLocalBatches[32][6];
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mLabLocalTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mLabLocalSrv;
    std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> mLabLocalDsv;
    int mLabLocalCapacity=0,mLabLocalCount=0,mLabLocalScene=-1;
    uintptr_t mLabLocalKeys[32]={};
    float mLabLocalOrigin[32][4]={};
    bool mLabNaviReady=false;
    int mLabNaviScene=-1;
    float mLabNaviOrigin[4]={};
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mLabNaviTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mLabNaviSrv;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mLabNaviDsv[6];
    Microsoft::WRL::ComPtr<ID3D11VertexShader> mLabNaviVs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mLabNaviPs;
    void CaptureJellyScene();
    void ProcessLightingPost(int framebuffer);
    void CopyLightingFramebuffer(int dst,int src,int sx0,int sy0,int sx1,int sy1,int dx0,int dy0,int dx1,int dy1);

    struct ReliefTexture {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
        int scene=-1;
        float normalMultiplier=1, parallaxMultiplier=1;
    };
    std::unordered_map<uint32_t,ReliefTexture> mReliefTextures;
    std::unordered_map<uint32_t,Microsoft::WRL::ComPtr<ID3D11SamplerState>> mReliefSamplers;
    std::string mReliefSource;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mReliefCb;
    void UploadRelief(const uint8_t* rgba,uint32_t width,uint32_t height);
    void BindRelief();
    void CaptureRelief();
    HMODULE mDX11Module;

    HMODULE mCompilerModule;
    pD3DCompile mD3dCompile;

    uint32_t mMsaaNumQualityLevels[D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT];

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mPerFrameCb;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mPerDrawCb;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mPerPrimDepthCb;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mCoordBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mCoordBufferSrv;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mDepthValueOutputBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mDepthValueOutputBufferCopy;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mDepthValueOutputUav;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> mComputeShader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> mComputeShaderMsaa;
    Microsoft::WRL::ComPtr<ID3DBlob> mComputeShaderMsaaBlob;
    size_t mCoordBufferSize;

#if DEBUG_D3D
    Microsoft::WRL::ComPtr<ID3D11Debug> debug;
#endif

    PerFrameCB mPerFrameCbData;
    PerDrawCB mPerDrawCbData;
    PerPrimDepthCB mPerPrimDepthCbData;

    std::map<std::pair<uint64_t, uint32_t>, struct ShaderProgramD3D11> mShaderProgramPool;

    std::vector<struct TextureData> mTextures;
    int mCurrentTile;
    uint32_t mCurrentTextureIds[SHADER_MAX_TEXTURES] = {};

    std::vector<FramebufferDX11> mFrameBuffers;

    // Current state

    struct ShaderProgramD3D11* mShaderProgram;

    int32_t mRenderTargetHeight;
    int mCurrentFramebuffer;
    FilteringMode mCurrentFilterMode = FILTER_NONE;

    // Previous states (to prevent setting states needlessly)

    struct ShaderProgramD3D11* mLastShaderProgram = nullptr;
    uint32_t mLastVertexBufferStride = 0;
    Microsoft::WRL::ComPtr<ID3D11BlendState> mLastBlendState = nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mLastResourceViews[SHADER_MAX_TEXTURES] = { nullptr, nullptr };
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mLastSamplerStates[SHADER_MAX_TEXTURES] = { nullptr, nullptr };

    D3D_PRIMITIVE_TOPOLOGY mLastPrimitaveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    // Cached staging texture for ReadFramebufferToCPU — avoids CreateTexture2D/Release per frame
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mReadbackStaging;
    uint32_t mReadbackStagingW = 0;
    uint32_t mReadbackStagingH = 0;
};

std::string gfx_direct3d_common_build_shader(size_t& numFloats, const CCFeatures& cc_features,
                                             bool include_root_signature, bool three_point_filtering, bool use_srgb);
} // namespace Fast
#endif
#endif
