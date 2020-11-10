#include "UIAnimationProps.h"
#include "App.h"
#include "State_Viewer.h"
#include <LWEJson.h>

//UIAnimationProps
void UIAnimationProps::Update(float dTime, LWEUIManager *UIMan, App *A) {
	const float ScrollSizeRatio = 0.2f;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	Scene *S = SV->GetScene();
	if (!S) return;
	LWEUI *Focused = UIMan->GetFocusedUI();
	float Time = SV->GetTime();
	float TotalTime = S->GetTotalTime();
	if (m_Playing) {
		if (TotalTime > 0.0f) Time = fmodf(Time + dTime, TotalTime);
		else Time = 0.0f;
		SV->SetTime(Time);
	}
	if (!isVisible()) return;
	float ScrollSize = TotalTime * ScrollSizeRatio;


	m_TimeSB->SetMaxScroll(TotalTime+ScrollSize).SetScrollSize(ScrollSize).SetScroll(Time);
	m_TimeLbl->SetText(LWUTF8I::Fmt<64>("Time: {:.2}/{:.2}", Time, TotalTime));

	if (Focused != m_FrameCntTI) m_FrameCntTI->Clear().InsertText(LWUTF8I::Fmt<32>("{}", m_FrameCnt));
	if (Focused != m_OffsetTI) m_OffsetTI->Clear().InsertText(LWUTF8I::Fmt<32>("{:.2}", m_Offset));
	return;
}

void UIAnimationProps::ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A) {
	return;
}

void UIAnimationProps::SerializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A) {
	JSon.MakeValueElement("FrameCnt", m_FrameCnt, Parent);
	JSon.MakeValueElement("FrameOffset", m_Offset, Parent);
	return;
}

void UIAnimationProps::DeserializeSettings(LWEJson &JSon, LWEJObject *Parent, App *A) {
	const uint32_t MinFrames = 1;
	LWEJObject *FrameCntObj = Parent->FindChild("FrameCnt", JSon);
	LWEJObject *FrameOffsetObj = Parent->FindChild("FrameOffset", JSon);

	uint32_t FrameCnt = m_FrameCnt;
	float Offset = m_Offset;
	if (FrameCntObj) FrameCnt = FrameCntObj->AsInt();
	if (FrameOffsetObj) Offset = FrameOffsetObj->AsFloat();
	m_FrameCnt = std::min<uint32_t>(std::max<uint32_t>(FrameCnt, MinFrames), MaxFrames);
	m_Offset = Offset;
	return;
}

void UIAnimationProps::PlayBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	m_PlayBtn.m_Label->SetText(m_Playing ? "Play" : "Stop");
	m_Playing = !m_Playing;
	return;
}

void UIAnimationProps::RewindBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	App *A = (App*)UserData;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	SV->SetTime(0.0f);
	return;
}

float UIAnimationProps::GetFrameTime(uint32_t Frame, App *A) {
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	Scene *S = SV->GetScene();
	if (!S) return 0.0f;
	return m_Offset + S->GetTotalTime() / m_FrameCnt * Frame;
}

int32_t UIAnimationProps::GetFrameAtTime(float Time, App *A) {
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	Scene *S = SV->GetScene();
	if (!S) return 0;
	float Seg = S->GetTotalTime() / m_FrameCnt;
	if (Seg > 0.0f) return (int32_t)((Time-m_Offset) / Seg);
	return 0;
}

void UIAnimationProps::NextFrameBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	App *A = (App*)UserData;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	int32_t CurrFrame = GetFrameAtTime(SV->GetTime(), A);
	CurrFrame = (CurrFrame + 1) % m_FrameCnt;
	SV->SetTime(GetFrameTime(CurrFrame, A));
	if (m_Playing) PlayBtnReleased(nullptr, 0, A);
	return;
}

void UIAnimationProps::PrevFrameBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	App *A = (App*)UserData;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	int32_t CurrFrame = GetFrameAtTime(SV->GetTime(), A);
	CurrFrame = CurrFrame == 0 ? m_FrameCnt - 1 : CurrFrame - 1;
	SV->SetTime(GetFrameTime(CurrFrame, A));
	if (m_Playing) PlayBtnReleased(nullptr, 0, A);
	return;
}

void UIAnimationProps::TimeSBPressed(LWEUI *UI, uint32_t EventCode, void *UserData) {
	if (m_Playing) PlayBtnReleased(nullptr, 0, UserData);
	return;
}

void UIAnimationProps::TimeSBChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	App *A = (App*)UserData;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	SV->SetTime(m_TimeSB->GetScroll());
	return;
}

void UIAnimationProps::FrameCntTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	const uint32_t MinFrames = 1;
	uint32_t FrameCnt = (uint32_t)atoi(m_FrameCntTI->GetLine(0)->m_Value);
	m_FrameCnt = std::min<uint32_t>(std::max<uint32_t>(FrameCnt, MinFrames), MaxFrames);
	return;
}

void UIAnimationProps::FrameOffsetTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	App *A = (App*)UserData;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	m_Offset = (float)atof(m_OffsetTI->GetLine(0)->m_Value);
	SV->SetTime(m_Offset);
	if (m_Playing) PlayBtnReleased(nullptr, 0, UserData);
	return;
}

UIAnimationProps::UIAnimationProps(const LWUTF8Iterator &Name, LWEUIManager *UIMan, UIViewer *Viewer, App *A) : UIItem(Name, UIMan), m_Viewer(Viewer) {
	UILabelBtn::MakeMethod(m_PlayBtn, LWUTF8I::Fmt<128>("{}.PlayBtn", Name), UIMan, &UIAnimationProps::PlayBtnReleased, this, A);
	UILabelBtn::MakeMethod(m_RewindBtn, LWUTF8I::Fmt<128>("{}.RewindBtn", Name), UIMan, &UIAnimationProps::RewindBtnReleased, this, A);
	UILabelBtn::MakeMethod(m_NextFrameBtn, LWUTF8I::Fmt<128>("{}.NextFrameBtn", Name), UIMan, &UIAnimationProps::NextFrameBtnReleased, this, A);
	UILabelBtn::MakeMethod(m_PrevFrameBtn, LWUTF8I::Fmt<128>("{}.PrevFrameBtn", Name), UIMan, &UIAnimationProps::PrevFrameBtnReleased, this, A);

	m_TimeSB = (LWEUIScrollBar *)UIMan->GetNamedUI(LWUTF8I::Fmt<128>("{}.TimeSB", Name));
	m_TimeLbl = (LWEUILabel *)UIMan->GetNamedUI(LWUTF8I::Fmt<128>("{}.TimeLbl", Name));
	m_FrameCntTI = (LWEUITextInput *)UIMan->GetNamedUI(LWUTF8I::Fmt<128>("{}.FrameCntTI", Name));
	m_OffsetTI = (LWEUITextInput *)UIMan->GetNamedUI(LWUTF8I::Fmt<128>("{}.FrameOffsetTI", Name));

	UIMan->RegisterMethodEvent(m_TimeSB, LWEUI::Event_Changed, &UIAnimationProps::TimeSBChanged, this, A);
	UIMan->RegisterMethodEvent(m_TimeSB, LWEUI::Event_Pressed, &UIAnimationProps::TimeSBPressed, this, A);

	UIMan->RegisterMethodEvent(m_FrameCntTI, LWEUI::Event_Changed, &UIAnimationProps::FrameCntTIChanged, this, A);
	UIMan->RegisterMethodEvent(m_OffsetTI, LWEUI::Event_Changed, &UIAnimationProps::FrameOffsetTIChanged, this, A);
};