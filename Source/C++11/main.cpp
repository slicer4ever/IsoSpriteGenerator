#include <LWCore/LWAllocators/LWAllocator_Default.h>
#include <LWCore/LWAllocators/LWAllocator_DefaultDebug.h>
#include "App.h"
#include "Logger.h"

int32_t LWMain(int32_t argc, LWUTF8Iterator *argv) {
	LWAllocator_Default DefAlloc;
	//LWAllocator_DefaultDebug DefAlloc;
	App *A = DefAlloc.Create<App>(DefAlloc);
	A->Run();
	LWAllocator::Destroy(A);
	if (DefAlloc.GetAllocatedBytes()) {
		//DefAlloc.OutputUnfreedIDs();
		LogCritical(LWUTF8I::Fmt<256>("Error: Memory leak, remaining bytes: {}", DefAlloc.GetAllocatedBytes()));
	}

	return 0;
}