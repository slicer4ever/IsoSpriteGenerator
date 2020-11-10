#ifndef UILIGHTINGPROPS_H
#define UILIGHTINGPROPS_H
#include "UIToolkit.h"

class App;

struct Light;

struct UIViewer;

struct UILightSunProps : public UIItem {
	void Update(float dTime, LWEUIManager *UIMan, App *A);

	void ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A);

	void SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void RotationTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	void PitchTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	void FlagToggled(UIToggle &Toggle, bool isToggled, void *UserData);

	Light MakeLightSource(void);

	UILightSunProps(const LWUTF8Iterator &Name, LWEUIManager *UIMan, App *A);

	UILightSunProps() = default;

	LWEUITextInput *m_RotationTI;
	LWEUITextInput *m_PitchTI;
	UIToggle m_ShadowCastTgl;
	UIToggle m_DrawSunTgl;

	float m_Rotation = 0.0f;
	float m_Pitch = LW_PI_4;
};

struct UILightIBLProps : public UIItem {
	void Update(float dTime, LWEUIManager *UIMan, App *A);

	void ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A);

	void SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void OpenbrdfBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	void OpenDiffuseBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	void OpenSpecularBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	//Returns the texture ID.
	uint32_t LoadImage(char8_t *PathBuffer, uint32_t PathBufferSize, App *A);

	UILightIBLProps(const LWUTF8Iterator &Name, LWEUIManager *UIMan, App *A);

	UILightIBLProps() = default;

	char8_t m_brdfPath[256]="";
	char8_t m_DiffusePath[256]="";
	char8_t m_SpecularPath[256]="";
	UILabelBtn m_OpenbrdfBtn;
	UILabelBtn m_OpenDiffuseBtn;
	UILabelBtn m_OpenSpecularBtn;
	LWEUIMaterial m_brdfMaterial;
	LWEUIMaterial m_DiffuseMaterial;
	LWEUIMaterial m_SpecularMaterial;
	LWEUIRect *m_brdfPreview = nullptr;
	LWEUIRect *m_DiffusePreview = nullptr;
	LWEUIRect *m_SpecularPreview = nullptr;
	uint32_t m_brdfID = 0;
	uint32_t m_DiffuseID = 0;
	uint32_t m_SpecularID = 0;
};

struct UILightingProps : public UIItem {

	void Update(float dTime, LWEUIManager *UIMan, App *A);

	void ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A);

	void SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void ToggleGroupChanged(UIToggleGroup &Group, uint32_t Index, UIToggle &Toggle, bool isToggled, void *UserData);

	bool isUsingSun(void);

	bool isUsingIBL(void);

	UILightingProps(const LWUTF8Iterator &Name, LWEUIManager *UIMan, UIViewer *Viewer, App *A);

	UILightingProps() = default;

	UIViewer *m_Viewer = nullptr;
	UIToggleGroup m_LightTypeTgl;
	UILightIBLProps m_IBLProps;
	UILightSunProps m_SunProps;

};

#endif