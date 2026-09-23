#include "Window_Symbian.h"

void CCAppUi::ConstructL() {
#ifdef CC_BUILD_SYMBIAN_AVKON
#ifdef EKA2
	BaseConstructL(CAknAppUi::EAknEnableSkin);
#else
	BaseConstructL(CAknAppUi::EAknEnableSkin | ENoAppResourceFile);
#endif
	SetKeyBlockMode(ENoKeyBlock);
#else
	BaseConstructL();
#endif
	
	iAppContainer = new (ELeave) CCContainer;
#ifdef EKA2
	iAppContainer->SetMopParent(this);
#endif
	iAppContainer->ConstructL(ClientRect(), this);
	AddToStackL(iAppContainer);
}

CCAppUi::~CCAppUi() {
	if (iAppContainer) {
		RemoveFromStack(iAppContainer);
		delete iAppContainer;
	}
}

void CCAppUi::DynInitMenuPaneL(TInt, CEikMenuPane*) { }

void CCAppUi::HandleForegroundEventL(TBool aForeground) {
	WindowInfo.Inactive = !aForeground;
	Event_RaiseVoid(&WindowEvents.InactiveChanged);
}

TKeyResponse CCAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	if (iAppContainer->useAknWsEventMonitor) {
		return EKeyWasNotConsumed;
	}
	
	return iAppContainer->DoHandleKeyEventL(&aKeyEvent, aType);
}

void CCAppUi::HandleCommandL(TInt aCommand) {	
	switch (aCommand) {
#ifdef CC_BUILD_SYMBIAN_AVKON
	case EAknSoftkeyBack:
#endif
	case EEikCmdExit: {
		WindowInfo.Exists = false;
		Window_RequestClose();
		Exit();
		break;
	}
	default:
		break;
	}
}
