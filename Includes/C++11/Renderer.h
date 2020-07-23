#ifndef RENDERER_H
#define RENDERER_H
#include <LWPlatform/LWWindow.h>
#include <LWVideo/LWTypes.h>
#include <LWETypes.h>
#include <LWCore/LWSVector.h>
#include <LWCore/LWSMatrix.h>
#include <LWCore/LWSQuaternion.h>
#include <LWEUIManager.h>
#include "Config.h"
#include "Material.h"
#include "Light.h"
#include <atomic>
#include <array>

//TODO: Fix reflection maps for IBL processing.

const uint32_t MaxPasses = 8; //Actual passes we can use
const uint32_t MaxRawPasses = 32; //Extra passes for cubemap's to use.
const uint32_t MaxLightsPerTile = 64;
const uint32_t MaxLights = 1024;
const uint32_t MaxBones = 32;
const uint32_t MaxPassElements = 4096 * 16;
const uint32_t MaxAnimations = 1024 * 16;
const uint32_t MaxModels = 8192 * 8;
const uint32_t MaxPendingGeometry = 1024;
const uint32_t MaxPendingTexture = 1024;

const uint32_t RenderDefault = 0;
const uint32_t RenderEmissions = 1;
const uint32_t RenderNormals = 2;
const uint32_t RenderAlbedo = 3;
const uint32_t RenderMetallic = 4;
const uint32_t RenderBits = 0xFF;
const uint32_t RenderIBLFlag = 0x80000000;

const int32_t RenderCount = 5;

class Mesh;

class Camera;

class Material;

struct Primitive;

struct RenderSettings {
	static const uint32_t ShadowQuality_Ultra = 0;
	static const uint32_t ShadowQuality_High = 1;
	static const uint32_t ShadowQuality_Med = 2;
	static const uint32_t ShadowQuality_Low = 3;
	static const uint32_t ShadowQuality_Count = 4;

	static const uint32_t ReflectionQuality_Ultra = 0;
	static const uint32_t ReflectionQuality_High = 1;
	static const uint32_t ReflectionQuality_Med = 2;
	static const uint32_t ReflectionQuality_Low = 3;
	static const uint32_t ReflectionQuality_Count = 4;

	uint32_t m_ShadowQuality = ShadowQuality_Ultra;
	uint32_t m_ReflectionQuality = ReflectionQuality_Ultra;
	uint32_t m_SampleCount = 4;

	RenderSettings() = default;
};

struct ParticleVert {
	LWSVector4f m_Position;
	LWVector4f m_TexCoord;
	LWSVector4f m_Tangent;
	LWSVector4f m_Normal;
};

struct PendingGeometry {
	char *m_Data = nullptr;
	uint32_t m_ID;
	uint32_t m_BufferType;
	uint32_t m_TypeSize;
	uint32_t m_Count;

	LWVideoBuffer *MakeBuffer(LWVideoDriver *Driver, LWAllocator &Allocator);

	void Finished(void);

	PendingGeometry(char *Data, uint32_t ID, uint32_t BufferType, uint32_t DataTypeSize, uint32_t Count);

	PendingGeometry() = default;
};

struct PendingTexture {
	LWImage *m_Image = nullptr;
	uint32_t m_ID;
};

struct GLight {
	LWSVector4f m_Position;
	LWSVector4f m_Direction;
	LWVector4f m_Color;
	LWVector4i m_ShadowIdxs;
};

struct GGlobalData {
	LWSMatrix4f ProjViewMatrixs[MaxRawPasses];
	LWSVector4f ViewPositions[MaxRawPasses];
	LWVector4f TargetValues[MaxRawPasses];
	LWSVector4f SunDirection;
	LWVector2f ScreenSize;
	LWVector2f iShadowCubeSize;
	LWVector2i ThreadDimensions;
	LWVector2i TileSize;
	int32_t LightCount;
	int32_t RenderOutput = 0;
};

struct GGaussianKernel {
	LWVector4f m_Factor;
	LWVector4f m_Direction;

	static void MakeKernel(LWVideoDriver *Driver, uint32_t Offset, float Radi, const LWVector2i &FBSize, char *KernelBuffer);
};

struct GPassData {
	LWSVector4f FrustrumPoints[6];
	int32_t PassIndex;
	int32_t Pad[3];
};

struct GMaterial {
	static const uint32_t NormalTexBit = 0x1;
	static const uint32_t OcclussionTexBit = 0x2;
	static const uint32_t EmissiveTexBit = 0x4;
	
	static const uint32_t NormalTexID = 0;
	static const uint32_t OcclussionTexID = 1;
	static const uint32_t EmissiveTexID = 2;

	static const uint32_t PBRAlbedoTexBit = 0x8;
	static const uint32_t PBRMetallicRoughnessTexBit = 0x10;
	static const uint32_t PBRAlbedoTexID = 3;
	static const uint32_t PBRMetallicRoughnessTexID = 4;

	static const uint32_t SGDiffuseColorTexBit = 0x8;
	static const uint32_t SGSpecularColorTexBit = 0x10;
	static const uint32_t SGDiffuseColorTexID = 3;
	static const uint32_t SGSpecularColorTexID = 4;

	static const uint32_t ULColorTexBit = 0x8;
	static const uint32_t ULColorTexID = 3;

	static const uint32_t SBBackTexBit = 0x1;
	static const uint32_t SBHorizonTexBit = 0x2;
	static const uint32_t SBGlowTexBit = 0x4;
	static const uint32_t SBBackTexID = 0;
	static const uint32_t SBHorizonTexID = 1;
	static const uint32_t SBGlowTexID = 2;

	LWVector4f MaterialColorA = LWVector4f(1.0f);
	LWVector4f MaterialColorB = LWVector4f(1.0f);
	LWVector4f EmissiveFactor = LWVector4f(0.0f);
	LWVector4f SubTextures[MaxTextures];
	uint32_t HasTexturesFlag = 0;
	uint32_t Pad[3];

	GMaterial() = default;

};


struct GModelData {
	LWSMatrix4f TransformMatrix;
	GMaterial Material;
};

struct GAnimData {
	LWSMatrix4f BoneMatrixs[MaxBones];
};

struct GModelTexture {
	uint32_t m_TextureID = 0;
	uint32_t m_TextureState = 0;

	GModelTexture(uint32_t TextureID, uint32_t TextureState);

	GModelTexture() = default;
};

struct  GFrameModel {
	static const uint32_t NoDepthOut = 0x1;
	static const uint32_t ForceDrawFirst = 0x2;
	static const uint32_t ForceDrawLast = 0x4;
	static const uint32_t ForceTransparency = 0x8;

	uint32_t m_VerticeID;
	uint32_t m_IndiceID;

	GModelTexture m_TextureList[MaxTextures];
	uint32_t m_PipelineID;
	uint32_t m_BufferIDs = 0;
	uint32_t m_Offset = 0;
	uint32_t m_Count = 0;
	uint32_t m_Flags = 0;

	void SetBufferIDs(uint32_t ModelID, uint32_t AnimID);

	uint32_t GetModelBufferID(void) const;

	uint32_t GetAnimBufferID(void) const;

	GFrameModel(uint32_t PipelineID, uint32_t VerticeID, uint32_t IndiceID, uint32_t Flags=0, uint32_t Offset = 0, uint32_t Count = 0);

	GFrameModel() = default;
};

struct GElement {
	uint32_t m_Index;
	float m_DistanceSq;

	bool operator < (const GElement &E) const;

	bool operator > (const GElement &E) const;

	GElement(uint32_t Index, float DistanceSq);

	GElement() = default;
};

struct GFramePass {
	enum {
		Shadowed = 0x1,
		Point = 0x2,
		Reflection = 0x4
	};

	LWSVector4f m_Frustum[6];
	LWSVector4f m_Position;
	LWSVector4f m_Forward;
	LWSVector4f m_Right;
	LWSVector4f m_Up;
	std::array<GElement, MaxPassElements> m_OpaqueElements;
	std::array<GElement, MaxPassElements> m_TransparentElements;
	std::atomic<uint32_t> m_TransparentCount;
	std::atomic<uint32_t> m_OpaqueCount;
	uint32_t m_Flag;
	uint32_t m_SourceIndex = 0;
	uint32_t m_TargetIndex = 0;
	uint32_t m_TargetFace = 0;
	uint32_t m_FrameID = -1;

	bool isShadowed(void) const;

	bool isPoint(void) const;

	bool isReflection(void) const;

	bool isInitialized(uint32_t FrameID) const;

	bool PushElement(uint32_t ID, uint32_t Flag, const LWSVector4f &Position, bool Transparent);

	bool SphereInFrustum(const LWSVector4f &Position, float Radius);

	bool AABBInFrustum(const LWSVector4f &AAMin, const LWSVector4f &AAMax);

	bool ConeInFrustrum(const LWSVector4f &Position, const LWSVector4f &Direction, float Length, float Theta);

	bool LightInFrustrum(const Light &L);

	void FinalizePass(uint32_t FrameID);

	void Initialize(Camera &Cam, GGlobalData &GlobalBlock, GPassData *PassData, uint32_t TargetID, uint32_t TargetFace, uint32_t SourceIndex, uint32_t FrameID, uint32_t PassID);

};

struct GFrame {
	static const uint32_t MainViewPass = 0;
	static const uint32_t OutlinePass = 1;
	static const uint32_t RTFirstPass = 2;
	static const uint32_t ReflectionPass = 2;

	static const uint32_t MaxShadowPasses = 8;
	static const uint32_t CascadeCount = 2;
	static const uint32_t MaxShadowRTs = 4;

	static const uint32_t MainViewBits = 0x1;
	static const uint32_t OutlineBits = 0x2;

	static const uint32_t MaxUIElements = 1024;
	static const uint32_t MaxParticleVertices = 8048 * 4;
	LWEUIFrame m_UIFrame;
	GFramePass m_PassList[MaxRawPasses];
	GFrameModel m_ModelList[MaxModels];
	GGlobalData m_GlobalData;
	LWSVector4f m_ShadowPosition;
	LWVector4f m_ViewBounds;
	LWVector4i m_TargetViewBounds;
	LWVector2i m_TargetTextureSize;
	std::array<GElement, MaxShadowRTs+1> m_ShadowLightList;
	LWVideoDriver *m_Driver = nullptr;
	char *m_PassDataBuffer = nullptr;
	char *m_AnimDataBuffer = nullptr;
	char *m_ModelDataBuffer = nullptr;
	GLight *m_LightsBuffer = nullptr;
	ParticleVert *m_ParticleVertices = nullptr;

	uint32_t m_SpriteFrame = 0;

	uint32_t m_ShadowArrayCount = 0;
	uint32_t m_ShadowCubeCount = 0;
	uint32_t m_ReflectionBits = 0;

	uint32_t m_FrameID = -1;
	uint32_t m_AnimCount = 0;
	uint32_t m_ModelCount = 0;
	uint32_t m_ShadowCount = 0;
	uint32_t m_ParticleCount = 0;
	uint32_t m_LightCount = 0;
	uint32_t m_RawPassCount = 0;

	GFrame &InitializeFrame(uint32_t FrameID);

	GFrame &FinalizeFrame(void);

	GPassData *GetPassDataAt(uint32_t i);

	GModelData *GetModelDataAt(uint32_t i);

	GAnimData *GetAnimDataAt(uint32_t i);

	GFrame &InitializeShadowPosition(const LWSVector4f &ShadowPos);

	GFrame &InitializeSunDirection(const LWSVector4f &SunDirection);

	uint32_t InitializePass(uint32_t PassID, Camera &Cam);

	uint32_t InitializePass(uint32_t PassID, uint32_t TargetID, uint32_t TargetFace, uint32_t SourceIndex, Camera &Cam);

	GFrame &InitializeRTPasses(const LWSVector4f &SceneAABBMin, const LWSVector4f &SceneAABBMax);

	//Copys the output of ViewBounds to the TargetBounds of the texture.
	GFrame &InitializeDirectionPass(const LWVector4f &ViewBounds, const LWVector4i &TargetBounds);

	uint32_t NextAnimation(void);

	uint32_t PushAnimation(LWSMatrix4f *BoneMatrixs, uint32_t BoneCount);

	uint32_t PassBitsInSphere(const LWSVector4f &Position, float Radius, uint32_t TargetPassBits);

	uint32_t PassBitsInAABB(const LWSVector4f &AAMin, const LWSVector4f &AAMax, uint32_t TargetPassBits);

	uint32_t PushModel(GFrameModel &Mdl, uint32_t PassBits, uint32_t AnimID, const LWSMatrix4f &Transform, const GMaterial &Material, bool Transparent);

	uint32_t WriteParticles(uint32_t Count);

	uint32_t PushLight(const Light &L);

	GFrame() = default;

	GFrame(LWVideoDriver *Driver, LWAllocator &Allocator);

	~GFrame();
};

class Renderer {
public:
	static const uint32_t MaxFrames = 3;
	static const uint32_t ScreenGaussianKernel = 0;
	static const uint32_t GaussianKernelCount = 2;

	static const uint32_t MetallicRoughnessTexOffset = 7;
	static const uint32_t SpecularGlossinessTexOffset = 7;
	static const uint32_t UnlitTexOffset = 3;
	static const uint32_t SkyboxTexOffset = 0;

	Renderer &SizeUpdated(LWWindow *Window);

	Renderer &LoadAssets(LWEAssetManager *AssetMan);

	Renderer &ApplySettings(const RenderSettings &Settings);

	GFrame *BeginFrame(void);

	Renderer &EndFrame(void);

	LWPipeline *PreparePipeline(GFrame &F, const GFrameModel &Mdl, bool isSkinned, bool Transparent, bool IsShadowed);

	Renderer &ApplyFrame(GFrame &F);

	Renderer &RenderUIFrame(LWEUIFrame &UIF);

	Renderer &RenderModel(GFrame &F, const GFrameModel &Mdl, uint32_t PassID, bool Transparent, bool IsShadowed);

	Renderer &RenderPass(GFrame &F, uint32_t PassID);

	Renderer &RenderBlurPass(GFrame &F, uint32_t KernelOffset, LWFrameBuffer *FB, LWTexture *SourceTex, LWTexture *TempTexture, LWTexture *ResultTex, uint32_t ResultLayer = 0, uint32_t ResultFace = 0);

	Renderer &RenderShadowPass(GFrame &F, uint32_t PassID);

	Renderer &CopyOutput(GFrame &F);

	Renderer &Render(LWWindow *Window);

	GMaterial PrepareGMaterial(GFrameModel &Mdl, const LWVector2f &AtlasSubPosition, const LWVector2f &AtlasSubSize, float TransparencyMult, Material &Mat);

	Renderer &WriteDebugLine(GFrame &F, uint32_t PassBits, const LWSVector4f &APnt, const LWSVector4f &BPnt, float Thickness, const LWVector4f &Color, uint32_t Flags = 0);

	Renderer &WriteLine(GFrame &F, uint32_t PassBits, const LWSVector4f &APnt, const LWSVector4f &BPnt, float Thickness, Material &Mat, uint32_t Flags = 0);

	Renderer &WriteDebugPoint(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, float Radius, const LWVector4f &Color, uint32_t Flags = 0);

	Renderer &WritePoint(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, float Radius, Material &Mat, uint32_t Flags = 0);

	Renderer &WriteDebugCone(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, const LWSVector4f &Dir, float Theta, float Length, const LWVector4f &Color, uint32_t Flags = 0);

	Renderer &WriteCone(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, const LWSVector4f &Dir, float Theta, float Length, Material &Mat, uint32_t Flags = 0);

	Renderer &WriteDebugCube(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, const LWSVector4f &Size, const LWVector4f &Color, uint32_t Flags = 0);

	Renderer &WriteCube(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, const LWSVector4f &Size, Material &Mat, uint32_t Flags = 0);

	Renderer &WriteDebugAABB(GFrame &F, uint32_t PassBits, const LWSVector4f &AAMin, const LWSVector4f &AAMax, float LineThickness, const LWVector4f &Color, uint32_t Flags = 0);

	Renderer &WriteDebugAxis(GFrame &F, uint32_t PassBits, const LWSMatrix4f &Transform, float LineLen, float LineThickness, bool IgnoreScale = true, uint32_t Flags = 0);

	Renderer &WriteDebugGeometry(GFrame &F, uint32_t VerticeID, uint32_t IndiceID, uint32_t PassBits, const LWSMatrix4f &Transform, const LWVector4f &Color, uint32_t Flags = 0);

	Renderer &WriteMesh(GFrame &F, Mesh *Msh, const LWSMatrix4f *AnimTransforms, uint32_t PassBits, const LWSMatrix4f &Transform, Material &Mat, uint32_t Flags = 0);

	ParticleVert *PrepareParticleVertices(GFrame &F, uint32_t PassBits, uint32_t VerticeCount, Material &Mat, uint32_t Flags);

	ParticleVert *WriteParticleRect(ParticleVert *V, const LWSVector4f &Pos, const LWSVector4f &Right, const LWSVector4f &Up, const LWVector2f &BtmLeftTC, const LWVector2f &TopRightTC, float Transparency, const LWSVector4f &Normal, const LWSVector4f &Tangent);

	ParticleVert *WriteParticleLine(ParticleVert *V, const LWSVector4f &PntA, const LWSVector4f &PntB, const LWSVector4f &Up, float Thickness, const LWVector2f &BtmLeftTC, const LWVector2f &TopRightTC, float Transparency, const LWSVector4f &Normal, const LWSVector4f &Tangent);

	uint32_t WriteMeshPrimitive(GFrame &F, Primitive &P, Mesh *Msh, uint32_t AnimID, uint32_t PassBits, const LWSMatrix4f &Transform, Material &Mat, uint32_t Flags = 0);

	uint32_t WriteGeometry(GFrame &F, uint32_t VerticeID, uint32_t IndiceID, uint32_t AnimID, uint32_t PassBits, const LWSMatrix4f &Transform, Material &Mat, uint32_t Flags = 0, uint32_t Offset = 0, uint32_t Count = 0);

	uint32_t PushPendingGeometry(uint32_t ID, uint32_t DataType, char *Data, uint32_t DataCnt, uint32_t DataSize, LWAllocator &Allocator, bool Copy);

	template<class Type>
	uint32_t PushPendingGeometry(uint32_t ID, uint32_t DataType, Type *Data, uint32_t DataCnt, LWAllocator &Allocator, bool Copy = false) {
		return PushPendingGeometry(ID, DataType, (char*)Data, DataCnt, sizeof(Type), Allocator, Copy);
	}

	uint32_t PushPendingTexture(uint32_t ID, LWImage *Image);

	void ProcessPendingGeometry(void);

	void ProcessPendingTextures(void);

	void SetIBLBrdfTexture(LWTexture *Tex);

	void SetIBLDiffuseTexture(LWTexture *Tex);

	void SetIBLSpecularTexture(LWTexture *Tex);

	LWTexture *GetTexture(uint32_t ID);

	LWTexture *GetOutputTexture(void);

	LWVideoBuffer *GetGeometry(uint32_t ID);

	bool GeometryIsLoaded(uint32_t ID) const;

	bool TextureIsLoaded(uint32_t ID) const;

	uint32_t NextGeometryID(void);

	uint32_t NextTextureID(void);

	bool MakeConePrimitive(void);

	bool MakeCubePrimitive(void);

	bool MakeSpherePrimitive(void);

	bool MakeHalfSpherePrimitive(void);

	bool MakeSkyBoxPrimitive(void);

	bool MakePlanePrimitive(void);

	bool MakePrimitives(void);

	uint32_t GetCurrentRenderedFrame(void) const;

	uint32_t GetParticleVertID(void) const;

	uint32_t GetParticleIdxID(void) const;

	uint32_t GetCubeVertID(void) const;

	uint32_t GetCubeIdxID(void) const;

	uint32_t GetSphereVertID(void) const;

	uint32_t GetHalfSphereVertID(void) const;

	uint32_t GetConeVertID(void) const;

	uint32_t GetConeIdxID(void) const;

	uint32_t GetPlaneVertID(void) const;

	uint32_t GetSkyBoxVertID(void) const;

	uint32_t GetSkyBoxIdxID(void) const;

	RenderSettings GetSettings(void) const;

	Renderer(LWVideoDriver *Driver, LWAllocator &Allocator);

	~Renderer();
private:
	GFrame m_Frames[MaxFrames];
	PendingTexture m_PendingTextures[MaxPendingTexture];
	PendingGeometry m_PendingGeometry[MaxPendingGeometry];
	RenderSettings m_Settings;
	LWAllocator &m_Allocator;
	LWVideoDriver *m_Driver = nullptr;
	
	LWShader *m_FontShader = nullptr;
	LWShader *m_UITextureShader = nullptr;
	LWShader *m_UIColorShader = nullptr;

	LWShader *m_StaticVertexShader = nullptr;
	LWShader *m_SkeletonVertexShader = nullptr;

	LWVideoBuffer *m_UIUniform = nullptr;
	LWVideoBuffer *m_LightDataBuffer = nullptr;
	LWVideoBuffer *m_LightArrayBuffer = nullptr;
	LWVideoBuffer *m_GlobalDataBlock = nullptr;
	LWVideoBuffer *m_PassDataBlock = nullptr;
	LWVideoBuffer *m_AnimDataBlock = nullptr;
	LWVideoBuffer *m_ModelDataBlock = nullptr;

	LWVideoBuffer *m_ParticleVertBuffer = nullptr;

	LWPipeline *m_UIPipeline = nullptr;
	LWPipeline *m_MetallicRoughnessPipeline = nullptr;
	LWPipeline *m_SpecularGlossinessPipeline = nullptr;
	LWPipeline *m_UnlitPipeline = nullptr;
	LWPipeline *m_ShadowPipeline = nullptr;
	LWPipeline *m_SkyboxPipeline = nullptr;
	LWPipeline *m_CloudPipeline = nullptr;
	LWPipeline *m_LightCullPipeline = nullptr;
	LWPipeline *m_PostProcessMS = nullptr;

	LWFrameBuffer *m_ScreenFB = nullptr;
	LWTexture *m_ScreenTexMS = nullptr;
	LWTexture *m_ScreenTex = nullptr;
	LWTexture *m_FinalScreenTex = nullptr;
	LWTexture *m_EmissionTexMS = nullptr;
	LWTexture *m_EmissionTex = nullptr;
	LWTexture *m_ScreenDepth = nullptr;

	LWFrameBuffer *m_HighlightFB = nullptr;
	LWTexture *m_HighlightTex = nullptr;
	LWTexture *m_HighlightTexMS = nullptr;

	LWFrameBuffer *m_BlurFB = nullptr;
	LWTexture *m_BlurTempTexture = nullptr;
	LWTexture *m_BEmissionTexture = nullptr;
	LWTexture *m_BHighlightTexture = nullptr;

	LWPipeline *m_FinalPipeline = nullptr;
	LWPipeline *m_GaussianPipeline = nullptr;

	LWFrameBuffer *m_ReflectionFrameBuffer = nullptr;
	LWTexture *m_ReflectionDepthmap = nullptr;
	LWTexture *m_ReflectionCubemap[2] = { nullptr, nullptr };

	LWFrameBuffer *m_OutputFramebuffer = nullptr;
	LWTexture *m_OutputTexture = nullptr;

	LWFrameBuffer *m_ShadowFrameBuffer = nullptr;
	LWFrameBuffer *m_ShadowCubeFrameBuffer = nullptr;
	LWTexture *m_ShadowTextureArray = nullptr;
	LWTexture *m_ShadowCubemapArray = nullptr;

	LWVideoBuffer *m_PostProcessGeometry = nullptr;
	LWVideoBuffer *m_CopyGeometry = nullptr;
	LWVideoBuffer *m_GaussianKernel = nullptr;

	std::unordered_map<uint32_t, LWVideoBuffer*> m_GeometryMap;
	std::unordered_map<uint32_t, LWTexture*> m_TextureMap;
	uint32_t m_NextTextureID = 0;
	uint32_t m_NextGeometryID = 0;

	uint32_t m_PendingGeomReadFrame = 0;
	uint32_t m_PendingGeomWriteFrame = 0;
	uint32_t m_PendingTexReadFrame = 0;
	uint32_t m_PendingTexWriteFrame = 0;

	uint32_t m_ReadFrame = 0;
	uint32_t m_WriteFrame = 0;

	uint32_t m_ParticleVertID = 0;
	uint32_t m_ParticleIdxID = 0;

	uint32_t m_ConeVertID = 0;
	uint32_t m_ConeIdxID = 0;
	uint32_t m_CubeVertID = 0;
	uint32_t m_CubeIdxID = 0;
	uint32_t m_SphereVertID = 0;
	uint32_t m_HalfSphereVertID = 0;
	uint32_t m_PlaneVertID = 0;
	uint32_t m_SkyBoxVertID = 0;
	uint32_t m_SkyBoxIdxID = 0;
	bool m_SizeChanged = true;

};

#endif