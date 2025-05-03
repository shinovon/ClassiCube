#include "Core.h"
#if defined CC_BUILD_SYMBIAN
#include "Errors.h"
#include "Platform.h"
#include <unistd.h>
#include <errno.h>


static cc_result Process_RawGetExePath(char* path, int* len) {
	// TODO
	
	return 0;
}

const struct UpdaterInfo Updater_Info = {
	"&eRedownload and reinstall to update", 0
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

cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	// TODO
	return chdir("C:\\data\\classicube") == -1 ? errno : 0;
}

void Platform_ShareScreenshot(const cc_string* filename) {
	
}

#endif
