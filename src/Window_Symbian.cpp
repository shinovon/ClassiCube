#include "Core.h"
#if defined CC_BUILD_SYMBIAN

#include <include/bitmap.h>
#include <e32base.h>
#include <coemain.h>
#include <e32keys.h>
#include <w32std.h>
#include <aknutils.h>
#include <aknnotewrappers.h>
#include <apgwgnam.h>
#include <stdlib.h>
extern "C" {
#include <stdapis/string.h>
#include <gles/egl.h>
#include "_WindowBase.h"
#include "Errors.h"
#include "Logger.h"
#include "String.h"
#include "Gui.h"
#include "Graphics.h"
#include "Game.h"
#include "VirtualKeyboard.h"
}

static cc_bool launcherMode;

class CWindow;

CWindow* window;

class CWindow : public CBase
{
public:
	static CWindow* NewL();
	void HandleWsEvent(const TWsEvent& aEvent);
	void AllocFrameBuffer(int width, int height);
	void DrawFramebuffer(Rect2D r, struct Bitmap* bmp);
	void FreeFrameBuffer();
	void ProcessEvents(float delta);
	void RequestClose();
	void InitEvents();
	~CWindow();
	
	TWsEvent iWsEvent;
	TRequestStatus iWsEventStatus;
	RWindow* iWindow;
	
private:
	CWindow();
	void ConstructL();
	void CreateWindowL();

	RWsSession iWsSession;
	RWindowGroup iWindowGroup;
	CWsScreenDevice* iWsScreenDevice;
	CWindowGc* iWindowGc;
	CFbsBitmap* iBitmap;
	CApaWindowGroupName* iWindowGroupName;

	TBool iEventsInitialized;
};

//

CWindow* CWindow::NewL() {
	CWindow* self = new (ELeave) CWindow();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
}

void CWindow::CreateWindowL() {
	iWsScreenDevice = new (ELeave) CWsScreenDevice(iWsSession);
	User::LeaveIfError(iWsScreenDevice->Construct());

	iWindowGroup = RWindowGroup(iWsSession);
	User::LeaveIfError(iWindowGroup.Construct(reinterpret_cast<TUint32>(this) - 1));
	iWindowGroup.SetOrdinalPosition(0);
	iWindowGroup.EnableScreenChangeEvents();
#ifdef CC_BUILD_TOUCH
	iWindowGroup.EnableReceiptOfFocus(EFalse);
#else
	iWindowGroup.EnableReceiptOfFocus(ETrue);
#endif

	iWindowGroupName = CApaWindowGroupName::NewL(iWsSession, iWindowGroup.Identifier());
	iWindowGroupName->SetAppUid(TUid::Uid(0xE212A5C2));
	iWindowGroupName->SetCaptionL(_L("ClassiCube"));
	iWindowGroupName->SetHidden(EFalse);
	iWindowGroupName->SetSystem(EFalse);
	iWindowGroupName->SetWindowGroupName(iWindowGroup);

	iWindow = new (ELeave) RWindow(iWsSession);

	TInt err = iWindow->Construct(iWindowGroup, reinterpret_cast<TUint32>(this));
	User::LeaveIfError(err);

	TPixelsTwipsAndRotation pixnrot;
	iWsScreenDevice->GetScreenModeSizeAndRotation(iWsScreenDevice->CurrentScreenMode(), pixnrot);

#ifdef CC_BUILD_SYMBIAN_MULTITOUCH
	iWindow->EnableAdvancedPointers();
#endif
	iWindow->Activate();
	iWindow->SetExtent(TPoint(0, 0), pixnrot.iPixelSize);
	iWindow->SetRequiredDisplayMode(iWsScreenDevice->DisplayMode());
	iWindow->SetVisible(ETrue);
	iWindow->EnableVisibilityChangeEvents();
	iWindow->PointerFilter(EPointerFilterDrag, 0);
	
	RWindowGroup rootWin = CCoeEnv::Static()->RootWin();
	CApaWindowGroupName* rootWindGroupName = 0;
	TRAP_IGNORE(rootWindGroupName = CApaWindowGroupName::NewL(iWsSession, rootWin.Identifier()));
	if (rootWindGroupName) {
		rootWindGroupName->SetHidden(ETrue);
		rootWindGroupName->SetWindowGroupName(rootWin);
	}

	WindowInfo.Focused = true;
	WindowInfo.Exists = true;
	WindowInfo.Handle.ptr = (void*) iWindow;
	
	TInt w = pixnrot.iPixelSize.iWidth,
		h = pixnrot.iPixelSize.iHeight;
	
	DisplayInfo.Width = w;
	DisplayInfo.Height = h;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width = w;
	WindowInfo.Height = h;
	WindowInfo.UIScaleX = DEFAULT_UI_SCALE_X;
	WindowInfo.UIScaleY = DEFAULT_UI_SCALE_Y;
	WindowInfo.SoftKeyboard = SOFT_KEYBOARD_VIRTUAL;
	
	iWsSession.EventReadyCancel();
}

CWindow::CWindow() {
	
}

CWindow::~CWindow() {
	if (iWindowGc) {
		delete iWindowGc;
		iWindowGc = NULL;
	}
	if (iWindow) {
		iWindow->SetOrdinalPosition(KOrdinalPositionSwitchToOwningWindow);
		iWindow->Close();
		delete iWindow;
		iWindow = NULL;
	}
}

void CWindow::ConstructL() {
	CCoeEnv* env = CCoeEnv::Static();
	if (!env) {
		User::Panic(_L("CoeEnv::Static not initialized"), 0);
	}
	
	iWsSession = env->WsSession();
	
	TRAPD(err, CreateWindowL());
	if (err) {
		User::Panic(_L("Window creation failed"), err);
	}
	
	TDisplayMode displayMode = iWindow->DisplayMode();
	TInt bufferSize = 0;

	switch (displayMode) {
	case EColor4K:
		bufferSize = 12;
		break;
	case EColor64K:
		bufferSize = 16;
		break;
	case EColor16M:
		bufferSize = 24;
		break;
	case EColor16MU:
	case EColor16MA:
		bufferSize = 32;
		break;
	default:
		break;
	}
	
	DisplayInfo.Depth = bufferSize;
}

static int ConvertKey(TInt aScanCode) {
	// TODO
	switch (aScanCode) {
	case 0x30:
		return CCKEY_0;
	case 0x31:
		return CCKEY_1;
	case 0x32:
		return CCKEY_2;
	case 0x33:
		return CCKEY_3;
	case 0x34:
		return CCKEY_4;
	case 0x35:
		return CCKEY_5;
	case 0x36:
		return CCKEY_6;
	case 0x37:
		return CCKEY_7;
	case 0x38:
		return CCKEY_8;
	case 0x39:
		return CCKEY_9;
	case EStdKeyUpArrow:
		return CCKEY_UP;
	case EStdKeyDownArrow:
		return CCKEY_DOWN;
	case EStdKeyLeftArrow:
		return CCKEY_LEFT;
	case EStdKeyRightArrow:
		return CCKEY_RIGHT;
	case EStdKeyBackspace:
		return CCKEY_BACKSPACE;
//	case EStdKeyDevice0:
//		return CCKEY_MENU;
	case EStdKeyDevice1:
		return CCKEY_ESCAPE;
	case EStdKeyDevice3:
		return CCKEY_ENTER;
	}
	
	return aScanCode;
}

void CWindow::HandleWsEvent(const TWsEvent& aWsEvent) {
	TInt eventType = aWsEvent.Type();
	switch (eventType) {
	case EEventKeyDown: {
		Input_Set(ConvertKey(aWsEvent.Key()->iScanCode), true);
		break;
	}
	case EEventKeyUp: {
		Input_Set(ConvertKey(aWsEvent.Key()->iScanCode), false);
		break;
	}
	case EEventScreenDeviceChanged: {
		TPixelsTwipsAndRotation pixnrot; 
		iWsScreenDevice->GetScreenModeSizeAndRotation(iWsScreenDevice->CurrentScreenMode(), pixnrot);
		if (pixnrot.iPixelSize != iWindow->Size()) {
			iWindow->SetExtent(TPoint(0, 0), pixnrot.iPixelSize);
			
			TInt w = pixnrot.iPixelSize.iWidth,
				h = pixnrot.iPixelSize.iHeight;
			
			DisplayInfo.Width = w;
			DisplayInfo.Height = h;
			
			WindowInfo.Width = w;
			WindowInfo.Height = h;
			
			Event_RaiseVoid(&WindowEvents.Resized);
		}
		break;
	}
//	case EEventFocusLost: {
//		if (!WindowInfo.Focused) break;
//		WindowInfo.Focused = false;
//		
//		Event_RaiseVoid(&WindowEvents.FocusChanged);
//		break;
//	}
//	case EEventFocusGained: {
//		if (WindowInfo.Focused) break;
//		WindowInfo.Focused = true;
//		
//		Event_RaiseVoid(&WindowEvents.FocusChanged);
//		break;
//	}
//	case EEventWindowVisibilityChanged: {
//		if (aWsEvent.Handle() == reinterpret_cast<TUint32>(this)) {
//			WindowInfo.Inactive = (aWsEvent.VisibilityChanged()->iFlags & TWsVisibilityChangedEvent::ECanBeSeen) == 0;
//
//			Event_RaiseVoid(&WindowEvents.InactiveChanged);
//		}
//		break;
//	}
#ifdef CC_BUILD_TOUCH
	case EEventPointer: {
#ifdef CC_BUILD_SYMBIAN_MULTITOUCH
		TAdvancedPointerEvent* pointer = aWsEvent.Pointer();
		long num = pointer->IsAdvancedPointerEvent() ? pointer->PointerNumber() : 0;
#else
		TPointerEvent* pointer = aWsEvent.Pointer();
		long num = 0;
#endif
		TPoint pos = pointer->iPosition;
		switch (pointer->iType) {
		case TPointerEvent::EButton1Down:
			Input_AddTouch(num, pos.iX, pos.iY);
			break;
		case TPointerEvent::EDrag:
			Input_AddTouch(num, pos.iX, pos.iY);
			break;
		case TPointerEvent::EButton1Up:
			Input_RemoveTouch(num, pos.iX, pos.iY);
			break;
		default:
			break;
		}
		break;
	}
#endif
	}
}

void CWindow::AllocFrameBuffer(int width, int height) {
	FreeFrameBuffer();
	if (!iWindowGc) {
		iWindowGc = new (ELeave) CWindowGc(iWsScreenDevice);
		iWindowGc->Construct();
	}
	iBitmap = new CFbsBitmap();
	iBitmap->Create(TSize(width, height), EColor16MA);
}

void CWindow::FreeFrameBuffer() {
	if (iWindowGc != NULL) {
		delete iWindowGc;
		iWindowGc = NULL;
	}
	if (iBitmap != NULL) {
		delete iBitmap;
		iBitmap = NULL;
	}
}

void CWindow::DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	if (!iWindowGc) return;
	iWindow->Invalidate(/*TRect(r.x, r.y, r.width, r.height)*/);
	iWindow->BeginRedraw();
	iWindowGc->Activate(*iWindow);

	if (iBitmap) {
//		iBitmap->BeginDataAccess();
		iBitmap->LockHeap();
		TUint8* data = (TUint8*) iBitmap->DataAddress();
		const TUint8* src = (TUint8*) bmp->scan0;
		for (TInt row = bmp->height - 1; row >= 0; --row) {
			memcpy(data, src, bmp->width * BITMAPCOLOR_SIZE);
			src += bmp->width * BITMAPCOLOR_SIZE;
			data += iBitmap->DataStride();
		}
//		iBitmap->EndDataAccess();
		iBitmap->UnlockHeap();
		
		iWindowGc->BitBlt(TPoint(0, 0), iBitmap/*, TRect(r.x, r.y, r.width, r.height)*/);
	}
	iWindowGc->Deactivate();
	iWindow->EndRedraw();
	iWsSession.Flush();
}

void CWindow::ProcessEvents(float delta) {
	while (iWsEventStatus != KRequestPending) {
		iWsSession.GetEvent(window->iWsEvent);
		HandleWsEvent(window->iWsEvent);
		iWsEventStatus = KRequestPending;
		iWsSession.EventReady(&iWsEventStatus);
	}
}

void CWindow::RequestClose() {
	Event_RaiseVoid(&WindowEvents.Closing);
}

void CWindow::InitEvents() {
	iWindow->Invalidate();
	iWindow->BeginRedraw();
	iWindow->EndRedraw();
	iWsSession.Flush();
	if (iEventsInitialized)
		return;
	iEventsInitialized = ETrue;
	iWsEventStatus = KRequestPending;
	iWsSession.EventReady(&iWsEventStatus);
}

//

void Window_PreInit(void) {
	CCoeEnv* env = new (ELeave) CCoeEnv();
	TRAPD(err, env->ConstructL(ETrue, 0));
	
	if (err != KErrNone) {
		User::Panic(_L("Failed to create CoeEnv"), 0);
	}
}

void Window_Init(void) {
	TRAPD(err, window = CWindow::NewL());
	if (err) {
		User::Panic(_L("Failed to initialize CWindow"), err);
	}
	
#ifdef CC_BUILD_TOUCH
	//TBool touch = AknLayoutUtils::PenEnabled();
	
	bool touch = true;
	Input_SetTouchMode(touch);
	Gui_SetTouchUI(touch);
#endif
}

void Window_Free(void) {
	CCoeEnv::Static()->DestroyEnvironment();
	delete CCoeEnv::Static();
}

void Window_Create2D(int width, int height) {
	launcherMode = true;
	window->InitEvents();
}
void Window_Create3D(int width, int height) {
	launcherMode = false;
	window->InitEvents();
}

void Window_Destroy(void) {
}

void Window_SetTitle(const cc_string* title) { }

void Clipboard_GetText(cc_string* value) { }

void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) { return WINDOW_STATE_FULLSCREEN; }

cc_result Window_EnterFullscreen(void) { return 0; }

cc_result Window_ExitFullscreen(void)  { return 0; }

int Window_IsObscured(void)            { return 0; }

void Window_Show(void) { }

void Window_SetSize(int width, int height) { }

void Window_RequestClose(void) {
	window->RequestClose();
}

void Window_ProcessEvents(float delta) {
	window->ProcessEvents(delta);
}

void Window_EnableRawMouse(void)  { Input.RawMode = true; }

void Window_UpdateRawMouse(void)  {  }

void Window_DisableRawMouse(void) { Input.RawMode = false; }

void Gamepads_Init(void) {
	
}

void Gamepads_Process(float delta) {
	
}

void ShowDialogCore(const char* title, const char* msg) {
	// TODO
	static const cc_string t2 = String_Init((char*) title, String_Length(title), String_Length(title));
	Logger_Log(&t2);
	static const cc_string msg2 = String_Init((char*) msg, String_Length(msg), String_Length(msg));
	Logger_Log(&msg2);
	
}

// TODO

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
	VirtualKeyboard_Open(args, launcherMode);
}

void OnscreenKeyboard_SetText(const cc_string* text) {
	VirtualKeyboard_SetText(text);
}

void OnscreenKeyboard_Close(void) {
	VirtualKeyboard_Close();
}

void Window_LockLandscapeOrientation(cc_bool lock) {
	// TODO
}

static void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
void Cursor_SetPosition(int x, int y) { }
static void Cursor_DoSetVisible(cc_bool visible) { }

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "bitmap");
	bmp->width  = width;
	bmp->height = height;
	window->AllocFrameBuffer(width, height);
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	window->DrawFramebuffer(r, bmp);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	window->FreeFrameBuffer();
	Mem_Free(bmp->scan0);
}

void GLContext_Create(void) {
	static EGLint attribs[] = {
		EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
		EGL_BUFFER_SIZE,       DisplayInfo.Depth,
		EGL_DEPTH_SIZE,        16,
		EGL_NONE
	};

	ctx_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(ctx_display, NULL, NULL);
	EGLint numConfigs;
	
	eglChooseConfig(ctx_display, attribs, &ctx_config, 1, &numConfigs);
	ctx_context = eglCreateContext(ctx_display, ctx_config, EGL_NO_CONTEXT, NULL);
	if (!ctx_context) Process_Abort2(eglGetError(), "Failed to create EGL context");
	GLContext_InitSurface();
}

#endif
