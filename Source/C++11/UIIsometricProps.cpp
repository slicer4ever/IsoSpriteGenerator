#include "UIIsometricProps.h"
#include "App.h"
#include "State_Viewer.h"

//UIIsometricProps
void UIIsometricProps::Update(float dTime, LWEUIManager *UIMan, App *A) {
	if (!isVisible()) return;
	LWEUI *Focused = UIMan->GetFocusedUI();

	if (Focused != m_DirectionsTI) m_DirectionsTI->Clear().InsertText(LWUTF8I::Fmt<32>("{}", m_DirectionCnt));
	if (Focused != m_OffsetTI) m_OffsetTI->Clear().InsertText(LWUTF8I::Fmt<32>("{:.2}", m_ThetaOffset * LW_RADTODEG));

	return;
}

void UIIsometricProps::ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A) {
	return;
}

void UIIsometricProps::SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	J.MakeValueElement("Directions", m_DirectionCnt, Parent);
	J.MakeValueElement("RotationOffset", m_ThetaOffset * LW_RADTODEG, Parent);
	return;
}

void UIIsometricProps::DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	const uint32_t MinDirections = 1;
	LWEJObject *JDirections = Parent->FindChild("Directions", J);
	LWEJObject *JRotationOffset = Parent->FindChild("RotationOffset", J);

	uint32_t Directions = m_DirectionCnt;
	float Offset = m_ThetaOffset;
	if (JDirections) Directions = JDirections->AsInt();
	if (JRotationOffset) Offset = JRotationOffset->AsFloat() * LW_DEGTORAD;
	m_DirectionCnt = std::min<uint32_t>(std::max<uint32_t>(Directions, MinDirections), MaxDirections);
	m_ThetaOffset = Offset;
	return;
}

void UIIsometricProps::DirectionsTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	const uint32_t MinDirections = 1;
	uint32_t Dir = atoi(m_DirectionsTI->GetLine(0)->m_Value);
	Dir = std::min<uint32_t>(std::max<uint32_t>(Dir, MinDirections), MaxDirections);
	m_DirectionCnt = Dir;
	return;
}

void UIIsometricProps::OffsetTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	App *A = (App*)UserData;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	float Theta = (float)atof(m_OffsetTI->GetLine(0)->m_Value) * LW_DEGTORAD;
	Theta = fmodf(Theta, LW_2PI);
	m_ThetaOffset = Theta;
	SV->SetModelTheta(m_ThetaOffset);
	return;
}

float UIIsometricProps::CalculateDirectionTheta(uint32_t Direction) {
	return m_ThetaOffset + Direction * (LW_2PI / m_DirectionCnt);
}

void UIIsometricProps::MdlNextBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	const float e = std::numeric_limits<float>::epsilon();
	App *A = (App*)UserData;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);

	float Theta = SV->GetModelTheta();
	uint32_t CurrentDir = 0;
	for (; CurrentDir < m_DirectionCnt; CurrentDir++) {
		if (fabs(Theta - CalculateDirectionTheta(CurrentDir)) <= e) break;
	}
	if (CurrentDir == m_DirectionCnt) CurrentDir = m_DirectionCnt - 1;
	CurrentDir = (CurrentDir + 1) % m_DirectionCnt;
	SV->SetModelTheta(CalculateDirectionTheta(CurrentDir));
	return;
}

void UIIsometricProps::MdlPrevBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	const float e = std::numeric_limits<float>::epsilon();
	App *A = (App*)UserData;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	Camera &Cam = SV->GetCamera();

	float Theta = SV->GetModelTheta();

	uint32_t CurrentDir = m_DirectionCnt - 1;
	for (; CurrentDir < m_DirectionCnt; CurrentDir--) {
		if (fabs(Theta - CalculateDirectionTheta(CurrentDir)) <= e) break;
	}
	if (CurrentDir >= m_DirectionCnt) CurrentDir = m_DirectionCnt - 1;
	CurrentDir = CurrentDir == 0 ? m_DirectionCnt - 1 : CurrentDir - 1;
	SV->SetModelTheta(CalculateDirectionTheta(CurrentDir));
	return;
}

UIIsometricProps::UIIsometricProps(const LWUTF8Iterator &Name, LWEUIManager *UIMan, UIViewer *Viewer, App *A) : UIItem(Name, UIMan), m_Viewer(Viewer) {
	UILabelBtn::MakeMethod(m_MdlNextBtn, LWUTF8I::Fmt<128>("{}.MdlNextBtn", Name), UIMan, &UIIsometricProps::MdlNextBtnReleased, this, A);
	UILabelBtn::MakeMethod(m_MdlPrevBtn, LWUTF8I::Fmt<128>("{}.MdlPrevBtn", Name), UIMan, &UIIsometricProps::MdlPrevBtnReleased, this, A);

	m_DirectionsTI = (LWEUITextInput *)UIMan->GetNamedUI(LWUTF8I::Fmt<128>("{}.DirectionCntTI", Name));
	m_OffsetTI = (LWEUITextInput *)UIMan->GetNamedUI(LWUTF8I::Fmt<128>("{}.OffsetTI", Name));

	UIMan->RegisterMethodEvent(m_DirectionsTI, LWEUI::Event_Changed, &UIIsometricProps::DirectionsTIChanged, this, A);
	UIMan->RegisterMethodEvent(m_OffsetTI, LWEUI::Event_Changed, &UIIsometricProps::OffsetTIChanged, this, A);
};