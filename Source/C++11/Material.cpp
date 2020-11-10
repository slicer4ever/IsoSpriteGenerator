#include "Material.h"
#include <LWCore/LWByteBuffer.h>
#include <LWEGLTFParser.h>

const LWVector4f Material::DefaultACloudSettings = LWVector4f(1.1f, 0.001f, 0.5f, 0.3f);
const LWVector4f Material::DefaultBCloudSettings = LWVector4f(0.2f, 1.0f, 0.0f, 0.0f);


bool MaterialTexture::Deserialize(MaterialTexture &Tex, LWByteBuffer &Buf) {
	Tex.m_TextureName = Buf.Read<uint32_t>();
	Tex.m_TextureState = Buf.Read<uint32_t>();
	return true;
}

uint32_t MaterialTexture::Serialize(LWByteBuffer &Buf) {
	uint32_t o = 0;
	o += Buf.Write<uint32_t>(m_TextureName);
	o += Buf.Write<uint32_t>(m_TextureState);
	return o;
}

bool Material::Deserialize(Material &Mat, LWByteBuffer &Buf) {
	uint32_t PipelineID = Buf.Read<uint32_t>();
	uint32_t NameHash = Buf.Read<uint32_t>();
	uint32_t Flag = Buf.Read<uint32_t>();
	uint32_t TexCnt = PipelineTextureCount(PipelineID);
	uint32_t ClrCnt = PipelineColorCount(PipelineID);
	Mat = Material(PipelineID, NameHash);
	Mat.SetFlag(Flag);
	for (uint32_t i = 0; i < ClrCnt; i++) LWETween<LWVector4f>::Deserialize(Mat.GetColorTween(i), Buf);
	for (uint32_t i = 0; i < TexCnt; i++) {
		MaterialTexture::Deserialize(Mat.GetTexture(i), Buf);
		LWETween<LWVector4f>::Deserialize(Mat.GetTextureTween(i), Buf);
	}
	Mat.UpdatedTweens();
	return true;
}

uint32_t Material::PipelineTextureCount(uint32_t PipelineID) {
	switch (PipelineID) {
	case PBRMetallicRoughness:
	case PBRSpecularGlossiness:
		return 5;
	case PBRUnlit:
		return 4;
	case Skybox:
		return 3;
	case Cloud:
		return 1;
	};
	return 0;
}

uint32_t Material::PipelineColorCount(uint32_t PipelineID) {
	switch (PipelineID) {
	case PBRMetallicRoughness:
	case PBRSpecularGlossiness:
		return 3;
	case PBRUnlit:
		return 2;
	};
	return 0;
}

uint32_t Material::Serialize(LWByteBuffer &Buf) {
	uint32_t TexCnt = GetTextureCount();
	uint32_t ClrCnt = GetColorCount();
	uint32_t o = 0;
	o += Buf.Write<uint32_t>(m_PipelineID);
	o += Buf.Write<uint32_t>(m_NameHash);
	o += Buf.Write<uint32_t>(m_Flag);
	for (uint32_t i = 0; i < ClrCnt; i++) o += m_ColorTweens[i].Serialize(Buf);
	for (uint32_t i = 0; i < TexCnt; i++) {
		o += m_TextureList[i].Serialize(Buf);
		o += m_TextureTweens[i].Serialize(Buf);
	}
	return o;
}

Material &Material::Update(float dTime) {
	if (!isPlaying()) return *this;
	bool Loop = isLooping();
	m_Time += dTime;
	if (Loop) m_Time = m_TotalTime > 0.0f ? fmodf(m_Time, m_TotalTime) : 0.0f;
	return *this;
}

Material &Material::Play(void) {
	m_Flag &= ~Paused;
	return *this;
}

Material &Material::Pause(void) {
	m_Flag |= Paused;
	return *this;
}

Material &Material::SetTransparent(bool iTransparent) {
	m_Flag = (m_Flag & ~Transparent) | (iTransparent ? Transparent : 0);
	return *this;
}

Material &Material::SetTime(float Time, bool Loop) {
	if (Loop) {
		if (m_TotalTime > 0.0f) Time = (float)fmodf(Time, m_TotalTime);
	}
	m_Time = Time;

	return *this;
}

Material &Material::SetFlag(uint32_t Flag) {
	m_Flag = Flag;
	return *this;
}

/*
Material &Material::SetPipelineID(uint32_t PipelineID, DataManager *DM, uint64_t lCurrentTime) {
	uint32_t oTexCnt = GetTextureCount();
	m_PipelineID = PipelineID;
	uint32_t nTexCnt = GetTextureCount();
	if (!DM) return *this;
	for (uint32_t i = nTexCnt; i < oTexCnt; i++) {
		MaterialTexture &MT = m_TextureList[i];
		DM->ReleaseReference(MT.m_Reference, lCurrentTime);
		MT.m_Reference = nullptr;
	}
	return *this;
}*/

Material &Material::MakeGLTFMaterial(LWEGLTFParser &P, LWEGLTFMaterial *Mat) {
	auto ApplyTexture = [&P, this](LWEGLTFTextureInfo &TexInfo, uint32_t TexID) {
		LWEGLTFTexture *T = P.GetTexture(TexInfo.m_TextureIndex);
		if (!T) return;
		LWEGLTFImage *I = P.GetImage(T->m_ImageID);
		if (!I) return;
		MaterialTexture &MT = m_TextureList[TexID];
		MakeFlatTexture(TexID, LWVector4f(TexInfo.m_Offset, TexInfo.m_Scale));
		MT.m_TextureID = TexInfo.m_TextureIndex;
		MT.m_TextureName = I->GetName().Hash();
		MT.m_TextureState = T->m_SamplerFlag;
	};
	uint32_t MatType = Mat->GetType();
	LWEGLTFMatMetallicRoughness &MR = Mat->m_MetallicRoughness;
	LWEGLTFMatSpecularGlossyness &SG = Mat->m_SpecularGlossy;
	m_NameHash = Mat->GetName().Hash();
	ApplyTexture(Mat->m_NormalMapTexture, NormalTexID);
	ApplyTexture(Mat->m_OcclussionTexture, OcclussionTexID);
	ApplyTexture(Mat->m_EmissiveTexture, EmissiveTexID);
	MakeFlatColor(EmissiveFactorClrTweenID, Mat->m_EmissiveFactor);

	if (MatType == LWEGLTFMaterial::MetallicRoughness) {
		m_PipelineID = PBRMetallicRoughness;
		MakeFlatColor(PBRAlbedoBaseFactorClrTweenID, MR.m_BaseColorFactor);
		MakeFlatColor(PBRMetallicRoughnessClrTweenID, LWVector4f(MR.m_MetallicFactor, MR.m_RoughnessFactor, 0.0f, 0.0f));
		ApplyTexture(MR.m_BaseColorTexture, PBRAlbedoTexID);
		ApplyTexture(MR.m_MetallicRoughnessTexture, PBRMetallicRoughnessTexID);
	} else if (MatType == LWEGLTFMaterial::SpecularGlossyness) {
		m_PipelineID = PBRSpecularGlossiness;
		MakeFlatColor(SGDiffuseFactorClrTweenID, SG.m_DiffuseFactor);
		MakeFlatColor(SGSpecularFactorClrTweenID, LWVector4f(SG.m_SpecularFactor, SG.m_Glossiness));
		ApplyTexture(SG.m_DiffuseTexture, SGDiffuseColorTexID);
		ApplyTexture(SG.m_SpecularGlossyTexture, SGSpecularColorTexID);
	} else if (MatType == LWEGLTFMaterial::Unlit) {
		m_PipelineID = PBRUnlit;
		MakeFlatColor(ULColorTweenID, MR.m_BaseColorFactor);
		ApplyTexture(MR.m_BaseColorTexture, ULColorTexID);
	}
	return *this;
}
/*
bool Material::MakeInstance(Material &Dst, DataManager *DM, uint64_t lCurrentTime) {
	Ref<TextureID> *oRefList[MaxTextures];
	uint32_t oTexCnt = Dst.GetTextureCount();
	for (uint32_t i = 0; i < oTexCnt; i++) oRefList[i] = Dst.GetTexture(i).m_Reference;
	Dst = *this;
	bool Res = Dst.InstanceTextures(DM, lCurrentTime);
	for (uint32_t i = 0; i < oTexCnt; i++) DM->ReleaseReference(oRefList[i], lCurrentTime);
	return Res;
}

bool Material::InstanceTextures(DataManager *DM, uint64_t lCurrentTime) {
	uint32_t TextureCount = GetTextureCount();
	for (uint32_t i = 0; i < TextureCount; i++) {
		Ref<TextureID> *oRef = m_TextureList[i].m_Reference;
		Ref<TextureID> *nRef = nullptr;
		if (m_TextureList[i].m_TextureName != 0)  nRef = DM->GetTextureReference(m_TextureList[i].m_TextureName);
		m_TextureList[i].m_Reference = nRef;
		DM->ReleaseReference(oRef, lCurrentTime);
	}
	return true;
}

void Material::Release(DataManager *DM, uint64_t lCurrentTime) {
	uint32_t TextureCount = GetTextureCount();
	for (uint32_t i = 0; i < TextureCount; i++) {
		Ref<TextureID> *oRef = m_TextureList[i].m_Reference;
		m_TextureList[i].m_Reference = nullptr;
		DM->ReleaseReference(oRef, lCurrentTime);
	}
}
*/
Material &Material::MakeFlatColor(uint32_t ColorID, const LWVector4f &Color) {
	m_ColorTweens[ColorID] = LWETween<LWVector4f>(m_ColorTweens[ColorID].GetInterpolation());
	m_ColorTweens[ColorID].Push(Color, 0);
	return *this;
}

Material &Material::MakeFlatTexture(uint32_t TextureID, const LWVector4f &SubTexture) {
	m_TextureTweens[TextureID] = LWETween<LWVector4f>(m_TextureTweens[TextureID].GetInterpolation());
	m_TextureTweens[TextureID].Push(SubTexture, 0);
	return *this;
}

Material &Material::SetTransparency(float Transparency) {
	m_Transparency = Transparency;
	return *this;
}

Material &Material::SetNameHash(uint32_t NameHash) {
	m_NameHash = NameHash;
	return *this;
}

Material &Material::RenamedTexture(uint32_t PrevTextureName, uint32_t NewTextureName) {
	uint32_t TexCnt = GetTextureCount();
	for (uint32_t i = 0; i < TexCnt; i++) {
		MaterialTexture &MT = m_TextureList[i];
		if (MT.m_TextureName == PrevTextureName) MT.m_TextureName = NewTextureName;
	}
	return *this;
}

Material &Material::UpdatedTweens(void) {
	float Time = 0.0f;
	uint32_t ClrCnt = GetColorCount();
	uint32_t TexCnt = GetTextureCount();
	for (uint32_t i = 0; i < ClrCnt; i++) Time = std::max<float>(Time, m_ColorTweens[i].GetTotalTime());
	for (uint32_t i = 0; i < TexCnt; i++) Time = std::max<float>(Time, m_TextureTweens[i].GetTotalTime());
	m_TotalTime = Time;
	return *this;
}

uint32_t Material::GetTextureCount(void) const {
	return PipelineTextureCount(m_PipelineID);
}

uint32_t Material::GetColorCount(void) const {
	return PipelineColorCount(m_PipelineID);
}

const LWETween<LWVector4f> &Material::GetColorTween(uint32_t i) const{
	return m_ColorTweens[i];
}

const LWETween<LWVector4f> &Material::GetTextureTween(uint32_t i) const{
	return m_TextureTweens[i];
}

LWETween<LWVector4f> &Material::GetColorTween(uint32_t i) {
	return m_ColorTweens[i];
}

LWETween<LWVector4f> &Material::GetTextureTween(uint32_t i) {
	return m_TextureTweens[i];
}

MaterialTexture &Material::GetTexture(uint32_t i) {
	return m_TextureList[i];
}

float Material::GetTotalTime(void) const {
	return m_TotalTime;
}

uint32_t Material::GetPipelineID(void) const {
  	return m_PipelineID;
}

uint32_t Material::GetNameHash(void) const {
	return m_NameHash;
}

uint32_t Material::GetFlag(void) const {
	return m_Flag;
}

float Material::GetTime(void) const {
	return m_Time;
}

float Material::GetTransparency(void) const {
	return m_Transparency;
}

bool Material::isLooping(void) const {
	return (m_Flag&Loop) != 0;
}

bool Material::isTransparent(void) const {
	return (m_Flag&Transparent) || m_Transparency < 1.0f;
}

bool Material::isPlaying(void) const {
	return (m_Flag&Paused) == 0;
}

Material::Material(uint32_t PipelineID, uint32_t NameHash) : m_PipelineID(PipelineID), m_NameHash(NameHash) {}
