#include "App.h"
#include <LWCore/LWAllocator.h>
#include <LWPlatform/LWVideoMode.h>
#include <LWVideo/LWVideoDriver.h>
#include <LWPlatform/LWWindow.h>
#include <LWCore/LWTimer.h>
#include <LWEXML.h>
#include <LWEAsset.h>
#include <LWEUIManager.h>
#include "Renderer.h"
#include "State_Viewer.h"
#include "Logger.h"
#include "Scene.h"
#include "Camera.h"

void App::UpdateJob(LWEJob &J, LWEJobThread &Th, LWEJobQueue &Q, uint64_t lCurrentTime) {
	if (m_LastUpdateTime > lCurrentTime) m_LastUpdateTime = lCurrentTime;
	float dTime = LWTimer::ToSecond(lCurrentTime - m_LastUpdateTime);
	m_LastUpdateTime = lCurrentTime;
	m_States[m_ActiveState]->Update(dTime, this, lCurrentTime);
	m_MessageLbl->SetVisible(lCurrentTime < m_MessageTime);

	GFrame *F = m_Renderer->BeginFrame();
	if (!F) return;
	m_States[m_ActiveState]->Draw(*F, m_Renderer, m_Window, this);

	m_UIManager->Draw(F->m_UIFrame, lCurrentTime);
	m_Renderer->EndFrame();
	return;
}

void App::InputJob(LWEJob &J, LWEJobThread &Th, LWEJobQueue &Q, uint64_t lCurrentTime) {
	if (m_LastInputTime > lCurrentTime) m_LastInputTime = lCurrentTime;
	float dTime = LWTimer::ToSecond(lCurrentTime - m_LastInputTime);
	m_LastInputTime = lCurrentTime;
	LWKeyboard *KB = m_Window->GetKeyboardDevice();

	m_Window->Update(lCurrentTime);
	m_JobQueue.SetFinished(KB->ButtonDown(LWKey::Esc) || m_Window->isFinished());
	m_States[m_ActiveState]->ProcessInput(dTime, m_Window, this, lCurrentTime);
	m_UIManager->Update(lCurrentTime);

	return;
}

void App::RenderJob(LWEJob &J, LWEJobThread &Th, LWEJobQueue &Q, uint64_t lCurrentTime) {
	m_Renderer->Render(m_Window);
	return;
}

void App::Run(void) {
	m_JobQueue.Start();
	m_JobQueue.RunThread(&m_JobQueue.GetMainThread(), &m_JobQueue);
	m_JobQueue.WaitForAllJoined();
	//m_JobQueue.OutputJobTimings();
	//m_JobQueue.OutputThreadTimings();
	return;
}

void App::SetMessage(const LWUTF8Iterator &Message) {
	LogEvent(Message);
	m_MessageLbl->SetText(Message);
	m_MessageTime = LWTimer::GetCurrent() + LWTimer::GetResolution() * MessageFreq;
	return;
}

bool App::LoadAssets(const LWUTF8Iterator &FilePath, const LWVideoMode &CurrMode) {
	LWEXML X;
	if (!LWEXML::LoadFile(X, m_Allocator, FilePath, true)) {
		LogCritical(LWUTF8I::Fmt<256>("Error with file: '{}'", FilePath));
		return false;
	}
	LWEAssetManager *oAssetManager = m_AssetManager;
	LWEUIManager *oUIManager = m_UIManager;
	m_AssetManager = m_Allocator.Create<LWEAssetManager>(m_Driver, nullptr, m_Allocator);
	m_UIManager = m_Allocator.Create<LWEUIManager>(m_Window, CurrMode.GetDPI().x, m_Allocator, nullptr, m_AssetManager);
	X.PushParser("AssetManager", &LWEAssetManager::XMLParser, m_AssetManager);
	X.PushParser("UIManager", &LWEUIManager::XMLParser, m_UIManager);
	X.Process();

	m_MessageLbl = (LWEUILabel *)m_UIManager->GetNamedUI("MessageLbl");

	for (uint32_t i = 0; i < State::Count; i++) {
		m_States[i]->LoadAssets(m_UIManager, m_AssetManager, this);
		m_States[i]->Deactivated(this);
	}
	m_States[m_ActiveState]->Activated(this);

	LWAllocator::Destroy(oAssetManager);
	LWAllocator::Destroy(oUIManager);

	m_Renderer->LoadAssets(m_AssetManager);
	m_Renderer->ApplySettings(RenderSettings());
	return true;
}

App &App::SetActiveState(uint32_t State) {
	m_States[m_ActiveState]->Deactivated(this);
	m_ActiveState = State;
	m_States[State]->Activated(this);
	return *this;
}

LWWindow *App::GetWindow(void) {
	return m_Window;
}

Renderer *App::GetRenderer(void) {
	return m_Renderer;
}

LWVideoDriver *App::GetVideoDriver(void) {
	return m_Driver;
}

LWEUIManager *App::GetUIManager(void) {
	return m_UIManager;
}

LWAllocator &App::GetAllocator(void) {
	return m_Allocator;
}

App::App(LWAllocator &Allocator) : m_Allocator(Allocator) {
	const char *DriverNames[] = LWVIDEODRIVER_NAMES;
	const char *PlatformNames[] = LWPLATFORM_NAMES;
	const char *ArchNames[] = LWARCH_NAMES;

	LWVideoMode CurrMode = LWVideoMode::GetActiveMode();
	LWVector2i TargetSize = LWVector2i(1280, 720);

	m_Window = m_Allocator.Create<LWWindow>("IsoSpriteGenerator", "ISG", m_Allocator, LWWindow::WindowedMode | LWWindow::KeyboardDevice | LWWindow::MouseDevice, CurrMode.GetSize() / 2 - TargetSize / 2, TargetSize);

	uint32_t TargetDriver = LWVideoDriver::OpenGL4_5 | LWVideoDriver::DirectX11_1;
	//TargetDriver |= LWVideoDriver::DebugLayer;
	m_Driver = LWVideoDriver::MakeVideoDriver(m_Window, TargetDriver);
	if (!m_Driver) {
		LogCritical("Error: Could not create video driver.");
		m_JobQueue.SetFinished(true);
		return;
	}
	m_Window->SetTitle(LWUTF8I::Fmt<256>("IsoSpriteGenerator | {} | {} | {}", DriverNames[m_Driver->GetDriverID()], PlatformNames[LWPLATFORM_ID], ArchNames[LWARCH_ID]));

	m_Renderer = m_Allocator.Create<Renderer>(m_Driver, m_Allocator);

	m_States[State::Viewer] = m_Allocator.Create<State_Viewer>(this, m_Allocator);

	if (!LoadAssets("App:UIData.xml", CurrMode)) {
		m_JobQueue.SetFinished(true);
		return;
	}

	m_JobQueue.PushJob(LWEJob::MakeMethod(&App::UpdateJob, this, nullptr, 0, 0, 0, 0, 0, 0, ~0x1));
	m_JobQueue.PushJob(LWEJob::MakeMethod(&App::InputJob, this, nullptr, 0, 0, 0, 0, 0, 0, 0x1));
	m_JobQueue.PushJob(LWEJob::MakeMethod(&App::RenderJob, this, nullptr, 0, 0, 0, 0, 0, 0, 0x1));
}

App::~App() {
	LWAllocator::Destroy((State_Viewer*)m_States[State::Viewer]);
	LWAllocator::Destroy(m_UIManager);
	LWAllocator::Destroy(m_AssetManager);
	LWAllocator::Destroy(m_Renderer);
	LWVideoDriver::DestroyVideoDriver(m_Driver);
	LWAllocator::Destroy(m_Window);
}
