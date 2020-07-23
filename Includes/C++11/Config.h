#ifndef CONFIG_H
#define CONFIG_H
#include <LWCore/LWTypes.h>
#include <LWCore/LWVector.h>

const uint32_t MaxTextures = 6;

struct GStaticVertice {
	LWVector4f m_Position = LWVector4f(0.0f, 0.0f, 0.0f, 1.0f);
	LWVector4f m_TexCoord;
	LWVector4f m_Tangent;
	LWVector4f m_Normal;
};

struct GSkeletonVertice {
	LWVector4f m_Position = LWVector4f(0.0f, 0.0f, 0.0f, 1.0f);
	LWVector4f m_TexCoord;
	LWVector4f m_Tangent;
	LWVector4f m_Normal;
	LWVector4f m_BoneWeights;
	LWVector4i m_BoneIndices;
};

#endif