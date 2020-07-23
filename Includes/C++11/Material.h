#ifndef MATERIAL_H
#define MATERIAL_H
#include <LWETween.h>
#include "Renderer.h"
//#include "DataManager.h"
#include "Config.h"
#include <LWETypes.h>

struct LWEGLTFMaterial;

struct MaterialTexture {
	//Ref<TextureID> *m_Reference = nullptr;
	uint32_t m_TextureID = -1;
	uint32_t m_TextureName = 0;
	uint32_t m_TextureState = 0;

	static bool Deserialize(MaterialTexture &Tex, LWByteBuffer &Buf);

	uint32_t Serialize(LWByteBuffer &Buf);
};

class Material {
public:
	static const uint32_t MaxColorTweens = 4;

	static const uint32_t PBRMetallicRoughness = 0;
	static const uint32_t PBRSpecularGlossiness = 1;
	static const uint32_t PBRUnlit = 2;
	static const uint32_t Skybox = 3;
	static const uint32_t Cloud = 4;

	static const uint32_t EmissiveFactorClrTweenID = 0;
	static const uint32_t PBRAlbedoBaseFactorClrTweenID = 1;
	static const uint32_t PBRMetallicRoughnessClrTweenID = 2;

	static const uint32_t SGDiffuseFactorClrTweenID = 1;
	static const uint32_t SGSpecularFactorClrTweenID = 2;
	static const uint32_t ULColorTweenID = 1;

	static const uint32_t NormalTexID = 0;
	static const uint32_t OcclussionTexID = 1;
	static const uint32_t EmissiveTexID = 2;

	static const uint32_t PBRAlbedoTexID = 3;
	static const uint32_t PBRMetallicRoughnessTexID = 4;

	static const uint32_t SGDiffuseColorTexID = 3;
	static const uint32_t SGSpecularColorTexID = 4;

	static const uint32_t ULColorTexID = 3;

	static const uint32_t SBBackTexID = 0;
	static const uint32_t SBHorizonTexID = 1;
	static const uint32_t SBGlowTexID = 2;

	static const uint32_t CLSettingsClrAID = 0;
	static const uint32_t CLSettingsClrBID = 1;

	//x = CloudScale, y=speed, z = clouddark, w = cloudlight
	static const LWVector4f DefaultACloudSettings;
	//x = cloudalpha, y = cloudcover, z = N/A, w = N/A(replaced by mat time).
	static const LWVector4f DefaultBCloudSettings;

	static const uint32_t Loop = 0x1;
	static const uint32_t Transparent = 0x2;
	static const uint32_t Paused = 0x4;

	static bool Deserialize(Material &Mat, LWByteBuffer &Buf);

	static uint32_t PipelineTextureCount(uint32_t PipelineID);

	static uint32_t PipelineColorCount(uint32_t PipelineID);

	/*
	bool MakeInstance(Material &Dst, DataManager *DM, uint64_t lCurrentTime);

	bool InstanceTextures(DataManager *DM, uint64_t lCurrentTime);

	void Release(DataManager *DM, uint64_t lCurrentTime);
	*/

	uint32_t Serialize(LWByteBuffer &Buf);

	Material &Update(float dTime);

	Material &Pause(void);

	Material &Play(void);

	Material &SetTime(float Time, bool Loop);

	Material &SetFlag(uint32_t Flag);

	Material &SetTransparent(bool iTransparent);

	Material &SetNameHash(uint32_t NameHash);

	Material &RenamedTexture(uint32_t PrevTextureName, uint32_t NewTextureName);

	//Material &SetPipelineID(uint32_t PipelineID, DataManager *DM = nullptr, uint64_t lCurrentTime = 0);

	Material &MakeGLTFMaterial(LWEGLTFParser &P, LWEGLTFMaterial *Mat);

	Material &MakeFlatColor(uint32_t ColorID, const LWVector4f &Color);

	Material &MakeFlatTexture(uint32_t TextureID, const LWVector4f &SubTexture);

	Material &SetTransparency(float Transparency);

	Material &UpdatedTweens(void);

	const LWETween<LWVector4f> &GetColorTween(uint32_t i) const;

	const LWETween<LWVector4f> &GetTextureTween(uint32_t i) const;

	LWETween<LWVector4f> &GetColorTween(uint32_t i);

	LWETween<LWVector4f> &GetTextureTween(uint32_t i);

	MaterialTexture &GetTexture(uint32_t i);

	uint32_t GetTextureCount(void) const;

	uint32_t GetColorCount(void) const;

	float GetTotalTime(void) const;

	uint32_t GetPipelineID(void) const;

	float GetTime(void) const;

	uint32_t GetNameHash(void) const;

	float GetTransparency(void) const;

	uint32_t GetFlag(void) const;

	bool isLooping(void) const;

	bool isTransparent(void) const;

	bool isPlaying(void) const;

	Material(uint32_t PipelineID, uint32_t NameHash);

	Material() = default;

private:
	uint32_t m_NameHash = 0;
	uint32_t m_PipelineID = 0;
	uint32_t m_Flag = 0;
	float m_Transparency = 1.0f;
	float m_Time = 0.0f;
	float m_TotalTime = 0.0f;
	LWETween<LWVector4f> m_ColorTweens[MaxColorTweens];
	LWETween<LWVector4f> m_TextureTweens[MaxTextures]; //SubTexture dimensions, where x,y = offset, and z,w = size(in uv units).
	MaterialTexture m_TextureList[MaxTextures];
};

#endif