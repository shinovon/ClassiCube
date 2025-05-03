#include "Core.h"
#if defined CC_BUILD_SYMBIAN

#include <include/bitmap.h>
#include <e32base.h>
#include <coemain.h>
#include <gles/egl.h>
#include <e32keys.h>
#include <w32std.h>
#include <aknutils.h>
#include <aknnotewrappers.h>
#include <stdlib.h>
#include <stdapis/string.h>

#include "_WindowBase.h"
#include "Errors.h"
#include "Logger.h"
#include "String.h"
#include "Gui.h"

static cc_bool launcherMode;

class CWsEventReceiver;
class CWindow;

CWindow* window;

class CWindow : public CBase
{
public:
	static CWindow* NewLC();
	void HandleWsEvent(const TWsEvent& aEvent);
	void DrawFramebuffer(Rect2D r, struct Bitmap* bmp);
	~CWindow();
private:
	CWindow();
	void ConstructL();
	
	void CreateNativeWindowL();
	
	RWindow* iWindow;
	CWsEventReceiver* iWsEventReceiver;
	RWsSession iWsSession;
	RWindowGroup iWindowGroup;
	CWsScreenDevice* iWsScreenDevice;
	CWindowGc* iWindowGc;
};

class CWsEventReceiver : public CActive
{
public:
    virtual void RunL();
    virtual void DoCancel();
    static CWsEventReceiver* NewL(CWindow& aParent);
    ~CWsEventReceiver();
private:
    CWsEventReceiver();
    void ConstructL(CWindow& aParent);
private:
    RWsSession iWsSession;
    CWindow* iParent;
};

//

CWindow* CWindow::NewLC() {
	CWindow* self = new (ELeave) CWindow();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;    
}

void CWindow::CreateNativeWindowL() {
	iWindow = new (ELeave) RWindow(iWsSession);
	    
	TInt err = iWindow->Construct(iWindowGroup, reinterpret_cast<TUint32>(this));
	User::LeaveIfError(err);

	iWindowGroup.EnableScreenChangeEvents();
		
	TPixelsTwipsAndRotation pixnrot; 
	iWsScreenDevice->GetScreenModeSizeAndRotation(iWsScreenDevice->CurrentScreenMode(), pixnrot);
		
	iWindow->SetExtent(TPoint(0, 0), pixnrot.iPixelSize);
					   
	iWindow->SetRequiredDisplayMode(iWsScreenDevice->DisplayMode());
	
	iWindow->Activate();
	iWindow->SetVisible(ETrue);
	iWindow->SetNonFading(ETrue); 
	iWindow->SetShadowDisabled(ETrue); 
	iWindow->EnableRedrawStore(EFalse); 
	iWindow->EnableVisibilityChangeEvents();
	iWindow->SetNonTransparent(); 
	iWindow->SetBackgroundColor(); 
	iWindow->SetOrdinalPosition(0);
	
	iWsEventReceiver = CWsEventReceiver::NewL(*this);
	
	WindowInfo.Exists = true;
	WindowInfo.Handle.ptr = iWindow;
	
	TInt w = pixnrot.iPixelSize.iWidth,
		h = pixnrot.iPixelSize.iHeight;
	
	DisplayInfo.Width = w;
	DisplayInfo.Height = h;
	
	WindowInfo.Width = w;
	WindowInfo.Height = h;
	
	iWindowGc = new (ELeave) CWindowGc(iWsScreenDevice);
	iWindowGc->Construct();
}

CWindow::CWindow() {
	
}

CWindow::~CWindow() {
	if (iWindow != NULL) {
		iWindow->SetOrdinalPosition(KOrdinalPositionSwitchToOwningWindow);
		iWindow->Close();
		delete iWindow;
		iWindow = NULL;
	}
	
	delete iWsEventReceiver;
}

void CWindow::ConstructL() {
	CCoeEnv* env = CCoeEnv::Static();
	if (!env) {
		User::Panic(_L("CoeEnv::Static not initialized"), 0);
	}
	
	iWsSession = env->WsSession();
	iWsScreenDevice = env->ScreenDevice();
	iWindowGroup = RWindowGroup(iWsSession);
	iWindowGroup.Construct((TUint32) this, ETrue, iWsScreenDevice);
	iWindowGroup.SetName(_L("ClassiCube"));
	
	TRAPD(err, CreateNativeWindowL());
	if (err) {
		User::Panic(_L("Window creation failed"), err);
	}
	
	TDisplayMode displayMode = iWindow->DisplayMode();
	TInt bufferSize = 0;

	switch (displayMode)
	{
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

void CWindow::HandleWsEvent(const TWsEvent& aWsEvent) {
	TInt eventType = aWsEvent.Type();
	// TODO
	switch (eventType) {
	case EEventKeyDown: {
		Input_Set(aWsEvent.Key()->iScanCode, true);
		break;
	}
	case EEventKeyUp: {
		Input_Set(aWsEvent.Key()->iScanCode, false);
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
	case EEventFocusLost: {
		WindowInfo.Focused = false;
		
		Event_RaiseVoid(&WindowEvents.FocusChanged);
		break;
	}
	case EEventFocusGained: {
		WindowInfo.Focused = true;
		
		Event_RaiseVoid(&WindowEvents.FocusChanged);
		break;
	}
	case EEventWindowVisibilityChanged: {
		if (aWsEvent.Handle() == reinterpret_cast<TUint32>(this)) {
			WindowInfo.Inactive = (aWsEvent.VisibilityChanged()->iFlags & TWsVisibilityChangedEvent::ECanBeSeen) == 0;
			
			Event_RaiseVoid(&WindowEvents.StateChanged);
		}
		break;
	}
	case EEventPointer: {
		TAdvancedPointerEvent* pointer = aWsEvent.Pointer();
		
		long num = pointer->PointerNumber();
		TPoint pos = pointer->iPosition;
		if ((pointer->iModifiers & TPointerEvent::EButton1Down) != 0) {
			Input_AddTouch(num, pos.iX, pos.iY);
		} else {
			Input_RemoveTouch(num, pos.iX, pos.iY);
		}
		break;
	}
	}
}


void CWindow::DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	
	iWindow->BeginRedraw();
	iWindowGc->Activate(*iWindow);
	CFbsBitmap* bitmap = new CFbsBitmap();
	bitmap->Create(TSize(bmp->width, bmp->height), EColor16MA);
	bitmap->BeginDataAccess();
	
	TUint8* data = (TUint8*) bitmap->DataAddress();
	const TUint8* src = (TUint8*) bmp->scan0;
	for (TInt row = bmp->height - 1; row >= 0; --row) {
		memcpy(data, src, bmp->width * BITMAPCOLOR_SIZE);
		src += bmp->width * BITMAPCOLOR_SIZE;
		data += bitmap->DataStride();
	}
	
	
	bitmap->EndDataAccess();
	iWindowGc->BitBlt(TPoint(r.x, r.y), bitmap, TRect(r.x, r.y, r.width, r.height));
	iWindowGc->Deactivate();
	iWindow->EndRedraw();
	delete bitmap;
}

// window server event receiver

CWsEventReceiver::CWsEventReceiver()
: CActive(CActive::EPriorityStandard) {
}

CWsEventReceiver::~CWsEventReceiver() {
	Cancel();
}

CWsEventReceiver* CWsEventReceiver::NewL(CWindow& aParent) {
CWsEventReceiver* self = new (ELeave) CWsEventReceiver;
	CleanupStack::PushL(self);
	self->ConstructL(aParent);
	CleanupStack::Pop(self);
	return self;
}

void CWsEventReceiver::ConstructL(CWindow& aParent) {
	iParent = &aParent;
	iWsSession = CCoeEnv::Static()->WsSession();
	iWsSession.EventReady(&iStatus);
	CActiveScheduler::Add(this);
	SetActive();
}

void CWsEventReceiver::RunL() {
	TWsEvent wsEvent;
	iWsSession.GetEvent(wsEvent);
	iParent->HandleWsEvent(wsEvent);
	
	iWsSession.EventReady(&iStatus);
	
	SetActive();
}

void CWsEventReceiver::DoCancel() {
	iWsSession.EventReadyCancel();
}

//

void Window_PreInit(void) {
	MYLOG("+Window_PreInit\n")
	CCoeEnv* env = new (ELeave) CCoeEnv();
	TRAPD(err, env->ConstructL(ETrue, 0));
	
	if (err != KErrNone) {
		User::Panic(_L("Failed to create CoeEnv"), 0);
	}
	MYLOG("-Window_PreInit\n")
}

void Window_Init(void) {
	MYLOG("+Window_Init\n")
	TRAPD(err, window = CWindow::NewLC());
	if (err) {
		User::Panic(_L("Failed to initialize CWindow"), err);
	}
	
	TBool touch = AknLayoutUtils::PenEnabled();
	Input_SetTouchMode(touch);
	Gui_SetTouchUI(touch);
	
	MYLOG("-Window_Init\n")
}

void Window_Free(void) {
	MYLOG("Window_Free\n")
	CCoeEnv::Static()->DestroyEnvironment();
	delete CCoeEnv::Static();
}

void Window_Create2D(int width, int height) {
	MYLOG("Window_Create2D\n")
	launcherMode = true;
}
void Window_Create3D(int width, int height) {
	MYLOG("Window_Create3D\n")
	launcherMode = false;
}

void Window_Destroy(void) {
	MYLOG("Window_Destroy\n")
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
	// TODO
}

void Window_ProcessEvents(float delta) {
	// TODO
}

void Window_EnableRawMouse(void)  {  }

void Window_UpdateRawMouse(void)  {  }

void Window_DisableRawMouse(void) {  }

void Gamepads_Init(void) {
	
}

void Gamepads_Process(float delta) {
	
}

void ShowDialogCore(const char* title, const char* msg) {
	// TODO
	MYLOG("ShowDialog\n")
	static const cc_string t2 = String_FromConst(title);
	Logger_Log(&t2);
	static const cc_string msg2 = String_FromConst(msg);
	Logger_Log(&msg2);
	
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }

void OnscreenKeyboard_SetText(const cc_string* text) { }

void OnscreenKeyboard_Close(void) { }

void Window_LockLandscapeOrientation(cc_bool lock) { }

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
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	MYLOG("+DrawFramebuffer\n")
	window->DrawFramebuffer(r, bmp);
	MYLOG("-DrawFramebuffer\n")
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void* GLContext_GetAddress(const char* function) {
	return (void*) eglGetProcAddress(function);
}

#endif
