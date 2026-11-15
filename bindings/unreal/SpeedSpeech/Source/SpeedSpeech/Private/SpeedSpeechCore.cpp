// Compiles the SpeedSpeech C core into this module (as C++, one translation unit).
#include "CoreMinimal.h"

// Route the synthesizer's allocations through Unreal's allocator.
#define SS_MALLOC(Size) FMemory::Malloc(Size)
#define SS_REALLOC(Ptr, Size) FMemory::Realloc((Ptr), (Size))
#define SS_FREE(Ptr) FMemory::Free(Ptr)

THIRD_PARTY_INCLUDES_START
#include "speedspeech.c"
THIRD_PARTY_INCLUDES_END

#undef SS_MALLOC
#undef SS_REALLOC
#undef SS_FREE
