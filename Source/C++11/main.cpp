#include <LWCore/LWAllocators/LWAllocator_Default.h>
#include <LWCore/LWAllocators/LWAllocator_DefaultDebug.h>
#include "App.h"
#include "Logger.h"

int32_t LWMain(int32_t argc, char **argv) {
	LWAllocator_Default DefAlloc;
	//LWAllocator_DefaultDebug DefAlloc;
	App *A = DefAlloc.Allocate<App>(DefAlloc);
	A->Run();
	LWAllocator::Destroy(A);
	if (DefAlloc.GetAllocatedBytes()) {
		//DefAlloc.OutputUnfreedIDs();
		LogCriticalf("Error: Memory leak, remaining bytes: %d", DefAlloc.GetAllocatedBytes());
	}

	return 0;
}