#include "UICameraControls.h"
#include <LWCore/LWMath.h>
#include <LWPlatform/LWWindow.h>
#include <LWEJson.h>
#include <algorithm>

//UIPerspectiveControls
void UIPerspectiveControls::Update(float dTime, LWEUIManager *UIMan, App *A) {
	LWEUI *Focused = UIMan->GetFocusedUI();
	CameraPerspective &P = m_UICamControls->m_Camera.GetPerspectivePropertys();
	if (Focused != m_FovTI) m_FovTI->Clear().InsertTextf("%.2f", false, false, 1.0f, P.m_FOV * LW_RADTODEG);
	return;
}

void UIPerspectiveControls::ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A) {
	return;
}


void UIPerspectiveControls::SerializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A) {
	CameraPerspective &P = m_UICamControls->m_Camera.GetPerspectivePropertys();
	JSon.MakeValueElement("Perspective_FoV", P.m_FOV*LW_RADTODEG, Parent);
	return;
}

void UIPerspectiveControls::DeserializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A) {
	CameraPerspective &P = m_UICamControls->m_Camera.GetPerspectivePropertys();
	LWEJObject *FoVObj = Parent->FindChild("Perspective_FoV", JSon);
	float Fov = P.m_FOV;
	if (FoVObj) Fov = FoVObj->AsFloat()*LW_DEGTORAD;
	P.m_FOV = std::min<float>(std::max<float>(Fov, 0.0f), LW_PI_2);
	return;
}

void UIPerspectiveControls::FovTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	float Fov = (float)atof(m_FovTI->GetLine(0)->m_Value)*LW_DEGTORAD;
	Fov = std::min<float>(std::max<float>(Fov, 0.0f), LW_PI_2);
	CameraPerspective &P = m_UICamControls->m_Camera.GetPerspectivePropertys();
	P.m_FOV = Fov;
	return;
}

UIPerspectiveControls::UIPerspectiveControls(const StackText &Name, LWEUIManager *UIMan, UICameraControls &CamControls) : UIItem(Name, UIMan), m_UICamControls(&CamControls) {
	m_FovTI = (LWEUITextInput *)UIMan->GetNamedUIf("%s.FovTI", Name());
	UIMan->RegisterMethodEvent(m_FovTI, LWEUI::Event_Changed, &UIPerspectiveControls::FovTIChanged, this, nullptr);
}

//UIOrthoControls
void UIOrthoControls::Update(float dTime, LWEUIManager *UIMan, App *A) {
	LWEUI *Focused = UIMan->GetFocusedUI();
	CameraOrtho &O = m_UICamControls->m_Camera.GetOrthoPropertys();
	if (Focused != m_WidthTI) m_WidthTI->Clear().InsertTextf("%.2f", false, false, 1.0f, O.m_Right);
	if (Focused != m_HeightTI) m_HeightTI->Clear().InsertTextf("%.2f", false, false, 1.0f, O.m_Top);
	return;
}

void UIOrthoControls::ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A) {
	return;
}

void UIOrthoControls::SerializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A) {
	CameraOrtho &O = m_UICamControls->m_Camera.GetOrthoPropertys();
	JSon.MakeValueElement("Ortho_Width", O.m_Right, Parent);
	JSon.MakeValueElement("Ortho_Height", O.m_Top, Parent);
	return;
}

void UIOrthoControls::DeserializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A) {
	const float MinWidth = 1.0f;
	const float MinHeight = 1.0f;
	CameraOrtho &O = m_UICamControls->m_Camera.GetOrthoPropertys();
	LWEJObject *JOrthoWidth = Parent->FindChild("Ortho_Width", JSon);
	LWEJObject *JOrthoHeight = Parent->FindChild("Ortho_Height", JSon);

	float Width = O.m_Right;
	float Height = O.m_Top;
	if (JOrthoWidth) Width = JOrthoWidth->AsFloat();
	if (JOrthoHeight) Height = JOrthoHeight->AsFloat();
	Width = std::max<float>(Width, MinWidth);
	Height = std::max<float>(Height, MinHeight);
	O.m_Left = -Width;
	O.m_Right = Width;
	O.m_Bottom = -Height;
	O.m_Top = Height;
	return;
}

void UIOrthoControls::WidthTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	const float MinWidth = 1.0f;
	float w = (float)atof(m_WidthTI->GetLine(0)->m_Value);
	w = std::max<float>(w, MinWidth);
	CameraOrtho &O = m_UICamControls->m_Camera.GetOrthoPropertys();
	O.m_Left = -w;
	O.m_Right = w;
	return;
}

void UIOrthoControls::HeightTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	const float MinHeight = 1.0f;
	float h = (float)atof(m_HeightTI->GetLine(0)->m_Value);
	h = std::max<float>(h, MinHeight);
	CameraOrtho &O = m_UICamControls->m_Camera.GetOrthoPropertys();
	O.m_Bottom = -h;
	O.m_Top = h;
	return;
}

UIOrthoControls::UIOrthoControls(const StackText &Name, LWEUIManager *UIMan, UICameraControls &CamControls) : UIItem(Name, UIMan), m_UICamControls(&CamControls) {
	m_WidthTI = (LWEUITextInput *)UIMan->GetNamedUIf("%s.WidthTI", Name());
	m_HeightTI = (LWEUITextInput *)UIMan->GetNamedUIf("%s.HeightTI", Name());

	UIMan->RegisterMethodEvent(m_WidthTI, LWEUI::Event_Changed, &UIOrthoControls::WidthTIChanged, this, nullptr);
	UIMan->RegisterMethodEvent(m_HeightTI, LWEUI::Event_Changed, &UIOrthoControls::HeightTIChanged, this, nullptr);
}

//UICameraControls
void UICameraControls::Update(float dTime, LWEUIManager *UIMan, App *A) {
	if (!isVisible()) return;
	LWEUI *Focused = UIMan->GetFocusedUI();
	LWSVector4f Dir = m_Camera.GetDirection();
	float Pitch = asinf(-Dir.y());
	float Theta = atan2f(Dir.z(), Dir.x());
	float Len = m_Camera.GetPosition().Length3();
	if (Focused != m_PitchTI) m_PitchTI->Clear().InsertTextf("%.2f", false, false, 1.0f, Pitch * LW_RADTODEG);
	if (Focused != m_RotationTI) m_RotationTI->Clear().InsertTextf("%.2f", false, false, 1.0f, Theta * LW_RADTODEG);
	if (Focused != m_DistanceTI) m_DistanceTI->Clear().InsertTextf("%.2f", false, false, 1.0f, Len);
	m_PerspectiveControls.SetVisible(!m_Camera.IsOrthoCamera());
	m_OrthoControls.SetVisible(m_Camera.IsOrthoCamera());
	if (m_PerspectiveControls.isVisible()) m_PerspectiveControls.Update(dTime, UIMan, A);
	if (m_OrthoControls.isVisible()) m_OrthoControls.Update(dTime, UIMan, A);
	return;
}

void UICameraControls::ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A) {
	const float MinZoom = 0.5f;
	const float MaxZoom = 800.0f;
	const float MouseDeadzone = 1.0f;
	const float ZoomRate = 0.5f;
	LWKeyboard *KB = Window->GetKeyboardDevice();
	LWMouse *Ms = Window->GetMouseDevice();
	float Dis = m_Camera.GetPosition().Length3();
	LWVector2f WndCenter = Window->GetSizef() * 0.5f;
	LWVector2f MouseMove = WndCenter - Ms->GetPositionf();
	if (!UIMan->isTextInputFocused()) {
		if (KB->ButtonPressed(LWKey::LShift)) m_ControlCamera = !m_ControlCamera;
		if (m_ControlCamera) {
			if (Ms->GetScroll()) {
				if (Ms->GetScroll() > 0) Dis += ZoomRate;
				else Dis -= ZoomRate;
			}
			Dis = std::min<float>(std::max<float>(Dis, MinZoom), MaxZoom);
			float MouseMoveDis = MouseMove.Length();
			if (!m_Camera.isCameraControlled()) {
				if (MouseMoveDis < MouseDeadzone) m_Camera.SetCameraControlled(true);
			} else {
				m_Camera.ProcessDirectionInputThird(LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f), Dis, LWVector2f(MouseMove.x, 0.0f), dTime, dTime, LWVector4f(-LW_PI, LW_PI, -LW_PI_2, LW_PI_2), true);
			}
			Window->SetMousePosition(WndCenter.CastTo<int32_t>());
		} else m_Camera.SetCameraControlled(false);

		Window->SetMouseVisible(!m_ControlCamera);
	}
	if (m_PerspectiveControls.isVisible()) m_PerspectiveControls.ProcessInput(dTime, UIMan, Window, A);
	if (m_OrthoControls.isVisible()) m_OrthoControls.ProcessInput(dTime, UIMan, Window, A);
	return;
}

void UICameraControls::SerializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A) {
	const uint32_t PerspectiveToggle = 0;
	const uint32_t OrthoToggle = 1;

	LWSVector4f Dir = m_Camera.GetDirection();
	float Pitch = asinf(-Dir.y())*LW_RADTODEG;
	float Theta = atan2f(Dir.z(), Dir.x())*LW_RADTODEG;
	float Len = m_Camera.GetPosition().Length3();
	uint32_t CType = m_TypeTglGroup.NextToggled();
	JSon.MakeValueElement("CPitch", Pitch, Parent);
	JSon.MakeValueElement("CTheta", Theta, Parent);
	JSon.MakeValueElement("CDistance", Len, Parent);
	JSon.MakeValueElement("CType", CType, Parent);
	if (CType == PerspectiveToggle) m_PerspectiveControls.SerializeSettings(JSon, Parent, A);
	else if (CType == OrthoToggle) m_OrthoControls.SerializeSettings(JSon, Parent, A);
	return;
}

void UICameraControls::DeserializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A) {
	const uint32_t PerspectiveToggle = 0;
	const uint32_t OrthoToggle = 1;
	const float MinZoom = 0.5f;
	const float MaxZoom = 800.0f;
	const float MinPitch = -LW_PI_2 + LW_DEGTORAD;
	const float MaxPitch = LW_PI_2 - LW_DEGTORAD;


	LWEJObject *JCPitch = Parent->FindChild("CPitch", JSon);
	LWEJObject *JCTheta = Parent->FindChild("CTheta", JSon);
	LWEJObject *JCDistance = Parent->FindChild("CDistance", JSon);
	LWEJObject *JCType = Parent->FindChild("CType", JSon);

	LWSVector4f Dir = m_Camera.GetDirection();
	float Pitch = asinf(-Dir.y());
	float Theta = atan2f(Dir.z(), Dir.x());
	float Len = m_Camera.GetPosition().Length3();

	if (JCPitch) Pitch = JCPitch->AsFloat() * LW_DEGTORAD;
	if (JCTheta) Theta = JCTheta->AsFloat() * LW_DEGTORAD;
	if (JCDistance) Len = JCDistance->AsFloat();
	if (JCType) {
		uint32_t CType = JCType->AsInt();
		m_TypeTglGroup.SetToggled(CType, true);
		if (CType == PerspectiveToggle) m_PerspectiveControls.DeserializeSettings(JSon, Parent, A);
		else if (CType == OrthoToggle) m_OrthoControls.DeserializeSettings(JSon, Parent, A);
	}

	Pitch = std::min<float>(std::max<float>(Pitch, MinPitch), MaxPitch);
	Len = std::min<float>(std::max<float>(Len, MinZoom), MaxZoom);

	Dir = Camera::MakeDirection(LWVector2f(Theta, -Pitch));
	LWSVector4f Pos = LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f) - Dir * Len;
	m_Camera.SetDirection(Dir).SetPosition(Pos);
	return;
}

void UICameraControls::TypeToggleChanged(UIToggleGroup &Grop, uint32_t TglIndex, UIToggle &Toggle, bool Toggled, void *UserData) {
	const float DefaultFov = LW_PI_4;
	const float DefaultWidth = 10.0f;
	const float DefaultHeight = 10.0f;
	if (!Toggled) return;
	bool isOrtho = TglIndex == 1;
	if (isOrtho == m_Camera.IsOrthoCamera()) return;

	if (isOrtho) {
		CameraOrtho &O = m_Camera.GetOrthoPropertys();
		O.m_Left = -DefaultWidth;
		O.m_Right = DefaultWidth;
		O.m_Top = DefaultHeight;
		O.m_Bottom = -DefaultHeight;
	} else {
		CameraPerspective &P = m_Camera.GetPerspectivePropertys();
		P.m_FOV = DefaultFov;
	}
	m_Camera.SetOrtho(isOrtho);
	return;
}

void UICameraControls::OutputToggleChanged(UIToggleGroup &Group, uint32_t TglIndex, UIToggle &Toggle, bool Toggled, void *UserData) {
	return;
}
	
void UICameraControls::PitchTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	const float MinPitch = -LW_PI_2 + LW_DEGTORAD;
	const float MaxPitch = LW_PI_2 - LW_DEGTORAD;

	float Pitch = -(float)atof(m_PitchTI->GetLine(0)->m_Value) * LW_DEGTORAD;
	Pitch = std::min<float>(std::max<float>(Pitch, MinPitch), MaxPitch);
	LWVector2f SphereDir = m_Camera.GetSphericalDirection();
	SphereDir.y = Pitch;
	float Dis = m_Camera.GetPosition().Length3();
	LWSVector4f Dir = Camera::MakeDirection(SphereDir);
	LWSVector4f Pos = LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f) - Dir * Dis;
	m_Camera.SetPosition(Pos).SetDirection(Dir);
	return;
}

void UICameraControls::RotationTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	float Theta = (float)atof(m_RotationTI->GetLine(0)->m_Value) * LW_DEGTORAD;
	LWVector2f SphereDir = m_Camera.GetSphericalDirection();
	SphereDir.x = Theta;
	float Dis = m_Camera.GetPosition().Length3();
	LWSVector4f Dir = Camera::MakeDirection(SphereDir);
	LWSVector4f Pos = LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f) - Dir * Dis;
	m_Camera.SetPosition(Pos).SetDirection(Dir);
	return;
}

void UICameraControls::DistanceTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	const float MinZoom = 0.5f;
	const float MaxZoom = 800.0f;
	float Dis = (float)atof(m_DistanceTI->GetLine(0)->m_Value);
	Dis = std::min<float>(std::max<float>(Dis, MinZoom), MaxZoom);
	LWSVector4f Dir = m_Camera.GetDirection();
	LWSVector4f Pos = LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f) - Dir * Dis;
	m_Camera.SetPosition(Pos).SetDirection(Dir);
	return;
}

UICameraControls::UICameraControls(const StackText &Name, LWEUIManager *UIMan) : UIItem(Name, UIMan) {	
	const float InitialFOV = LW_PI_4;
	const float InitialPitch = LW_PI_4;
	const float InitialTheta = LW_PI_4;
	const float InitialDistance = 40.0f;
	const LWSVector4f InitDir = Camera::MakeDirection(LWVector2f(InitialTheta, -InitialPitch));
	const LWSVector4f InitPos = LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f) - InitDir * InitialDistance;
	m_Camera = Camera(InitPos, InitDir, LWSVector4f(0.0f, 1.0f, 0.0f, 0.0f), 1.0f, InitialFOV, 0.1f, 1000.0f, 0);
	new (&m_OrthoControls) UIOrthoControls(StackText("%s.OrthoControls", Name()), UIMan, *this);
	new (&m_PerspectiveControls) UIPerspectiveControls(StackText("%s.PerspectiveControls", Name()), UIMan, *this);

	m_PitchTI = (LWEUITextInput *)UIMan->GetNamedUIf("%s.PitchTI", Name());
	m_RotationTI = (LWEUITextInput *)UIMan->GetNamedUIf("%s.RotationTI", Name());
	m_DistanceTI = (LWEUITextInput *)UIMan->GetNamedUIf("%s.DistanceTI", Name());

	UIMan->RegisterMethodEvent(m_PitchTI, LWEUI::Event_Changed, &UICameraControls::PitchTIChanged, this, nullptr);
	UIMan->RegisterMethodEvent(m_DistanceTI, LWEUI::Event_Changed, &UICameraControls::DistanceTIChanged, this, nullptr);
	UIMan->RegisterMethodEvent(m_RotationTI, LWEUI::Event_Changed, &UICameraControls::RotationTIChanged, this, nullptr);

	UIToggleGroup::MakeMethod(m_TypeTglGroup, UIToggleGroup::AlwaysOneActive, &UICameraControls::TypeToggleChanged, this, nullptr);
	m_TypeTglGroup.PushToggle(StackText("%s.PerspectiveTgl", Name()), UIMan);
	m_TypeTglGroup.PushToggle(StackText("%s.OrthoTgl", Name()), UIMan);

	UIToggleGroup::MakeMethod(m_OutputTglGroup, UIToggleGroup::AlwaysOneActive, &UICameraControls::OutputToggleChanged, this, nullptr);
	m_OutputTglGroup.PushToggle(StackText("%s.DefaultTgl", Name()), UIMan);
	m_OutputTglGroup.PushToggle(StackText("%s.EmissionTgl", Name()), UIMan);
	m_OutputTglGroup.PushToggle(StackText("%s.NormalTgl", Name()), UIMan);
	m_OutputTglGroup.PushToggle(StackText("%s.AlbedoTgl", Name()), UIMan);
	m_OutputTglGroup.PushToggle(StackText("%s.MetallicTgl", Name()), UIMan);
}
