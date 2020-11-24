#include "Renderer.h"
#include <LWPlatform/LWWindow.h>
#include <LWVideo/LWVideoDriver.h>
#include <LWEAsset.h>
#include <LWCore/LWMatrix.h>
#include <LWCore/LWVector.h>
#include <LWVideo/LWFrameBuffer.h>
#include <LWESGeometry3D.h>
#include "Camera.h"
#include "Logger.h"
#include "Mesh.h"

//PendingGeometry
LWVideoBuffer *PendingGeometry::MakeBuffer(LWVideoDriver *Driver, LWAllocator &Allocator) {
	LWVideoBuffer *Buf = Driver->CreateVideoBuffer(m_BufferType, LWVideoBuffer::Static, m_TypeSize, m_Count, Allocator, (uint8_t*)m_Data);
	if (!Buf) LogCritical(LWUTF8I::Fmt<128>("Error could not create buffer for id: {}", m_ID));
	return Buf;
}

void PendingGeometry::Finished(void) {
	m_Data = LWAllocator::Destroy(m_Data);
	return;
}

PendingGeometry::PendingGeometry(char *Data, uint32_t ID, uint32_t BufferType, uint32_t DataTypeSize, uint32_t Count) : m_Data(Data), m_ID(ID), m_BufferType(BufferType), m_TypeSize(DataTypeSize), m_Count(Count) {}

GModelTexture::GModelTexture(uint32_t TextureID, uint32_t TextureState) : m_TextureID(TextureID), m_TextureState(TextureState) {}

void GFrameModel::SetBufferIDs(uint32_t ModelID, uint32_t AnimID) {
	m_BufferIDs = (ModelID | (AnimID << 16));
	return;
}

uint32_t GFrameModel::GetModelBufferID(void) const {
	return (m_BufferIDs & 0xFFFF);
}

uint32_t GFrameModel::GetAnimBufferID(void) const {
	return (m_BufferIDs >> 16) & 0xFFFF;
}

GFrameModel::GFrameModel(uint32_t PipelineID, uint32_t VerticeID, uint32_t IndiceID, uint32_t Flags, uint32_t Offset, uint32_t Count) : m_VerticeID(VerticeID), m_IndiceID(IndiceID), m_PipelineID(PipelineID), m_Offset(Offset), m_Count(Count), m_Flags(Flags) {}

//GGaussianKernel
void GGaussianKernel::MakeKernel(LWVideoDriver *Driver, uint32_t Offset, float Radi, const LWVector2i &FBSize, char *KernelBuffer) {
	const LWVector4f GFactor = LWVector4f(0.06136f, 0.24477f, 0.38774f, 0.0f);
	LWVector2f iSize = Radi / FBSize.CastTo<float>();
	GGaussianKernel *HoriKernel = Driver->GetUniformPaddedAt<GGaussianKernel>(Offset, KernelBuffer);
	GGaussianKernel *VertKernel = Driver->GetUniformPaddedAt<GGaussianKernel>(Offset + 1, KernelBuffer);
	*HoriKernel = { GFactor, LWVector4f(iSize.x, 0.0f, 0.0f, 0.0f) };
	*VertKernel = { GFactor, LWVector4f(0.0f, iSize.y, 0.0f, 0.0f) };
	return;
}

//GElement
bool GElement::operator < (const GElement &E) const {
	return m_DistanceSq < E.m_DistanceSq;
}

bool GElement::operator > (const GElement &E) const {
	return m_DistanceSq > E.m_DistanceSq;
}

GElement::GElement(uint32_t Index, float DistanceSq) : m_Index(Index), m_DistanceSq(DistanceSq) {}

//GFramePass
bool GFramePass::PushElement(uint32_t ID, uint32_t Flag, const LWSVector4f &Position, bool Transparent) {
	const float Inf = 1000000.0f;
	const float Near = 0.0f;
	float DisSq = Position.DistanceSquared3(m_Position);
	if ((Flag & GFrameModel::ForceDrawFirst) != 0) DisSq = Near;
	if ((Flag & GFrameModel::ForceDrawLast) != 0) DisSq = Inf;

	if (Transparent) {
		uint32_t i = m_TransparentCount.fetch_add(1);
		if (i >= MaxPassElements) {
			LogWarn("Transparent list elements has been exhausted.");
			return false;
		}
		m_TransparentElements[i] = GElement(ID, DisSq);
	} else {
		uint32_t i = m_OpaqueCount.fetch_add(1);
		if (i >= MaxPassElements) {
			LogWarn("Opaque list elements has been exhausted.");
			return false;
		}
		m_OpaqueElements[i] = GElement(ID, DisSq);
	}
	return true;
}

void GFramePass::FinalizePass(uint32_t FrameID) {
	if (!isInitialized(FrameID)) return;
	uint32_t TransCnt = std::min<uint32_t>(m_TransparentCount.load(), MaxPassElements);
	uint32_t OpaqCnt = std::min<uint32_t>(m_OpaqueCount.load(), MaxPassElements);
	std::sort(m_TransparentElements.begin(), m_TransparentElements.begin() + TransCnt, std::greater<>());
	std::sort(m_OpaqueElements.begin(), m_OpaqueElements.begin() + OpaqCnt);
	m_TransparentCount = TransCnt;
	m_OpaqueCount = OpaqCnt;
	return;
}

bool GFramePass::isPoint(void) const {
	return (m_Flag & Point) != 0;
}

bool GFramePass::isShadowed(void) const {
	return (m_Flag & Shadowed) != 0;
}

bool GFramePass::isReflection(void) const {
	return (m_Flag & Reflection) != 0;
}

bool GFramePass::isInitialized(uint32_t FrameID) const {
	return m_FrameID == FrameID;
}

void GFramePass::Initialize(Camera &Cam, GGlobalData &GlobalBlock, GPassData *PassData, uint32_t TargetID, uint32_t TargetFace, uint32_t SourceIndex, uint32_t FrameID, uint32_t PassID) {
	const LWSVector4f FaceDirs[] = { LWSVector4f(-1.0f, 0.0f, 0.0f, 0.0f), LWSVector4f( 1.0f, 0.0f, 0.0f, 0.0f), LWSVector4f(0.0f,-1.0f, 0.0f, 0.0f), LWSVector4f(0.0f, 1.0f, 0.0f, 0.0f), LWSVector4f(0.0f, 0.0f,-1.0f, 0.0f), LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f) };
	const LWSVector4f FaceUps[] = { LWSVector4f(0.0f,-1.0f, 0.0f, 0.0f), LWSVector4f(0.0f,-1.0f, 0.0f, 0.0f), LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f), LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f), LWSVector4f(0.0f,-1.0f, 0.0f, 0.0f), LWSVector4f(0.0f,-1.0f, 0.0f, 0.0f) };

	//Need to swap y faces for openGL.
	uint32_t OGLFaceMap[] = { 0,1,3,2,4,5 };
	uint32_t Face = LWMatrix4_UseDXOrtho ? TargetFace : OGLFaceMap[TargetFace];
	//uint32_t Face = OGLFaceMap[TargetFace];

	m_Position = Cam.GetPosition();
	m_TransparentCount.store(0);
	m_OpaqueCount.store(0);
	m_Flag = (Cam.IsShadowCaster() ? Shadowed : 0) | (Cam.IsPointCamera() ? Point : 0) | (Cam.IsReflection() ? Reflection : 0);
	if (Cam.IsPointCamera()) {
		Camera FaceCam = Camera(m_Position, FaceDirs[Face], FaceUps[Face], 1.0f, LW_PI_2, 0.1f, Cam.GetPointPropertys().m_Radius, false);
		FaceCam.MakeViewDirections(m_Forward, m_Right, m_Up);
		FaceCam.BuildFrustrumPoints(PassData->FrustrumPoints);
		float Near = FaceCam.GetPerspectivePropertys().m_Near;
		float Far = FaceCam.GetPerspectivePropertys().m_Far;
		GlobalBlock.ProjViewMatrixs[PassID] = FaceCam.GetProjViewMatrix();
		GlobalBlock.TargetValues[PassID].y = (Far + Near) / (Near - Far);
		GlobalBlock.TargetValues[PassID].z = 2.0f * Far * Near / (Near - Far);
		std::copy(FaceCam.GetViewFrustrum(), FaceCam.GetViewFrustrum() + 6, m_Frustum);
	} else {
		Cam.MakeViewDirections(m_Forward, m_Right, m_Up);
		Cam.BuildFrustrumPoints(PassData->FrustrumPoints);
		GlobalBlock.ProjViewMatrixs[PassID] = Cam.GetProjViewMatrix();
		std::copy(Cam.GetViewFrustrum(), Cam.GetViewFrustrum() + 6, m_Frustum);
	}
	GlobalBlock.TargetValues[PassID].x = (float)TargetID;
	GlobalBlock.TargetValues[PassID].w = (float)SourceIndex;
	GlobalBlock.ViewPositions[PassID] = m_Position;
	PassData->PassIndex = PassID;
	m_SourceIndex = SourceIndex;
	m_TargetIndex = TargetID;
	m_TargetFace = TargetFace;
	m_FrameID = FrameID;
	return;
}

bool GFramePass::SphereInFrustum(const LWSVector4f &Position, float Radius) {
	return LWESphereInFrustum(Position, Radius, m_Position, m_Frustum);

}

bool GFramePass::AABBInFrustum(const LWSVector4f &AAMin, const LWSVector4f &AAMax) {
	LWSVector4f hLen = (AAMax - AAMin) * 0.5f;
	LWSVector4f Pos = AAMin + hLen;
	return SphereInFrustum(Pos, hLen.Max() * 1.5f);
}

bool GFramePass::ConeInFrustrum(const LWSVector4f &Position, const LWSVector4f &Direction, float Length, float Theta) {
	return LWEConeInFrustum(Position, Direction, Theta, Length, m_Position, m_Frustum);
}

bool GFramePass::LightInFrustrum(const Light &L) {
	uint32_t lType = L.GetLightType();
	if (lType == Light::AmbientLight) return true;
	else if (lType == Light::DirectionalLight) return true;
	else if (lType == Light::PointLight) return SphereInFrustum(L.m_Position, L.GetPointRadius());
	else if (lType == Light::SpotLight) return ConeInFrustrum(L.m_Position, L.m_Direction, L.GetSpotLength(), L.GetSpotTheta());
	return false;
}

//GFrame
GFrame &GFrame::InitializeFrame(uint32_t FrameID) {
	m_UIFrame.m_TextureCount = 0;
	m_ModelCount = 0;
	m_AnimCount = 0;
	m_LightCount = 0;
	m_ParticleCount = 0;
	m_ShadowCount = 0;
	m_RawPassCount = 0;
	m_ReflectionBits = 0;
	m_TargetTextureSize = LWVector2i(0);
	m_ViewBounds = LWVector4f(0.0f);
	m_TargetViewBounds = LWVector4i(0);
	m_FrameID = FrameID;
	return *this;
}

GFrame &GFrame::FinalizeFrame(void) {
	m_UIFrame.m_Mesh->Finished();
	m_GlobalData.LightCount = m_LightCount;
	for (uint32_t i = 0; i < MaxRawPasses; i++) m_PassList[i].FinalizePass(m_FrameID);
	return *this;
}

GPassData *GFrame::GetPassDataAt(uint32_t i) {
	return m_Driver->GetUniformPaddedAt<GPassData>(i, m_PassDataBuffer);
}

GModelData *GFrame::GetModelDataAt(uint32_t i) {
	return m_Driver->GetUniformPaddedAt<GModelData>(i, m_ModelDataBuffer);
}

GAnimData *GFrame::GetAnimDataAt(uint32_t i) {
	return m_Driver->GetUniformPaddedAt<GAnimData>(i, m_AnimDataBuffer);
}

GFrame &GFrame::InitializeShadowPosition(const LWSVector4f &ShadowPos) {
	m_ShadowPosition = ShadowPos;
	return *this;
}

GFrame &GFrame::InitializeSunDirection(const LWSVector4f &SunDirection) {
	m_GlobalData.SunDirection = SunDirection;
	return *this;
}

uint32_t GFrame::InitializePass(uint32_t PassID, uint32_t TargetID, uint32_t TargetFace, uint32_t SourceIndex, Camera &Cam) {
	GPassData *P = GetPassDataAt(PassID);
	m_PassList[PassID].Initialize(Cam, m_GlobalData, P, TargetID, TargetFace, SourceIndex, m_FrameID, PassID);
	Cam.SetPassID(PassID);
	return 1<<PassID;
};

uint32_t GFrame::InitializePass(uint32_t PassID, Camera &Cam) {
	GPassData *P = GetPassDataAt(PassID);
	m_PassList[PassID].Initialize(Cam, m_GlobalData, P, 0, 0, 0, m_FrameID, PassID);
	Cam.SetPassID(PassID);
	return 1<<PassID;
}

GFrame &GFrame::InitializeDirectionPass(const LWVector4f &ViewBounds, const LWVector4i &TargetBounds) {
	m_ViewBounds = ViewBounds;
	m_TargetViewBounds = TargetBounds;
	return *this;
}

GFrame &GFrame::InitializeRTPasses(const LWSVector4f &SceneAABBMin, const LWSVector4f &SceneAABBMax) {
	uint32_t o = RTFirstPass;
	GFramePass &MV = m_PassList[MainViewPass];
	GPassData *MVP = GetPassDataAt(MainViewPass);
	if (!MV.isInitialized(m_FrameID)) return *this;
	m_ShadowArrayCount = m_ShadowCubeCount = 0;
	auto MakeDirectionLightPass = [this, &SceneAABBMin, &SceneAABBMax](GLight &GL, uint32_t LightIndex, GFramePass &MV, GPassData *MVP, uint32_t &POffset, uint32_t &ArrayCount, uint32_t MaxArrayElements) -> uint32_t {
		Camera CascadeList[CascadeCount];
		if (ArrayCount >= MaxShadowRTs) return 0;
		uint32_t Cnt = std::min<uint32_t>(CascadeCount, std::min<uint32_t>(MaxPasses - POffset, MaxArrayElements - ArrayCount));
		Camera::MakeCascadeCameraViews(GL.m_Direction, MV.m_Position, MVP->FrustrumPoints, m_GlobalData.ProjViewMatrixs[MainViewPass], CascadeList, Cnt, SceneAABBMin, SceneAABBMax);
		uint32_t Bits = 0;
		for (uint32_t i = 0; i < Cnt; i++) Bits |= InitializePass(POffset + i, ArrayCount++, 0, LightIndex, CascadeList[i]);
		GL.m_ShadowIdxs.x = POffset++;
		if (Cnt == 2) GL.m_ShadowIdxs.y = POffset++;
		return Bits;
	};

	auto MakeSpotLightPass = [this](GLight &GL, uint32_t LightIndex, uint32_t &POffset, uint32_t &ArrayCount, uint32_t MaxArrayElements, bool isShadowCaster, bool isReflector) -> uint32_t {
		if (ArrayCount >= MaxShadowRTs) return 0;
		if (POffset >= MaxPasses) return 0;
		float Theta = GL.m_Position.w - 1.0f;
		float Length = GL.m_Direction.w;
		Camera DirCamera = Camera(GL.m_Position.AAAB(LWSVector4f(1.0f)), GL.m_Direction.AAAB(LWSVector4f(0.0f)), LWSVector4f(0.0f, 1.0f, 0.0f, 0.0f), 1.0f, Theta * 2.0f, 0.1f, Length, (isShadowCaster ? Camera::ShadowCaster : 0) | (isReflector ? Camera::Reflection : 0));
		uint32_t Bits = InitializePass(POffset, ArrayCount++, 0, LightIndex, DirCamera);
		GL.m_ShadowIdxs.x = POffset++;
		return Bits;
	};

	auto MakePointLightPass = [this](GLight &GL, uint32_t LightIndex, uint32_t &POffset, uint32_t &ArrayCount, uint32_t MaxArrayElements, bool isShadowCaster, bool isReflector) -> uint32_t {
		if (ArrayCount >= MaxShadowRTs) return 0;
		if (POffset >= MaxPasses) return 0;
		if (MaxPasses + m_RawPassCount + 5 > MaxRawPasses) return 0;
		Camera PntCamera = Camera(GL.m_Position, GL.m_Direction.x + GL.m_Direction.y, (isShadowCaster ? Camera::ShadowCaster : 0) | (isReflector ? Camera::Reflection : 0));
		uint32_t Bits = 0;
		Bits |= InitializePass(POffset, ArrayCount, 0, LightIndex, PntCamera);
		for (uint32_t i = 1; i < 6; i++) Bits |= InitializePass(MaxPasses + m_RawPassCount + i, ArrayCount, i, LightIndex, PntCamera);
		GL.m_ShadowIdxs.x = POffset;
		POffset++;
		m_RawPassCount += 5;
		ArrayCount++;
		return Bits;
	};
	
	for (uint32_t i = 0; i < m_ShadowCount && o<RTFirstPass+MaxPasses; i++) {
		GElement &Caster = m_ShadowLightList[i];
		GLight &GL = m_LightsBuffer[Caster.m_Index];
		uint32_t lType = Light::LightType(GL.m_Position.w);
		if (lType == Light::DirectionalLight) MakeDirectionLightPass(GL, Caster.m_Index, MV, MVP, o, m_ShadowArrayCount, MaxShadowRTs);
		else if (lType == Light::SpotLight) MakeSpotLightPass(GL, Caster.m_Index, o, m_ShadowArrayCount, MaxShadowRTs, true, false);
		else if (lType == Light::PointLight) MakePointLightPass(GL, Caster.m_Index, o, m_ShadowCubeCount, MaxShadowRTs, true, false);
	}
	return *this;
}

uint32_t GFrame::NextAnimation(void) {
	if(m_AnimCount>=MaxAnimations){
		LogWarn("GAnimData has been exhausted.");
		return -1;
	}
	return m_AnimCount++;
}

uint32_t GFrame::PushAnimation(LWSMatrix4f *BoneMatrixs, uint32_t BoneCount) {
	if(m_AnimCount>=MaxAnimations){
		LogWarn("GAnimData has been exhausted.");
		return -1;
	}
	GAnimData *A = GetAnimDataAt(m_AnimCount);
	std::copy(BoneMatrixs, BoneMatrixs + BoneCount, A->BoneMatrixs);
	return m_AnimCount++;
}

uint32_t GFrame::PassBitsInSphere(const LWSVector4f &Position, float Radius, uint32_t TargetPassBits) {
	uint32_t InViewBits = 0;
	for (uint32_t i = 0; i < MaxRawPasses; i++) {
		uint32_t B = 1 << i;
		if ((B&TargetPassBits)==0) continue;
		if (!m_PassList[i].isInitialized(m_FrameID)) continue;
		if (!m_PassList[i].SphereInFrustum(Position, Radius)) continue;
		InViewBits |= B;
	}
	return InViewBits;
}

uint32_t GFrame::PassBitsInAABB(const LWSVector4f &AAMin, const LWSVector4f &AAMax, uint32_t TargetPassBits) {
	uint32_t InViewBits = 0;
	LWSVector4f hLen = (AAMax - AAMin) * 0.5f;
	LWSVector4f Pos = AAMin + hLen;
	float Radi = hLen.Max() * 1.5f;
	return PassBitsInSphere(Pos, Radi, TargetPassBits);
}

uint32_t GFrame::PushModel(GFrameModel &Mdl, uint32_t PassBits, uint32_t AnimID, const LWSMatrix4f &Transform, const GMaterial &Material, bool Transparent) {
	if(m_ModelCount>=MaxModels){
		LogWarn("GFrameModel has been exhausted.");
		return -1;
	}
	GModelData *M = GetModelDataAt(m_ModelCount);
	M->TransformMatrix = Transform;
	M->Material = Material;
	m_ModelList[m_ModelCount] = Mdl;
	m_ModelList[m_ModelCount].SetBufferIDs(m_ModelCount, AnimID);
	LWSVector4f Pos = Transform[3];
	for(uint32_t i=0;i<MaxRawPasses;i++){
		if (((1<<i)&PassBits)==0) continue;
		if(!m_PassList[i].isInitialized(m_FrameID)) continue;
		m_PassList[i].PushElement(m_ModelCount, Mdl.m_Flags, Pos, Transparent);
	}
	return m_ModelCount++;
}

uint32_t GFrame::PushLight(const Light &L) {
	if(m_LightCount>=MaxLights){
		LogWarn("Lights has been exhausted.");
		return -1;
	}
	if (m_PassList[MainViewPass].isInitialized(m_FrameID)) {
		if (!m_PassList[MainViewPass].LightInFrustrum(L)) return -1;
	}
	uint32_t LightID = m_LightCount++;
	m_LightsBuffer[LightID] = { L.m_Position, L.m_Direction, L.m_Color, LWVector4i(-1) };
	if (!L.isShadowCaster()) return LightID;
	if (!m_PassList[MainViewPass].isInitialized(m_FrameID)) return LightID;
	uint32_t lType = L.GetLightType();
	GElement E = { LightID, 100000.0f };
	if (lType == Light::AmbientLight) return LightID;
	if (lType == Light::DirectionalLight) E.m_DistanceSq = 0.0f;
	else E.m_DistanceSq = m_ShadowPosition.DistanceSquared3(L.m_Position);
	auto First = m_ShadowLightList.begin();
	auto Last = First + m_ShadowCount;
	auto Loc = std::lower_bound(First, Last, E);
	if (Loc != Last) std::copy_backward(Loc, Last, Last + 1);
	else if (m_ShadowCount >= MaxShadowRTs) return LightID;
	*Loc = E;
	m_ShadowCount = std::min<uint32_t>(m_ShadowCount + 1, MaxShadowRTs);
	return LightID;
}

uint32_t GFrame::WriteParticles(uint32_t Count) {
	if (m_ParticleCount + Count > MaxParticleVertices) return -1;
	uint32_t P = m_ParticleCount;
	m_ParticleCount += Count;
	return P;
}

GFrame::GFrame(LWVideoDriver *Driver, LWAllocator &Allocator) : m_Driver(Driver) {
	LWVideoBuffer *UIVerts = m_Driver->CreateVideoBuffer<LWVertexUI>(LWVideoBuffer::Vertex, LWVideoBuffer::WriteDiscardable | LWVideoBuffer::LocalCopy, MaxUIElements * 6, Allocator, nullptr);
	m_UIFrame.m_Mesh = LWVertexUI::MakeMesh(Allocator, UIVerts, 0);
	m_PassDataBuffer = m_Driver->AllocatePadded<GPassData>(MaxRawPasses, Allocator);
	m_ModelDataBuffer = m_Driver->AllocatePadded<GModelData>(MaxModels, Allocator);
	m_AnimDataBuffer = m_Driver->AllocatePadded<GAnimData>(MaxAnimations, Allocator);
	m_ParticleVertices = Allocator.Allocate<ParticleVert>(MaxParticleVertices);
	m_LightsBuffer = Allocator.Allocate<GLight>(MaxLights);
}

GFrame::~GFrame() {
	m_UIFrame.m_Mesh->Destroy(m_Driver);
	LWAllocator::Destroy(m_PassDataBuffer);
	LWAllocator::Destroy(m_ModelDataBuffer);
	LWAllocator::Destroy(m_AnimDataBuffer);
	LWAllocator::Destroy(m_ParticleVertices);
	LWAllocator::Destroy(m_LightsBuffer);
}

//Renderer
Renderer &Renderer::SizeUpdated(LWWindow *Window) {
	if (!m_SizeChanged) return *this;
	LWVector2f WndSize = Window->GetSizef();
	LWMatrix4f UIOrtho = LWMatrix4f::Ortho(0.0f, WndSize.x, 0.0f, WndSize.y, 0.0f, 1.0f);
	m_Driver->UpdateVideoBuffer(m_UIUniform, (const uint8_t*)&UIOrtho, sizeof(LWMatrix4f));

	LWVector2i TileSize = LWVector2i(32, 32);
	LWVector2i LocalThreads = LWVector2i(32, 32);
	LWVector2i TexSize = (Window->GetSize() + (TileSize - 1)) / TileSize;
	LWVector2i TotalThreads = (TexSize + (LocalThreads - 1)) / LocalThreads;

	for (uint32_t i = 0; i < MaxFrames; i++) {
		m_Frames[i].m_GlobalData.ScreenSize = WndSize;
		m_Frames[i].m_GlobalData.ThreadDimensions = TotalThreads;
		m_Frames[i].m_GlobalData.TileSize = TileSize;
	}
	LWVideoBuffer *oLightArray = m_LightArrayBuffer;
	m_LightArrayBuffer = m_Driver->CreateVideoBuffer<uint32_t>(LWVideoBuffer::ImageBuffer, LWVideoBuffer::GPUResource, MaxLightsPerTile * (TotalThreads.x * TotalThreads.y * LocalThreads.x * LocalThreads.y), m_Allocator, nullptr);
	m_LightCullPipeline->SetResource("LightArray", m_LightArrayBuffer);
	m_MetallicRoughnessPipeline->SetResource("LightArray", m_LightArrayBuffer);
	m_SpecularGlossinessPipeline->SetResource("LightArray", m_LightArrayBuffer);

	if (oLightArray) m_Driver->DestroyVideoBuffer(oLightArray);

	if (m_ScreenFB) {
		m_Driver->DestroyFrameBuffer(m_ScreenFB);
		m_Driver->DestroyTexture(m_ScreenTex);
		m_Driver->DestroyTexture(m_FinalScreenTex);
		m_Driver->DestroyTexture(m_ScreenTexMS);
		m_Driver->DestroyTexture(m_EmissionTex);
		m_Driver->DestroyTexture(m_EmissionTexMS);
		m_Driver->DestroyTexture(m_ScreenDepth);
	}
	m_ScreenFB = m_Driver->CreateFrameBuffer(WndSize.CastTo<int32_t>(), m_Allocator);
	m_ScreenTexMS = m_Driver->CreateTexture2DMS(LWTexture::RenderTarget, LWImage::RGBA8, m_ScreenFB->GetSize(), m_Settings.m_SampleCount, m_Allocator);
	m_ScreenTex = m_Driver->CreateTexture2D(LWTexture::MinLinear | LWTexture::MagLinear | LWTexture::RenderTarget, LWImage::RGBA8, m_ScreenFB->GetSize(), nullptr, 0, m_Allocator);
	m_FinalScreenTex = m_Driver->CreateTexture2D(LWTexture::MinLinear | LWTexture::MagLinear | LWTexture::RenderTarget, LWImage::RGBA8, m_ScreenFB->GetSize(), nullptr, 0, m_Allocator);
	m_EmissionTexMS = m_Driver->CreateTexture2DMS(LWTexture::RenderTarget, LWImage::RGBA8, m_ScreenFB->GetSize(), m_Settings.m_SampleCount, m_Allocator);
	m_EmissionTex = m_Driver->CreateTexture2D(LWTexture::MinLinear | LWTexture::MagLinear | LWTexture::RenderTarget, LWImage::RGBA8, m_ScreenFB->GetSize(), nullptr, 0, m_Allocator);
	m_ScreenDepth = m_Driver->CreateTexture2DMS(LWTexture::RenderTarget, LWImage::DEPTH24STENCIL8, m_ScreenFB->GetSize(), m_Settings.m_SampleCount, m_Allocator);

	if (m_HighlightFB) {
		m_Driver->DestroyFrameBuffer(m_HighlightFB);
		m_Driver->DestroyTexture(m_HighlightTex);
		m_Driver->DestroyTexture(m_HighlightTexMS);
	}
	LWVector2i HighlightSize = WndSize.CastTo<int32_t>();
	m_HighlightFB = m_Driver->CreateFrameBuffer(HighlightSize, m_Allocator);
	m_HighlightTex = m_Driver->CreateTexture2D(LWTexture::MinLinear | LWTexture::MagLinear | LWTexture::RenderTarget, LWImage::RGBA8, HighlightSize, nullptr, 0, m_Allocator);
	m_HighlightTexMS = m_Driver->CreateTexture2DMS(LWTexture::RenderTarget, LWImage::RGBA8, HighlightSize, m_Settings.m_SampleCount, m_Allocator);

	if (m_BlurFB) {
		m_Driver->DestroyFrameBuffer(m_BlurFB);
		m_Driver->DestroyTexture(m_BlurTempTexture);
		m_Driver->DestroyTexture(m_BEmissionTexture);
		m_Driver->DestroyTexture(m_BHighlightTexture);
	}
	LWVector2i BlurSize = (WndSize / 2).CastTo<int32_t>();
	m_BlurFB = m_Driver->CreateFrameBuffer(BlurSize, m_Allocator);
	m_BlurTempTexture = m_Driver->CreateTexture2D(LWTexture::MinLinear | LWTexture::MagLinear | LWTexture::RenderTarget, LWImage::RGBA8, BlurSize, nullptr, 0, m_Allocator);
	m_BEmissionTexture = m_Driver->CreateTexture2D(LWTexture::MinLinear | LWTexture::MagLinear | LWTexture::RenderTarget, LWImage::RGBA8, BlurSize, nullptr, 0, m_Allocator);
	m_BHighlightTexture = m_Driver->CreateTexture2D(LWTexture::MinLinear | LWTexture::MagLinear | LWTexture::RenderTarget, LWImage::RGBA8, BlurSize, nullptr, 0, m_Allocator);


	uint32_t KernelSize = m_Driver->GetUniformPaddedLength<GGaussianKernel>(GaussianKernelCount);
	char *GKernel = m_Allocator.Allocate<char>(KernelSize);
	GGaussianKernel::MakeKernel(m_Driver, ScreenGaussianKernel, 10.0f, m_ScreenFB->GetSize(), GKernel);
	if (m_GaussianKernel) m_Driver->DestroyVideoBuffer(m_GaussianKernel);
	m_GaussianKernel = m_Driver->CreatePaddedVideoBuffer<GGaussianKernel>(LWVideoBuffer::Uniform, LWVideoBuffer::Static, GaussianKernelCount, m_Allocator, GKernel);
	LWAllocator::Destroy(GKernel);

	m_SizeChanged = false;
	return *this;
}

Renderer &Renderer::LoadAssets(LWEAssetManager *AssetMan) {
	m_FontShader = AssetMan->GetAsset<LWShader>("UIFontShader");
	m_UITextureShader = AssetMan->GetAsset<LWShader>("UITextureShader");
	m_UIColorShader = AssetMan->GetAsset<LWShader>("UIColorShader");

	m_StaticVertexShader = AssetMan->GetAsset<LWShader>("StaticVertexShader");
	m_SkeletonVertexShader = AssetMan->GetAsset<LWShader>("SkeletonVertexShader");

	m_MetallicRoughnessPipeline = AssetMan->GetAsset<LWPipeline>("MetallicRoughnessPipeline");
	m_SpecularGlossinessPipeline = AssetMan->GetAsset<LWPipeline>("SpecularGlossinessPipeline");
	m_UnlitPipeline = AssetMan->GetAsset<LWPipeline>("UnlitPipeline");
	m_ShadowPipeline = AssetMan->GetAsset<LWPipeline>("ShadowPipeline");
	m_SkyboxPipeline = AssetMan->GetAsset<LWPipeline>("SkyboxPipeline");
	m_CloudPipeline = AssetMan->GetAsset<LWPipeline>("CloudPipeline");
	m_PostProcessMS = AssetMan->GetAsset<LWPipeline>("PostProcessMSPipeline");

	m_FinalPipeline = AssetMan->GetAsset<LWPipeline>("FinalPipeline");
	m_GaussianPipeline = AssetMan->GetAsset<LWPipeline>("GaussianPipeline");

	m_LightCullPipeline = AssetMan->GetAsset<LWPipeline>("LightCullPipeline");

	m_MetallicRoughnessPipeline->SetUniformBlock(0, m_GlobalDataBlock);
	m_SpecularGlossinessPipeline->SetUniformBlock(0, m_GlobalDataBlock);
	m_UnlitPipeline->SetUniformBlock(0, m_GlobalDataBlock);
	//m_SkyboxPipeline->SetUniformBlock(0, m_GlobalDataBlock);
	m_ShadowPipeline->SetUniformBlock(0, m_GlobalDataBlock);
	//m_CloudPipeline->SetUniformBlock(0, m_GlobalDataBlock);

	m_MetallicRoughnessPipeline->SetResource("Lights", m_LightDataBuffer);
	m_SpecularGlossinessPipeline->SetResource("Lights", m_LightDataBuffer);
	m_ShadowPipeline->SetResource("Lights", m_LightDataBuffer);

	m_LightCullPipeline->SetUniformBlock(0, m_GlobalDataBlock);
	m_LightCullPipeline->SetUniformBlock(1, m_PassDataBlock);
	m_LightCullPipeline->SetResource("Lights", m_LightDataBuffer);

	m_UIPipeline = AssetMan->GetAsset<LWPipeline>("UIPipeline");
	m_UIPipeline->SetUniformBlock(0, m_UIUniform);

	return *this;
}

GFrame *Renderer::BeginFrame(void) {
	if (m_WriteFrame - m_ReadFrame >= MaxFrames-1) return nullptr;
	GFrame &F = m_Frames[(m_WriteFrame % MaxFrames)];
	F.InitializeFrame(m_WriteFrame);
	return &F;
}

Renderer &Renderer::EndFrame(void) {
	m_WriteFrame++;
	return *this;
}

LWPipeline *Renderer::PreparePipeline(GFrame &F, const GFrameModel &Mdl, bool isSkinned, bool Transparent, bool IsShadowed) {
	auto ApplyFlagsToPipeline = [this](LWPipeline *P, LWShader *VertexShader, uint32_t Flags, bool Transparent) -> LWPipeline* {
		P->SetVertexShader(VertexShader);
		P->SetBlendMode(Transparent || (Flags&GFrameModel::ForceTransparency)!=0, LWPipeline::BLEND_SRC_ALPHA, LWPipeline::BLEND_ONE_MINUS_SRC_ALPHA);
		P->SetDepthOutput((Flags & GFrameModel::NoDepthOut) == 0);
		m_Driver->UpdatePipelineStages(P);
		return P;
	};
	auto ApplyTexture = [this](LWPipeline *P, const GFrameModel &Mdl, uint32_t TexID, uint32_t RscOffset) {
		const GModelTexture &T = Mdl.m_TextureList[TexID];
		LWTexture *Tex = nullptr;
		auto Iter = m_TextureMap.find(T.m_TextureID);
		if (Iter != m_TextureMap.end()) {
			Tex = Iter->second;
			if (Tex) Tex->SetTextureState(T.m_TextureState);
		}
		P->SetResource(RscOffset + TexID, Tex);
	};
	LWShader *VertShader = isSkinned ? m_SkeletonVertexShader : m_StaticVertexShader;

	if (IsShadowed) return ApplyFlagsToPipeline(m_ShadowPipeline, VertShader, Mdl.m_Flags, false);
	LWPipeline *P = nullptr;
	if (Mdl.m_PipelineID == Material::PBRMetallicRoughness) {
		P = m_MetallicRoughnessPipeline;
		ApplyTexture(P, Mdl, GMaterial::NormalTexID, MetallicRoughnessTexOffset);
		ApplyTexture(P, Mdl, GMaterial::OcclussionTexID, MetallicRoughnessTexOffset);
		ApplyTexture(P, Mdl, GMaterial::EmissiveTexID, MetallicRoughnessTexOffset);
		ApplyTexture(P, Mdl, GMaterial::PBRAlbedoTexID, MetallicRoughnessTexOffset);
		ApplyTexture(P, Mdl, GMaterial::PBRMetallicRoughnessTexID, MetallicRoughnessTexOffset);
	} else if (Mdl.m_PipelineID == Material::PBRSpecularGlossiness) {
		P = m_SpecularGlossinessPipeline;
		ApplyTexture(P, Mdl, GMaterial::NormalTexID, SpecularGlossinessTexOffset);
		ApplyTexture(P, Mdl, GMaterial::OcclussionTexID, SpecularGlossinessTexOffset);
		ApplyTexture(P, Mdl, GMaterial::EmissiveTexID, SpecularGlossinessTexOffset);
		ApplyTexture(P, Mdl, GMaterial::SGDiffuseColorTexID, SpecularGlossinessTexOffset);
		ApplyTexture(P, Mdl, GMaterial::SGSpecularColorTexID, SpecularGlossinessTexOffset);
	} else if (Mdl.m_PipelineID == Material::PBRUnlit) {
		P = m_UnlitPipeline;
		ApplyTexture(P, Mdl, GMaterial::NormalTexID, UnlitTexOffset);
		ApplyTexture(P, Mdl, GMaterial::OcclussionTexID, UnlitTexOffset);
		ApplyTexture(P, Mdl, GMaterial::EmissiveTexID, UnlitTexOffset);
		ApplyTexture(P, Mdl, GMaterial::ULColorTexID, UnlitTexOffset);
	} else if (Mdl.m_PipelineID == Material::Skybox) {
		P = m_SkyboxPipeline;
		ApplyTexture(P, Mdl, GMaterial::SBBackTexID, SkyboxTexOffset);
		ApplyTexture(P, Mdl, GMaterial::SBHorizonTexID, SkyboxTexOffset);
		ApplyTexture(P, Mdl, GMaterial::SBGlowTexID, SkyboxTexOffset);
	} else if (Mdl.m_PipelineID == Material::Cloud) {
		P = m_CloudPipeline;
	}
	return ApplyFlagsToPipeline(P, VertShader, Mdl.m_Flags, Transparent);
}

Renderer &Renderer::ApplyFrame(GFrame &F) {
	F.FinalizeFrame();
	m_Driver->UpdateVideoBuffer(m_LightDataBuffer, (uint8_t*)F.m_LightsBuffer, sizeof(GLight) * F.m_LightCount);
	m_Driver->UpdateVideoBuffer(m_GlobalDataBlock, (uint8_t*)&F.m_GlobalData, sizeof(GGlobalData));
	m_Driver->UpdateVideoBuffer(m_PassDataBlock, (uint8_t*)F.m_PassDataBuffer, m_Driver->GetUniformPaddedLength<GPassData>(MaxRawPasses));
	m_Driver->UpdateVideoBuffer(m_AnimDataBlock, (uint8_t*)F.m_AnimDataBuffer, m_Driver->GetUniformPaddedLength<GAnimData>(F.m_AnimCount));
	m_Driver->UpdateVideoBuffer(m_ModelDataBlock, (uint8_t*)F.m_ModelDataBuffer, m_Driver->GetUniformPaddedLength<GModelData>(F.m_ModelCount));
	m_Driver->UpdateVideoBuffer(m_ParticleVertBuffer, (uint8_t*)F.m_ParticleVertices, sizeof(ParticleVert) * F.m_ParticleCount);
	return *this;
}

Renderer &Renderer::RenderUIFrame(LWEUIFrame &UIF) {
	uint32_t o = 0;
	for (uint32_t i = 0; i < UIF.m_TextureCount; i++) {
		LWShader *S = UIF.m_FontTexture[i] ? m_FontShader : (UIF.m_Textures[i] ? m_UITextureShader : m_UIColorShader);
		m_UIPipeline->SetPixelShader(S);
		m_UIPipeline->SetResource(0, UIF.m_Textures[i]);
		m_Driver->DrawMesh(m_UIPipeline, LWVideoDriver::Triangle, UIF.m_Mesh, UIF.m_VertexCount[i], o);
		o += UIF.m_VertexCount[i];
	}
	return *this;
}

Renderer &Renderer::ApplySettings(const RenderSettings &Settings) {
	const LWVector2i ShadowSizes[RenderSettings::ShadowQuality_Count] = { LWVector2i(2048, 2048), LWVector2i(1024, 1024), LWVector2i(512, 512), LWVector2i(256, 256) };
	const LWVector2i CubeShadowSizes[RenderSettings::ShadowQuality_Count] = { LWVector2i(1024, 1024), LWVector2i(512, 512), LWVector2i(256, 256), LWVector2i(128, 128) };
	const LWVector2i ReflectionSizes[RenderSettings::ReflectionQuality_Count] = { LWVector2i(1024, 1024), LWVector2i(512, 512), LWVector2i(256, 256), LWVector2i(128, 128) };

	//Update shadow settings.
	if (Settings.m_ShadowQuality != m_Settings.m_ShadowQuality || !m_ShadowFrameBuffer) {
		if (m_ShadowFrameBuffer) {
			m_Driver->DestroyFrameBuffer(m_ShadowFrameBuffer);
			m_Driver->DestroyFrameBuffer(m_ShadowCubeFrameBuffer);
			m_Driver->DestroyTexture(m_ShadowTextureArray);
			m_Driver->DestroyTexture(m_ShadowCubemapArray);
		}
		m_ShadowFrameBuffer = m_Driver->CreateFrameBuffer(ShadowSizes[Settings.m_ShadowQuality], m_Allocator);
		m_ShadowCubeFrameBuffer = m_Driver->CreateFrameBuffer(CubeShadowSizes[Settings.m_ShadowQuality], m_Allocator);

		m_ShadowTextureArray = m_Driver->CreateTexture2DArray(LWTexture::RenderTarget | LWTexture::MinLinear | LWTexture::MagLinear | LWTexture::CompareModeRefTexture | LWTexture::CompareLessEqual, LWImage::DEPTH32, m_ShadowFrameBuffer->GetSize(), GFrame::MaxShadowRTs, nullptr, 0, m_Allocator);
		m_ShadowCubemapArray = m_Driver->CreateTextureCubeArray(LWTexture::RenderTarget | LWTexture::MinLinear | LWTexture::MagLinear | LWTexture::CompareModeRefTexture | LWTexture::CompareLessEqual, LWImage::DEPTH32, m_ShadowCubeFrameBuffer->GetSize(), GFrame::MaxShadowRTs, nullptr, 0, m_Allocator);

		m_MetallicRoughnessPipeline->SetResource("DepthTex", m_ShadowTextureArray);
		m_SpecularGlossinessPipeline->SetResource("DepthTex", m_ShadowTextureArray);
		m_MetallicRoughnessPipeline->SetResource("DepthCubeTex", m_ShadowCubemapArray);
		m_SpecularGlossinessPipeline->SetResource("DepthCubeTex", m_ShadowCubemapArray);
		LWVector2f iShadowCubeSize = 1.0f / m_ShadowCubeFrameBuffer->GetSizef();
		for (uint32_t i = 0; i < MaxFrames; i++) m_Frames[i].m_GlobalData.iShadowCubeSize = iShadowCubeSize;
	}

	//Update reflection settings.
	if (Settings.m_ReflectionQuality != Settings.m_ReflectionQuality || !m_ReflectionFrameBuffer) {
		if (m_ReflectionFrameBuffer) {
			m_Driver->DestroyFrameBuffer(m_ReflectionFrameBuffer);
			m_Driver->DestroyTexture(m_ReflectionDepthmap);
			m_Driver->DestroyTexture(m_ReflectionCubemap[0]);
			m_Driver->DestroyTexture(m_ReflectionCubemap[1]);
		}
		m_ReflectionFrameBuffer = m_Driver->CreateFrameBuffer(ReflectionSizes[Settings.m_ReflectionQuality], m_Allocator);
		m_ReflectionDepthmap = m_Driver->CreateTexture2D(LWTexture::RenderTarget, LWImage::DEPTH24STENCIL8, m_ReflectionFrameBuffer->GetSize(), nullptr, 0, m_Allocator);
		m_ReflectionCubemap[0] = m_Driver->CreateTextureCubeMap(LWTexture::RenderTarget | LWTexture::MinLinearMipmapLinear | LWTexture::MagLinear | LWTexture::MakeMipmaps, LWImage::RGBA8, m_ReflectionFrameBuffer->GetSize(), nullptr, 0, m_Allocator);
		m_ReflectionCubemap[1] = m_Driver->CreateTextureCubeMap(LWTexture::RenderTarget | LWTexture::MinLinearMipmapLinear | LWTexture::MagLinear | LWTexture::MakeMipmaps, LWImage::RGBA8, m_ReflectionFrameBuffer->GetSize(), nullptr, 0, m_Allocator);
		m_ReflectionFrameBuffer->SetAttachment(LWFrameBuffer::Depth, m_ReflectionDepthmap);
	
		//m_MetallicRoughnessPipeline->SetResource("SpecularEnvTex", m_ReflectionCubemap);

	}

	m_Settings = Settings;
	m_SizeChanged = true;
	return *this;
}

Renderer &Renderer::RenderModel(GFrame &F, const GFrameModel &Mdl, uint32_t PassID, bool Transparent, bool IsShadowed) {
	LWVideoBuffer *VBuffer = nullptr;
	LWVideoBuffer *IBuffer = nullptr;
	uint32_t Count = 0;
	auto VIter = m_GeometryMap.find(Mdl.m_VerticeID);
	if (VIter == m_GeometryMap.end()) return *this;
	if (!VIter->second) return *this;
	VBuffer = VIter->second;
	Count = VBuffer->GetLength();
	if (Mdl.m_IndiceID) {
		auto IIter = m_GeometryMap.find(Mdl.m_IndiceID);
		if (IIter == m_GeometryMap.end()) return *this;
		if (!IIter->second) return *this;
		IBuffer = IIter->second;
		Count = IBuffer->GetLength();
	}
	Count = Mdl.m_Count ? Mdl.m_Count : Count;
	LWPipeline *P = PreparePipeline(F, Mdl, VBuffer->GetTypeSize() == sizeof(GSkeletonVertice), Transparent, IsShadowed);
	P->SetPaddedUniformBlock<GPassData>(1, m_PassDataBlock, PassID, m_Driver);
	P->SetPaddedUniformBlock<GAnimData>(2, m_AnimDataBlock, Mdl.GetAnimBufferID(), m_Driver);
	P->SetPaddedUniformBlock<GModelData>(3, m_ModelDataBlock, Mdl.GetModelBufferID(), m_Driver);
	m_Driver->DrawBuffer(P, LWVideoDriver::Triangle, VBuffer, IBuffer, Count, VBuffer->GetTypeSize(), Mdl.m_Offset);
	return *this;
}

Renderer &Renderer::RenderPass(GFrame &F, uint32_t PassID) {
	GFramePass &Pass = F.m_PassList[PassID];
	if (!Pass.isInitialized(F.m_FrameID)) return *this;
	bool isShadowed = Pass.isShadowed();
	for (uint32_t i = 0; i < Pass.m_OpaqueCount; i++) {
		GElement &E = Pass.m_OpaqueElements[i];
		RenderModel(F, F.m_ModelList[E.m_Index], PassID, false, isShadowed);
	}
	for (uint32_t i = 0; i < Pass.m_TransparentCount; i++) {
		GElement &E = Pass.m_TransparentElements[i];
		RenderModel(F, F.m_ModelList[E.m_Index], PassID, true, isShadowed);
	}
	return *this;
}

Renderer &Renderer::RenderBlurPass(GFrame &F, uint32_t KernelOffset, LWFrameBuffer *FB, LWTexture *SourceTex, LWTexture *TempTex, LWTexture *ResultTex, uint32_t ResultLayer, uint32_t ResultFace) {
	FB->SetAttachment(LWFrameBuffer::Color0, TempTex);
	m_Driver->SetFrameBuffer(FB, true);
	m_GaussianPipeline->SetPaddedUniformBlock<GGaussianKernel>(0, m_GaussianKernel, KernelOffset, m_Driver);
	m_GaussianPipeline->SetResource(0, SourceTex);
	m_Driver->DrawBuffer(m_GaussianPipeline, LWVideoDriver::Triangle, m_PostProcessGeometry, nullptr, 6, sizeof(LWVertexTexture));
	FB->SetCubeAttachment(LWFrameBuffer::Color0, ResultTex, ResultFace, ResultLayer);
	m_GaussianPipeline->SetPaddedUniformBlock<GGaussianKernel>(0, m_GaussianKernel, KernelOffset + 1, m_Driver);
	m_GaussianPipeline->SetResource(0, TempTex);
	m_Driver->DrawBuffer(m_GaussianPipeline, LWVideoDriver::Triangle, m_PostProcessGeometry, nullptr, 6, sizeof(LWVertexTexture));
	return *this;
}

Renderer &Renderer::RenderShadowPass(GFrame &F, uint32_t PassID) {
	GFramePass &P = F.m_PassList[PassID];
	bool isPoint = P.isPoint();
	if (isPoint) {
		m_ShadowCubeFrameBuffer->SetCubeAttachment(LWFrameBuffer::Depth, m_ShadowCubemapArray, P.m_TargetFace, P.m_TargetIndex);
		m_Driver->SetFrameBuffer(m_ShadowCubeFrameBuffer, true);
	} else {
		m_ShadowFrameBuffer->SetAttachment(LWFrameBuffer::Depth, m_ShadowTextureArray, P.m_TargetIndex);
		m_Driver->SetFrameBuffer(m_ShadowFrameBuffer, true);
	}
	m_Driver->ClearDepth(1.0f);
	RenderPass(F, PassID);
	return *this;
}

Renderer &Renderer::CopyOutput(GFrame &F) {
	//Update output dimensions if needed:
	LWVector2i CurrentSize = m_OutputFramebuffer ? m_OutputFramebuffer->GetSize() : LWVector2i();
	
	if (F.m_TargetTextureSize.x > 0 && F.m_TargetTextureSize != CurrentSize) {
		
		if (m_OutputFramebuffer) {
			m_Driver->DestroyTexture(m_OutputTexture);
			m_Driver->DestroyFrameBuffer(m_OutputFramebuffer);
		}
		m_OutputFramebuffer = m_Driver->CreateFrameBuffer(F.m_TargetTextureSize, m_Allocator);
		m_OutputTexture = m_Driver->CreateTexture2DArray(LWTexture::MinLinear | LWTexture::MagLinear | LWTexture::RenderTarget, LWImage::RGBA8, m_OutputFramebuffer->GetSize(), RenderCount, nullptr, 0, m_Allocator);
	}
	if (!m_OutputFramebuffer) return *this;
	if (F.m_TargetViewBounds == LWVector4i(0)) return *this;
	//Flip y dimension
	LWVector2f TLTex = LWVector2f(F.m_ViewBounds.x, 1.0f - F.m_ViewBounds.y);
	LWVector2f TRTex = LWVector2f(F.m_ViewBounds.z, 1.0f - F.m_ViewBounds.y);
	LWVector2f BLTex = LWVector2f(F.m_ViewBounds.x, 1.0f - F.m_ViewBounds.w);
	LWVector2f BRTex = LWVector2f(F.m_ViewBounds.z, 1.0f - F.m_ViewBounds.w);
	LWVector2f WndSize = m_FinalScreenTex->Get2DSize().CastTo<float>();
	//Update only for target geometry from FinalScreenTex.
	LWVertexUI OutGeom[6] = { 
									LWVertexUI(LWVector4f(0.0f, WndSize.y, 0.0f, 1.0f), LWVector4f(1.0f), LWVector4f(BLTex, 0.0f, 0.0f)),
									LWVertexUI(LWVector4f(0.0f, 0.0f, 0.0f, 1.0f), LWVector4f(1.0f), LWVector4f(TLTex, 0.0f, 0.0f)),
									LWVertexUI(LWVector4f(WndSize.x, 0.0f, 0.0f, 1.0f), LWVector4f(1.0f), LWVector4f(TRTex, 0.0f, 0.0f)),
									LWVertexUI(LWVector4f(WndSize.x, 0.0f, 0.0f, 1.0f), LWVector4f(1.0f), LWVector4f(TRTex, 0.0f, 0.0f)),
									LWVertexUI(LWVector4f(WndSize.x, WndSize.y, 0.0f, 1.0f), LWVector4f(1.0f), LWVector4f(BRTex, 0.0f, 0.0f)),
									LWVertexUI(LWVector4f(0.0f, WndSize.y, 0.0f, 1.0f), LWVector4f(1.0f), LWVector4f(BLTex, 0.0f, 0.0f)) };
	uint32_t RType = F.m_GlobalData.RenderOutput & RenderBits;

	m_Driver->UpdateVideoBuffer(m_CopyGeometry, (uint8_t*)OutGeom, sizeof(LWVertexUI) * 6);
	m_OutputFramebuffer->SetAttachment(LWFrameBuffer::Color0, m_OutputTexture, RType);
	m_Driver->SetFrameBuffer(m_OutputFramebuffer, false);
	m_Driver->ViewPort(F.m_TargetViewBounds);
	if(F.m_SpriteFrame==0) m_Driver->ClearColor(0x0);
	m_UIPipeline->SetPixelShader(m_UITextureShader);
	m_UIPipeline->SetResource(0, m_FinalScreenTex);
	m_Driver->DrawBuffer(m_UIPipeline, LWVideoDriver::Triangle, m_CopyGeometry, nullptr, 6, sizeof(LWVertexUI));
	return *this;
}

Renderer &Renderer::Render(LWWindow *Window) {
	m_SizeChanged = m_SizeChanged || Window->SizeUpdated();
	if (!m_Driver->Update()) return *this;
	ProcessPendingGeometry();
	ProcessPendingTextures();
	SizeUpdated(Window);
	if(m_ReadFrame!=m_WriteFrame){
		ApplyFrame(m_Frames[m_ReadFrame % MaxFrames]);
		m_ReadFrame++;
	}
	if (!m_ReadFrame) return *this;
	GFrame &F = m_Frames[(m_ReadFrame - 1) % MaxFrames];
	
	//Disabled Forward+ implementation.
	//m_Driver->Dispatch(m_LightCullPipeline, LWVector3i(F.m_GlobalData.ThreadDimensions, 1));

	//Do RenderTarget passes:
	for(uint32_t i=GFrame::RTFirstPass;i<MaxRawPasses;i++){
		GFramePass &Pass = F.m_PassList[i];
		if(!Pass.isInitialized(F.m_FrameID)) continue;
		if (Pass.isShadowed()) RenderShadowPass(F, i);
	}

	//Main View Pass
	m_ScreenFB->SetAttachment(LWFrameBuffer::Color0, m_ScreenTexMS);
	m_ScreenFB->SetAttachment(LWFrameBuffer::Color1, m_EmissionTexMS);
	m_ScreenFB->SetAttachment(LWFrameBuffer::Depth, m_ScreenDepth);
	m_Driver->SetFrameBuffer(m_ScreenFB, true);
	m_Driver->ClearColor(0x0).ClearDepth(1.0f);
	RenderPass(F, GFrame::MainViewPass);

	m_ScreenFB->ClearAttachments().SetAttachment(LWFrameBuffer::Color0, m_ScreenTex);
	m_PostProcessMS->SetResource(0, m_ScreenTexMS);
	m_Driver->DrawBuffer(m_PostProcessMS, LWVideoDriver::Triangle, m_PostProcessGeometry, nullptr, 6, sizeof(LWVertexTexture));

	m_ScreenFB->SetAttachment(LWFrameBuffer::Color0, m_EmissionTex);
	m_PostProcessMS->SetResource(0, m_EmissionTexMS);
	m_Driver->DrawBuffer(m_PostProcessMS, LWVideoDriver::Triangle, m_PostProcessGeometry, nullptr, 6, sizeof(LWVertexTexture));

	//Highlighted object pass
	m_HighlightFB->SetAttachment(LWFrameBuffer::Color0, m_HighlightTexMS);
	m_HighlightFB->SetAttachment(LWFrameBuffer::Depth, m_ScreenDepth);
	m_Driver->SetFrameBuffer(m_HighlightFB, true);
	m_Driver->ClearColor(0x0);
	RenderPass(F, GFrame::OutlinePass);

	m_HighlightFB->ClearAttachments().SetAttachment(LWFrameBuffer::Color0, m_HighlightTex);
	m_PostProcessMS->SetResource(0, m_HighlightTexMS);
	m_Driver->DrawBuffer(m_PostProcessMS, LWVideoDriver::Triangle, m_PostProcessGeometry, nullptr, 6, sizeof(LWVertexTexture));

	//Do blurring on emissive sources:
	RenderBlurPass(F, ScreenGaussianKernel, m_BlurFB, m_EmissionTex, m_BlurTempTexture, m_BEmissionTexture);
	//Do blurring on highlighted sources:
	RenderBlurPass(F, ScreenGaussianKernel, m_BlurFB, m_HighlightTex, m_BlurTempTexture, m_BHighlightTexture);

	//Composite final image:
	m_ScreenFB->ClearAttachments().SetAttachment(LWFrameBuffer::Color0, m_FinalScreenTex);
	m_Driver->SetFrameBuffer(m_ScreenFB, true);

	m_FinalPipeline->SetResource("ColorTex", m_ScreenTex);
	m_FinalPipeline->SetResource("EmissiveTex", m_BEmissionTexture);
	m_FinalPipeline->SetResource("PreHighlightTex", m_HighlightTex);
	m_FinalPipeline->SetResource("PostHighlightTex", m_BHighlightTexture);
	m_Driver->DrawBuffer(m_FinalPipeline, LWVideoDriver::Triangle, m_PostProcessGeometry, nullptr, 6, sizeof(LWVertexTexture));

	//Copy sprite outputs to render target.
	CopyOutput(F);


	//Render everything to screen:
	m_Driver->SetFrameBuffer(nullptr, true);
	m_UIPipeline->SetPixelShader(m_UITextureShader);
	m_UIPipeline->SetResource(0, m_FinalScreenTex);
	m_Driver->DrawBuffer(m_FinalPipeline, LWVideoDriver::Triangle, m_PostProcessGeometry, nullptr, 6, sizeof(LWVertexTexture));
	RenderUIFrame(F.m_UIFrame);
	m_Driver->Present(1);
	return *this;
}

GMaterial Renderer::PrepareGMaterial(GFrameModel &Mdl, const LWVector2f &AtlasSubPosition, const LWVector2f &AtlasSubSize, float TransparencyMult, Material &Mat) {
	float Time = Mat.GetTime();
	uint32_t TexCnt = Mat.GetTextureCount();
	uint32_t PipelineID = Mat.GetPipelineID();
	GMaterial GMat;
	LWVector4f T = LWVector4f(1.0f, 1.0f, 1.0f, Mat.GetTransparency() * TransparencyMult);
	for (uint32_t i = 0; i < TexCnt; i++) {
		GModelTexture &GT = Mdl.m_TextureList[i];
		MaterialTexture &MT = Mat.GetTexture(i);
		GT.m_TextureID = MT.m_TextureID;

		//GT.m_TextureID = MT.m_Reference ? MT.m_Reference->m_Data.m_ID : 0;
		if (GT.m_TextureID) GMat.HasTexturesFlag |= (1 << i);
		GT.m_TextureState = MT.m_TextureState;
		LWVector4f SubTex = Mat.GetTextureTween(i).GetValue(Time, LWVector4f(0.0f, 0.0f, 1.0f, 1.0f));
		SubTex = LWVector4f(SubTex.xy() + AtlasSubPosition, SubTex.zw() * AtlasSubSize);
		GMat.SubTextures[i] = SubTex;
	}
	if (PipelineID == Material::PBRMetallicRoughness) {
		GMat.MaterialColorA = Mat.GetColorTween(Material::PBRAlbedoBaseFactorClrTweenID).GetValue(Time, LWVector4f(1.0f)) * T;
		GMat.MaterialColorB = Mat.GetColorTween(Material::PBRMetallicRoughnessClrTweenID).GetValue(Time, LWVector4f(1.0f));
		GMat.EmissiveFactor = Mat.GetColorTween(Material::EmissiveFactorClrTweenID).GetValue(Time, LWVector4f(0.0f, 0.0f, 0.0f, 1.0f));
	} else if (PipelineID == Material::PBRSpecularGlossiness) {
		GMat.MaterialColorA = Mat.GetColorTween(Material::SGDiffuseFactorClrTweenID).GetValue(Time, LWVector4f(1.0f)) * T;
		GMat.MaterialColorB = Mat.GetColorTween(Material::SGSpecularFactorClrTweenID).GetValue(Time, LWVector4f(1.0f));
		GMat.EmissiveFactor = Mat.GetColorTween(Material::EmissiveFactorClrTweenID).GetValue(Time, LWVector4f(0.0f, 0.0f, 0.0f, 1.0f));
	} else if (PipelineID == Material::PBRUnlit) {
		GMat.MaterialColorA = Mat.GetColorTween(Material::ULColorTweenID).GetValue(Time, LWVector4f(1.0f)) * T;
		GMat.EmissiveFactor = Mat.GetColorTween(Material::EmissiveFactorClrTweenID).GetValue(Time, LWVector4f(0.0f, 0.0f, 0.0f, 1.0f));
	}else if(PipelineID==Material::Skybox){
		//Nothing to be done for skybox.
	} else if (PipelineID == Material::Cloud) {
		GMat.MaterialColorA = Mat.GetColorTween(Material::CLSettingsClrAID).GetValue(Time, Material::DefaultACloudSettings);
		GMat.MaterialColorB = Mat.GetColorTween(Material::CLSettingsClrBID).GetValue(Time, Material::DefaultBCloudSettings);
		GMat.MaterialColorB.w = Mat.GetTime();
	}
	return GMat;
}

Renderer &Renderer::WriteDebugLine(GFrame &F, uint32_t PassBits, const LWSVector4f &APnt, const LWSVector4f &BPnt, float Thickness, const LWVector4f &Color, uint32_t Flags) {
	LWSVector4f Dir = (BPnt - APnt);
	float Len = Dir.Length3();
	LWSVector4f nDir = Dir.Normalize3();
	LWSMatrix4f Rot;
	LWSVector4f Up;
	LWSVector4f Right;
	nDir.Orthogonal3(Right, Up);
	Rot = LWSMatrix4f(Right, Up, nDir, LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f));
	float hLen = Len * 0.5f;
	LWSVector4f MidPnt = APnt + nDir * hLen;
	LWSMatrix4f Transform = LWSMatrix4f(Thickness, Thickness, hLen, 1.0f) * Rot * LWSMatrix4f::Translation(MidPnt);
	return WriteDebugGeometry(F, m_CubeVertID, m_CubeIdxID, F.PassBitsInSphere(MidPnt, hLen, PassBits), Transform, Color, Flags);
}

Renderer &Renderer::WriteLine(GFrame &F, uint32_t PassBits, const LWSVector4f &APnt, const LWSVector4f &BPnt, float Thickness, Material &Mat, uint32_t Flags) {
	LWSVector4f Dir = (BPnt - APnt);
	float Len = Dir.Length3();
	LWSVector4f nDir = Dir.Normalize3();
	LWSMatrix4f Rot;
	LWSVector4f Up;
	LWSVector4f Right;
	nDir.Orthogonal3(Right, Up);
	Rot = LWSMatrix4f(Right, Up, nDir, LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f));
	float hLen = Len * 0.5f;
	LWSVector4f MidPnt = APnt + nDir * hLen;
	LWSMatrix4f Transform = LWSMatrix4f(Thickness, Thickness, hLen, 1.0f) * Rot * LWSMatrix4f::Translation(MidPnt);
	WriteGeometry(F, m_CubeVertID, m_CubeIdxID, 0, F.PassBitsInSphere(MidPnt, hLen, PassBits), Transform, Mat, Flags);
	return *this;
}

Renderer &Renderer::WriteDebugPoint(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, float Radius, const LWVector4f &Color, uint32_t Flags) {
	LWSMatrix4f Transform = LWSMatrix4f(Radius, Radius, Radius, 1.0f) * LWSMatrix4f::Translation(Pos);
	WriteDebugGeometry(F, m_SphereVertID, 0, F.PassBitsInSphere(Pos, fabs(Radius), PassBits), Transform, Color, Flags);
	return *this;
}

Renderer &Renderer::WritePoint(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, float Radius, Material &Mat, uint32_t Flags) {
	LWSMatrix4f Transform = LWSMatrix4f(Radius, Radius, Radius, 1.0f) * LWSMatrix4f::Translation(Pos);
	WriteGeometry(F, m_SphereVertID, 0, 0, F.PassBitsInSphere(Pos, fabs(Radius), PassBits), Transform, Mat, Flags);
	return *this;
}

Renderer &Renderer::WriteDebugAABB(GFrame &F, uint32_t PassBits, const LWSVector4f &AAMin, const LWSVector4f &AAMax, float LineThickness, const LWVector4f &Color, uint32_t Flags) {
	LWSVector4f Am = AAMin;
	LWSVector4f Bm = AAMin.BAAA(AAMax);
	LWSVector4f Cm = AAMin.ABAA(AAMax);
	LWSVector4f Dm = AAMin.BBAA(AAMax);

	LWSVector4f Ax = AAMax;
	LWSVector4f Bx = AAMax.BAAA(AAMin);
	LWSVector4f Cx = AAMax.ABAA(AAMin);
	LWSVector4f Dx = AAMax.BBAA(AAMin);

	WriteDebugLine(F, PassBits, Am, Bm, LineThickness, Color, Flags);
	WriteDebugLine(F, PassBits, Am, Cm, LineThickness, Color, Flags);
	WriteDebugLine(F, PassBits, Bm, Dm, LineThickness, Color, Flags);
	WriteDebugLine(F, PassBits, Cm, Dm, LineThickness, Color, Flags);

	WriteDebugLine(F, PassBits, Ax, Bx, LineThickness, Color, Flags);
	WriteDebugLine(F, PassBits, Ax, Cx, LineThickness, Color, Flags);
	WriteDebugLine(F, PassBits, Bx, Dx, LineThickness, Color, Flags);
	WriteDebugLine(F, PassBits, Cx, Dx, LineThickness, Color, Flags);

	WriteDebugLine(F, PassBits, Am, Dx, LineThickness, Color, Flags);
	WriteDebugLine(F, PassBits, Bm, Cx, LineThickness, Color, Flags);
	WriteDebugLine(F, PassBits, Cm, Bx, LineThickness, Color, Flags);
	WriteDebugLine(F, PassBits, Dm, Ax, LineThickness, Color, Flags);
	return *this;
}

Renderer &Renderer::WriteDebugCone(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, const LWSVector4f &Dir, float Theta, float Length, const LWVector4f &Color, uint32_t Flags) {
	const LWSVector4f Forward = LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f);
	const uint32_t ConeRadiCnt = 30;
	float T = tanf(Theta) * Length;
	LWSMatrix4f Transform;
	float d = Forward.Dot3(Dir);
	if (d < -1.0f + std::numeric_limits<float>::epsilon()) Transform = LWSMatrix4f(1.0f, 1.0f, -1.0f, 1.0f); //Flip 180Degrees.
	else if (d < 1.0f - std::numeric_limits<float>::epsilon()) {
		LWSVector4f Up = Forward.Cross3(Dir).Normalize3();
		LWSVector4f Rt = Up.Cross3(Dir).Normalize3();
		Up = Dir.Cross3(Rt);
		Transform = LWSMatrix4f(Rt, Up, Dir, LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f));
	}
	Transform = LWSMatrix4f(T, T, Length, 1.0f) * Transform * LWSMatrix4f::Translation(Pos);
	WriteDebugGeometry(F, m_ConeVertID, m_ConeIdxID, F.PassBitsInSphere(Pos, Length*1.5f, PassBits), Transform, Color, Flags);
	return *this;
}

Renderer &Renderer::WriteCone(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, const LWSVector4f &Dir, float Theta, float Length, Material &Mat, uint32_t Flags) {
	const LWSVector4f Forward = LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f);
	const uint32_t ConeRadiCnt = 30;
	float T = tanf(Theta) * Length;
	LWSMatrix4f Transform;
	float d = Forward.Dot3(Dir);
	if (d < -1.0f + std::numeric_limits<float>::epsilon()) Transform = LWSMatrix4f(1.0f, 1.0f, -1.0f, 1.0f); //Flip 180Degrees.
	else if (d < 1.0f - std::numeric_limits<float>::epsilon()) {
		LWSVector4f Up = Forward.Cross3(Dir).Normalize3();
		LWSVector4f Rt = Up.Cross3(Dir).Normalize3();
		Up = Dir.Cross3(Rt);
		Transform = LWSMatrix4f(Rt, Up, Dir, LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f));
	}
	Transform = LWSMatrix4f(T, T, Length, 1.0f) * Transform * LWSMatrix4f::Translation(Pos);
	WriteGeometry(F, m_ConeVertID, m_ConeIdxID, 0, F.PassBitsInSphere(Pos, Length, PassBits), Transform, Mat, Flags);
	return *this;
}

Renderer &Renderer::WriteDebugCube(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, const LWSVector4f &Size, const LWVector4f &Color, uint32_t Flags) {
	LWSVector4f hSize = (Size * 0.5f).AAAB(LWSVector4f());
	LWSMatrix4f Transform = LWSMatrix4f(hSize.x, hSize.y, hSize.z, 1.0f) * LWSMatrix4f::Translation(Pos);
	WriteDebugGeometry(F, m_CubeVertID, m_CubeIdxID, F.PassBitsInAABB(Pos - hSize, Pos + hSize, PassBits), Transform, Color, Flags);
	return *this;
}

Renderer &Renderer::WriteCube(GFrame &F, uint32_t PassBits, const LWSVector4f &Pos, const LWSVector4f &Size, Material &Mat, uint32_t Flags) {
	LWSVector4f hSize = (Size * 0.5f).AAAB(LWSVector4f());
	LWSMatrix4f Transform = LWSMatrix4f(hSize.x, hSize.y, hSize.z, 1.0f) * LWSMatrix4f::Translation(Pos);
	WriteGeometry(F, m_CubeVertID, m_CubeIdxID, 0, F.PassBitsInAABB(Pos - hSize, Pos + hSize, PassBits), Transform, Mat, Flags);
	return *this;
}

Renderer &Renderer::WriteDebugAxis(GFrame &F, uint32_t PassBits, const LWSMatrix4f &Transform, float LineLen, float LineThickness, bool IgnoreScale, uint32_t Flags) {

	LWSVector4f xAxis = LWSVector4f(1.0f, 0.0f, 0.0f, 0.0f) * Transform;
	LWSVector4f yAxis = LWSVector4f(0.0f, 1.0f, 0.0f, 0.0f) * Transform;
	LWSVector4f zAxis = LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f) * Transform;
	if (IgnoreScale) {
		xAxis = xAxis.Normalize3();
		yAxis = yAxis.Normalize3();
		zAxis = zAxis.Normalize3();
	}
	xAxis *= LineLen;
	yAxis *= LineLen;
	zAxis *= LineLen;
	LWSVector4f Pos = Transform[3];
	WriteDebugLine(F, PassBits, Pos, Pos + xAxis, LineThickness, LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), Flags);
	WriteDebugLine(F, PassBits, Pos, Pos - xAxis, LineThickness, LWVector4f(0.5f, 0.0f, 0.0f, 0.5f), Flags);
	WriteDebugLine(F, PassBits, Pos, Pos + yAxis, LineThickness, LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), Flags);
	WriteDebugLine(F, PassBits, Pos, Pos - yAxis, LineThickness, LWVector4f(0.0f, 0.5f, 0.0f, 0.5f), Flags);
	WriteDebugLine(F, PassBits, Pos, Pos + zAxis, LineThickness, LWVector4f(0.0f, 0.0f, 1.0f, 1.0f), Flags);
	WriteDebugLine(F, PassBits, Pos, Pos - zAxis, LineThickness, LWVector4f(0.0f, 0.0f, 0.5f, 0.5f), Flags);
	return *this;
}

Renderer &Renderer::WriteDebugGeometry(GFrame &F, uint32_t VerticeID, uint32_t IndiceID, uint32_t PassBits, const LWSMatrix4f &Transform, const LWVector4f &Color, uint32_t Flags) {
	if (!PassBits) return *this;
	GFrameModel Mdl = GFrameModel(Material::PBRUnlit, VerticeID, IndiceID, Flags);
	GMaterial Mat;
	Mat.MaterialColorA = Color;
	uint32_t i = F.PushModel(Mdl, PassBits, 0, Transform, Mat, Color.w < 1.0f);
	return *this;
}

ParticleVert *Renderer::PrepareParticleVertices(GFrame &F, uint32_t PassBits, uint32_t VerticeCount, Material &Mat, uint32_t Flags) {
	uint32_t V = F.WriteParticles(VerticeCount);
	if (V == -1) return nullptr;
	WriteGeometry(F, m_ParticleVertID, m_ParticleIdxID, 0, PassBits, LWSMatrix4f(), Mat, Flags, (V / 4) * 6, (VerticeCount / 4) * 6);
	return F.m_ParticleVertices + V;
}

ParticleVert *Renderer::WriteParticleRect(ParticleVert *V, const LWSVector4f &Pos, const LWSVector4f &Right, const LWSVector4f &Up, const LWVector2f &BtmLeftTC, const LWVector2f &TopRightTC, float Transparency, const LWSVector4f &Normal, const LWSVector4f &Tangent) {
	*(V++) = { Pos - Right - Up, LWVector4f(BtmLeftTC.x, BtmLeftTC.y, Transparency, 0.0f), Normal, Tangent };
	*(V++) = { Pos - Right + Up, LWVector4f(BtmLeftTC.x, TopRightTC.y, Transparency, 0.0f), Normal, Tangent };
	*(V++) = { Pos + Right + Up, LWVector4f(TopRightTC.x, TopRightTC.y, Transparency, 0.0f), Normal, Tangent };
	*(V++) = { Pos + Right - Up, LWVector4f(TopRightTC.x, BtmLeftTC.y, Transparency, 0.0f), Normal, Tangent };
	return V;
}

ParticleVert *Renderer::WriteParticleLine(ParticleVert *V, const LWSVector4f &PntA, const LWSVector4f &PntB, const LWSVector4f &Up, float Thickness, const LWVector2f &BtmLeftTC, const LWVector2f &TopRightTC, float Transparency, const LWSVector4f &Normal, const LWSVector4f &Tangent) {
	LWSVector4f nDir = (PntB - PntA).Normalize3();
	LWSVector4f nPerp = nDir.Cross3(Up) * Thickness;
	*(V++) = { PntA + nPerp, LWVector4f(BtmLeftTC.x, BtmLeftTC.y, Transparency, 0.0f), Normal, Tangent };
	*(V++) = { PntA - nPerp, LWVector4f(BtmLeftTC.x, TopRightTC.y, Transparency, 0.0f), Normal, Tangent };
	*(V++) = { PntB - nPerp, LWVector4f(TopRightTC.x, TopRightTC.y, Transparency, 0.0f), Normal, Tangent };
	*(V++) = { PntB + nPerp, LWVector4f(TopRightTC.x, BtmLeftTC.y, Transparency, 0.0f), Normal, Tangent };
	return V;
}


Renderer &Renderer::WriteMesh(GFrame &F, Mesh *Msh, const LWSMatrix4f *AnimTransforms, uint32_t PassBits, const LWSMatrix4f &Transform, Material &Mat, uint32_t Flags) {
	LWSMatrix4f DefTransforms[MaxBones];
	uint32_t AnimID = 0;
	uint32_t PrimCount = Msh->GetPrimitiveCount();
	uint32_t VertID = Msh->GetVertices().m_ID;
	uint32_t IndID = Msh->GetIndices().m_ID;
	if (Msh->GetBoneCount()) {
		AnimID = F.NextAnimation();
		if (AnimTransforms) Msh->BuildRenderMatrixs(AnimTransforms, F.GetAnimDataAt(AnimID)->BoneMatrixs);
		else {
			Msh->BuildBindTransforms(DefTransforms);
			Msh->BuildRenderMatrixs(DefTransforms, F.GetAnimDataAt(AnimID)->BoneMatrixs);
		}
	}
	LWSVector4f AAMin, AAMax;
	Msh->TransformBounds(Transform, AAMin, AAMax);
	uint32_t PBits = F.PassBitsInAABB(AAMin, AAMax, PassBits);
	if (!PBits) return *this;
	for (uint32_t i = 0; i < PrimCount; i++) {
		Primitive &P = Msh->GetPrimitive(i);
		WriteGeometry(F, VertID, IndID, AnimID, PBits, Transform, Mat, Flags, P.m_Offset, P.m_Count);
	}
	return *this;
}

uint32_t Renderer::WriteMeshPrimitive(GFrame &F, Primitive &P, Mesh *Msh, uint32_t AnimID, uint32_t PassBits, const LWSMatrix4f &Transform, Material &Mat, uint32_t Flags) {
	uint32_t VertID = Msh->GetVertices().m_ID;
	uint32_t IndID = Msh->GetIndices().m_ID;
	return WriteGeometry(F, VertID, IndID, AnimID, PassBits, Transform, Mat, Flags, P.m_Offset, P.m_Count);
}

uint32_t Renderer::WriteGeometry(GFrame &F, uint32_t VerticeID, uint32_t IndiceID, uint32_t AnimID, uint32_t PassBits, const LWSMatrix4f &Transform, Material &Mat, uint32_t Flags, uint32_t Offset, uint32_t Count) {
	if (!PassBits) return -1;
	GFrameModel Mdl = GFrameModel(Mat.GetPipelineID(), VerticeID, IndiceID, Flags, Offset, Count);
	GMaterial M = PrepareGMaterial(Mdl, LWVector2f(0.0f), LWVector2f(1.0f), 1.0f, Mat);
	return F.PushModel(Mdl, PassBits, AnimID, Transform, M, Mat.isTransparent());
}

uint32_t Renderer::PushPendingGeometry(uint32_t ID, uint32_t DataType, char *Data, uint32_t DataCnt, uint32_t DataSize, LWAllocator &Allocator, bool Copy) {
	if (m_PendingGeomWriteFrame - m_PendingGeomReadFrame >= MaxPendingGeometry) return 0;
	ID = ID ? ID : NextGeometryID();
	char *D = Data;
	if (Copy) {
		uint32_t Len = DataCnt * DataSize;
		D = Allocator.Allocate<char>(Len);
		std::copy(Data, Data + Len, D);
	}
	PendingGeometry &P = m_PendingGeometry[m_PendingGeomWriteFrame % MaxPendingGeometry];
	P = PendingGeometry(D, ID, DataType, DataSize, DataCnt);
	m_PendingGeomWriteFrame++;
	return ID;
}

uint32_t Renderer::PushPendingTexture(uint32_t ID, LWImage *Image) {
	if (m_PendingTexWriteFrame - m_PendingTexReadFrame >= MaxPendingTexture) return 0;
	ID = ID ? ID : NextTextureID();
	uint32_t Idx = m_PendingTexWriteFrame % MaxPendingTexture;
	m_PendingTextures[Idx] = { Image, ID };
	m_PendingTexWriteFrame++;
	return ID;
}

void Renderer::ProcessPendingGeometry(void) {
	const uint32_t MaxPerFrame = 5;
	for (uint32_t i = 0; i < MaxPerFrame && m_PendingGeomReadFrame != m_PendingGeomWriteFrame; i++, m_PendingGeomReadFrame++) {
		PendingGeometry &PGeom = m_PendingGeometry[m_PendingGeomReadFrame % MaxPendingGeometry];
		LWVideoBuffer *Buf = nullptr;
		if (PGeom.m_Count) {
			if (!(Buf = PGeom.MakeBuffer(m_Driver, m_Allocator))) continue;
		}
		auto Iter = m_GeometryMap.find(PGeom.m_ID);
		if (Iter == m_GeometryMap.end()) {
			m_GeometryMap.emplace(PGeom.m_ID, Buf);
		} else {
			LWVideoBuffer *oBuf = Iter->second;
			Iter->second = Buf;
			if (oBuf) m_Driver->DestroyVideoBuffer(oBuf);
		}
		PGeom.Finished();
	}
	return;
}

void Renderer::ProcessPendingTextures(void) {
	const uint32_t MaxPerFrame = 5;
	for (uint32_t i = 0; i < MaxPerFrame && m_PendingTexReadFrame != m_PendingTexWriteFrame; i++, m_PendingTexReadFrame++) {
		PendingTexture &PTex = m_PendingTextures[m_PendingTexReadFrame % MaxPendingTexture];
		LWTexture *Tex = nullptr;
		if (PTex.m_Image) Tex = m_Driver->CreateTexture(0, *PTex.m_Image, m_Allocator);
		auto Iter = m_TextureMap.find(PTex.m_ID);
		if (Iter == m_TextureMap.end()) {
			m_TextureMap.emplace(PTex.m_ID, Tex);
		} else {
			LWTexture *oTex = Iter->second;
			Iter->second = Tex;
			if (oTex) m_Driver->DestroyTexture(oTex);
		}
		LWAllocator::Destroy(PTex.m_Image);
	}
	return;
}

void Renderer::SetIBLBrdfTexture(LWTexture *Tex) {
	m_MetallicRoughnessPipeline->SetResource("brdfLUTTex", Tex);
	m_SpecularGlossinessPipeline->SetResource("brdfLUTTex", Tex);
	return;
}

void Renderer::SetIBLDiffuseTexture(LWTexture *Tex) {
	m_MetallicRoughnessPipeline->SetResource("DiffuseEnvTex", Tex);
	m_SpecularGlossinessPipeline->SetResource("DiffuseEnvTex", Tex);
	return;
}

void Renderer::SetIBLSpecularTexture(LWTexture *Tex) {
	m_MetallicRoughnessPipeline->SetResource("SpecularEnvTex", Tex);
	m_SpecularGlossinessPipeline->SetResource("SpecularEnvTex", Tex);
	return;
}

LWTexture *Renderer::GetTexture(uint32_t ID) {
	auto Iter = m_TextureMap.find(ID);
	if (Iter == m_TextureMap.end()) return nullptr;
	return Iter->second;
}

LWVideoBuffer *Renderer::GetGeometry(uint32_t ID) {
	auto Iter = m_GeometryMap.find(ID);
	if (Iter == m_GeometryMap.end()) return nullptr;
	return Iter->second;
}

bool Renderer::GeometryIsLoaded(uint32_t ID) const {
	auto Iter = m_GeometryMap.find(ID);
	if (Iter == m_GeometryMap.end()) return false;
	if (!Iter->second) return false;
	return true;
}

bool Renderer::TextureIsLoaded(uint32_t ID) const {
	auto Iter = m_TextureMap.find(ID);
	if (Iter == m_TextureMap.end()) return false;
	if (!Iter->second) return false;
	return true;
}

uint32_t Renderer::NextGeometryID(void) {
	return ++m_NextGeometryID;
}

uint32_t Renderer::NextTextureID(void) {
	return ++m_NextTextureID;
}

bool Renderer::MakeConePrimitive(void){
	const uint32_t ConeRadiCnt = 30;
	GStaticVertice Verts[ConeRadiCnt + 2];
	uint16_t Idxs[ConeRadiCnt * 6];

	m_ConeVertID = NextGeometryID();
	m_ConeIdxID = NextGeometryID();

	Verts[0] = { LWVector4f(0.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 0.0f), LWVector4f(0.0f), LWVector4f(0.0f) };
	Verts[1] = { LWVector4f(0.0f, 0.0f, 1.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 0.0f), LWVector4f(0.0f), LWVector4f(0.0f) };
	for (uint32_t n = 0; n < ConeRadiCnt; n++) {
		float T = LW_2PI / ConeRadiCnt * n;
		LWVector2f D = LWVector2f::MakeTheta(T);
		Verts[n + 2] = { LWVector4f(D.x, D.y, 1.0f, 1.0f), LWVector4f(0.0f), LWVector4f(0.0f), LWVector4f(0.0f) };
	
		uint32_t a = n * 3;
		uint32_t b = (n * 3) + (ConeRadiCnt * 3);
		Idxs[a + 0] = 0;
		Idxs[a + 1] = ((n + 1) % ConeRadiCnt) + 2;
		Idxs[a + 2] = (n)+2;

		Idxs[b + 0] = 1;
		Idxs[b + 1] = (n)+2;
		Idxs[b + 2] = ((n + 1) % ConeRadiCnt) + 2;
	}
	LWVideoBuffer *VBuffer = m_Driver->CreateVideoBuffer<GStaticVertice>(LWVideoBuffer::Vertex, LWVideoBuffer::Static, ConeRadiCnt + 2, m_Allocator, Verts);
	LWVideoBuffer *IBuffer = m_Driver->CreateVideoBuffer<uint16_t>(LWVideoBuffer::Index16, LWVideoBuffer::Static, ConeRadiCnt * 6, m_Allocator, Idxs);
	m_GeometryMap.emplace(m_ConeVertID, VBuffer);
	m_GeometryMap.emplace(m_ConeIdxID, IBuffer);
	return true;
}

bool Renderer::MakeCubePrimitive(void) {
	GStaticVertice Verts[24];
	uint16_t Idxs[36] = {
		0, 2, 1, 1, 2, 3, //Back
		4, 5, 6, 5, 7, 6, //Front
		8, 9,10, 9,11,10, //Left
	   12,14,13,13,14,15, //Right
	   16,17,18,17,19,18, //Top
	   20,22,21,21,22,23 //Btm
	};
	//Back
	Verts[0] = { LWVector4f(-1.0f, -1.0f, -1.0f, 1.0f), LWVector4f(1.0f, 1.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, -1.0f, 0.0f) };
	Verts[1] = { LWVector4f(1.0f, -1.0f, -1.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, -1.0f, 0.0f) };
	Verts[2] = { LWVector4f(-1.0f,  1.0f, -1.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, -1.0f, 0.0f) };
	Verts[3] = { LWVector4f(1.0f,  1.0f, -1.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, -1.0f, 0.0f) };

	//Front
	Verts[4] = { LWVector4f(-1.0f, -1.0f,  1.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 1.0f, 0.0f) };
	Verts[5] = { LWVector4f(1.0f, -1.0f,  1.0f, 1.0f), LWVector4f(1.0f, 1.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 1.0f, 0.0f) };
	Verts[6] = { LWVector4f(-1.0f,  1.0f,  1.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 1.0f, 0.0f) };
	Verts[7] = { LWVector4f(1.0f,  1.0f,  1.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 1.0f, 0.0f) };

	//Left
	Verts[8] = { LWVector4f(-1.0f, -1.0f, -1.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(-1.0f, 0.0f, 0.0f, 0.0f) };
	Verts[9] = { LWVector4f(-1.0f, -1.0f,  1.0f, 1.0f), LWVector4f(1.0f, 1.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(-1.0f, 0.0f, 0.0f, 0.0f) };
	Verts[10] = { LWVector4f(-1.0f,  1.0f, -1.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(-1.0f, 0.0f, 0.0f, 0.0f) };
	Verts[11] = { LWVector4f(-1.0f,  1.0f,  1.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(-1.0f, 0.0f, 0.0f, 0.0f) };

	//Right
	Verts[12] = { LWVector4f(1.0f, -1.0f, -1.0f, 1.0f), LWVector4f(1.0f, 1.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f) };
	Verts[13] = { LWVector4f(1.0f, -1.0f,  1.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f) };
	Verts[14] = { LWVector4f(1.0f,  1.0f, -1.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f) };
	Verts[15] = { LWVector4f(1.0f,  1.0f,  1.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 0.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f) };

	//Top
	Verts[16] = { LWVector4f(-1.0f,  1.0f, -1.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f) };
	Verts[17] = { LWVector4f(-1.0f,  1.0f,  1.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f) };
	Verts[18] = { LWVector4f(1.0f,  1.0f, -1.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f) };
	Verts[19] = { LWVector4f(1.0f,  1.0f,  1.0f, 1.0f), LWVector4f(1.0f, 1.0f, 0.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f) };

	//Bottom
	Verts[20] = { LWVector4f(-1.0f, -1.0f, -1.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f,-1.0f, 0.0f, 0.0f) };
	Verts[21] = { LWVector4f(-1.0f, -1.0f,  1.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f,-1.0f, 0.0f, 0.0f) };
	Verts[22] = { LWVector4f(1.0f, -1.0f, -1.0f, 1.0f), LWVector4f(1.0f, 1.0f, 0.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f,-1.0f, 0.0f, 0.0f) };
	Verts[23] = { LWVector4f(1.0f, -1.0f,  1.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f,-1.0f, 0.0f, 0.0f) };
	/*
	//Create Normals and tangents.
	for (uint32_t i = 0; i < 24; i++) {
		LWVector3f Fwrd = Verts[i].m_Position.xyz().Normalize();
		LWVector3f Right;
		LWVector3f Up;
		Fwrd.Othogonal(Right, Up);
		Verts[i].m_Normal = LWVector4f(Fwrd, 0.0f);
		Verts[i].m_Tangent = LWVector4f(Right, 1.0f);
	}*/

	m_CubeVertID = NextGeometryID();
	m_CubeIdxID = NextGeometryID();
	LWVideoBuffer *VBuffer = m_Driver->CreateVideoBuffer<GStaticVertice>(LWVideoBuffer::Vertex, LWVideoBuffer::Static, 24, m_Allocator, Verts);
	LWVideoBuffer *IBuffer = m_Driver->CreateVideoBuffer<uint16_t>(LWVideoBuffer::Index16, LWVideoBuffer::Static, 36, m_Allocator, Idxs);
	m_GeometryMap.emplace(m_CubeVertID, VBuffer);
	m_GeometryMap.emplace(m_CubeIdxID, IBuffer);
	return true;
}

bool Renderer::MakeSpherePrimitive(void) {

	//Sphere
	const uint32_t verticalSteps = 20;
	const uint32_t horizontalSteps = 20;
	const uint32_t TotalVertices = verticalSteps * horizontalSteps * 6;
	GStaticVertice Verts[TotalVertices];
	uint32_t o = 0;

	float uStep = 1.0f / (horizontalSteps+1);
	float vStep = 1.0f / verticalSteps;
	float v = LW_PI_2;
	for (uint32_t y = 1; y <= verticalSteps; y++) {
		float nv = LW_PI_2 + LW_PI / (float)verticalSteps * (float)y;
		float h = 0.0f;
		for (uint32_t x = 1; x <= horizontalSteps; x++) {
			float nh = LW_2PI / (float)horizontalSteps * (float)x;
			LWVector3f TopLeftPnt = LWVector3f(cosf(h) * cosf(v), sinf(v), sinf(h) * cosf(v));
			LWVector3f BtmLeftPnt = LWVector3f(cosf(h) * cosf(nv), sinf(nv), sinf(h) * cosf(nv));
			LWVector3f TopRightPnt = LWVector3f(cosf(nh) * cosf(v), sinf(v), sinf(nh) * cosf(v));
			LWVector3f BtmRightPnt = LWVector3f(cosf(nh) * cosf(nv), sinf(nv), sinf(nh) * cosf(nv));

			LWVector2f TCTopLeft = LWVector2f((x - 1) * uStep, (y - 1) * vStep);
			LWVector2f TCBtmLeft = LWVector2f((x - 1) * uStep, y * vStep);
			LWVector2f TCTopRight = LWVector2f(x * uStep, (y - 1) * vStep);
			LWVector2f TCBtmRight = LWVector2f(x * uStep, y * vStep);

			LWVector3f TLUp = LWVector3f();
			LWVector3f TLRight = LWVector3f();
			LWVector3f BLUp = LWVector3f();
			LWVector3f BLRight = LWVector3f();
			LWVector3f TRUp = LWVector3f();
			LWVector3f TRRight = LWVector3f();
			LWVector3f BRUp = LWVector3f();
			LWVector3f BRRight = LWVector3f();

			TopLeftPnt.Othogonal(TLRight, TLUp);
			BtmLeftPnt.Othogonal(BLRight, BLUp);
			TopRightPnt.Othogonal(TRRight, TRUp);
			BtmRightPnt.Othogonal(BRRight, BRUp);

			Verts[o++] = { LWVector4f(TopLeftPnt, 1.0f), LWVector4f(TCTopLeft, 0.0f, 0.0f), LWVector4f(TLUp, 1.0f), LWVector4f(TopLeftPnt, 0.0f) };
			Verts[o++] = { LWVector4f(TopRightPnt, 1.0f), LWVector4f(TCTopRight, 0.0f, 0.0f), LWVector4f(TRUp, 1.0f), LWVector4f(TopRightPnt, 0.0f) };
			Verts[o++] = { LWVector4f(BtmRightPnt, 1.0f), LWVector4f(TCBtmRight, 0.0f, 0.0f), LWVector4f(BRUp, 1.0f), LWVector4f(BtmRightPnt, 0.0f) };
			Verts[o++] = { LWVector4f(TopLeftPnt, 1.0f), LWVector4f(TCTopLeft, 0.0f, 0.0f), LWVector4f(TLUp, 1.0f), LWVector4f(TopLeftPnt, 0.0f) };
			Verts[o++] = { LWVector4f(BtmRightPnt, 1.0f), LWVector4f(TCBtmRight, 0.0f, 0.0f), LWVector4f(BRUp, 1.0f), LWVector4f(BtmRightPnt, 0.0f) };
			Verts[o++] = { LWVector4f(BtmLeftPnt, 1.0f), LWVector4f(TCBtmLeft, 0.0f, 0.0f), LWVector4f(BLUp, 1.0f), LWVector4f(BtmLeftPnt, 0.0f) };
			h = nh;
		}
		v = nv;
	}
	m_SphereVertID = NextGeometryID();
	LWVideoBuffer *VBuffer = m_Driver->CreateVideoBuffer<GStaticVertice>(LWVideoBuffer::Vertex, LWVideoBuffer::Static, TotalVertices, m_Allocator, Verts);
	m_GeometryMap.emplace(m_SphereVertID, VBuffer);
	return true;
}

bool Renderer::MakeHalfSpherePrimitive(void) {
	const uint32_t verticalSteps = 10;
	const uint32_t horizontalSteps = 20;
	const uint32_t TotalVertices = verticalSteps * horizontalSteps * 6 + horizontalSteps * 3;
	GStaticVertice Verts[TotalVertices];
	uint32_t o = 0;

	float uStep = 1.0f / (horizontalSteps+1);
	float vStep = 1.0f / verticalSteps;
	float v = LW_PI_2;
	for (uint32_t y = 1; y <= verticalSteps; y++) {
		float nv = LW_PI_2 + LW_PI_2 / (float)verticalSteps * (float)y;
		float h = 0.0f;
		for (uint32_t x = 1; x <= horizontalSteps; x++) {
			float nh = LW_2PI / (float)horizontalSteps * (float)x;
			LWVector3f TopLeftPnt = LWVector3f(cosf(h) * cosf(v), sinf(v), sinf(h) * cosf(v));
			LWVector3f BtmLeftPnt = LWVector3f(cosf(h) * cosf(nv), sinf(nv), sinf(h) * cosf(nv));
			LWVector3f TopRightPnt = LWVector3f(cosf(nh) * cosf(v), sinf(v), sinf(nh) * cosf(v));
			LWVector3f BtmRightPnt = LWVector3f(cosf(nh) * cosf(nv), sinf(nv), sinf(nh) * cosf(nv));

			LWVector2f TCTopLeft = LWVector2f((x - 1) * uStep, (y - 1) * vStep);
			LWVector2f TCBtmLeft = LWVector2f((x - 1) * uStep, y * vStep);
			LWVector2f TCTopRight = LWVector2f(x * uStep, (y - 1) * vStep);
			LWVector2f TCBtmRight = LWVector2f(x * uStep, y * vStep);

			LWVector3f TLUp = LWVector3f();
			LWVector3f TLRight = LWVector3f();
			LWVector3f BLUp = LWVector3f();
			LWVector3f BLRight = LWVector3f();
			LWVector3f TRUp = LWVector3f();
			LWVector3f TRRight = LWVector3f();
			LWVector3f BRUp = LWVector3f();
			LWVector3f BRRight = LWVector3f();

			TopLeftPnt.Othogonal(TLRight, TLUp);
			BtmLeftPnt.Othogonal(BLRight, BLUp);
			TopRightPnt.Othogonal(TRRight, TRUp);
			BtmRightPnt.Othogonal(BRRight, BRUp);

			Verts[o++] = { LWVector4f(TopLeftPnt, 1.0f), LWVector4f(TCTopLeft, 0.0f, 0.0f), LWVector4f(TLUp, 1.0f), LWVector4f(TopLeftPnt, 0.0f) };
			Verts[o++] = { LWVector4f(TopRightPnt, 1.0f), LWVector4f(TCTopRight, 0.0f, 0.0f), LWVector4f(TRUp, 1.0f), LWVector4f(TopRightPnt, 0.0f) };
			Verts[o++] = { LWVector4f(BtmRightPnt, 1.0f), LWVector4f(TCBtmRight, 0.0f, 0.0f), LWVector4f(BRUp, 1.0f), LWVector4f(BtmRightPnt, 0.0f) };
			Verts[o++] = { LWVector4f(TopLeftPnt, 1.0f), LWVector4f(TCTopLeft, 0.0f, 0.0f), LWVector4f(TLUp, 1.0f), LWVector4f(TopLeftPnt, 0.0f) };
			Verts[o++] = { LWVector4f(BtmRightPnt, 1.0f), LWVector4f(TCBtmRight, 0.0f, 0.0f), LWVector4f(BRUp, 1.0f), LWVector4f(BtmRightPnt, 0.0f) };
			Verts[o++] = { LWVector4f(BtmLeftPnt, 1.0f), LWVector4f(TCBtmLeft, 0.0f, 0.0f), LWVector4f(BLUp, 1.0f), LWVector4f(BtmLeftPnt, 0.0f) };
			h = nh;
		}
		v = nv;
	}
	float h = 0.0f;
	for (uint32_t x = 1; x <= horizontalSteps; x++) {
		float nh = LW_2PI / (float)horizontalSteps * (float)x;
		LWVector3f PntA = LWVector3f(cosf(h), 0.0f, sinf(h));
		LWVector3f PntB = LWVector3f(cosf(nh), 0.0f, sinf(nh));
		LWVector3f PntC = LWVector3f(0.0f);
		LWVector4f Tangent = LWVector4f(1.0f, 0.0f, 0.0f, 1.0f);
		LWVector4f Normal = LWVector4f(0.0f, -1.0f, 0.0f, 0.0f);
		Verts[o++] = { LWVector4f(PntA, 1.0f), LWVector4f((PntA.xz() + 1.0f) * 0.5f, 0.0f, 0.0f), Tangent, Normal };
		Verts[o++] = { LWVector4f(PntB, 1.0f), LWVector4f((PntB.xz() + 1.0f) * 0.5f, 0.0f, 0.0f), Tangent, Normal };
		Verts[o++] = { LWVector4f(PntC, 1.0f), LWVector4f((PntC.xz() + 1.0f) * 0.5f, 0.0f, 0.0f), Tangent, Normal };
		h = nh;
	}
	m_HalfSphereVertID = NextGeometryID();
	LWVideoBuffer *VBuffer = m_Driver->CreateVideoBuffer<GStaticVertice>(LWVideoBuffer::Vertex, LWVideoBuffer::Static, o, m_Allocator, Verts);
	m_GeometryMap.emplace(m_HalfSphereVertID, VBuffer);
	return true;
}

bool Renderer::MakePlanePrimitive(void) {
	GStaticVertice Verts[6];
	Verts[0] = { LWVector4f(-1.0f, -1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 1.0f, 0.0f) };
	Verts[1] = { LWVector4f(1.0f, -1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 1.0f, 0.0f) };
	Verts[2] = { LWVector4f(-1.0f,  1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 1.0f, 0.0f) };
	Verts[3] = { LWVector4f(1.0f, -1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 1.0f, 0.0f) };
	Verts[4] = { LWVector4f(1.0f, 1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 1.0f, 0.0f) };
	Verts[5] = { LWVector4f(-1.0f,  1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 1.0f, 0.0f) };
	
	m_PlaneVertID = NextGeometryID();
	LWVideoBuffer *VBuffer = m_Driver->CreateVideoBuffer<GStaticVertice>(LWVideoBuffer::Vertex, LWVideoBuffer::Static, 6, m_Allocator, Verts);
	m_GeometryMap.emplace(m_PlaneVertID, VBuffer);
	return true;
}


bool Renderer::MakeSkyBoxPrimitive(void) {
	const uint32_t HorizontalSteps = 10;
	const float h = 0.03f;
	float uStep = 1.0f / HorizontalSteps;
	GStaticVertice Verts[HorizontalSteps*6+HorizontalSteps*3];
	float hStep = 0.0f;
	uint32_t o = 0;
	float Inner = 0.5f;
	float iInner = 1.0f / Inner;
	Verts[o++] = { LWVector4f( 0.0f, h, 0.0f, 1.0f), LWVector4f(0.5f, 0.5f, 0.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f) };
	Verts[o++] = { LWVector4f( 1.0f,-h, 0.0f, 0.0f), LWVector4f(1.0f, 0.5f, 1.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f) };
	Verts[o++] = { LWVector4f( 0.0f,-h, 1.0f, 0.0f), LWVector4f(0.5f, 1.0f, 1.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f) };
	Verts[o++] = { LWVector4f(-1.0f,-h, 0.0f, 0.0f), LWVector4f(0.0f, 0.5f, 1.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f) };
	Verts[o++] = { LWVector4f( 0.0f,-h,-1.0f, 0.0f), LWVector4f(0.5f, 0.0f, 1.0f, 0.0f), LWVector4f(1.0f, 0.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f) };
	
	uint16_t Idxs[12] = { 0, 1, 2,  0, 2, 3,  0,3,4,  0, 4, 1 };

	m_SkyBoxVertID = NextGeometryID();
	m_SkyBoxIdxID = NextGeometryID();
	LWVideoBuffer *VBuffer = m_Driver->CreateVideoBuffer<GStaticVertice>(LWVideoBuffer::Vertex, LWVideoBuffer::Static, o, m_Allocator, Verts);
	LWVideoBuffer *IBuffer = m_Driver->CreateVideoBuffer<uint16_t>(LWVideoBuffer::Index16, LWVideoBuffer::Static, 12, m_Allocator, Idxs);
	m_GeometryMap.emplace(m_SkyBoxVertID, VBuffer);
	m_GeometryMap.emplace(m_SkyBoxIdxID, IBuffer);
	return true;
}

bool Renderer::MakePrimitives(void) {
	if (!MakeConePrimitive()) return false;
	if (!MakeCubePrimitive()) return false;
	if (!MakeSpherePrimitive()) return false;
	if (!MakeHalfSpherePrimitive()) return false;
	if (!MakePlanePrimitive()) return false;
	if (!MakeSkyBoxPrimitive()) return false;
	return true;
}

uint32_t Renderer::GetCurrentRenderedFrame(void) const {
	return m_ReadFrame-1;
}

LWTexture *Renderer::GetOutputTexture(void) {
	return m_OutputTexture;
}

uint32_t Renderer::GetParticleVertID(void) const {
	return m_ParticleVertID;
}

uint32_t Renderer::GetParticleIdxID(void) const {
	return m_ParticleIdxID;
}

uint32_t Renderer::GetCubeVertID(void) const {
	return m_CubeVertID;
}

uint32_t Renderer::GetCubeIdxID(void) const {
	return m_CubeIdxID;
}

uint32_t Renderer::GetSphereVertID(void) const {
	return m_SphereVertID;
}

uint32_t Renderer::GetHalfSphereVertID(void) const {
	return m_HalfSphereVertID;
}

uint32_t Renderer::GetConeVertID(void) const {
	return m_ConeVertID;
}

uint32_t Renderer::GetConeIdxID(void) const {
	return m_ConeIdxID;
}

uint32_t Renderer::GetPlaneVertID(void) const {
	return m_PlaneVertID;
}

uint32_t Renderer::GetSkyBoxVertID(void) const {
	return m_SkyBoxVertID;
}

uint32_t Renderer::GetSkyBoxIdxID(void) const {
	return m_SkyBoxIdxID;
}

RenderSettings Renderer::GetSettings(void) const {
	return m_Settings;
}

Renderer::Renderer(LWVideoDriver *Driver, LWAllocator &Allocator) : m_Allocator(Allocator), m_Driver(Driver) {

	m_UIUniform = Driver->CreateVideoBuffer<LWMatrix4f>(LWVideoBuffer::Uniform, LWVideoBuffer::WriteDiscardable, 1, m_Allocator, nullptr);
	m_LightDataBuffer = m_Driver->CreateVideoBuffer<GLight>(LWVideoBuffer::ImageBuffer, LWVideoBuffer::WriteDiscardable, MaxLights, m_Allocator, nullptr);
	m_GlobalDataBlock = m_Driver->CreatePaddedVideoBuffer<GGlobalData>(LWVideoBuffer::Uniform, LWVideoBuffer::WriteDiscardable, 1, m_Allocator, nullptr);
	m_PassDataBlock = m_Driver->CreatePaddedVideoBuffer<GPassData>(LWVideoBuffer::Uniform, LWVideoBuffer::WriteDiscardable, MaxRawPasses, m_Allocator, nullptr);
	m_AnimDataBlock = m_Driver->CreatePaddedVideoBuffer<GAnimData>(LWVideoBuffer::Uniform, LWVideoBuffer::WriteDiscardable, MaxAnimations, m_Allocator, nullptr);
	m_ModelDataBlock = m_Driver->CreatePaddedVideoBuffer<GModelData>(LWVideoBuffer::Uniform, LWVideoBuffer::WriteDiscardable, MaxModels, m_Allocator, nullptr);

	LWVertexTexture PostProcessGeom[6] = { LWVertexTexture(LWVector4f(-1.0f, 1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 0.0f)),
										LWVertexTexture(LWVector4f(-1.0f,-1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 1.0f, 0.0f, 0.0f)),
										LWVertexTexture(LWVector4f(1.0f,-1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 1.0f, 0.0f, 0.0f)),
										LWVertexTexture(LWVector4f(1.0f,-1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 1.0f, 0.0f, 0.0f)),
										LWVertexTexture(LWVector4f(1.0f, 1.0f, 0.0f, 1.0f), LWVector4f(1.0f, 0.0f, 0.0f, 0.0f)),
										LWVertexTexture(LWVector4f(-1.0f, 1.0f, 0.0f, 1.0f), LWVector4f(0.0f, 0.0f, 0.0f, 0.0f)) };

	m_PostProcessGeometry = m_Driver->CreateVideoBuffer<LWVertexTexture>(LWVideoBuffer::Vertex, LWVideoBuffer::Static, 6, Allocator, PostProcessGeom);
	m_CopyGeometry = m_Driver->CreateVideoBuffer<LWVertexUI>(LWVideoBuffer::Vertex, LWVideoBuffer::WriteDiscardable, 6, Allocator, nullptr);

	m_ParticleVertID = NextGeometryID();
	m_ParticleIdxID = NextGeometryID();

	uint32_t ParticleIdxCount = GFrame::MaxParticleVertices / 4 * 6;
	uint32_t *ParticleIdxBuffer = Allocator.Allocate<uint32_t>(ParticleIdxCount);
	for (uint32_t i = 0, n = 0; i < ParticleIdxCount; i += 6, n += 4) {
		ParticleIdxBuffer[i + 0] = n; ParticleIdxBuffer[i + 1] = n + 1; ParticleIdxBuffer[i + 2] = n + 2;
		ParticleIdxBuffer[i + 3] = n + 2; ParticleIdxBuffer[i + 4] = n + 3; ParticleIdxBuffer[i + 5] = n;
	}
	m_ParticleVertBuffer = m_Driver->CreateVideoBuffer<ParticleVert>(LWVideoBuffer::Vertex, LWVideoBuffer::WriteDiscardable, GFrame::MaxParticleVertices, Allocator, nullptr);
	LWVideoBuffer *VParticleIdxBuffer = m_Driver->CreateVideoBuffer<uint32_t>(LWVideoBuffer::Index32, LWVideoBuffer::Static, ParticleIdxCount, Allocator, ParticleIdxBuffer);
	LWAllocator::Destroy(ParticleIdxBuffer);
	m_GeometryMap.emplace(m_ParticleVertID, m_ParticleVertBuffer);
	m_GeometryMap.emplace(m_ParticleIdxID, VParticleIdxBuffer);

	for (uint32_t i = 0; i < MaxFrames; i++) new (&m_Frames[i]) GFrame(m_Driver, Allocator);
	MakePrimitives();
}

Renderer::~Renderer() {
	m_Driver->DestroyVideoBuffer(m_UIUniform);
	m_Driver->DestroyVideoBuffer(m_PassDataBlock);
	m_Driver->DestroyVideoBuffer(m_AnimDataBlock);
	m_Driver->DestroyVideoBuffer(m_ModelDataBlock);
	m_Driver->DestroyVideoBuffer(m_GlobalDataBlock);
	m_Driver->DestroyVideoBuffer(m_LightDataBuffer);
	m_Driver->DestroyVideoBuffer(m_PostProcessGeometry);
	m_Driver->DestroyVideoBuffer(m_CopyGeometry);
	if (m_LightArrayBuffer) m_Driver->DestroyVideoBuffer(m_LightArrayBuffer);
	if (m_GaussianKernel) m_Driver->DestroyVideoBuffer(m_GaussianKernel);
	if (m_ScreenFB) {
		m_Driver->DestroyFrameBuffer(m_ScreenFB);
		m_Driver->DestroyTexture(m_ScreenTexMS);
		m_Driver->DestroyTexture(m_EmissionTexMS);
		m_Driver->DestroyTexture(m_FinalScreenTex);
		m_Driver->DestroyTexture(m_ScreenTex);
		m_Driver->DestroyTexture(m_EmissionTex);
		m_Driver->DestroyTexture(m_ScreenDepth);
	}
	if (m_HighlightFB) {
		m_Driver->DestroyFrameBuffer(m_HighlightFB);
		m_Driver->DestroyTexture(m_HighlightTex);
		m_Driver->DestroyTexture(m_HighlightTexMS);
	}
	if (m_BlurFB) {
		m_Driver->DestroyFrameBuffer(m_BlurFB);
		m_Driver->DestroyTexture(m_BlurTempTexture);
		m_Driver->DestroyTexture(m_BEmissionTexture);
		m_Driver->DestroyTexture(m_BHighlightTexture);
	}
	if (m_OutputFramebuffer) {
		m_Driver->DestroyFrameBuffer(m_OutputFramebuffer);
		m_Driver->DestroyTexture(m_OutputTexture);
	}
	if (m_ShadowFrameBuffer) {
		m_Driver->DestroyFrameBuffer(m_ShadowFrameBuffer);
		m_Driver->DestroyFrameBuffer(m_ShadowCubeFrameBuffer);
		m_Driver->DestroyTexture(m_ShadowTextureArray);
		m_Driver->DestroyTexture(m_ShadowCubemapArray);
	}
	if (m_ReflectionFrameBuffer) {
		m_Driver->DestroyFrameBuffer(m_ReflectionFrameBuffer);
		m_Driver->DestroyTexture(m_ReflectionDepthmap);
		m_Driver->DestroyTexture(m_ReflectionCubemap[0]);
		m_Driver->DestroyTexture(m_ReflectionCubemap[1]);
	}

	for (auto &&Iter : m_GeometryMap) {
		if (Iter.second) m_Driver->DestroyVideoBuffer(Iter.second);
	}
	for (auto &&Iter : m_TextureMap) {
		if (Iter.second) m_Driver->DestroyTexture(Iter.second);
	}
}