#include "Core.h"
#if defined CC_BUILD_SYMBIAN
extern "C" {
#include "Errors.h"
#include "Platform.h"
#include "Logger.h"
#include "String.h"
#include <unistd.h>
#include <errno.h>
}
#include <e32base.h>

extern "C" void Symbian_Init(void);

const struct UpdaterInfo Updater_Info = {
	"&eRedownload and reinstall to update", 0, NULL
};

cc_bool Updater_Clean(void) { return true; }
cc_result Updater_Start(const char** action) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_MarkExecutable(void) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) {
	return ERR_NOT_SUPPORTED;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	// Directory is already set by platform: !:/private/e212a5c2
	return 0;
}

void Platform_ShareScreenshot(const cc_string* filename) {
	
}

static CC_NOINLINE cc_uint32 CalcMemSize(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 numBytes;
	if (!numElems || !elemsSize) return 1; /* treat 0 size as 1 byte */
	
	numBytes = numElems * elemsSize;
	if (numBytes / elemsSize != numElems) return 0; /* Overflow */
	return numBytes;
}

void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? User::Alloc(size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? User::AllocZ(size) : NULL;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? User::ReAlloc(mem, size) : NULL;
}

void Mem_Free(void* mem) {
	if (mem) User::Free(mem);
}

static void ExceptionHandler(TExcType type) {
	cc_string msg; char msgB[64];
	String_InitArray(msg, msgB);
	String_AppendConst(&msg, "Exception: ");
	String_AppendInt(&msg, (int) type);
	msg.buffer[msg.length] = '\0';
	
	Logger_DoAbort(0, msg.buffer, 0);
	
	User::HandleException((TUint32*) &type);
}

void CrashHandler_Install(void) {
#if !defined _DEBUG
	User::SetExceptionHandler(ExceptionHandler, 0xffffffff);
#endif
}

#endif
