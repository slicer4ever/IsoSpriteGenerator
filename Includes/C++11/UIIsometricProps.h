#ifndef UIISOMETRICPROPS_H
#define UIISOMETRICPROPS_H
#include "UIToolkit.h"

class App;

struct UIViewer;

struct UIIsometricProps : public UIItem {
	static const uint32_t MaxDirections = 32;

	void Update(float dTime, LWEUIManager *UIMan, App *A);

	void ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A);

	void SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void DirectionsTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	void OffsetTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	float CalculateDirectionTheta(uint32_t Direction);

	void MdlNextBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	void MdlPrevBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	UIIsometricProps(const StackText &Name, LWEUIManager *UIMan, UIViewer *Viewer, App *A);

	UIIsometricProps() = default;

	UILabelBtn m_MdlNextBtn;
	UILabelBtn m_MdlPrevBtn;
	UIViewer *m_Viewer = nullptr;
	LWEUITextInput *m_DirectionsTI = nullptr;
	LWEUITextInput *m_OffsetTI = nullptr;

	uint32_t m_DirectionCnt = 1;
	float m_ThetaOffset = 0.0f;
};

#endif