#include "UIViewer.h"
#include "App.h"
#include "State_Viewer.h"
#include <LWPlatform/LWWindow.h>

//UIViewer
void UIViewer::Update(float dTime, LWEUIManager *UIMan, App *A) {
	m_FileProps.Update(dTime, UIMan, A);
	m_CameraProps.Update(dTime, UIMan, A);
	m_IsometricProps.Update(dTime, UIMan, A);
	m_AnimationProps.Update(dTime, UIMan, A);
	m_LightingProps.Update(dTime, UIMan, A);
	return;
}

void UIViewer::ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A) {
	LWKeyboard *KB = Window->GetKeyboardDevice();
	m_FileProps.ProcessInput(dTime, UIMan, Window, A);
	m_CameraProps.ProcessInput(dTime, UIMan, Window, A);
	m_IsometricProps.ProcessInput(dTime, UIMan, Window, A);
	m_AnimationProps.ProcessInput(dTime, UIMan, Window, A);
	m_LightingProps.ProcessInput(dTime, UIMan, Window, A);
	if (UIMan->isTextInputFocused()) return;
	if (KB->ButtonDown(LWKey::LCtrl)) {
		if (KB->ButtonPressed(LWKey::Key1)) m_ToggleList.SetToggled(0, true);
		else if (KB->ButtonPressed(LWKey::Key2)) m_ToggleList.SetToggled(1, true);
		else if (KB->ButtonPressed(LWKey::Key3)) m_ToggleList.SetToggled(2, true);
		else if (KB->ButtonPressed(LWKey::Key4)) m_ToggleList.SetToggled(3, true);
		else if (KB->ButtonPressed(LWKey::Key5)) m_ToggleList.SetToggled(4, true);
	}
	return;
}

void UIViewer::SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	LWEJObject *Settings = J.MakeObjectElement("Settings", Parent);
	m_FileProps.SerializeSettings(J, Settings, A);
	m_CameraProps.SerializeSettings(J, Settings, A);
	m_IsometricProps.SerializeSettings(J, Settings, A);
	m_AnimationProps.SerializeSettings(J, Settings, A);
	m_LightingProps.SerializeSettings(J, Settings, A);
	return;
}

void UIViewer::DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	LWEJObject *Settings = Parent ? Parent->FindChild("Settings", J) : J.Find("Settings");
	m_FileProps.DeserializeSettings(J, Settings, A);
	m_CameraProps.DeserializeSettings(J, Settings, A);
	m_IsometricProps.DeserializeSettings(J, Settings, A);
	m_AnimationProps.DeserializeSettings(J, Settings, A);
	m_LightingProps.DeserializeSettings(J, Settings, A);
	return;
}

void UIViewer::ToggleChanged(UIToggleGroup &Group, uint32_t Index, UIToggle &Tgl, bool isToggled, void *UserData) {
	m_FileProps.SetVisible(m_ToggleList.isToggled(0));
	m_CameraProps.SetVisible(m_ToggleList.isToggled(1));
	m_IsometricProps.SetVisible(m_ToggleList.isToggled(2));
	m_AnimationProps.SetVisible(m_ToggleList.isToggled(3));
	m_LightingProps.SetVisible(m_ToggleList.isToggled(4));
	return;
}

UIViewer::UIViewer(const StackText &Name, LWEUIManager *UIMan, App *A) : UIItem(Name, UIMan) {
	new (&m_FileProps) UIFile(StackText("%s.UIFile", Name()), UIMan, this, A);
	new (&m_CameraProps) UICameraControls(StackText("%s.UICameraControls", Name()), UIMan);
	new (&m_IsometricProps) UIIsometricProps(StackText("%s.UIIsoProps", Name()), UIMan, this, A);
	new (&m_AnimationProps) UIAnimationProps(StackText("%s.UIAnimationProps", Name()), UIMan, this, A);
	new (&m_LightingProps) UILightingProps(StackText("%s.UILightingProps", Name()), UIMan, this, A);

	UIToggleGroup::MakeMethod(m_ToggleList, UIToggleGroup::AlwaysOneActive, &UIViewer::ToggleChanged, this, A);
	m_ToggleList.PushToggle(StackText("%s.FileTgl", Name()), UIMan);
	m_ToggleList.PushToggle(StackText("%s.CameraTgl", Name()), UIMan);
	m_ToggleList.PushToggle(StackText("%s.IsometricTgl", Name()), UIMan);
	m_ToggleList.PushToggle(StackText("%s.AnimationTgl", Name()), UIMan);
	m_ToggleList.PushToggle(StackText("%s.LightingTgl", Name()), UIMan);
	m_ToggleList.SetToggled(0, true);
	m_FileProps.SetVisible(true);
}
