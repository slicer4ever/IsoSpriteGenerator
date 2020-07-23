#include "Light.h"


uint32_t Light::LightType(float w) {
	if (w < 0.0f) return AmbientLight;
	else if (w == 0.0f) return DirectionalLight;
	else if (w == 1.0f) return PointLight;
	return SpotLight;
}

Light &Light::SetShadowCaster(bool iShadowCaster) {
	m_Flag = (m_Flag & ~ShadowCaster) | (iShadowCaster ? ShadowCaster : 0);
	return *this;
}

uint32_t Light::GetLightType(void) const {
	return LightType(m_Position.w());
}

bool Light::isShadowCaster(void) const {
	return (m_Flag & ShadowCaster) != 0;
}

Light &Light::SetPointInnerRadius(float Radius) {
	m_Direction.sX(Radius);
	return *this;
}

Light &Light::SetPointFalloffRadius(float Radius) {
	m_Direction.sY(Radius);
	return *this;
}

Light &Light::SetSpotTheta(float Theta) {
	m_Position.sW(1.0f + Theta);
	return *this;
}

Light &Light::SetSpotLength(float Length) {
	m_Direction.sW(Length);
	return *this;
}

Light &Light::SetIntensity(float Intensity) {
	m_Color.w = Intensity;
	return *this;
}

void Light::GetAABounds(LWSVector4f &Min, LWSVector4f &Max) const {
	uint32_t lType = GetLightType();
	if (lType == AmbientLight || lType == DirectionalLight) {
		Min = Max = m_Position;
		return;
	} else if (lType == PointLight) {
		float InnerRadius = m_Direction.x();
		float OuterRadius = m_Direction.y();
		float Radi = InnerRadius + OuterRadius;
		Min = m_Position - LWSVector4f(Radi, Radi, Radi, 0.0f);
		Max = m_Position + LWSVector4f(Radi, Radi, Radi, 0.0f);
	} else if (lType == DirectionalLight) {
		float Radi = m_Direction.w();
		Min = m_Position - LWSVector4f(Radi, Radi, Radi, 0.0f);
		Max = m_Position + LWSVector4f(Radi, Radi, Radi, 0.0f);
	}
	return;
}

float Light::GetPointRadius(void) const {
	return m_Direction.x() + m_Direction.y();
}

float Light::GetPointInnerRadius(void) const {
	return m_Direction.x();
}

float Light::GetPointFalloffRadius(void) const {
	return m_Direction.y();
}

float Light::GetSpotTheta(void) const {
	return m_Position.w() - 1.0f;
}

float Light::GetSpotLength(void) const {
	return m_Direction.w();
}

float Light::GetIntensity(void) const {
	return m_Color.w;
}

Light::Light(const LWSVector4f &Direction, const LWVector4f &Color, float Intensity, uint32_t Flag) : m_Direction(Direction), m_Color(LWVector4f(Color.xyz(), Intensity)), m_Flag(Flag) {}

Light::Light(const LWVector4f &Color, float Intensity, uint32_t Flag) : m_Position(LWSVector4f(0.0f, 0.0f, 0.0f, -1.0f - Intensity)), m_Color(LWVector4f(Color.xyz(), 1.0f)), m_Flag(Flag) {}

Light::Light(const LWSVector4f &Position, float InnerRadius, float OuterRadius, const LWVector4f &Color, float Intensity, uint32_t Flag) : m_Position(Position.AAAB(LWSVector4f(1.0f))), m_Direction(LWSVector4f(InnerRadius, OuterRadius, 0.0f, 0.0f)), m_Color(LWVector4f(Color.xyz(), Intensity)), m_Flag(Flag) {}

Light::Light(const LWSVector4f &Position, const LWSVector4f &Direction, float Theta, float Length, const LWVector4f &Color, float Intensity, uint32_t Flag) : m_Position(Position.AAAB(LWSVector4f(1.0f + Theta))), m_Direction(Direction.AAAB(LWSVector4f(Length))), m_Color(LWVector4f(Color.xyz(), Intensity)), m_Flag(Flag) {}
