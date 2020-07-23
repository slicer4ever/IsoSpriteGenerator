#ifndef UIANIMATIONPROPS_H
#define UIANIMATIONPROPS_H
#include "UIToolkit.h"

class App;

struct UIViewer;

struct UIAnimationProps : public UIItem {
	static const uint32_t MaxFrames = 64;

	void Update(float dTime, LWEUIManager *UIMan, App *A);

	void ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A);

	void SerializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A);

	void DeserializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A);

	void PlayBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	void RewindBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	void NextFrameBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	void PrevFrameBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	float GetFrameTime(uint32_t Frame, App *A);

	int32_t GetFrameAtTime(float Time, App *A);

	void TimeSBPressed(LWEUI *UI, uint32_t EventCode, void *UserData);

	void TimeSBChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	void FrameCntTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	void FrameOffsetTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	UIAnimationProps(const StackText &Name, LWEUIManager *UIMan, UIViewer *Viewer, App *A);

	UIAnimationProps() = default;

	UILabelBtn m_PlayBtn;
	UILabelBtn m_RewindBtn;
	UILabelBtn m_NextFrameBtn;
	UILabelBtn m_PrevFrameBtn;
	UIViewer *m_Viewer = nullptr;
	LWEUILabel *m_TimeLbl = nullptr;
	LWEUIScrollBar *m_TimeSB = nullptr;
	LWEUITextInput *m_FrameCntTI = nullptr;
	LWEUITextInput *m_OffsetTI = nullptr;
	bool m_Playing = false;
	uint32_t m_FrameCnt = 1;
	float m_Offset = 0.0f;
};

#endif