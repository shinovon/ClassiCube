#include "Window_Symbian.h"

const cc_uint8 key_map[] = {
	EStdKeyBackspace, CCKEY_BACKSPACE,
	EStdKeyTab, CCKEY_TAB,
	EStdKeyEnter, CCKEY_ENTER,
	EStdKeyEscape, CCKEY_ESCAPE,
	EStdKeySpace, CCKEY_SPACE,
	EStdKeyPrintScreen, CCKEY_PRINTSCREEN,
	EStdKeyPause, CCKEY_PAUSE,
	EStdKeyHome, CCKEY_HOME,
	EStdKeyEnd, CCKEY_END,
	EStdKeyPageUp, CCKEY_PAGEUP,
	EStdKeyPageDown, CCKEY_PAGEDOWN,
	EStdKeyInsert, CCKEY_INSERT,
	EStdKeyDelete, CCKEY_DELETE,
	EStdKeyLeftShift, CCKEY_LSHIFT,
	EStdKeyRightShift, CCKEY_RSHIFT,
	EStdKeyLeftAlt, CCKEY_LALT,
	EStdKeyRightAlt, CCKEY_RALT,
	EStdKeyLeftCtrl, CCKEY_LCTRL,
	EStdKeyRightCtrl, CCKEY_RCTRL,
	EStdKeyLeftFunc, CCKEY_LWIN,
	EStdKeyRightFunc, CCKEY_RWIN,
	EStdKeyNumLock, CCKEY_NUMLOCK,
	EStdKeyScrollLock, CCKEY_SCROLLLOCK,

	0x30, CCKEY_0,
	0x31, CCKEY_1,
	0x32, CCKEY_2,
	0x33, CCKEY_3,
	0x34, CCKEY_4,
	0x35, CCKEY_5,
	0x36, CCKEY_6,
	0x37, CCKEY_7,
	0x38, CCKEY_8,
	0x39, CCKEY_9,

	EStdKeyComma, CCKEY_COMMA,
	EStdKeyFullStop, CCKEY_PERIOD,
	EStdKeyForwardSlash, CCKEY_SLASH,
	EStdKeyBackSlash, CCKEY_BACKSLASH,
	EStdKeySemiColon, CCKEY_SEMICOLON,
	EStdKeySingleQuote, CCKEY_QUOTE,
	EStdKeyHash, '#',
	EStdKeySquareBracketLeft, CCKEY_LBRACKET,
	EStdKeySquareBracketRight, CCKEY_RBRACKET,
	EStdKeyMinus, CCKEY_MINUS,
	EStdKeyEquals, CCKEY_EQUALS,

	EStdKeyNkpForwardSlash, CCKEY_KP_DIVIDE,
	EStdKeyNkpAsterisk, CCKEY_KP_MULTIPLY,
	EStdKeyNkpMinus, CCKEY_KP_MINUS,
	EStdKeyNkpPlus, CCKEY_KP_PLUS,
	EStdKeyNkpEnter, CCKEY_KP_ENTER,
	EStdKeyNkp1, CCKEY_KP1,
	EStdKeyNkp2, CCKEY_KP2,
	EStdKeyNkp3, CCKEY_KP3,
	EStdKeyNkp4, CCKEY_KP4,
	EStdKeyNkp5, CCKEY_KP5,
	EStdKeyNkp6, CCKEY_KP6,
	EStdKeyNkp7, CCKEY_KP7,
	EStdKeyNkp8, CCKEY_KP8,
	EStdKeyNkp9, CCKEY_KP9,
	EStdKeyNkp0, CCKEY_KP0,
	EStdKeyNkpFullStop, CCKEY_KP_DECIMAL,

	EStdKeyIncVolume, CCKEY_VOLUME_UP,
	EStdKeyDecVolume, CCKEY_VOLUME_DOWN,

	EStdKeyDevice0, CCKEY_F1,
	EStdKeyDevice1, CCKEY_ESCAPE,
	EStdKeyDevice3, CCKEY_ENTER,
};

static int MapScanCode(TInt aScanCode, TInt aModifiers) {
	const TInt EModifierRotateBy90=0x00400000;
	const TInt EModifierRotateBy180=0x00800000;
	const TInt EModifierRotateBy270=0x01000000;
	
	if (aScanCode == EStdKeyLeftArrow) {
	    if (aModifiers & EModifierRotateBy90) return CCKEY_UP;
	    if (aModifiers & EModifierRotateBy180) return CCKEY_RIGHT;
	    if (aModifiers & EModifierRotateBy270) return CCKEY_DOWN;
	    return CCKEY_LEFT;
	} else if (aScanCode == EStdKeyRightArrow) {
		if (aModifiers & EModifierRotateBy90) return CCKEY_DOWN;
		if (aModifiers & EModifierRotateBy180) return CCKEY_LEFT;
		if (aModifiers & EModifierRotateBy270) return CCKEY_UP;
		return CCKEY_RIGHT;
	} else if (aScanCode == EStdKeyUpArrow) {
		if (aModifiers & EModifierRotateBy90) return CCKEY_RIGHT;
		if (aModifiers & EModifierRotateBy180) return CCKEY_DOWN;
		if (aModifiers & EModifierRotateBy270) return CCKEY_LEFT;
		return CCKEY_UP;
	} else if (aScanCode == EStdKeyDownArrow) {
		if (aModifiers & EModifierRotateBy90) return CCKEY_LEFT;
		if (aModifiers & EModifierRotateBy180) return CCKEY_UP;
		if (aModifiers & EModifierRotateBy270) return CCKEY_RIGHT;
		return CCKEY_DOWN;
	}
	
	for (size_t i = 0; i < sizeof(key_map); i += 2) {
		if (key_map[i] == aScanCode) {
			return key_map[i + 1];
		}
	}

	return aScanCode < INPUT_COUNT ? aScanCode : INPUT_NONE;
}

void CCAppUi::ConstructL() {
#ifdef CC_BUILD_SYMBIAN_AVKON
	BaseConstructL(CAknAppUi::EAknEnableSkin | ENoAppResourceFile);
#else
	BaseConstructL(ENoAppResourceFile);
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

TKeyResponse CCAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	if (!iAppContainer->events_mutex) return EKeyWasNotConsumed;
	
	CCEvent event = { 0 };
	event.i1 = MapScanCode(aKeyEvent.iScanCode, aKeyEvent.iModifiers);
	switch (aType) {
	case EEventKey: {
		int code = aKeyEvent.iCode;
		if (code != 0 && (code < ENonCharacterKeyBase || code > ENonCharacterKeyBase + ENonCharacterKeyCount)) {
			event.i1 = code;
			event.type = CC_KEY_INPUT;
			iAppContainer->PushEvent(&event);
			return EKeyWasConsumed;
		}
		break;
	}
	case EEventKeyDown: {
		if (event.i1 != INPUT_NONE) {
			event.type = CC_KEY_DOWN;
			iAppContainer->PushEvent(&event);
		}
		return EKeyWasConsumed;
	}
	case EEventKeyUp: {
		if (event.i1 != INPUT_NONE) {
			event.type = CC_KEY_UP;
			iAppContainer->PushEvent(&event);
		}
		return EKeyWasConsumed;
	}
	default:
		return EKeyWasNotConsumed;
	}
	return EKeyWasNotConsumed;
}

void CCAppUi::HandleForegroundEventL(TBool aForeground) {
	WindowInfo.Inactive = !aForeground;
	Event_RaiseVoid(&WindowEvents.InactiveChanged);
	
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
