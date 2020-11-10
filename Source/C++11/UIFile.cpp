#include "UIFile.h"
#include <LWPlatform/LWWindow.h>
#include "State_Viewer.h"
#include "App.h"
#include "UIIsometricProps.h"
#include "UIAnimationProps.h"
#include <LWEJson.h>

//Sprite
void Sprite::CalculateSpriteOffsets(const LWVector2f &WndSize, Camera &Cam) {
	LWSVector4f CenterPnt = m_MinBounds + (m_MaxBounds - m_MinBounds) * 0.5f;
	LWVector2f ScreenCtr = Cam.Project(LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f), WndSize).AsVec4().xy();
	LWVector2f BoundsCenter = m_ViewBounds.xy();
	m_SpriteCenter = BoundsCenter - ScreenCtr;
	m_ViewBounds = m_ViewBounds / LWVector4f(WndSize, WndSize);
	return;
}

Sprite::Sprite(const LWVector4f &ViewBounds, const LWSVector4f &MinBounds, const LWSVector4f &MaxBounds, uint32_t Direction, float Time) : m_ViewBounds(ViewBounds), m_MinBounds(MinBounds), m_MaxBounds(MaxBounds), m_Direction(Direction), m_Time(Time) {}

//UIFile
void UIFile::Update(float dTime, LWEUIManager *UIMan, App *A) {
	const float UpdateFreq = 1.0f;//1 second.
	if (!isVisible()) {
		m_NextUpdateTime = 0.0f;
		return;
	}
	LWWindow *Wnd = A->GetWindow();
	m_NextUpdateTime -= dTime;
	if (m_NextUpdateTime < 0.0f) {
		m_TexSize = CalculateSpriteLocations(Wnd->GetSizef(), A, m_SpriteList);
		m_NextUpdateTime += UpdateFreq;
	}
	m_TextureSizeLbl->SetText(LWUTF8I::Fmt<64>("Texture Size: {}x{}", m_TexSize.x, m_TexSize.y));
	return;
}

void UIFile::ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A) {
	if (UIMan->isTextInputFocused()) return;
	LWKeyboard *KB = Window->GetKeyboardDevice();
	if (KB->ButtonDown(LWKey::LCtrl)) {
		if (KB->ButtonPressed(LWKey::O)) SelectFileBtnReleased(nullptr, 0, A);
		if (KB->ButtonPressed(LWKey::S)) ExportFileBtnReleased(nullptr, 0, A);
	}
	return;
}

void UIFile::SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	//Create bitmask of settings.
	uint32_t ExportSettings = m_ExportTgls.GetToggledMask();
	uint32_t PackingSettings = m_PackingTgls.GetToggledMask();
	uint32_t MetaSettings = m_MetaDataTgls.GetToggledMask();

	J.MakeValueElement("ExportSettings", ExportSettings, Parent);
	J.MakeValueElement("PackingSettings", PackingSettings, Parent);
	J.MakeValueElement("MetaSettings", MetaSettings, Parent);
	return;
}

void UIFile::DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A) {
	LWEJObject *JExportSettings = Parent->FindChild("ExportSettings", J);
	LWEJObject *JPackingSettings = Parent->FindChild("PackingSettings", J);
	LWEJObject *JMetaSettings = Parent->FindChild("MetaSettings", J);

	if (JExportSettings) {
		uint32_t ExportMask = JExportSettings->AsInt();
		m_ExportTgls.ApplyToggledMask(ExportMask);
	}
	if (JPackingSettings) {
		uint32_t PackingMask = JPackingSettings->AsInt();
		m_PackingTgls.ApplyToggledMask(PackingMask);
	}
	if (JMetaSettings) {
		uint32_t MetaSettings = JMetaSettings->AsInt();
		m_MetaDataTgls.ApplyToggledMask(MetaSettings);
	}
	return;
}

void UIFile::SelectFileBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	char8_t Buffer[256];
	App *A = (App*)UserData;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	if (!LWWindow::MakeLoadFileDialog("*.gltf:GLTF File:*.glb:GLB File", Buffer, sizeof(Buffer))) return;
	SV->LoadScene(Buffer, A);
	return;
}

void UIFile::ExportFileBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData) {
	char8_t Buffer[256];
	App *A = (App*)UserData;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	if (!SV->GetScene()) {
		A->SetMessage("Must load a model first.");
		return;
	}
	if (!GetExportTypeCount()) {
		A->SetMessage("No export setting selected.");
		return;
	}
	if (!LWWindow::MakeSaveFileDialog("*.png:PNG File", Buffer, sizeof(Buffer))) return;
	SV->Export(Buffer);
	return;
}

uint32_t UIFile::GetExportTypeCount(void) {
	uint32_t Count = 0;
	for (uint32_t i = 0; i < RenderCount; i++) {
		if (m_ExportTgls.isToggled(i)) Count++;
	}
	return Count;
}

uint32_t UIFile::GetExportRenderSetting(uint32_t Idx) {
	for (uint32_t i = 0; i < RenderCount; i++) {
		if (m_ExportTgls.isToggled(i)) {
			if (!Idx) return i;
			Idx--;
		}
	}
	return 0;
}

LWVector2i UIFile::CalculateTightSpriteLocations(const LWVector2f &WndSize, Scene *S, Camera &Cam, UIIsometricProps &IsoProps, UIAnimationProps &AnimProps, App *A, std::vector<Sprite> &SpriteArray){
	const int32_t BorderSize = 1;
	uint32_t DirectionCnt = IsoProps.m_DirectionCnt;
	uint32_t FrameCnt = AnimProps.m_FrameCnt;
	int32_t cY = 0; //CurrentY
	int32_t Tallest = 0; //Tallest Sprite for the line.
	int32_t Width = 0; //Longest sprite length.
	for (uint32_t i = 0; i < DirectionCnt; i++) {
		uint32_t x = 0;
		LWSMatrix4f Rotation = LWSMatrix4f::RotationY(IsoProps.CalculateDirectionTheta(i));
		for (uint32_t n = 0; n < FrameCnt; n++) {
			float Time = AnimProps.GetFrameTime(n, A);
			LWSVector4f MinBounds, MaxBounds;
			LWVector4i Bounds = S->CaclulateBounding(Time, Rotation, WndSize, Cam, BorderSize, MinBounds, MaxBounds);
			LWVector2i Size = Bounds.zw() - Bounds.xy();
			Sprite S = Sprite(Bounds.CastTo<float>(), MinBounds, MaxBounds, i, Time);
			S.CalculateSpriteOffsets(WndSize, Cam);
			S.m_TexPosition = LWVector2i(x, cY);
			S.m_TexSize = LWVector2i(Size.x, Size.y);
			SpriteArray.push_back(S);
			x += Size.x;
			Width = std::max<uint32_t>(x, Width);
			Tallest = std::max<uint32_t>(Tallest, Size.y);
		}
		cY += Tallest;
		Tallest = 0;
	}
	return LWVector2i(Width, cY);
}

LWVector2i UIFile::CalculateLargestSpriteLocations(const LWVector2f &WndSize, Scene *S, Camera &Cam, UIIsometricProps &IsoProps, UIAnimationProps &AnimProps, App *A, std::vector<Sprite> &SpriteArray){
	const int32_t BorderSize = 1;
	uint32_t DirectionCnt = IsoProps.m_DirectionCnt;
	uint32_t FrameCnt = AnimProps.m_FrameCnt;

	LWVector2i Largest = LWVector2i();
	//Calculate largest size first.
	for (uint32_t i = 0; i < DirectionCnt; i++) {
		LWSMatrix4f Rotation = LWSMatrix4f::RotationY(IsoProps.CalculateDirectionTheta(i));
		for (uint32_t n = 0; n < FrameCnt; n++) {
			float Time = AnimProps.GetFrameTime(n, A);
			LWSVector4f MinBounds, MaxBounds;
			LWVector4i Bounds = S->CaclulateBounding(Time, Rotation, WndSize, Cam, BorderSize, MinBounds, MaxBounds);
			LWVector2i Size = Bounds.zw() - Bounds.xy();
			Sprite S = Sprite(Bounds.CastTo<float>(), MinBounds, MaxBounds, i, Time);
			SpriteArray.push_back(S);
			Largest = Largest.Max(Size);
		}
	}
	for (uint32_t i = 0; i < DirectionCnt; i++) {
		for (uint32_t n = 0; n < FrameCnt; n++) {
			Sprite &S = SpriteArray[i * FrameCnt + n];
			S.m_TexPosition = LWVector2i(n * Largest.x, i * Largest.y);
			S.m_TexSize = Largest;
			//Center sprite.
			LWVector2f Size = S.m_ViewBounds.zw() - S.m_ViewBounds.xy();
			LWVector2f Offset = (S.m_TexSize.CastTo<float>() - Size)*0.5f;
			S.m_ViewBounds = S.m_ViewBounds + LWVector4f(-Offset, Offset);
			S.CalculateSpriteOffsets(WndSize, Cam);
		}
	}
	return Largest * LWVector2i(FrameCnt, DirectionCnt);
}

LWVector2i UIFile::CalculateSpriteLocations(const LWVector2f &WndSize, App *A, std::vector<Sprite> &SpriteArray) {
	const uint32_t LargestPacking = 0;
	const uint32_t TightPacking = 1;
	State_Viewer *SV = A->GetState<State_Viewer>(State::Viewer);
	Scene *S = SV->GetScene();
	Camera &Cam = SV->GetCamera();
	if (!S) return LWVector2i();
	uint32_t PackType = m_PackingTgls.NextToggled();
	UIIsometricProps &IsoProps = m_Viewer->m_IsometricProps;
	UIAnimationProps &AnimProps = m_Viewer->m_AnimationProps;
	SpriteArray.clear();
	LWVector2i TexSize = LWVector2i();
	if (PackType == LargestPacking) TexSize = CalculateLargestSpriteLocations(WndSize, S, Cam, IsoProps, AnimProps, A, SpriteArray);
	else if (PackType = TightPacking) TexSize = CalculateTightSpriteLocations(WndSize, S, Cam, IsoProps, AnimProps, A, SpriteArray);
	TexSize = LWVector2i(LWNext2N((uint32_t)TexSize.x), LWNext2N((uint32_t)TexSize.y));
	return TexSize;
}

void UIFile::ExportsTglChanged(UIToggleGroup &TglGroup, uint32_t ToggleID, UIToggle &Toggle, bool Toggled, void *UserData) {
	return;
}

void UIFile::PackingTglChanged(UIToggleGroup &TglGroup, uint32_t ToggleID, UIToggle &Toggle, bool Toggled, void *UserData) {
	m_NextUpdateTime = 0.0f;
	return;
}

void UIFile::MetaDataTglChanged(UIToggleGroup &TglGroup, uint32_t ToggleID, UIToggle &Toggle, bool Toggled, void *UserData) {
	return;
}

UIFile::UIFile(const LWUTF8Iterator &Name, LWEUIManager *UIMan, UIViewer *Viewer, App *A) : UIItem(Name, UIMan), m_Viewer(Viewer) {
	UILabelBtn::MakeMethod(m_SelectFileBtn, LWUTF8I::Fmt<128>("{}.SelectFileBtn", Name), UIMan, &UIFile::SelectFileBtnReleased, this, A);
	UILabelBtn::MakeMethod(m_ExportFileBtn, LWUTF8I::Fmt<128>("{}.ExportBtn", Name), UIMan, &UIFile::ExportFileBtnReleased, this, A);

	m_TextureSizeLbl = (LWEUILabel *)UIMan->GetNamedUI(LWUTF8I::Fmt<128>("{}.TexSizeLbl", Name));

	UIToggleGroup::MakeMethod(m_ExportTgls, UIToggleGroup::AllowMultipleToggles, &UIFile::ExportsTglChanged, this, A);
	m_ExportTgls.PushToggle(LWUTF8I::Fmt<128>("{}.ExportDefaultTgl", Name), UIMan);
	m_ExportTgls.PushToggle(LWUTF8I::Fmt<128>("{}.ExportEmissionsTgl", Name), UIMan);
	m_ExportTgls.PushToggle(LWUTF8I::Fmt<128>("{}.ExportNormalsTgl", Name), UIMan);
	m_ExportTgls.PushToggle(LWUTF8I::Fmt<128>("{}.ExportAlbedoTgl", Name), UIMan);
	m_ExportTgls.PushToggle(LWUTF8I::Fmt<128>("{}.ExportMetallicTgl", Name), UIMan);
	m_ExportTgls.SetToggled(0, true);

	UIToggleGroup::MakeMethod(m_PackingTgls, UIToggleGroup::AlwaysOneActive, &UIFile::PackingTglChanged, this, A);
	m_PackingTgls.PushToggle(LWUTF8I::Fmt<128>("{}.LargestTileTgl", Name), UIMan);
	m_PackingTgls.PushToggle(LWUTF8I::Fmt<128>("{}.TightTilesTgl", Name), UIMan);

	UIToggleGroup::MakeMethod(m_MetaDataTgls, UIToggleGroup::AllowMultipleToggles, &UIFile::MetaDataTglChanged, this, A);
	m_MetaDataTgls.PushToggle(LWUTF8I::Fmt<128>("{}.MetaCenterTgl", Name), UIMan);
	//m_MetaDataTgls.PushToggle(StackText("%s.MetaBonesTgl", Name()), UIMan);
	m_MetaDataTgls.SetToggled(0, true);

}