#ifndef UICAMERACONTROLS_H
#define UICAMERACONTROLS_H
#include "UIToolkit.h"
#include "Camera.h"

struct UICameraControls;

class App;

struct UIPerspectiveControls : public UIItem {
	void Update(float dTime, LWEUIManager *UIMan, App *A);

	void ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A);

	void SerializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A);

	void DeserializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A);

	void FovTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	UIPerspectiveControls(const StackText &Name, LWEUIManager *UIMan, UICameraControls &CamControls);

	UIPerspectiveControls() = default;

	UICameraControls *m_UICamControls = nullptr;
	LWEUITextInput *m_FovTI = nullptr;
};

struct UIOrthoControls : public UIItem {
	void Update(float dTime, LWEUIManager *UIMan, App *A);

	void ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A);

	void SerializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A);

	void DeserializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A);

	void WidthTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	void HeightTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	UIOrthoControls(const StackText &Name, LWEUIManager *UIMan, UICameraControls &CamControls);

	UIOrthoControls() = default;

	UICameraControls *m_UICamControls = nullptr;
	LWEUITextInput *m_WidthTI = nullptr;
	LWEUITextInput *m_HeightTI = nullptr;
};

struct UICameraControls : public UIItem {

	void Update(float dTime, LWEUIManager *UIMan, App *A);

	void ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A);

	void SerializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A);

	void DeserializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A);

	void TypeToggleChanged(UIToggleGroup &Group, uint32_t TglIndex, UIToggle &Toggle, bool Toggled, void *UserData);

	void OutputToggleChanged(UIToggleGroup &Group, uint32_t TglIndex, UIToggle &Toggle, bool Toggled, void *UserData);

	void PitchTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	void RotationTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	void DistanceTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	UICameraControls(const StackText &Name, LWEUIManager *UIMan);

	UICameraControls() = default;

	UIPerspectiveControls m_PerspectiveControls;
	UIOrthoControls m_OrthoControls;
	Camera m_Camera;
	UIToggleGroup m_TypeTglGroup;
	UIToggleGroup m_OutputTglGroup;
	LWEUITextInput *m_PitchTI = nullptr;
	LWEUITextInput *m_RotationTI = nullptr;
	LWEUITextInput *m_DistanceTI = nullptr;
	bool m_ControlCamera = false;
};

#endif