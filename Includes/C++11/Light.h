#ifndef LIGHT_H
#define LIGHT_H
#include <LWCore/LWTypes.h>
#include <LWCore/LWVector.h>
#include <LWCore/LWSVector.h>

struct Light {
	static const uint32_t AmbientLight = 0;
	static const uint32_t DirectionalLight = 1;
	static const uint32_t PointLight = 2;
	static const uint32_t SpotLight = 3;
	static const uint32_t ShadowCaster = 0x1;

	LWSVector4f m_Position;
	LWSVector4f m_Direction;
	LWVector4f m_Color;
	uint32_t m_Flag = 0;

	static uint32_t LightType(float w);

	Light &SetShadowCaster(bool iShadowCaster);

	Light &SetPointInnerRadius(float Radius);

	Light &SetPointFalloffRadius(float Radius);

	Light &SetSpotTheta(float Theta);

	Light &SetSpotLength(float Length);

	Light &SetIntensity(float Intensity);

	uint32_t GetLightType(void) const;

	float GetPointRadius(void) const;

	float GetPointInnerRadius(void) const;

	float GetPointFalloffRadius(void) const;

	float GetSpotTheta(void) const;

	float GetSpotLength(void) const;

	float GetIntensity(void) const;

	bool isShadowCaster(void) const;

	void GetAABounds(LWSVector4f &Min, LWSVector4f &Max) const;

	//Directional Light.
	Light(const LWSVector4f &Direction, const LWVector4f &Color, float Intensity, uint32_t Flag);

	//Ambient Light.
	Light(const LWVector4f &Color, float Intensity, uint32_t Flag);

	//Point Light.
	Light(const LWSVector4f &Position, float InnerRadius, float OuterRadius, const LWVector4f &Color, float Intensity, uint32_t Flag);

	//Spot Light.
	Light(const LWSVector4f &Position, const LWSVector4f &Direction, float Theta, float Length, const LWVector4f &Color, float Intensity, uint32_t Flag);

	Light() = default;
};

#endif