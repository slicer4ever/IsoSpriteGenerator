#ifndef UIFILE_H
#define UIFILE_H
#include "UIToolkit.h"
#include <LWCore/LWSVector.h>

class App;

class Scene;

class Camera;

struct UIViewer;

struct UIIsometricProps;

struct UIAnimationProps;

struct Sprite {
	LWVector2i m_TexPosition = LWVector2i();
	LWVector2i m_TexSize = LWVector2i();
	LWVector4f m_ViewBounds = LWVector4f();
	LWSVector4f m_MinBounds = LWSVector4f();
	LWSVector4f m_MaxBounds = LWSVector4f();
	LWVector2f m_SpriteCenter = LWVector2f();
	uint32_t m_Direction = 0;
	float m_Time = 0.0f;

	void CalculateSpriteOffsets(const LWVector2f &WndSize, Camera &Cam);

	Sprite(const LWVector4f &ViewBounds, const LWSVector4f &MinBounds, const LWSVector4f &MaxBounds, uint32_t Direction, float Time);

	Sprite() = default;
};

struct UIFile : public UIItem {

	void Update(float dTime, LWEUIManager *UIMan, App *A);

	void ProcessInput(float dTime, LWEUIManager *UIMan, LWWindow *Window, App *A);

	void SerializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void DeserializeSettings(LWEJson &J, LWEJObject *Parent, App *A);

	void SelectFileBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	void ExportFileBtnReleased(LWEUI *UI, uint32_t EventCode, void *UserData);

	//Calculates for tight packing.
	LWVector2i CalculateTightSpriteLocations(const LWVector2f &WndSize, Scene *S, Camera &Cam, UIIsometricProps &IsoProps, UIAnimationProps &AnimProps, App *A, std::vector<Sprite> &SpriteArray);

	//Calculates for largest packing.
	LWVector2i CalculateLargestSpriteLocations(const LWVector2f &WndSize, Scene *S, Camera &Cam, UIIsometricProps &IsoProps, UIAnimationProps &AnimProps, App *A, std::vector<Sprite> &SpriteArray);

	//Calculates sprite locations on the final sprite texture, stores the location into SpriteArray(x, y, width, height).
	//returns total texture size.
	LWVector2i CalculateSpriteLocations(const LWVector2f &WndSize, App *A, std::vector<Sprite> &SpriteArray);

	//Returns the number of export settings that are enabled.
	uint32_t GetExportTypeCount(void);

	//Returns the render setting for the specified idx of the enabled idx's.
	uint32_t GetExportRenderSetting(uint32_t Idx);

	void ExportsTglChanged(UIToggleGroup &TglGroup, uint32_t ToggleID, UIToggle &Toggle, bool Toggled, void *UserData);

	void PackingTglChanged(UIToggleGroup &TglGroup, uint32_t ToggleID, UIToggle &Toggle, bool Toggled, void *UserData);

	void MetaDataTglChanged(UIToggleGroup &TglGroup, uint32_t ToggleID, UIToggle &Toggle, bool Toggled, void *UserData);

	UIFile(const StackText &Name, LWEUIManager *UIMan, UIViewer *Viewer, App *A);

	UIFile() = default;

	UILabelBtn m_SelectFileBtn;
	UILabelBtn m_ExportFileBtn;
	UIToggleGroup m_ExportTgls;
	UIToggleGroup m_PackingTgls;
	UIToggleGroup m_MetaDataTgls;
	std::vector<Sprite> m_SpriteList;
	LWEUILabel *m_TextureSizeLbl = nullptr;
	UIViewer *m_Viewer = nullptr;
	LWVector2i m_TexSize = LWVector2i();
	float m_NextUpdateTime = 0.0f;
};

#endif