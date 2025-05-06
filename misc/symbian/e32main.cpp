#include <include/bitmap.h>
#include <e32std.h>
#include <string.h>
#include <stdlib.h>
#include <apgtask.h>

#include "Core.h"
#include "Constants.h"
#include "main.h"
#include "Logger.h"

static void ExceptionHandler(TExcType type) {
	MYLOG("Exception\n");
	switch(type) {
	case EExcIntegerDivideByZero:
		MYLOG("EExcIntegerDivideByZero\n");
		break;
	case EExcAccessViolation:
		MYLOG("EExcAccessViolation\n");
		break;
	case EExcStackFault:
		MYLOG("EExcStackFault\n");
		break;
	case EExcPageFault:
		MYLOG("EExcPageFault\n");
		break;
	}
	cc_string msg; char msgB[64];
	String_InitArray(msg, msgB);
	String_AppendConst(&msg, "ExcType: ");
	String_AppendInt(&msg, (int) type);
	Logger_Log(&msg);
	User::HandleException(NULL);
}

int main(int argc, char** argv) {
	CTrapCleanup *cleanup = CTrapCleanup::New();
//	SpawnPosixServerThread();
	
//	int argc = 0;
//	char **argv = 0;
//	char **envp = 0;

//	__crt0(argc, argv, envp);
	
//	_REENT;
//#ifndef _DEBUG
	User::SetExceptionHandler(ExceptionHandler, 0xffffffff);
//#endif
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
