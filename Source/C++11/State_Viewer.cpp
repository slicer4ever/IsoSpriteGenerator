#include "State_Viewer.h"
#include <LWVideo/LWVideoDriver.h>
#include "App.h"
#include "Light.h"
#include "Logger.h"
#include "UICameraControls.h"
#include "UILightingProps.h"
#include <LWEJson.h>


const char *State_Viewer::RenderPathNames[] = { "", "_Emissions", "_Normals", "_Albedo", "_MetallicRough" };
const char *State_Viewer::SettingPath = "App:Settings.json";

void State_Viewer::Update(float dTime, App *A, uint64_t lCurrentTime) {
	LWEUIManager *UIMan = A->GetUIManager();
	if (!m_ViewScene) return;
	m_UIViewer.Update(dTime, UIMan, A);
	return;
}

void State_Viewer::Draw(GFrame &F, Renderer *R, LWWindow *Window, App *A) {
	const int32_t BorderSize = 1;
	const float SunDistance = 25.0f;
	LWVector2f WndSize = Window->GetSizef();
	if (!m_ViewScene) return;
	UICameraControls &CamCtrls = m_UIViewer.m_CameraProps;
	UILightingProps &LightProps = m_UIViewer.m_LightingProps;
	Camera &Cam = CamCtrls.m_Camera;
	Light SunLight;
	Cam.SetAspect(Window->GetAspect()).BuildFrustrum();
	if(!ConfigureFrameExportSettings(F, R, Window, A)){
		//Use current camera output setting if not an export frame.
		F.m_GlobalData.RenderOutput = CamCtrls.m_OutputTglGroup.NextToggled();
	}
	//Use IBL or Sun directional light.
	if (LightProps.isUsingIBL()) F.m_GlobalData.RenderOutput |= RenderIBLFlag;
	else {
		SunLight = LightProps.m_SunProps.MakeLightSource();
		F.PushLight(SunLight);
	}

	//Initialize required render passes.
	F.InitializePass(GFrame::MainViewPass, Cam);
	F.InitializePass(GFrame::OutlinePass, Cam);
	//Initialize shadow render pass.
	F.InitializeRTPasses(LWSVector4f(-10.0f), LWSVector4f(10.0f));
	m_ViewScene->DrawScene(F, R, m_Time, ~GFrame::OutlineBits, LWSMatrix4f::RotationY(m_ModelTheta));

	//Draw sun
	if (!m_Exporting) {
		if (LightProps.isUsingSun() && LightProps.m_SunProps.m_DrawSunTgl.isToggled()) {
			LWSVector4f Pos = LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f) - SunLight.m_Direction * SunDistance;
			R->WriteDebugCone(F, GFrame::MainViewBits, Pos, SunLight.m_Direction, LW_PI_4, 5.0f, LWVector4f(1.0f));
		}
	}
	
	//Draw tight bounding volume.
	LWSVector4f MinBounds, MaxBounds;
	LWVector4i B = m_ViewScene->CaclulateBounding(m_Time, LWSMatrix4f::RotationY(m_ModelTheta), WndSize, Cam, BorderSize, MinBounds, MaxBounds);
	LWVector4f Bf = B.CastTo<float>();
	LWEUIMaterial Mat = LWEUIMaterial(LWVector4f(0.0f, 0.0f, 1.0f, 1.0f));
	LWVector2f BL = LWVector2f(Bf.x, Bf.y);
	LWVector2f BR = LWVector2f(Bf.z, Bf.y);
	LWVector2f TL = LWVector2f(Bf.x, Bf.w);
	LWVector2f TR = LWVector2f(Bf.z, Bf.w);
	F.m_UIFrame.WriteLine(&Mat, BL, BR, 2.0f);
	F.m_UIFrame.WriteLine(&Mat, BL, TL, 2.0f);
	F.m_UIFrame.WriteLine(&Mat, TL, TR, 2.0f);
	F.m_UIFrame.WriteLine(&Mat, BR, TR, 2.0f);
	return;
}

bool State_Viewer::ConfigureFrameExportSettings(GFrame &F, Renderer *R, LWWindow *Window, App *A) {
	if (!m_Exporting) return false;
	LWVector2f WndSize = Window->GetSizef();
	UIFile &FileProps = m_UIViewer.m_FileProps;
	UIIsometricProps &IsoProps = m_UIViewer.m_IsometricProps;
	uint32_t ExportCnt = FileProps.GetExportTypeCount();
	if (m_ExportFirstFrame == -1) {
		//Initialize exporting sprites.
		m_ExportFirstFrame = F.m_FrameID;
		m_ExportTexSize = FileProps.CalculateSpriteLocations(WndSize, A, m_ExportList);
		m_ExportFinalFrame = m_ExportFirstFrame + (uint32_t)m_ExportList.size() * ExportCnt;
		if (!m_ExportList.size()) {
			A->SetMessage("Error: Something went wrong calculating sprite sizes.");
			m_Exporting = false;
			return false;
		}
	}
	uint32_t ID = F.m_FrameID - m_ExportFirstFrame;
	uint32_t ExportID = ID / (uint32_t)m_ExportList.size();
	if (ExportID >= ExportCnt) return false;
	//Get current sprite index for rendering setting..
	ID = ID % m_ExportList.size();
	F.m_SpriteFrame = ID;
	F.m_GlobalData.RenderOutput = FileProps.GetExportRenderSetting(ExportID);

	Sprite &S = m_ExportList[ID];
	m_Time = S.m_Time;
	m_ModelTheta = IsoProps.CalculateDirectionTheta(S.m_Direction);
	F.m_TargetTextureSize = m_ExportTexSize;
	F.m_TargetViewBounds = LWVector4i(S.m_TexPosition, S.m_TexSize);
	F.m_ViewBounds = S.m_ViewBounds;
	return true;
}

void State_Viewer::ProcessInput(float dTime, LWWindow *Window, App *A, uint64_t lCurrentTime) {
	LWEUIManager *UIMan = A->GetUIManager();
	if (m_Exporting) {
		if (m_ExportFinalFrame == -1) return;
		FinalizeExport(A->GetRenderer(), A);
		return;
	}
	m_UIViewer.ProcessInput(dTime, UIMan, Window, A);
	return;
}

void State_Viewer::Activated(App *A) {
	m_UIViewer.SetVisible(true);
	return;
}

void State_Viewer::Deactivated(App *A) {
	m_UIViewer.SetVisible(false);
	return;
}

void State_Viewer::LoadAssets(LWEUIManager *UIManager, LWEAssetManager *AssetManager, App *A) {
	new (&m_UIViewer) UIViewer("UIViewer", UIManager, A);
	LoadSettings(SettingPath, A);
	return;
}

bool State_Viewer::LoadScene(const char *Path, App *A) {
	LWAllocator &Alloc = A->GetAllocator();
	//Dispose of very old scene if it's still around.
	m_OldScene =LWAllocator::Destroy(m_OldScene);
	Scene *S = Alloc.Allocate<Scene>();
	if (!Scene::LoadGLTFFile(*S, Path, A->GetRenderer(), Alloc)) {
		A->SetMessage("Error loading gltf model.");
		LWAllocator::Destroy(S);
		m_OldScene = nullptr;
		return false;
	}
	A->SetMessage("Loaded model.");
	//Push previous scene to old scene to prevent data races until oldscene is fully cleared.
	m_Time = 0.0f;
	m_OldScene = m_ViewScene;
	m_ViewScene = S;
	return true;
}

void State_Viewer::SetTime(float Time) {
	m_Time = Time;
	return;
}

bool State_Viewer::FinalizeExport(Renderer *R, App *A) {
	//Additional output names.
	char PathNoExt[256]="";
	char Ext[256]="";
	if (m_ExportFinalFrame >= R->GetCurrentRenderedFrame()) return false;
	LWAllocator &Alloc = A->GetAllocator();
	LWVideoDriver *Driver = A->GetVideoDriver();
	LWTexture *OutputTex = R->GetOutputTexture();
	UIFile &UIFileProps = m_UIViewer.m_FileProps;
	if (!OutputTex) {
		A->SetMessage("Error: Texture failed to create(possibly too large.)");
		m_Exporting = false;
		return false;
	}
	//Strip off extension:
	LWFileStream::GetExtension(m_ExportPath, Ext, sizeof(Ext));
	uint32_t ExtLen = (uint32_t)strlen(Ext);
	if (ExtLen) ExtLen += 1; //Get the '.'
	uint32_t Len = (uint32_t)strlen(m_ExportPath)-ExtLen;
	strncat(PathNoExt, m_ExportPath, Len);

	//Download image from gpu and write output.
	LWImage OutputImg = LWImage(OutputTex->Get2DSize(), OutputTex->GetPackType(), nullptr, 0, A->GetAllocator());
	for (uint32_t i = 0; i < RenderCount; i++) {
		if (!UIFileProps.m_ExportTgls.isToggled(i)) continue;
		if (!Driver->DownloadTexture2DArray(OutputTex, 0, i, OutputImg.GetTexels(0))) {
			A->SetMessage("Error occurred while exporting.");
			m_Exporting = false;
			return false;
		}
		//Add final extensions.
		*Ext = '\0';
		snprintf(Ext, sizeof(Ext), "%s%s.png", PathNoExt, RenderPathNames[i]);
		if (!LWImage::SaveImagePNG(OutputImg, Ext, A->GetAllocator())) {
			A->SetMessage(StackText("Error occurred saving file '%s'", m_ExportPath));
			m_Exporting = false;
			return false;
		}
	}
	//Export Meta-data:
	if (!ExportMetaData(PathNoExt, A)) {
		m_Exporting = false;
		return true;
	}
	//Save settings.
	SaveSettings(SettingPath, A);
	m_Exporting = false;
	A->SetMessage("Finished exporting.");
	return true;
}

bool State_Viewer::ExportMetaData(const char *ExportPathNoExt, App *A) {
	const char *RenderImageNames[] = { "Color", "Emissions", "Normals", "Albedo", "Metallic" };
	char PathBuffer[256];
	char Buffer[1024 * 128]; //128kb buffer for json.
	UIFile &UIFileProps = m_UIViewer.m_FileProps;
	UIIsometricProps &IsoProps = m_UIViewer.m_IsometricProps;
	UIAnimationProps &AnimProps = m_UIViewer.m_AnimationProps;
	bool isCenterProps = UIFileProps.m_MetaDataTgls.isToggled(0);
	uint32_t DirectionCnt = IsoProps.m_DirectionCnt;
	uint32_t FrameCnt = AnimProps.m_FrameCnt;

	LWAllocator &Alloc = A->GetAllocator();
	LWEJson J = LWEJson(Alloc);
	LWFileStream::MakeFileName(ExportPathNoExt, PathBuffer, sizeof(PathBuffer));
	for (uint32_t i = 0; i < RenderCount; i++) {
		if (!UIFileProps.m_ExportTgls.isToggled(i)) continue;
		J.MakeStringElementf(RenderImageNames[i], "%s%s.png", nullptr, PathBuffer, RenderPathNames[i]);
	}
	J.MakeValueElement("TotalTime", m_ViewScene->GetTotalTime());
	J.MakeValueElement("TimeOffset", AnimProps.m_Offset);
	J.MakeValueElement("RotationOffset", IsoProps.m_ThetaOffset * LW_RADTODEG);
	LWEJObject *JFramesObj = J.MakeArrayElement("Frames", nullptr);
	for (uint32_t i = 0; i < FrameCnt; i++) {
		float Time = AnimProps.GetFrameTime(i, A);
		LWEJObject *JFrameObj = J.PushArrayObjectElement(JFramesObj);
		J.MakeValueElement("Time", Time, JFrameObj);
		LWEJObject *JSpritesObj = J.MakeArrayElement("Sprites", JFrameObj);
		for (uint32_t n = 0; n < DirectionCnt; n++) {
			Sprite &S = m_ExportList[n * FrameCnt + i];
			LWEJObject *JSpriteObj = J.PushArrayObjectElement(JSpritesObj);
			J.MakeValueElement("x", S.m_TexPosition.x, JSpriteObj);
			J.MakeValueElement("y", S.m_TexPosition.y, JSpriteObj);
			J.MakeValueElement("width", S.m_TexSize.x, JSpriteObj);
			J.MakeValueElement("height", S.m_TexSize.y, JSpriteObj);
			if (isCenterProps) {
				J.MakeValueElement("xOffset", S.m_SpriteCenter.x, JSpriteObj);
				J.MakeValueElement("yOffset", S.m_SpriteCenter.y, JSpriteObj);
			}
		}
	}

	uint32_t Len = J.Serialize(Buffer, sizeof(Buffer), true);
	snprintf(PathBuffer, sizeof(PathBuffer), "%s.json", ExportPathNoExt);
	LWFileStream Stream;
	if (!LWFileStream::OpenStream(Stream, PathBuffer, LWFileStream::WriteMode | LWFileStream::BinaryMode, Alloc, nullptr)) {
		A->SetMessage(StackText("Error occurred while opening file '%s'", PathBuffer));
		return false;
	}
	Stream.Write(Buffer, Len);
	return true;
}

bool State_Viewer::SaveSettings(const char *Path, App *A) {
	char Buffer[1024 * 128]; //128kb buffer.
	LWAllocator &Alloc = A->GetAllocator();
	LWFileStream Stream;
	LWEJson J = LWEJson(Alloc);
	m_UIViewer.SerializeSettings(J, nullptr, A);
	uint32_t Len = J.Serialize(Buffer, sizeof(Buffer), true);
	if (!LWFileStream::OpenStream(Stream, Path, LWFileStream::WriteMode | LWFileStream::BinaryMode, Alloc)) {
		LogWarnf("Error: Could not open setting file: '%s' to write to.", Path);
		return false;
	}
	Stream.Write(Buffer, Len);
	return true;
}

bool State_Viewer::LoadSettings(const char *Path, App *A) {
	LWAllocator &Alloc = A->GetAllocator();
	LWEJson J = LWEJson(Alloc);
	if (!LWEJson::LoadFile(J, Path, Alloc, nullptr)) {
		LogWarnf("Error: Could not load/parse settings json file: '%s'", Path);
		return false;
	}
	m_UIViewer.DeserializeSettings(J, nullptr, A);
	return true;
}

bool State_Viewer::Export(const char *ExportPath) {
	*m_ExportPath = '\0';
	//Initialize export settings.
	strncat(m_ExportPath, ExportPath, sizeof(m_ExportPath));
	m_ExportFirstFrame = -1;
	m_ExportFinalFrame = -1;
	m_Exporting = true;
	return true;
}

void State_Viewer::SetModelTheta(float Theta) {
	m_ModelTheta = Theta;
	return;
}

Scene *State_Viewer::GetScene(void) {
	return m_ViewScene;
}

Camera &State_Viewer::GetCamera(void) {
	return m_UIViewer.m_CameraProps.m_Camera;
}

float State_Viewer::GetModelTheta(void) const {
	return m_ModelTheta;
}

float State_Viewer::GetTime(void) const {
	return m_Time;
}

State_Viewer::State_Viewer(App *A, LWAllocator &Allocator) {
}

State_Viewer::~State_Viewer() {
	LWAllocator::Destroy(m_OldScene);
	LWAllocator::Destroy(m_ViewScene);
}