#include "Camera.h"
#include "Renderer.h"
#include <LWCore/LWMath.h>
#include <LWCore/LWSVector.h>
#include <LWCore/LWSMatrix.h>
#include <LWESGeometry3D.h>
#include <iostream>
#include <cstdarg>

LWSMatrix4f CameraPoint::MakeMatrix(void) const {
	float iRadi = 1.0f / m_Radius;
	return LWSMatrix4f(iRadi, iRadi, iRadi, 1.0f);
}

CameraPoint &CameraPoint::BuildFrustrum(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result) {
	LWSVector4f R = LWSVector4f(m_Radius);
	Result[0] = Fwrd.AAAB(R);
	Result[1] = (-Fwrd).AAAB(R);
	Result[2] = Right.AAAB(R);
	Result[3] = (-Right).AAAB(R);
	Result[4] = (-Up).AAAB(R);
	Result[5] = Up.AAAB(R);
	return *this;
}

CameraPoint &CameraPoint::BuildFrustrumPoints(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result) {
	LWSVector4f R = LWSVector4f(m_Radius);
	LWSVector4f NC = -Fwrd * R;
	LWSVector4f FC = Fwrd * R;

	Result[0] = NC - Right * R - Up * R;//LWVector4f(NC - Right * m_Radius - Up * m_Radius, -m_Radius);
	Result[1] = NC + Right * R - Up * R;// LWVector4f(NC + Right * m_Radius - Up * m_Radius, m_Radius);
	Result[2] = NC + Right * m_Radius + Up * R;
	Result[3] = FC - Right * m_Radius - Up * R;
	Result[4] = FC + Right * m_Radius - Up * R;
	Result[5] = FC + Right * m_Radius + Up * R;
	Result[0] = Result[0].AAAB(-R);
	Result[1] = Result[1].AAAB(R);
	return *this;
}

CameraPoint::CameraPoint(float Radius) : m_Radius(Radius) {}


LWSMatrix4f CameraPerspective::MakeMatrix(void) const {
	return LWSMatrix4f::Perspective(m_FOV, m_Aspect, m_Near, m_Far);
}

CameraPerspective &CameraPerspective::BuildFrustrum(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result) {
	float t = tanf(m_FOV*0.5f);
	float nh = m_Near * t;
	float nw = nh * m_Aspect;

	LWSVector4f NC = Fwrd * m_Near;

	LWSVector4f NT = (NC - (Up*nh)).Normalize();
	LWSVector4f NB = (NC + (Up*nh)).Normalize();
	LWSVector4f NR = (NC - (Right*nw)).Normalize();
	LWSVector4f NL = (NC + (Right*nw)).Normalize();

	Result[0] = Fwrd;// LWVector4f(Fwrd, m_Near);
	Result[1] = -Fwrd;// LWVector4f(-Fwrd, m_Far);
	Result[2] = NR.Cross3(Up);// LWVector4f(NR.Cross(Up), 0.0f);
	Result[3] = (-NL).Cross3(Up);// LWVector4f(-NL.Cross(Up), 0.0f);
	Result[4] = (-NT).Cross3(Right); // LWVector4f(-NT.Cross(Right), 0.0f);
	Result[5] = NB.Cross3(Right);// LWVector4f(NB.Cross(Right), 0.0f);
	Result[0] = Result[0].AAAB(LWSVector4f(m_Near));
	Result[1] = Result[1].AAAB(LWSVector4f(m_Far));
	return *this;
}

CameraPerspective &CameraPerspective::BuildFrustrumPoints(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result) {
	float t = tanf(m_FOV*0.5f);
	float nh = m_Near * t;
	float nw = nh * m_Aspect;

	float fh = m_Far * t;
	float fw = fh * m_Aspect;

	LWSVector4f NC = Fwrd * m_Near;
	LWSVector4f FC = Fwrd * m_Far;

	Result[0] = NC - Right * nw + Up * nh;//LWVector4f(NC - Right * nw + Up * nh, m_Near);
	Result[1] = NC + Right * nw + Up * nh;// LWVector4f(NC + Right * nw + Up * nh, m_Far);
	Result[2] = NC - Right * nw - Up * nh;// , 0.0f);

	Result[3] = FC - Right * fw + Up * fh;// , 0.0f);
	Result[4] = FC + Right * fw + Up * fh;// , 0.0f);
	Result[5] = FC - Right * fw - Up * fh;// , 0.0f);
	Result[0] = Result[0].AAAB(LWSVector4f(m_Near));
	Result[1] = Result[1].AAAB(LWSVector4f(m_Far));
	return *this;
}

CameraPerspective::CameraPerspective(float FOV, float Aspect, float Near, float Far) : m_FOV(FOV), m_Aspect(Aspect), m_Near(Near), m_Far(Far) {}



LWSMatrix4f CameraOrtho::MakeMatrix(void) const {
	return LWSMatrix4f::Ortho(m_Left, m_Right, m_Bottom, m_Top, m_Near, m_Far);
}

CameraOrtho &CameraOrtho::BuildFrustrum(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result) {
	Result[0] = Fwrd.AAAB(LWSVector4f(m_Near));// LWVector4f(Fwrd, m_Near);
	Result[1] = (-Fwrd).AAAB(LWSVector4f(m_Far));// (-Fwrd, m_Far);
	Result[2] = Right.AAAB(LWSVector4f(-m_Left));// LWVector4f(Right, -m_Left);
	Result[3] = (-Right).AAAB(LWSVector4f(m_Right));// LWVector4f(-Right, m_Right);
	Result[4] = (-Up).AAAB(LWSVector4f(m_Top));// LWVector4f(-Up, m_Top);
	Result[5] = Up.AAAB(LWSVector4f(-m_Bottom)); // LWVector4f(Up, -m_Bottom);
	//for (uint32_t i = 0; i < 6; i++) std::cout << i << ": " << Result[i] << std::endl;
	return *this;
}

CameraOrtho &CameraOrtho::BuildFrustrumPoints(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result) {
	LWSVector4f NC = Fwrd * m_Near;
	LWSVector4f FC = Fwrd * m_Far;
	Result[0] = NC + Right * m_Left + Up * m_Bottom;// , m_Near);
	Result[1] = NC + Right * m_Right + Up * m_Bottom;// , m_Far);
	Result[2] = NC + Right * m_Right + Up * m_Top;// , 0.0f);

	Result[3] = FC + Right * m_Left + Up * m_Bottom;// , 0.0f);
	Result[4] = FC + Right * m_Right + Up * m_Bottom;// , 0.0f);
	Result[5] = FC + Right * m_Right + Up * m_Top;// , 0.0f);
	Result[0] = Result[0].AAAB(LWSVector4f(m_Near));
	Result[1] = Result[1].AAAB(LWSVector4f(m_Far));
	return *this;
}

CameraOrtho::CameraOrtho(float Left, float Right, float Near, float Far, float Top, float Bottom) : m_Left(Left), m_Right(Right), m_Near(Near), m_Far(Far), m_Top(Top), m_Bottom(Bottom) {}


uint32_t Camera::GetPassesForSphereInCameras(Camera *Cameras, uint32_t CameraCnt, const LWSVector4f &Position, float Radius) {
	uint32_t PassBits = 0;
	for (uint32_t i = 0; i < CameraCnt; i++) {
		if (Cameras[i].SphereInFrustrum(Position, Radius)) PassBits |= Cameras[i].GetPassBit();
	}
	return PassBits;
}

uint32_t Camera::GetPassesForSphereInCamerasf(uint32_t CameraCnt, const LWSVector4f &Position, float Radius, ...) {
	Camera CamList[64];
	va_list lst;
	va_start(lst, Radius);
	for (uint32_t i = 0; i < CameraCnt; i++) CamList[i] = *va_arg(lst, Camera*);
	va_end(lst);
	return GetPassesForSphereInCameras(CamList, CameraCnt, Position, Radius);
}

uint32_t Camera::GetPassesForConeInCameras(Camera *Cameras, uint32_t CameraCnt, const LWSVector4f &Position, const LWSVector4f &Direction, float Length, float Theta) {
	auto ConeInPlane = [](const LWSVector4f &Plane, const LWSVector4f &Pos, const LWSVector4f &Dir, float Len, float Radius) {
		LWSVector4f M = Plane.Cross3(Dir).Cross3(Dir).Normalize3();
		LWSVector4f Q = Pos + Dir * Len - M * Radius;
		float md = Pos.Dot(Plane);
		float mq = Q.Dot(Plane);
		return mq >= 0.0f || md >= 0.0f;
	};
	uint32_t PassBits = 0;
	float Radi = tanf(Theta)*Length;
	for (uint32_t i = 0; i < CameraCnt; i++) {
		const LWSVector4f *CF = Cameras[i].GetViewFrustrum();
		LWSVector4f P = Position - Cameras[i].GetPosition();
		bool Inside = true;
		for (uint32_t n = 0; n < 6 && Inside; n++) Inside = ConeInPlane(CF[n], P, Direction, Length, Radi);
		if(!Inside) continue;
		PassBits |= Cameras[i].GetPassBit();
	}
	return PassBits;
}

uint32_t Camera::GetPassesForConeInCamerasf(uint32_t CameraCnt, const LWSVector4f &Position, const LWSVector4f &Direction, float Length, float Theta, ...) {
	Camera CamList[64];
	va_list lst;
	va_start(lst, Theta);
	for (uint32_t i = 0; i < CameraCnt; i++) CamList[i] = *va_arg(lst, Camera*);
	va_end(lst);
	return GetPassesForConeInCameras(CamList, CameraCnt, Position, Direction, Length, Theta);
}

uint32_t Camera::MakeCascadeCameraViews(const LWSVector4f &LightDir, const LWSVector4f &ViewPosition, const LWSVector4f *ViewFrustumPoints, const LWSMatrix4f &ProjViewMatrix, Camera *CamBuffer, uint32_t CascadeCnt, const LWSVector4f &SceneAABBMin, const LWSVector4f &SceneAABBMax) {
	LWSVector4f U = LWSVector4f(0.0f, 1.0f, 0.0f, 0.0f);
	if (fabs(U.Dot3(LightDir)) >= 1.0f - std::numeric_limits<float>::epsilon()) U = LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f);
	LWSVector4f R = LightDir.Cross3(U).Normalize3();
	U = R.Cross3(LightDir);
	LWSVector4f NTL = ViewFrustumPoints[0];
	LWSVector4f NTR = ViewFrustumPoints[1];
	LWSVector4f NBL = ViewFrustumPoints[2];

	LWSVector4f FTL = ViewFrustumPoints[3];
	LWSVector4f FTR = ViewFrustumPoints[4];
	LWSVector4f FBL = ViewFrustumPoints[5];

	LWSVector4f NX = NTR - NTL;
	LWSVector4f NY = NBL - NTL;

	LWSVector4f FX = FTR - FTL;
	LWSVector4f FY = FBL - FTL;

	LWSVector4f NBR = NTL + NX + NY;
	LWSVector4f FBR = FTL + FX + FY;

	LWSVector4f TL = FTL - NTL;
	LWSVector4f TR = FTR - NTR;
	LWSVector4f BL = FBL - NBL;
	LWSVector4f BR = FBR - NBR;

	//Max of 4 cascades.
	CascadeCnt = std::min<uint32_t>(CascadeCnt, 4);
	float Far = ViewFrustumPoints[1].w();
	float MinDistance = std::min<float>(200.0f * 3.0f, Far * 0.6f);
	//Manually adjusted cascaded distances, depending on CasecadeCnt, and minimum distance
	float SDistances[5] = { 0.0f, (MinDistance * 0.33f) / Far, MinDistance / Far, 0.6f, 1.0f };
	SDistances[CascadeCnt] = 1.0f;
	
	LWSVector4f AABBPnts[8] = { SceneAABBMin,
								SceneAABBMin.BAAA(SceneAABBMax),
								SceneAABBMin.AABA(SceneAABBMax),
								SceneAABBMin.BABA(SceneAABBMax),
								SceneAABBMin.ABAA(SceneAABBMax),
								SceneAABBMin.BBAA(SceneAABBMax),
								SceneAABBMin.ABBA(SceneAABBMax),
								SceneAABBMax };
	for (uint32_t i = 0; i < 8; i++) AABBPnts[i] = AABBPnts[i] * ProjViewMatrix;
	for (uint32_t i = 0; i < CascadeCnt; i++) {
		float iL = SDistances[i];
		float nL = SDistances[i + 1];
		LWSVector4f P[8];
		P[0] = NTL + iL * TL;
		P[1] = NTR + iL * TR;
		P[2] = NBL + iL * BL;
		P[3] = NBR + iL * BR;

		P[4] = NTL + nL * TL;
		P[5] = NTR + nL * TR;
		P[6] = NBL + nL * BL;
		P[7] = NBR + nL * BR;

		LWSVector4f Min = LWSVector4f();
		LWSVector4f Max = LWSVector4f();
		for (uint32_t n = 1; n < 8; n++) {
			LWSVector4f C = P[n] - P[0];
			LWSVector4f Pnt = LWSVector4f(R.Dot3(C), U.Dot3(C), LightDir.Dot3(C), 1.0f);
			Min = Min.Min(Pnt);
			Max = Max.Max(Pnt);
		}
		for (uint32_t i = 0; i < 8; i++) {
			LWSVector4f z = LWSVector4f(LightDir.Dot3(AABBPnts[i]));
			Min = Min.Min(Min.AABA(z));
		}
		LWVector4f vMin = Min.AsVec4();
		LWVector4f vMax = Max.AsVec4();
		P[0] += ViewPosition + LightDir * vMin.z;
		CamBuffer[i] = Camera(P[0].AAAB(LWSVector4f(1.0f)), LightDir, U, vMin.x, vMax.x, vMin.y, vMax.y, 0.0f, (vMax.z - vMin.z), ShadowCaster);
		CamBuffer[i].BuildFrustrum();
	}
	return CascadeCnt;
}


LWVector2f Camera::MakeSphereDirection(const LWSVector4f &Direction) {
	LWVector4f D = Direction.AsVec4();
	return LWVector2f(atan2f(D.z, D.x), asinf(D.y));
}

LWSVector4f Camera::MakeDirection(const LWVector2f &SphereDir) {
	float c = cosf(SphereDir.y);
	return LWSVector4f(cosf(SphereDir.x)*c, sinf(SphereDir.y), sinf(SphereDir.x)*c, 0.0f);
}

Camera &Camera::SetCameraControlled(bool Control) {
	m_CameraControlled = Control;
	m_ControlToggled = false;
	return *this;
}

Camera &Camera::SetPosition(const LWSVector4f &Position) {
	m_Position = Position;
	return *this;
}

Camera &Camera::SetDirection(const LWSVector4f &Direction) {
	m_Direction = Direction;
	return *this;
}

Camera &Camera::SetUp(const LWSVector4f &Up) {
	m_Up = Up;
	return *this;
}

Camera &Camera::SetAspect(float Aspect) {
	if (IsPointCamera()) return *this;
	if (IsOrthoCamera()) return *this;
	m_Perspective.m_Aspect = Aspect;
	return *this;
}

Camera &Camera::ProcessDirectionInputThird(const LWSVector4f &Center, float Radius, const LWVector2f &MouseDis, float HorizontalSens, float VerticalSens, const LWVector4f &MinMaxXY, bool Controlling){
	if (!Controlling || !m_CameraControlled) {
		m_PrevCameraControlled = false;
		return *this;
	}
	if (m_PrevCameraControlled) {
		LWVector2f CamDir = GetSphericalDirection();
		CamDir.x += MouseDis.x*HorizontalSens;
		CamDir.y += MouseDis.y*VerticalSens;
		if (CamDir.x > LW_PI) CamDir.x -= LW_2PI;
		if (CamDir.x < -LW_PI) CamDir.x += LW_2PI;
		CamDir.x = std::min<float>(std::max<float>(CamDir.x, MinMaxXY.x), MinMaxXY.y);
		CamDir.y = std::min<float>(std::max<float>(CamDir.y, MinMaxXY.z), MinMaxXY.w);
		SetSphericalDirection(CamDir);
		m_Position = Center - GetDirection() * Radius;
	}
	m_PrevCameraControlled = true;
	return *this;
}

Camera &Camera::ProcessDirectionInputFirst(const LWVector2f &MouseDis, float HorizontalSens, float VerticalSens, const LWVector4f &MinMaxXY, bool Controlling) {
	if (!Controlling || !m_CameraControlled) {
		m_PrevCameraControlled = false;
		return *this;
	}
	if (m_PrevCameraControlled) {
		LWVector2f CamDir = GetSphericalDirection();
		CamDir.x += MouseDis.x*HorizontalSens;
		CamDir.y += MouseDis.y*VerticalSens;
		if (CamDir.x > LW_PI) CamDir.x -= LW_2PI;
		if (CamDir.x < -LW_PI) CamDir.x += LW_2PI;
		CamDir.x = std::min<float>(std::max<float>(CamDir.x, MinMaxXY.x), MinMaxXY.y);
		CamDir.y = std::min<float>(std::max<float>(CamDir.y, MinMaxXY.z), MinMaxXY.w);
		SetSphericalDirection(CamDir);
	}
	m_PrevCameraControlled = true;
	return *this;
}

Camera &Camera::SetSphericalDirection(const LWVector2f &SphereCoordinates) {
	m_Direction = MakeDirection(SphereCoordinates);
	return *this;
}

Camera &Camera::ToggleCameraControl(void) {
	m_ControlToggled = true;
	return *this;
}

LWSVector4f Camera::UnProject(const LWVector2f &ScreenPnt, float Depth, const LWVector2f &WndSize) const {
	LWSVector4f Pnt = LWSVector4f(ScreenPnt / WndSize * 2.0f - 1.0f, Depth*2.0f - 1.0f, 1.0f);
	Pnt = Pnt * (GetViewMatrix() * GetProjMatrix()).Inverse();
	float w = Pnt.w();
	if (fabs(w) <= std::numeric_limits<float>::epsilon()) return m_Position;
	w = 1.0f / w;
	return LWSVector4f(Pnt * w);
}

LWSVector4f Camera::Project(const LWSVector4f &Pnt, const LWVector2f &WndSize) const {
	LWSVector4f P = Pnt * GetViewMatrix()*GetProjMatrix();
	float w = P.w();
	if (fabs(w) <= std::numeric_limits<float>::epsilon()) return LWSVector4f(-1.0f);
	w = 1.0f / w;
	P *= w;
	return (P * LWSVector4f(0.5f, 0.5f, 1.0f, 1.0f) + LWSVector4f(0.5f, 0.5f, 1.0f, 0.0f)) * LWSVector4f(WndSize.x, WndSize.y, 0.5f, 1.0f);
	//return LWVector3f((P.x*0.5f + 0.5f)*WndSize.x, (P.y*0.5f + 0.5f)*WndSize.y, (1.0f+P.z)*0.5f);
}

bool Camera::UnProjectAgainstPlane(const LWVector2f &ScreenPnt, const LWVector2f &WndSize, const LWSVector4f &Plane, LWSVector4f &Pnt) const {
	LWSMatrix4f Matrix = (GetViewMatrix()*GetProjMatrix()).Inverse();
	LWSVector4f NearPnt = LWSVector4f(ScreenPnt / WndSize * 2.0f - 1.0f, -1.0f, 1.0f);
	LWSVector4f FarPnt = NearPnt.AABB(LWVector4f(1.0f));

	LWSVector4f Near = NearPnt * Matrix;
	LWSVector4f Far = FarPnt * Matrix;
	float nw = Near.w();
	float fw = Far.w();
	if (fabs(nw) < std::numeric_limits<float>::epsilon()) Near = m_Position;// LWVector4f(m_Position, 1.0f);
	else Near = Near * (1.0f / nw);
	if (fabs(fw) < std::numeric_limits<float>::epsilon()) Far = m_Position;
	else Far = Far * (1.0f / fw);

	LWSVector4f Dir = Far - Near;
	float Dis = 1.0f;
	if (!LWERayPlaneIntersect(Near, Dir, Plane, &Dis)) return false;
	Pnt = Near + Dir * Dis;
	return true;
}


Camera &Camera::SetOrtho(bool isOrtho) {
	m_Flag = (m_Flag&~OrthoSource) | (isOrtho ? OrthoSource : 0);
	return *this;
}

Camera &Camera::SetPointSource(bool isPointSource) {
	m_Flag = (m_Flag&~PointSource) | (isPointSource ? PointSource : 0);
	return *this;
}

Camera &Camera::SetShadowCaster(bool isShadowCaster) {
	m_Flag = (m_Flag&~ShadowCaster) | (isShadowCaster ? ShadowCaster : 0);
	return *this;
}

Camera &Camera::SetPassID(uint32_t PassID) {
	m_PassID = PassID;
	return *this;
}

Camera &Camera::BuildFrustrum(void) {
	LWSVector4f U = m_Up;
	if (fabs(m_Direction.Dot3(U)) >= 1.0f - std::numeric_limits<float>::epsilon()) U = LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f);
	LWSVector4f R = m_Direction.Cross3(U).Normalize3();
	U = R.Cross3(m_Direction);

	if (IsPointCamera()) m_Point.BuildFrustrum(m_Direction, U, R, m_ViewFrustrum);
	else if (IsOrthoCamera()) m_Ortho.BuildFrustrum(m_Direction, U, R, m_ViewFrustrum);
	else m_Perspective.BuildFrustrum(m_Direction, U, R, m_ViewFrustrum);
	return *this;
}

Camera &Camera::BuildFrustrumPoints(LWSVector4f *Result) {
	LWSVector4f U = m_Up;
	if (fabs(m_Direction.Dot3(U)) >= 1.0f - std::numeric_limits<float>::epsilon()) U = LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f);
	LWSVector4f R = m_Direction.Cross3(U).Normalize3();
	U = R.Cross3(m_Direction);

	if (IsPointCamera()) m_Point.BuildFrustrumPoints(m_Direction, U, R, Result);
	else if (IsOrthoCamera()) m_Ortho.BuildFrustrumPoints(m_Direction, U, R, Result);
	else m_Perspective.BuildFrustrumPoints(m_Direction, U, R, Result);
	return *this;
}

LWSMatrix4f Camera::GetViewMatrix(void) const {
	LWSVector4f U = m_Up;
	if (fabs(m_Direction.Dot3(U)) >= 1.0f - std::numeric_limits<float>::epsilon()) U = LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f);

	return LWSMatrix4f::LookAt(m_Position, m_Position + m_Direction, U).Inverse();
}

LWSMatrix4f Camera::GetDirectionMatrix(void) const {
	LWSVector4f U = m_Up;
	if (fabs(m_Direction.Dot3(U)) >= 1.0f - std::numeric_limits<float>::epsilon()) U = LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f);

	return LWSMatrix4f::LookAt(m_Position, m_Position + m_Direction, U);
}

LWSMatrix4f Camera::GetProjMatrix(void) const {
	if (IsPointCamera()) return m_Point.MakeMatrix();
	else if (IsOrthoCamera()) return m_Ortho.MakeMatrix();

	return m_Perspective.MakeMatrix();
}

LWSMatrix4f Camera::GetProjViewMatrix(void) const {
	return GetViewMatrix()*GetProjMatrix();
}

LWSVector4f Camera::GetPosition(void) const {
	return m_Position;
}

LWSVector4f Camera::GetDirection(void) const {
	return m_Direction;
}

LWSVector4f Camera::GetFlatDirection(void) const {
	LWSVector4f Z = LWSVector4f();
	LWSVector4f Dir = m_Direction.ABAA(Z);
	return Dir.Normalize3();
}

LWSVector4f Camera::GetUp(void) const {
	return m_Up;
}

CameraOrtho &Camera::GetOrthoPropertys(void) {
	return m_Ortho;
}

CameraPerspective &Camera::GetPerspectivePropertys(void) {
	return m_Perspective;
}

CameraPoint &Camera::GetPointPropertys(void) {
	return m_Point;
}

Camera &Camera::MakeViewDirections(LWSVector4f &Forward, LWSVector4f &Right, LWSVector4f &Up) {
	LWSVector4f U = m_Up;
	float D = fabs(U.Dot3(m_Direction));
	if (D >= 1.0f - std::numeric_limits<float>::epsilon()) U = LWSVector4f(0.0f, 0.0f, 1.0f, 0.0f);
	Forward = m_Direction;
	Right = m_Direction.Cross3(U).Normalize3();
	Up = Right.Cross3(Forward);
	return *this;
}

LWVector2f Camera::GetSphericalDirection(void) const {
	return MakeSphereDirection(m_Direction);
}

const LWSVector4f *Camera::GetViewFrustrum(void) const {
	return m_ViewFrustrum;
}

bool Camera::SphereInFrustrum(const LWSVector4f &Position, float Radius) {
	return LWESphereInFrustum(Position, Radius, m_Position, m_ViewFrustrum);
}

bool Camera::AABBInFrustrum(const LWSVector4f &AAMin, const LWSVector4f &AAMax) {
	return LWEAABBInFrustum(AAMin, AAMax, m_Position, m_ViewFrustrum);
}

bool Camera::ConeInFrustrum(const LWSVector4f &Position, const LWSVector4f &Direction, float Length, float Theta) {
	return LWEConeInFrustum(Position, Direction, Theta, Length, m_Position, m_ViewFrustrum);
}

bool Camera::isCameraControlled(void) const {
	return m_CameraControlled;
}

bool Camera::isControlToggled(void) const {
	return m_ControlToggled;
}

bool Camera::IsOrthoCamera(void) const {
	return (m_Flag&OrthoSource)!=0;
}

bool Camera::IsPointCamera(void) const {
	return (m_Flag&PointSource)!=0;
}

bool Camera::IsShadowCaster(void) const {
	return (m_Flag&ShadowCaster) != 0;
}

bool Camera::IsReflection(void) const {
	return (m_Flag & Reflection) != 0;
}

uint32_t Camera::GetPassID(void) const {
	return m_PassID;
}

uint32_t Camera::GetPassBit(void) const {
	return 1<<m_PassID;
}

Camera::Camera(const LWSVector4f &Position, const LWSVector4f &ViewDirection, const LWSVector4f &Up, float Aspect, float Fov, float Near, float Far, uint32_t Flag) : m_Position(Position), m_Direction(ViewDirection), m_Up(Up), m_Flag(Flag) {
	m_Perspective = CameraPerspective(Fov, Aspect, Near, Far);
	BuildFrustrum();
}

Camera::Camera(const LWSVector4f &Position, const LWSVector4f &ViewDirection, const LWSVector4f &Up, float Left, float Right, float Bottom, float Top, float Near, float Far, uint32_t Flag) : m_Position(Position), m_Direction(ViewDirection), m_Up(Up), m_Flag(Flag | OrthoSource) {
	m_Ortho = CameraOrtho(Left, Right, Near, Far, Top, Bottom);
	BuildFrustrum();
}

Camera::Camera(const LWSVector4f &Position, float Radius, uint32_t Flag) : m_Position(Position), m_Flag(Flag|PointSource) {
	m_Point = CameraPoint(Radius);
	BuildFrustrum();
}

Camera::Camera() {
	m_Perspective = CameraPerspective(LW_PI_4, 1.0f, 0.1f, 10000.0f);
}
