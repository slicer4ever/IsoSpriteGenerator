#ifndef STATE_VIEWER_H
#define STATE_VIEWER_H
#include "State.h"
#include "Scene.h"
#include "UIViewer.h"

class State_Viewer : public State {
public:
	//PathNames appened to exports.
	static const char *RenderPathNames[];
	static const char *SettingPath;

	void Update(float dTime, App *A, uint64_t lCurrentTime);

	void Draw(GFrame &F, Renderer *R, LWWindow *Window, App *A);

	//Returns true if the frame is an export frame, otherwise false for normal rendering.
	bool ConfigureFrameExportSettings(GFrame &F, Renderer *R, LWWindow *Window, App *A);

	void ProcessInput(float dTime, LWWindow *Window, App *A, uint64_t lCurrentTime);

	void Activated(App *A);

	void Deactivated(App *A);

	void LoadAssets(LWEUIManager *UIManager, LWEAssetManager *AssetManager, App *A);

	void SetTime(float Time);

	bool LoadScene(const char *Path, App *A);

	bool LoadSettings(const char *Path, App *A);

	bool SaveSettings(const char *Path, App *A);

	bool FinalizeExport(Renderer *R, App *A);

	bool ExportMetaData(const char *ExportPathNoExt, App *A);

	bool Export(const char *ExportPath);

	void SetModelTheta(float Theta);

	Scene *GetScene(void);

	Camera &GetCamera(void);

	float GetModelTheta(void) const;

	float GetTime(void) const;

	State_Viewer(App *A, LWAllocator &Allocator);

	~State_Viewer();
private:
	char m_ExportPath[256];
	UIViewer m_UIViewer;
	Scene *m_ViewScene = nullptr;
	Scene *m_OldScene = nullptr;
	bool m_Exporting = false;
	float m_ModelTheta = 0.0f;
	std::vector<Sprite> m_ExportList;
	LWVector2i m_ExportTexSize = LWVector2i();
	uint32_t m_ExportFirstFrame = -1;
	uint32_t m_ExportFinalFrame = -1;
	float m_Time = 0.0f;
};

#endif