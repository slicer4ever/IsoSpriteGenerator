#include "UIToolkit.h"
#include <LWPlatform/LWWindow.h>
#include <LWVideo/LWTexture.h>
#include <LWVideo/LWVideoDriver.h>
#include <LWVideo/LWImage.h>

//UIItem
UIItem &UIItem::SetVisible(bool Visible) {
	m_UI->SetVisible(Visible);
	return *this;
}

bool UIItem::isVisible(void) {
	return m_UI->isVisible();
}

UIItem::UIItem(const StackText &Name, LWEUIManager *UIManager) {
	m_UI = UIManager->GetNamedUI(Name());
}

//UILabelBtn
UILabelBtn::UILabelBtn(const StackText &Name, LWEUIManager *UIManager, LWEUIEventCallback ButtonReleasedFunc, void *UserData) : UIItem(Name, UIManager) {
	m_Label = (LWEUILabel *)UIManager->GetNamedUIf("%s.Label", Name());
	m_Button = (LWEUIButton *)UIManager->GetNamedUIf("%s.Button", Name());
	if (ButtonReleasedFunc) UIManager->RegisterEvent(m_Button, LWEUI::Event_Released, ButtonReleasedFunc, UserData);
}


//UIToggle
void UIToggle::ToggleBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	if (isLocked()) return;
	SetToggled(!isToggled());
	if (m_Callback) m_Callback(*this, isToggled(), UserData);
	return;
}

UIToggle &UIToggle::SetLocked(bool Locked) {
	m_ToggleLockedUI->SetVisible(Locked);
	return *this;
}

UIToggle &UIToggle::SetToggled(bool Toggled) {
	m_ToggleUI->SetVisible(Toggled);
	return *this;
}

bool UIToggle::isToggled(void) const {
	return m_ToggleUI->isVisible();
}

bool UIToggle::isLocked(void) const {
	return m_ToggleLockedUI->isVisible();
}

UIToggle::UIToggle(const StackText &Name, LWEUIManager *UIMan, UIToggleCallback Callback, void *UserData) : UIItem(Name, UIMan), m_Callback(Callback) {
	m_ToggleUI = UIMan->GetNamedUIf("%s.ToggleUI", Name());
	m_ToggleLockedUI = UIMan->GetNamedUIf("%s.LockedUI", Name());
	m_Label = (LWEUILabel *)UIMan->GetNamedUIf("%s.Label", Name());
	m_Button = (LWEUIButton*)UIMan->GetNamedUIf("%s.Button", Name());

	UIMan->RegisterMethodEvent(m_Button, LWEUI::Event_Released, &UIToggle::ToggleBtnReleased, this, UserData);
}

//UIListDialog
void UIListDialog::SearchTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	Populate();
}

void UIListDialog::ListBoxReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	if (!m_SelectedFunc) return;
	uint32_t OverItem = m_ListBox->GetItemOver();
	if (OverItem == -1) return;
	LWEUIListBoxItem *Item = m_ListBox->GetItem(OverItem);
	m_SelectedFunc(*this, Item, m_UserData);
	return;
}

void UIListDialog::ScrollChanged(LWEUI *UI, uint32_t EventCode, void *UserData) {
	if (UI == m_ListSB) {
		m_ListBox->SetScroll(m_ListSB->GetScroll(), m_UIManager->GetScale());
	} else {
		m_ListSB->SetScroll(m_ListBox->GetScroll());
	}
	return;
}

void UIListDialog::ListBoxVisible(LWEUI *UI, uint32_t EventCode, void *UserData) {
	m_ListSB->SetScrollSize(m_ListBox->GetScrollPageSize());
	m_ListSB->SetVisible(m_ListSB->GetScrollSize() < m_ListSB->GetMaxScroll());
}

bool UIListDialog::PushItem(const StackText &Value, void *UserData) {
	const LWETextLine *Line = m_SearchTI->GetLine(0);
	if (Line->m_CharLength) {
		if (!LWText::Compare(Value(), Line->m_Value, Line->m_RawLength)) return false;
	}
	m_ListBox->PushItem(Value(), UserData);
	m_ListSB->SetMaxScroll(m_ListBox->GetScrollMaxSize(m_UIManager->GetScale()));
	m_ListSB->SetScrollSize(m_ListBox->GetScrollPageSize());
	m_ListSB->SetVisible(m_ListSB->GetScrollSize() < m_ListSB->GetMaxScroll());
	return true;
}


UIItem &UIListDialog::SetVisible(bool Visibile) {
	UIItem::SetVisible(Visibile);
	if (!Visibile) return *this;
	m_SearchTI->Clear();
	Populate();
	return *this;
}

void UIListDialog::Populate(void) {
	m_ListBox->Clear();
	if (m_PopulateFunc) m_PopulateFunc(*this, m_UserData);
	m_ListSB->SetScroll(0.0f);
	return;
}

UIListDialog::UIListDialog(const StackText &Name, LWEUIManager *UIMan, UISearchPopulateCallback PopulateCB, UISearchSelectedCallback SelectedCB, void *UserData) : UIItem(Name, UIMan), m_UIManager(UIMan), m_UserData(UserData), m_PopulateFunc(PopulateCB), m_SelectedFunc(SelectedCB) {
	m_ListSB = (LWEUIScrollBar *)UIMan->GetNamedUIf("%s.ListSB", Name());
	m_SearchTI = (LWEUITextInput *)UIMan->GetNamedUIf("%s.SearchTI", Name());
	m_ListBox = (LWEUIListBox *)UIMan->GetNamedUIf("%s.List", Name());

	UIMan->RegisterMethodEvent(m_ListSB, LWEUI::Event_Changed, &UIListDialog::ScrollChanged, this, UserData);
	UIMan->RegisterMethodEvent(m_ListBox, LWEUI::Event_Changed, &UIListDialog::ScrollChanged, this, UserData);
	UIMan->RegisterMethodEvent(m_SearchTI, LWEUI::Event_Changed, &UIListDialog::SearchTIChanged, this, UserData);
	UIMan->RegisterMethodEvent(m_ListBox, LWEUI::Event_Released, &UIListDialog::ListBoxReleased, this, UserData);
	UIMan->RegisterMethodEvent(m_ListBox, LWEUI::Event_Visible, &UIListDialog::ListBoxVisible, this, UserData);
}

//UIToggleGroup
void UIToggleGroup::ToggleChanged(UIToggle &T, bool Active, void *UserData) {
	uint32_t ToggleIndex = 0;
	uint32_t ActiveCount = 0;
	bool AllowMultiple = (m_Flag&AllowMultipleToggles)!=0;
	bool MustHaveOne = (m_Flag&AlwaysOneActive) != 0;

	for (uint32_t i=0; i < m_ToggleCount;i++) {
		if (&T == &m_ToggleGroup[i]) ToggleIndex = i;
		ActiveCount += m_ToggleGroup[i].isToggled() ? 1 : 0;
	}
	if (ActiveCount == 0 && MustHaveOne) { //Retoggle this button.
		T.SetToggled(true);
		return;
	}
	if (AllowMultiple) {
		if (m_ToggleChangeCallback) m_ToggleChangeCallback(*this, ToggleIndex, T, Active, m_UserData);
		return;
	}
	for (uint32_t i = 0; i < m_ToggleCount; i++) {
		if(&T==&m_ToggleGroup[i]) continue;
		if (m_ToggleGroup[i].isToggled()) {
			m_ToggleGroup[i].SetToggled(false);
			if (m_ToggleChangeCallback) m_ToggleChangeCallback(*this, i, T, false, m_UserData);
		}
	}
	if (m_ToggleChangeCallback) m_ToggleChangeCallback(*this, ToggleIndex, T, Active, m_UserData);
	return;
}

bool UIToggleGroup::PushToggle(const StackText &Name, LWEUIManager *UIMan) {
	if (m_ToggleCount >= MaxToggles) return false;
	bool MustHaveOne = (m_Flag&AlwaysOneActive) != 0;
	UIToggle::MakeMethod(m_ToggleGroup[m_ToggleCount], Name, UIMan, &UIToggleGroup::ToggleChanged, this, m_UserData);
	if (m_ToggleCount == 0 && MustHaveOne) {
		m_ToggleGroup[m_ToggleCount].SetToggled(true);
		if(m_ToggleChangeCallback) m_ToggleChangeCallback(*this, 0, m_ToggleGroup[0], true, m_UserData);
	}
	m_ToggleGroup[m_ToggleCount].SetLocked(isLocked());
	m_ToggleCount++;
	return true;
}

UIToggleGroup &UIToggleGroup::SetToggled(uint32_t i, bool Toggled) {
	if (isLocked()) return *this;
	if (i >= m_ToggleCount) return *this;
	bool AllowMultiple = (m_Flag&AllowMultipleToggles) != 0;
	bool MustHaveOne = (m_Flag&AlwaysOneActive) != 0;
	if (m_ToggleGroup[i].isToggled() == Toggled) return *this;
	uint32_t ActiveCount = 0;
	for (uint32_t i = 0; i < m_ToggleCount; i++) ActiveCount += m_ToggleGroup[i].isToggled() ? 1 : 0;
	if (ActiveCount == 1) {
		if (MustHaveOne && !Toggled) return *this;
		if (!AllowMultiple && Toggled) {
			uint32_t ToggleID = NextToggled();
			m_ToggleGroup[ToggleID].SetToggled(false);
			if (m_ToggleChangeCallback) {
				m_ToggleChangeCallback(*this, ToggleID, m_ToggleGroup[ToggleID], false, m_UserData);
			}
		}
	}
	m_ToggleGroup[i].SetToggled(Toggled);
	if (m_ToggleChangeCallback) m_ToggleChangeCallback(*this, i, m_ToggleGroup[i], Toggled, m_UserData);
	return *this;
}

UIToggleGroup &UIToggleGroup::ApplyToggledMask(uint32_t Mask) {
	for (uint32_t i = 0; i < m_ToggleCount; i++) SetToggled(i, (Mask & (1 << i)) != 0);
	return *this;
}

UIToggleGroup &UIToggleGroup::SetLocked(bool Lock) {
	m_Flag = (m_Flag&~Locked) | (Lock ? Locked : 0);
	for (uint32_t i = 0; i < m_ToggleCount; i++) m_ToggleGroup[i].SetLocked(Lock);
	return *this;
}

uint32_t UIToggleGroup::GetToggledMask(void) const {
	uint32_t Mask = 0;
	for (uint32_t i = 0; i < m_ToggleCount; i++) Mask |= m_ToggleGroup[i].isToggled() ? (1 << i) : 0;
	return Mask;
}

bool UIToggleGroup::isLocked(void) const {
	return (m_Flag&Locked) != 0;
}

bool UIToggleGroup::isToggled(uint32_t i) const {
	if (i >= m_ToggleCount) return false;
	return m_ToggleGroup[i].isToggled();
}

uint32_t UIToggleGroup::NextToggled(uint32_t Prev) {
	bool Found = Prev == -1;
	for (uint32_t i = 0; i < m_ToggleCount; i++) {
		if (m_ToggleGroup[i].isToggled() && Found) return i;
		Found = i == Prev || Found;
	}
	return -1;
}

UIToggleGroup::UIToggleGroup(uint32_t Flags, UIGroupToggleCallback ToggleChangeCallback, void *UserData) : m_Flag(Flags), m_ToggleChangeCallback(ToggleChangeCallback), m_UserData(UserData) {}