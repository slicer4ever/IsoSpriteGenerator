#ifndef STATE_H
#define STATE_H
#include <LWCore/LWTypes.h>
#include <LWPlatform/LWWindow.h>
#include <LWEUIManager.h>
#include <LWEAsset.h>

class App;

struct GFrame;

class Renderer;

class State {
public:
	enum {
		Viewer,
		Count
	};

	virtual void Update(float dTime, App *A, uint64_t lCurrentTime) = 0;

	virtual void Draw(GFrame &F, Renderer *R, LWWindow *Window, App *A) = 0;

	virtual void ProcessInput(float dTime, LWWindow *Window, App *A, uint64_t lCurrentTime) = 0;

	virtual void Activated(App *A) = 0;

	virtual void Deactivated(App *A) = 0;

	virtual void LoadAssets(LWEUIManager *UIManager, LWEAssetManager *AssetManager, App *A) = 0;

private:
};

#endif