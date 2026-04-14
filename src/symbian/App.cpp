#include "Window_Symbian.h"

#ifdef EKA2
TUid CCApp::AppDllUid() const {
	return KUidClassiCube;
}

CApaDocument* CCApp::CreateDocumentL() {
	return CCDocument::NewL(*this);
}

LOCAL_C CApaApplication* NewApplication();

CApaApplication* NewApplication() {
	return new CCApp;
}

GLDEF_C TInt E32Main();

TInt E32Main() {
	return EikStart::RunApplication(NewApplication);
}
#else
#ifdef __WINS__
GLDEF_C TInt E32Dll(TDllReason) {
	return KErrNone;
}

TUid CCApp::AppDllUid() const {
	return KUidClassiCube;
}

CApaDocument* CCApp::CreateDocumentL() {
	return CCDocument::NewL(*this);
}

EXPORT_C CApaApplication* NewApplication();

CApaApplication* NewApplication() {
	return new CCApp;
}

#else
TInt RunUIThread(TAny *aPtr) {
	User::LeaveIfError(RThread().Rename(_L("ClassiCube")));
	CEikonEnv* env = new CEikonEnv();
	TRAPD(err, env->ConstructL());
	User::LeaveIfError(err);
	TRAP(err, {
		CCAppUi* ui = new CCAppUi;
		ui->ConstructL();
	});
	User::LeaveIfError(err);
	env->ExecuteD();
	return 0;
}

TInt E32Main() {
	CTrapCleanup* cleanup = CTrapCleanup::New();
	RWsSession ws;
	TInt tries = 0;
	for (;;) {
		TInt err = ws.Connect();
		if (err == KErrNone) break;
		if (tries < 5) {
			User::After(300000000);
			tries++;
			continue;
		}
		User::Leave(err);
	}
	ws.Close();
	RunUIThread(NULL);
//	delete cleanup;
	return 0;
}
#endif
#endif
