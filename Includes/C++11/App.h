#ifndef APP_H
#define APP_H
#include <LWCore/LWTypes.h>
#include <LWPlatform/LWTypes.h>
#include <LWVideo/LWTypes.h>
#include <LWETypes.h>
#include <LWEJobQueue.h>
#include "State.h"

class Renderer;

class Scene;

class App {
public:
	static const uint32_t MessageFreq = 3; //Seconds.

	void UpdateJob(LWEJob &J, LWEJobThread &Th, LWEJobQueue &Q, uint64_t lCurrentTime);

	void InputJob(LWEJob &J, LWEJobThread &Th, LWEJobQueue &Q, uint64_t lCurrentTime);

	void RenderJob(LWEJob &J, LWEJobThread &Th, LWEJobQueue &Q, uint64_t lCurrentTime);

	void Run(void);

	void SetMessage(const LWUTF8Iterator &Message);

	bool LoadAssets(const LWUTF8Iterator &FilePath, const LWVideoMode &CurrMode);

	template<class Type>
	Type *GetState(uint32_t State) {
		return (Type*)m_States[State];
	}

	App &SetActiveState(uint32_t State);

	LWWindow *GetWindow(void);

	Renderer *GetRenderer(void);

	LWVideoDriver *GetVideoDriver(void);

	LWEUIManager *GetUIManager(void);

	LWAllocator &GetAllocator(void);

	App(LWAllocator &Allocator);

	~App();
private:
	LWAllocator &m_Allocator;
	LWEJobQueue m_JobQueue;
	State *m_States[State::Count];
	LWWindow *m_Window = nullptr;
	LWVideoDriver *m_Driver = nullptr;
	Renderer *m_Renderer = nullptr;
	LWEUIManager *m_UIManager = nullptr;
	LWEAssetManager *m_AssetManager = nullptr;
	LWEUILabel *m_MessageLbl = nullptr;
	uint64_t m_LastUpdateTime = -1;
	uint64_t m_LastInputTime = -1;
	uint64_t m_MessageTime = 0;
	uint32_t m_ActiveState = State::Viewer;
};

#endif