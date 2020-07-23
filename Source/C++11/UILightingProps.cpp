#include "UILightingProps.h"
#include <LWPlatform/LWWindow.h>
#include <LWVideo/LWImage.h>
#include "Light.h"
#include "Camera.h"
#include "Renderer.h"
#include "App.h"
#include "Logger.h"
#include <LWEJson.h>

//UISunProps
void UILightSunProps::Update(float dTime, LWEUIManager *UIMan, App *A) {
	if (!isVisible()) return;
	LWEUI *Focused = UIMan->GetFocusedUI();

	if (Focused != m_RotationTI) m_RotationTI->Clear().InsertTextf("%.2f", false, false, 1.0f, m_Rotation*LW_RADTODEG);
	if (Focused != m_PitchTI) m_PitchTI->Clear().InsertTextf("%.2f", false, false, 1.0f, m_Pitch*LW_RADTODEG);
	return;
}

void UILightSunProps::ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A) {
	LWKeyboard *KB = Window->GetKeyboardDevice();
	if (UIMan->isTextInputFocused()) return;
	if (KB->ButtonDown(LWKey::LCtrl)) {
		if (KB->ButtonPressed(LWKey::L)) m_DrawSunTgl.SetToggled(!m_DrawSunTgl.isToggled());
	}
	return;
}

void UILightSunProps::SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	J.MakeValueElement("SunPitch", m_Pitch * LW_RADTODEG, Parent);
	J.MakeValueElement("SunRotation", m_Rotation * LW_RADTODEG, Parent);
	return;
}

void UILightSunProps::DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	const float MinPitch = -LW_PI_2 + LW_DEGTORAD;
	const float MaxPitch = LW_PI_2 - LW_DEGTORAD;

	LWEJObject *JSunPitch = Parent->FindChild("SunPitch", J);
	LWEJObject *JSunRotation = Parent->FindChild("SunRotation", J);

	float Pitch = m_Pitch;
	float Rotation = m_Rotation;
	if (JSunPitch) Pitch = JSunPitch->AsFloat()*LW_DEGTORAD;
	if (JSunRotation) Rotation = JSunRotation->AsFloat()*LW_DEGTORAD;
	m_Pitch = std::min<float>(std::max<float>(Pitch, MinPitch), MaxPitch);
	m_Rotation = fmodf(Rotation, LW_2PI);
	return;
}

void UILightSunProps::RotationTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	m_Rotation = (float)atof(m_RotationTI->GetLine(0)->m_Value)*LW_DEGTORAD;
	return;
}

void UILightSunProps::PitchTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	const float MinPitch = -LW_PI_2 + LW_DEGTORAD;
	const float MaxPitch = LW_PI_2 - LW_DEGTORAD;
	float Pitch = (float)atof(m_PitchTI->GetLine(0)->m_Value)*LW_DEGTORAD;
	m_Pitch = std::min<float>(std::max<float>(Pitch, MinPitch), MaxPitch);
	return;
}

void UILightSunProps::FlagToggled(UIToggle &Toggle, bool isToggled, void *UserData) {
	return;
}

Light UILightSunProps::MakeLightSource(void) {
	uint32_t Flags = 0;
	if (m_ShadowCastTgl.isToggled()) Flags |= Light::ShadowCaster;
	return Light(Camera::MakeDirection(LWVector2f(m_Rotation, -m_Pitch)), LWVector4f(1.0f), 1.0f, Flags);
}

UILightSunProps::UILightSunProps(const StackText &Name, LWEUIManager *UIMan, App *A) : UIItem(Name, UIMan) {
	m_RotationTI = (LWEUITextInput *)UIMan->GetNamedUIf("%s.DirectionTI", Name());
	m_PitchTI = (LWEUITextInput *)UIMan->GetNamedUIf("%s.PitchTI", Name());

	UIMan->RegisterMethodEvent(m_RotationTI, LWEUI::Event_Changed, &UILightSunProps::RotationTIChanged, this, A);
	UIMan->RegisterMethodEvent(m_PitchTI, LWEUI::Event_Changed, &UILightSunProps::PitchTIChanged, this, A);

	UIToggle::MakeMethod(m_ShadowCastTgl, StackText("%s.ShadowCastFlag", Name()), UIMan, &UILightSunProps::FlagToggled, this, A);
	UIToggle::MakeMethod(m_DrawSunTgl, StackText("%s.DrawSunFlag", Name()), UIMan, &UILightSunProps::FlagToggled, this, A);
}

//UILightIBLProps
void UILightIBLProps::Update(float dTime, LWEUIManager *UIMan, App *A) {
	if (!isVisible()) return;
	Renderer *R = A->GetRenderer();
	LWTexture *brdfTex = R->GetTexture(m_brdfID);
	LWTexture *DiffuseTex = R->GetTexture(m_DiffuseID);
	LWTexture *SpecularTex = R->GetTexture(m_SpecularID);

	if (m_brdfMaterial.m_Texture != brdfTex) {
		m_brdfMaterial.m_Texture = brdfTex;
		m_brdfMaterial.m_ColorA = brdfTex ? LWVector4f(1.0f) : LWVector4f(0.0f, 0.0f, 0.0f, 1.0f);
		if (brdfTex) R->SetIBLBrdfTexture(brdfTex);
	}
	if (m_DiffuseMaterial.m_Texture != DiffuseTex) {
		m_DiffuseMaterial.m_Texture = DiffuseTex;
		m_DiffuseMaterial.m_ColorA = brdfTex ? LWVector4f(1.0f) : LWVector4f(0.0f, 0.0f, 0.0f, 1.0f);
		if (DiffuseTex) R->SetIBLDiffuseTexture(DiffuseTex);
	}
	if (m_SpecularMaterial.m_Texture != SpecularTex) {
		m_SpecularMaterial.m_Texture = SpecularTex;
		m_SpecularMaterial.m_ColorA = brdfTex ? LWVector4f(1.0f) : LWVector4f(0.0f, 0.0f, 0.0f, 1.0f);
		if (SpecularTex) R->SetIBLSpecularTexture(SpecularTex);
	}

	return;
}

void UILightIBLProps::ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A) {
	return;
}

void UILightIBLProps::SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	LWEJObject *JIBLbrdf = J.MakeStringElement("IBLbrdf", "", Parent);
	LWEJObject *JIBLDiffuse = J.MakeStringElement("IBLDiffuse", "", Parent);
	LWEJObject *JIBLSpecular = J.MakeStringElement("IBLSpecular", "", Parent);

	//TODO: Fix escape text bug in LWEJson implementation.
	JIBLbrdf->SetValuef(J.GetAllocator(), m_brdfPath);
	JIBLDiffuse->SetValuef(J.GetAllocator(), m_DiffusePath);
	JIBLSpecular->SetValuef(J.GetAllocator(), m_SpecularPath);

	return;
}

void UILightIBLProps::DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	LWEJObject *JIBLbrdf = Parent->FindChild("IBLbrdf", J);
	LWEJObject *JIBLDiffuse = Parent->FindChild("IBLDiffuse", J);
	LWEJObject *JIBLSpecular = Parent->FindChild("IBLSpecular", J);

	auto LoadPath = [](char *PathBuffer, uint32_t PathBufferSize, const char *JPath, App *A) -> uint32_t {
		LWAllocator &Alloc = A->GetAllocator();
		Renderer *R = A->GetRenderer();
		*PathBuffer = '\0';
		if (!*JPath) return 0;
		strncat(PathBuffer, JPath, PathBufferSize);
		LWImage *Img = Alloc.Allocate<LWImage>();
		if (!LWImage::LoadImage(*Img, PathBuffer, Alloc)) {
			LogWarnf("Error: Could not load file: '%s'", PathBuffer);
			LWAllocator::Destroy(Img);
			*PathBuffer = '\0';
			return 0;
		}
		return R->PushPendingTexture(0, Img);
	};


	if (JIBLbrdf) m_brdfID = LoadPath(m_brdfPath, sizeof(m_brdfPath), JIBLbrdf->m_Value, A);
	if (JIBLDiffuse) m_DiffuseID = LoadPath(m_DiffusePath, sizeof(m_DiffusePath), JIBLDiffuse->m_Value, A);
	if (JIBLSpecular) m_SpecularID = LoadPath(m_SpecularPath, sizeof(m_SpecularPath), JIBLSpecular->m_Value, A);
	return;
}

uint32_t UILightIBLProps::LoadImage(char *PathBuffer, uint32_t PathBufferSize, App *A) {
	LWAllocator &Alloc = A->GetAllocator();
	Renderer *R = A->GetRenderer();
	if (!LWWindow::MakeLoadFileDialog("*.png;*.dds;*.tga\0Texture File\0\0", PathBuffer, PathBufferSize)) return 0;
	LWImage *Img = Alloc.Allocate<LWImage>();
	if (!LWImage::LoadImage(*Img, PathBuffer, Alloc)) {
		A->SetMessage(StackText("Error: Failed to load texture: '%s'", PathBuffer));
		*PathBuffer = '\0';
		LWAllocator::Destroy(Img);
		return 0;
	}
	return R->PushPendingTexture(0, Img);
}

void UILightIBLProps::OpenbrdfBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	App *A = (App*)UserData;
	uint32_t ID = LoadImage(m_brdfPath, sizeof(m_brdfPath), A);
	if (!ID) return;
	m_brdfID = ID;
	A->SetMessage("Loaded brdf");
	return;
}

void UILightIBLProps::OpenDiffuseBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	App *A = (App*)UserData;
	uint32_t ID = LoadImage(m_DiffusePath, sizeof(m_DiffusePath), A);
	if (!ID) return;
	m_DiffuseID = ID;
	A->SetMessage("Loaded Diffuse");
	return; 
}

void UILightIBLProps::OpenSpecularBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	App *A = (App*)UserData;
	uint32_t ID = LoadImage(m_SpecularPath, sizeof(m_SpecularPath), A);
	if (!ID) return;
	m_SpecularID = ID;
	A->SetMessage("Loaded Specular");
	return; 
}

UILightIBLProps::UILightIBLProps(const StackText &Name, LWEUIManager *UIMan, App *A) : UIItem(Name, UIMan) {
	UILabelBtn::MakeMethod(m_OpenbrdfBtn, StackText("%s.brdfBtn", Name()), UIMan, &UILightIBLProps::OpenbrdfBtnReleased, this, A);
	UILabelBtn::MakeMethod(m_OpenDiffuseBtn, StackText("%s.DiffuseBtn", Name()), UIMan, &UILightIBLProps::OpenDiffuseBtnReleased, this, A);
	UILabelBtn::MakeMethod(m_OpenSpecularBtn, StackText("%s.SpecularBtn", Name()), UIMan, &UILightIBLProps::OpenSpecularBtnReleased, this, A);

	m_brdfPreview = (LWEUIRect *)UIMan->GetNamedUIf("%s.brdfPreview", Name());
	m_DiffusePreview = (LWEUIRect *)UIMan->GetNamedUIf("%s.DiffusePreview", Name());
	m_SpecularPreview = (LWEUIRect *)UIMan->GetNamedUIf("%s.SpecularPreview", Name());

	m_brdfMaterial = LWEUIMaterial(LWVector4f(0.0f, 0.0f, 0.0f, 1.0f));
	m_DiffuseMaterial = LWEUIMaterial(LWVector4f(0.0f, 0.0f, 0.0f, 1.0f));
	m_SpecularMaterial = LWEUIMaterial(LWVector4f(0.0f, 0.0f, 0.0f, 1.0f));

	m_brdfPreview->SetMaterial(&m_brdfMaterial);
	m_DiffusePreview->SetMaterial(&m_DiffuseMaterial);
	m_SpecularPreview->SetMaterial(&m_SpecularMaterial);
}

//UILightingProps
void UILightingProps::Update(float dTime, LWEUIManager *UIMan, App *A) {
	m_SunProps.Update(dTime, UIMan, A);
	m_IBLProps.Update(dTime, UIMan, A);
	return;
}

void UILightingProps::ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A) {
	m_SunProps.ProcessInput(dTime, UIMan, Window, A);
	m_IBLProps.ProcessInput(dTime, UIMan, Window, A);
	return;
}

void UILightingProps::SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	uint32_t LightMask = m_LightTypeTgl.GetToggledMask();
	J.MakeValueElement("LightType", LightMask, Parent);
	m_SunProps.SerializeSettings(J, Parent, A);
	m_IBLProps.SerializeSettings(J, Parent, A);
	return;
}

void UILightingProps::DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	LWEJObject *JLightType = Parent->FindChild("LightType", J);
	if (JLightType) {
		uint32_t LightType = JLightType->AsInt();
		m_LightTypeTgl.ApplyToggledMask(LightType);
	}
	m_SunProps.DeserializeSettings(J, Parent, A);
	m_IBLProps.DeserializeSettings(J, Parent, A);
	return;
}

bool UILightingProps::isUsingSun(void) {
	return m_LightTypeTgl.isToggled(0);
}

bool UILightingProps::isUsingIBL(void) {
	return m_LightTypeTgl.isToggled(1);
}

void UILightingProps::ToggleGroupChanged(UIToggleGroup &Group, uint32_t Index, UIToggle &Toggle, bool isToggled, void *UserData) {
	m_SunProps.SetVisible(m_LightTypeTgl.isToggled(0));
	m_IBLProps.SetVisible(m_LightTypeTgl.isToggled(1));
	return;
}

UILightingProps::UILightingProps(const StackText &Name, LWEUIManager *UIMan, UIViewer *Viewer, App *A) : UIItem(Name, UIMan), m_Viewer(Viewer) {
	new (&m_SunProps) UILightSunProps(StackText("%s.UILightSunProps", Name()), UIMan, A);
	new (&m_IBLProps) UILightIBLProps(StackText("%s.UILightIBLProps", Name()), UIMan, A);

	UIToggleGroup::MakeMethod(m_LightTypeTgl, UIToggleGroup::AlwaysOneActive, &UILightingProps::ToggleGroupChanged, this, A);
	m_LightTypeTgl.PushToggle(StackText("%s.SunTgl", Name()), UIMan);
	m_LightTypeTgl.PushToggle(StackText("%s.IBLTgl", Name()), UIMan);
	m_SunProps.SetVisible(true);
}
