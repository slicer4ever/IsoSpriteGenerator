#ifndef UIVIEWER_H
#define UIVIEWER_H
#include "UIToolkit.h"
#include "UIFile.h"
#include "UICameraControls.h"
#include "UIIsometricProps.h"
#include "UIAnimationProps.h"
#include "UILightingProps.h"

struct UIViewer : public UIItem {

	void Update(float dTime, LWEUIManager *UIMan, App *A);

	void ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A);

	void SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void ToggleChanged(UIToggleGroup &Group, uint32_t Index, UIToggle &Tgl, bool isToggled, void *UserData);

	UIViewer(const LWUTF8Iterator &Name, LWEUIManager *UIMan, App *A);

	UIViewer() = default;

	UIToggleGroup m_ToggleList;
	UIFile m_FileProps;
	UICameraControls m_CameraProps;
	UIIsometricProps m_IsometricProps;
	UIAnimationProps m_AnimationProps;
	UILightingProps m_LightingProps;
};

#endif