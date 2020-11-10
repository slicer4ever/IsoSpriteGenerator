#ifndef UITOOLKIT_H
#define UITOOLKIT_H
#include <LWEUIManager.h>
#include <LWEUI/LWEUIButton.h>
#include <LWEUI/LWEUILabel.h>
#include <LWEUI/LWEUIScrollBar.h>
#include <LWEUI/LWEUIListBox.h>
#include <LWEUI/LWEUITextInput.h>
#include <LWEUI/LWEUIRect.h>

struct UIToggle;

struct UIToggleGroup;

struct UIListDialog;

class UITweenRect;

typedef std::function<void(UIToggle &, bool, void *)> UIToggleCallback;
typedef std::function<void(UIToggleGroup &, uint32_t, UIToggle &, bool, void *)> UIGroupToggleCallback;

typedef std::function<void(UIListDialog &, void *)> UISearchPopulateCallback;
typedef std::function<void(UIListDialog &, LWEUIListBoxItem *, void*)> UISearchSelectedCallback;

struct UIItem {
	virtual UIItem &SetVisible(bool Visible);

	bool isVisible(void);

	UIItem(const LWUTF8Iterator &Name, LWEUIManager *UIManager);

	UIItem() = default;
	
	LWEUI *m_UI;
};

struct UILabelBtn : public UIItem {
	template<class Method, class Obj>
	static void MakeMethod(UILabelBtn &LblBtn, const LWUTF8Iterator &Name, LWEUIManager *UIManager, Method CB, Obj *O, void *UserData) {
		new (&LblBtn) UILabelBtn(Name, UIManager, std::bind(CB, O, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), UserData);
		return;
	}

	UILabelBtn(const LWUTF8Iterator &Name, LWEUIManager *UIManager, LWEUIEventCallback ButtonReleasedFunc, void *UserData);

	UILabelBtn() = default;

	LWEUILabel *m_Label;
	LWEUIButton *m_Button;
};

struct UIToggle : UIItem {
	template<class Method, class Obj>
	static void MakeMethod(UIToggle &Tgl, const LWUTF8Iterator &Name, LWEUIManager *UIManager, Method CB, Obj *O, void *UserData) {
		new (&Tgl) UIToggle(Name, UIManager, std::bind(CB, O, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), UserData);
	}

	void ToggleBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	UIToggle &SetToggled(bool Toggled);

	UIToggle &SetLocked(bool Locked);

	bool isToggled(void) const;

	bool isLocked(void) const;

	UIToggle(const LWUTF8Iterator &Name, LWEUIManager *UIMan, UIToggleCallback Callback, void *UserData);

	UIToggle() = default;

	UIToggleCallback m_Callback;
	LWEUI *m_ToggleUI;
	LWEUI *m_ToggleLockedUI;
	LWEUILabel *m_Label;
	LWEUIButton *m_Button;
};

struct UIListDialog : public UIItem {
	
	template<class MethodPop, class MethodSel, class Obj>
	static void MakeMethod(UIListDialog &Dialog, const LWUTF8Iterator &Name, LWEUIManager *UIMan, MethodPop PopulateMethod, MethodSel SelectedMethod, Obj *O, void *UserData) {
		new (&Dialog) UIListDialog(Name, UIMan, std::bind(PopulateMethod, O, std::placeholders::_1, std::placeholders::_2), std::bind(SelectedMethod, O, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), UserData);
	}

	template<class MethodPop, class MethodSel, class Obj>
	void SetMethods(MethodPop PopulateMethod, MethodSel SelectedMethod, Obj *O) {
		m_PopulateFunc = std::bind(PopulateMethod, O, std::placeholders::_1, std::placeholders::_2);
		m_SelectedFunc = std::bind(SelectedMethod, O, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		return;
	}

	void SearchTIChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	void ListBoxReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	void ScrollChanged(LWEUI *UI, uint32_t EventCode, void *UserData);

	void ListBoxVisible(LWEUI *UI, uint32_t EventCode, void *UserData);

	UIItem &SetVisible(bool Visibile);

	bool PushItem(const LWUTF8Iterator &Value, void *UserData);

	void Populate(void);

	UIListDialog(const LWUTF8Iterator &Name, LWEUIManager *UIMan, UISearchPopulateCallback PopulateCB, UISearchSelectedCallback SelectedCB, void *UserData);

	UIListDialog() = default;

	LWEUIManager *m_UIManager;
	LWEUIListBox *m_ListBox;
	LWEUIScrollBar *m_ListSB;
	LWEUITextInput *m_SearchTI;
	void *m_UserData;
	UISearchPopulateCallback m_PopulateFunc;
	UISearchSelectedCallback m_SelectedFunc;
};

struct UIToggleGroup {
	static const uint32_t AllowMultipleToggles = 0x1;
	static const uint32_t AlwaysOneActive = 0x2;
	static const uint32_t Locked = 0x4;
	static const uint32_t MaxToggles = 16;

	template<class Method, class Obj>
	static void MakeMethod(UIToggleGroup &TGroup, uint32_t Flags, Method CB, Obj *O, void *UserData) {
		new (&TGroup) UIToggleGroup(Flags, std::bind(CB, O, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), UserData);
	}

	void ToggleChanged(UIToggle &T, bool Active, void *UserData);

	bool PushToggle(const LWUTF8Iterator &Name, LWEUIManager *UIMan);

	UIToggleGroup &SetToggled(uint32_t i, bool Toggled);

	UIToggleGroup &ApplyToggledMask(uint32_t Mask);

	UIToggleGroup &SetLocked(bool Locked);

	uint32_t NextToggled(uint32_t Prev = -1);

	uint32_t GetToggledMask(void) const;

	bool isLocked(void) const;

	bool isToggled(uint32_t i) const;

	UIToggleGroup(uint32_t Flags, UIGroupToggleCallback ToggleChangeCallback, void *UserData);

	UIToggleGroup() = default;

	UIToggle m_ToggleGroup[MaxToggles];
	UIGroupToggleCallback m_ToggleChangeCallback;
	uint32_t m_Flag = 0;
	uint32_t m_ToggleCount = 0;
	void *m_UserData = nullptr;
};

#endif