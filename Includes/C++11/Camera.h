#ifndef CAMERA_H
#define CAMERA_H
#include <LWCore/LWTypes.h>
#include <LWCore/LWSVector.h>
#include <LWCore/LWSMatrix.h>

struct CameraPoint {
	float m_Radius;
	float m_Padding[5];

	LWSMatrix4f MakeMatrix(void) const;

	CameraPoint &BuildFrustrum(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result);

	CameraPoint &BuildFrustrumPoints(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result);

	CameraPoint(float Radius);

	CameraPoint() = default;
};

struct CameraPerspective {
	float m_FOV;
	float m_Aspect;
	float m_Near;
	float m_Far;
	float m_Padding[2];

	LWSMatrix4f MakeMatrix(void) const;

	CameraPerspective &BuildFrustrum(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result);

	CameraPerspective &BuildFrustrumPoints(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result);

	CameraPerspective(float FOV, float Aspect, float Near, float Far);

	CameraPerspective() = default;
};

struct CameraOrtho {
	float m_Left;
	float m_Right;
	float m_Near;
	float m_Far;
	float m_Top;
	float m_Bottom;

	LWSMatrix4f MakeMatrix(void) const;

	CameraOrtho &BuildFrustrum(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result);

	CameraOrtho &BuildFrustrumPoints(const LWSVector4f &Fwrd, const LWSVector4f &Up, const LWSVector4f &Right, LWSVector4f *Result);

	CameraOrtho(float Left, float Right, float Near, float Far, float Top, float Bottom);

	CameraOrtho() = default;
};

class Camera {
public:
	enum {
		PointSource = 0x1,
		OrthoSource = 0x2,
		ShadowCaster=0x4,
		Reflection=0x8
	};

	static LWVector2f MakeSphereDirection(const LWSVector4f &Direction);

	static LWSVector4f MakeDirection(const LWVector2f &SphereDir);

	static uint32_t GetPassesForSphereInCameras(Camera *Cameras, uint32_t CameraCnt, const LWSVector4f &Position, float Radius);

	static uint32_t GetPassesForSphereInCamerasf(uint32_t CameraCnt, const LWSVector4f &Position, float Radius, ...);

	static uint32_t GetPassesForConeInCameras(Camera *Cameras, uint32_t CameraCnt, const LWSVector4f &Position, const LWSVector4f &Direction, float Length, float Theta);

	static uint32_t GetPassesForConeInCamerasf(uint32_t CameraCnt, const LWSVector4f &Position, const LWSVector4f &Direction, float Length, float Theta, ...);

	static uint32_t MakeCascadeCameraViews(const LWSVector4f &LightDir, const LWSVector4f &ViewPosition, const LWSVector4f *ViewFrustumPoints, const LWSMatrix4f &ProjViewMatrix, Camera *CamBuffer, uint32_t CascadeCnt, const LWSVector4f &SceneAABBMin, const LWSVector4f &SceneAABBMax);

	Camera &SetPosition(const LWSVector4f &Position);

	Camera &SetDirection(const LWSVector4f &Direction);

	//Min/Max = -180-180, x/y = Horizontal Min and Max.  z/w = Vertical Min and Max.
	Camera &ProcessDirectionInputFirst(const LWVector2f &MouseDis, float HorizontalSens, float VerticalSens, const LWVector4f &MinMaxXY, bool Controlling);

	//Min/Max = -180-180, x/y = Horizontal Min and Max. z/w = Vertical Min and Max.
	Camera &ProcessDirectionInputThird(const LWSVector4f &Center, float Radius, const LWVector2f &MouseDis, float HorizontalSens, float VericalSens, const LWVector4f &MinMaxXY, bool Controlling);

	Camera &SetUp(const LWSVector4f &Up);

	Camera &SetAspect(float Aspect);

	Camera &BuildFrustrum(void);

	Camera &BuildFrustrumPoints(LWSVector4f *Result); //Builds 6 frustrum points, where 0 = Near top left, 1 = top right, 2 = bottom left.  3 = Far top left, 4 = top right, 5 = bottom left.  used in light culling system to build sub frustrums.

	Camera &SetCameraControlled(bool Control);

	Camera &SetSphericalDirection(const LWVector2f &SphereCoordinates);

	Camera &ToggleCameraControl(void);

	LWSVector4f UnProject(const LWVector2f &ScreenPnt, float Depth, const LWVector2f &WndSize) const;

	LWSVector4f Project(const LWSVector4f &Pnt, const LWVector2f &WndSize) const;

	bool UnProjectAgainstPlane(const LWVector2f &ScreenPnt, const LWVector2f &WndSize, const LWSVector4f &Plane, LWSVector4f &Pnt) const;

	Camera &SetOrtho(bool isOrtho);

	Camera &SetPointSource(bool isPointLightSource);

	Camera &SetShadowCaster(bool isShadowCaster);

	Camera &SetPassID(uint32_t PassID);

	CameraOrtho &GetOrthoPropertys(void);

	CameraPerspective &GetPerspectivePropertys(void);

	CameraPoint &GetPointPropertys(void);

	Camera &MakeViewDirections(LWSVector4f &Forward, LWSVector4f &Right, LWSVector4f &Up);

	//Use this matrix for transforming objects around the camera(this inverses the GetDirectionMatrix()).
	LWSMatrix4f GetViewMatrix() const;

	//Use this matrix if extracting the camera's fwrd/up/right for particles.
	LWSMatrix4f GetDirectionMatrix() const;

	LWSMatrix4f GetProjMatrix(void) const;

	LWSMatrix4f GetProjViewMatrix(void) const;

	LWSVector4f GetPosition(void) const;

	LWSVector4f GetDirection(void) const;

	LWSVector4f GetFlatDirection(void) const;

	LWSVector4f GetUp(void) const;

	LWVector2f GetSphericalDirection(void) const;

	const LWSVector4f *GetViewFrustrum(void) const;

	bool isCameraControlled(void) const;

	bool isControlToggled(void) const;

	bool SphereInFrustrum(const LWSVector4f &Position, float Radius);

	bool ConeInFrustrum(const LWSVector4f &Position, const LWSVector4f &Direction, float Length, float Theta);

	bool AABBInFrustrum(const LWSVector4f &AAMin, const LWSVector4f &AAMax);

	bool IsOrthoCamera(void) const;

	bool IsPointCamera(void) const;

	bool IsShadowCaster(void) const;

	bool IsReflection(void) const;

	uint32_t GetPassID(void) const;

	uint32_t GetPassBit(void) const;

	Camera(const LWSVector4f &Position, const LWSVector4f &ViewDirection, const LWSVector4f &Up, float Aspect, float Fov, float Near, float Far, uint32_t Flag);

	Camera(const LWSVector4f &Position, const LWSVector4f &ViewDirection, const LWSVector4f &Up, float Left, float Right, float Bottom, float Top, float Near, float Far, uint32_t Flag);

	Camera(const LWSVector4f &Position, float Radius, uint32_t Flag);

	Camera();
private:
	union {
		CameraPerspective m_Perspective;
		CameraOrtho m_Ortho;
		CameraPoint m_Point;
	};
	LWSVector4f m_ViewFrustrum[6];
	LWSVector4f m_Position;
	LWSVector4f m_Direction = LWSVector4f(0.0f, 0.0f, -1.0f, 0.0f);
	LWSVector4f m_Up = LWSVector4f(0.0f, 1.0f, 0.0f, 0.0f);
	bool m_CameraControlled = false;
	bool m_PrevCameraControlled = false;
	bool m_ControlToggled = false;
	uint32_t m_PassID = 0;
	uint32_t m_Flag = 0;

};

#endif