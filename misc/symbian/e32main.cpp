#include <include/bitmap.h>
#include <e32std.h>
#include <string.h>
#include <stdlib.h>
#include <apgtask.h>

#include "Core.h"
#include "Constants.h"
#include "main.h"

int main(int argc, char** argv) {
	CTrapCleanup *cleanup = CTrapCleanup::New();
//	SpawnPosixServerThread();
	
//	int argc = 0;
//	char **argv = 0;
//	char **envp = 0;

//	__crt0(argc, argv, envp);
	
//	_REENT;
	
	RThread currentThread;
	RProcess thisProcess;
	TParse exeName;
	exeName.Set(thisProcess.FileName(), NULL, NULL);
	currentThread.RenameMe(exeName.Name());
	//currentThread.SetProcessPriority(EPriorityLow);
	//currentThread.SetPriority(EPriorityMuchLess);
	
	TInt ret = main_real(argc, argv);
	
//	_cleanup();
	delete cleanup;
	return ret;
}
