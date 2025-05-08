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
}

static cc_bool launcherMode;

class CWindow;
class CWsEventReceiver;

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
    CWsEventReceiver* iWsEventReceiver;
    
    TBool iEventsInitialized;
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
	User::LeaveIfError(iWindowGroup.Construct(reinterpret_cast<TUint32>(this)));
	iWindowGroup.SetOrdinalPosition(0);
	iWindowGroup.EnableScreenChangeEvents();
	iWindowGroup.EnableReceiptOfFocus(EFalse);
	
	iWindowGroupName = CApaWindowGroupName::NewL(iWsSession, iWindowGroup.Identifier());
	iWindowGroupName->SetAppUid(TUid::Uid(0xE212A5C2));
	iWindowGroupName->SetCaptionL(_L("ClassiCube"));
	iWindowGroupName->SetHidden(EFalse);
	iWindowGroupName->SetSystem(EFalse);
	iWindowGroupName->SetWindowGroupName(iWindowGroup);
	
	iWindow = new (ELeave) RWindow(iWsSession);
	    
	TInt err = iWindow->Construct(iWindowGroup, reinterpret_cast<TUint32>(this) - 1);
	User::LeaveIfError(err);
		
	TPixelsTwipsAndRotation pixnrot;
	iWsScreenDevice->GetScreenModeSizeAndRotation(iWsScreenDevice->CurrentScreenMode(), pixnrot);
	
	iWindow->Activate();
	iWindow->SetExtent(TPoint(0, 0), pixnrot.iPixelSize);
	iWindow->SetRequiredDisplayMode(iWsScreenDevice->DisplayMode());
	iWindow->SetVisible(ETrue);
	iWindow->EnableVisibilityChangeEvents();
	
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
	
	WindowInfo.Width = w;
	WindowInfo.Height = h;
	
	//iWsSession.EventReadyCancel();
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
	if (iWsEventReceiver) {
		delete iWsEventReceiver;
		iWsEventReceiver = NULL;
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
	
	if (!iWsEventReceiver){
		TRAPD(err,
			iWsEventReceiver = CWsEventReceiver::NewL(*this);
		);
		if (err) {
			User::Panic(_L("Failed to create CWsEventReceiver"), 0);
		}
	}
}

static int ConvertKey(TInt aScanCode) {
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
//	MYLOG("HandleWsEvent\n");
//	cc_string msg; char msgB[64];
//	String_InitArray(msg, msgB);
//	String_AppendConst(&msg, "HandleWsEvent: ");
//	String_AppendInt(&msg, (int) eventType);
//	Logger_Log(&msg);
	// TODO
	switch (eventType) {
	case EEventKeyDown: {
		MYLOG("EEventKeyDown\n");
		Input_Set(ConvertKey(aWsEvent.Key()->iScanCode), true);
		break;
	}
	case EEventKeyUp: {
		MYLOG("EEventKeyUp\n");
		Input_Set(ConvertKey(aWsEvent.Key()->iScanCode), false);
		break;
	}
	case EEventScreenDeviceChanged: {
		MYLOG("EEventScreenDeviceChanged\n");
		TPixelsTwipsAndRotation pixnrot; 
		iWsScreenDevice->GetScreenModeSizeAndRotation(iWsScreenDevice->CurrentScreenMode(), pixnrot);
		if (pixnrot.iPixelSize != iWindow->Size()) {
			MYLOG("Resized\n");
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
		MYLOG("EEventFocusLost\n");
		//WindowInfo.Focused = false;
		
		//Event_RaiseVoid(&WindowEvents.FocusChanged);
		break;
	}
	case EEventFocusGained: {
		MYLOG("EEventFocusGained\n");
		//WindowInfo.Focused = true;
		
		//Event_RaiseVoid(&WindowEvents.FocusChanged);
		break;
	}
	case EEventWindowVisibilityChanged: {
		if (aWsEvent.Handle() == reinterpret_cast<TUint32>(this)) {
			MYLOG("EEventWindowVisibilityChanged\n");
			//WindowInfo.Inactive = (aWsEvent.VisibilityChanged()->iFlags & TWsVisibilityChangedEvent::ECanBeSeen) == 0;

			//Event_RaiseVoid(&WindowEvents.InactiveChanged);
		}
		break;
	}
	case EEventPointer: {
		MYLOG("EEventPointer\n");
		/*TAdvancedPointerEvent*/ TPointerEvent* pointer = aWsEvent.Pointer();
		
		//long num = pointer->PointerNumber();
		long num = 1;
		TPoint pos = pointer->iPosition;
		switch (pointer->iType) {
		case TPointerEvent::EButton1Down:
			MYLOG("AddTouch\n");
			Input_AddTouch(num, pos.iX, pos.iY);
			break;
		case TPointerEvent::EDrag:
			MYLOG("UpdateTouch\n");
			Input_UpdateTouch(num, pos.iX, pos.iY);
			break;
		case TPointerEvent::EButton1Up:
			MYLOG("RemoveTouch\n");
			Input_RemoveTouch(num, pos.iX, pos.iY);
			break;
		default:
			break;
		}
		break;
	}
	}
}

void CWindow::AllocFrameBuffer(int width, int height) {
	if (!iWindowGc) {
		iWindowGc = new (ELeave) CWindowGc(iWsScreenDevice);
		iWindowGc->Construct();
	}
	FreeFrameBuffer();
	iBitmap = new CFbsBitmap();
	iBitmap->Create(TSize(width, height), EColor16MA);
}

void CWindow::FreeFrameBuffer() {
	if (iBitmap != NULL) {
		delete iBitmap;
		iBitmap = NULL;
	}
}

void CWindow::DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	if (!iWindowGc)
		return;
	iWindow->Invalidate(/*TRect(r.x, r.y, r.width, r.height)*/);
	iWindow->BeginRedraw();
	iWindowGc->Activate(*iWindow);

	if (iBitmap) {
		iBitmap->BeginDataAccess();
		TUint8* data = (TUint8*) iBitmap->DataAddress();
		const TUint8* src = (TUint8*) bmp->scan0;
		for (TInt row = bmp->height - 1; row >= 0; --row) {
			memcpy(data, src, bmp->width * BITMAPCOLOR_SIZE);
			src += bmp->width * BITMAPCOLOR_SIZE;
			data += iBitmap->DataStride();
		}
		iBitmap->EndDataAccess();
		
		iWindowGc->BitBlt(TPoint(r.x, r.y), iBitmap, TRect(r.x, r.y, r.width, r.height));
	}
	iWindowGc->Deactivate();
	iWindow->EndRedraw();
	//iWsSession.Flush();
}

void CWindow::ProcessEvents(float delta) {
//	MYLOG("+ProcessEvents\n")
//	TBool block = !WindowInfo.Focused;
	RThread thread;
    TInt error = KErrNone;
    
//    if (block && !thread.RequestCount()) {
//    	User::WaitForAnyRequest();
//    	CActiveScheduler::RunIfReady(error, CActive::EPriorityIdle);
//    }
	
	while (thread.RequestCount()) {
		TInt res = CActiveScheduler::RunIfReady(error, CActive::EPriorityIdle);
		if (res)
			User::WaitForAnyRequest();
	}
    
    /*
	while (iWsEventStatus != KRequestPending) {
		iWsSession.GetEvent(window->iWsEvent);
		HandleWsEvent(window->iWsEvent);
		iWsEventStatus = KRequestPending;
		iWsSession.EventReady(&iWsEventStatus);
	}
	*/
//	MYLOG("-ProcessEvents\n")
}

void CWindow::RequestClose() {
	// TODO
	Event_RaiseVoid(&WindowEvents.Closing);
}

void CWindow::InitEvents() {
	MYLOG("+InitEvents\n")
//	if (iEventsInitialized)
//		return;
//	iEventsInitialized = ETrue;
//	iWsEventStatus = KRequestPending;
//	iWsSession.EventReady(&iWsEventStatus);
	
	MYLOG("-InitEvents\n")
}

//

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
	
	ctx_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (!eglInitialize(ctx_display, NULL, NULL)) {
		User::Panic(_L("Failed to initialize EGL"), 0);
	}

	TRAP(err,
	delete CActiveScheduler::Current();            
	CActiveScheduler* actScheduler = new (ELeave) CActiveScheduler();    
	CActiveScheduler::Install(actScheduler);
	);
	
	if (err != KErrNone) {
		User::Panic(_L("Failed to initialize CActiveScheduler"), 0);
	}
	
	MYLOG("-Window_PreInit\n")
}

void Window_Init(void) {
	MYLOG("+Window_Init\n")
	TRAPD(err, window = CWindow::NewL());
	MYLOG("Window_Init 1\n")
	if (err) {
		User::Panic(_L("Failed to initialize CWindow"), err);
	}
	
	//TBool touch = AknLayoutUtils::PenEnabled();
	
	bool touch = true;
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
	window->InitEvents();
}
void Window_Create3D(int width, int height) {
	MYLOG("Window_Create3D\n")
	launcherMode = false;
	window->InitEvents();
}

void Window_Destroy(void) {
	MYLOG("Window_Destroy\n")
	CCoeEnv::Static()->DestroyEnvironment();
	delete CCoeEnv::Static();
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
	static const cc_string t2 = String_Init((char*) title, String_Length(title), String_Length(title));
	Logger_Log(&t2);
	static const cc_string msg2 = String_Init((char*) msg, String_Length(msg), String_Length(msg));
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

#if 0
static GfxResourceID fb_tex, fb_vb;
static void AllocateVB(void) {
	MYLOG("+AllocateVB");
	fb_vb = Gfx_CreateVb(VERTEX_FORMAT_TEXTURED, 4);
	struct VertexTextured* data = (struct VertexTextured*)Gfx_LockVb(fb_vb,
															VERTEX_FORMAT_TEXTURED, 4);
	data[0].x = -1.0f; data[0].y = -1.0f; data[0].z = 0.0f; data[0].Col = PACKEDCOL_WHITE; data[0].U = 0.0f; data[0].V = 1.0f;
	data[1].x =  1.0f; data[1].y = -1.0f; data[1].z = 0.0f; data[1].Col = PACKEDCOL_WHITE; data[1].U = 1.0f; data[1].V = 1.0f;
	data[2].x =  1.0f; data[2].y =  1.0f; data[2].z = 0.0f; data[2].Col = PACKEDCOL_WHITE; data[2].U = 1.0f; data[2].V = 0.0f;
	data[3].x = -1.0f; data[3].y =  1.0f; data[3].z = 0.0f; data[3].Col = PACKEDCOL_WHITE; data[3].U = 0.0f; data[2].V = 0.0f;

	Gfx_UnlockVb(fb_vb);
	MYLOG("-AllocateVB");
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	MYLOG("+AllocFramebuffer");
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "bitmap");
	bmp->width  = width;
	bmp->height = height;

	if (!Gfx.Created) {
		Gfx_Create();
	}
	fb_tex = Gfx_AllocTexture(bmp, bmp->width, TEXTURE_FLAG_NONPOW2, false);
	AllocateVB();

	Game.Width  = Window_Main.Width;
	Game.Height = Window_Main.Height;
	Gfx_OnWindowResize();
	MYLOG("-AllocFramebuffer");
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	MYLOG("+DrawFramebuffer");
	struct Bitmap part;
	part.scan0  = Bitmap_GetRow(bmp, r.y) + r.x;
	part.width  = r.width;
	part.height = r.height;

	Gfx_BeginFrame();
	Gfx_BindIb(Gfx.DefaultIb);
	Gfx_UpdateTexture(fb_tex, r.x, r.y, &part, bmp->width, false);

	Gfx_LoadMatrix(MATRIX_VIEW, &Matrix_Identity);
	Gfx_LoadMatrix(MATRIX_PROJ, &Matrix_Identity);
	Gfx_SetDepthTest(false);
	Gfx_SetAlphaTest(false);
	Gfx_SetAlphaBlending(false);

	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindTexture(fb_tex);
	Gfx_BindVb(fb_vb);
	Gfx_DrawVb_IndexedTris(4);
	Gfx_EndFrame();
	MYLOG("-DrawFramebuffer");
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	MYLOG("+FreeFramebuffer");
	Mem_Free(bmp->scan0);
	Gfx_DeleteTexture(&fb_tex);
	Gfx_DeleteVb(&fb_vb);
	MYLOG("-FreeFramebuffer");
}
#else
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	MYLOG("+AllocFramebuffer\n");
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "bitmap");
	bmp->width  = width;
	bmp->height = height;
	window->AllocFrameBuffer(width, height);
	MYLOG("-AllocFramebuffer\n");
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	MYLOG("+DrawFramebuffer\n");
	window->DrawFramebuffer(r, bmp);
	MYLOG("-DrawFramebuffer\n");
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	MYLOG("+FreeFramebuffer\n");
	window->FreeFrameBuffer();
	Mem_Free(bmp->scan0);
	MYLOG("-FreeFramebuffer\n");
}
#endif

static void GLContext_InitSurface(void) {
	MYLOG("+GLContext_InitSurface\n")
	if (!window) return; /* window not created or lost */
	ctx_surface = eglCreateWindowSurface(ctx_display, ctx_config, window->iWindow, NULL);
	MYLOG("GLContext_InitSurface 1\n")
	if (!ctx_context)
		ctx_context = eglCreateContext(ctx_display, ctx_config, EGL_NO_CONTEXT, NULL);
	if (!ctx_context) Process_Abort2(eglGetError(), "Failed to create EGL context");

	if (!ctx_surface) return;
	eglMakeCurrent(ctx_display, ctx_surface, ctx_surface, ctx_context);
	MYLOG("-GLContext_InitSurface\n")
}
/*
void* GLContext_GetAddress(const char* function) {
	MYLOG("GLContext_GetAddress");
	return (void*) eglGetProcAddress(function);
}
*/
#endif
