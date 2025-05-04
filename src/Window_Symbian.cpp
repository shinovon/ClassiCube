#include "Core.h"
#if defined CC_BUILD_SYMBIAN

#include <include/bitmap.h>
#include <e32base.h>
#include <coemain.h>
#include <e32keys.h>
#include <w32std.h>
#include <aknutils.h>
#include <aknnotewrappers.h>
#include <stdlib.h>
#include <stdapis/string.h>
extern "C" {
#include <gles/egl.h>
#include "_WindowBase.h"
}
#include "Errors.h"
#include "Logger.h"
#include "String.h"
#include "Gui.h"
#include "Graphics.h"
#include "Game.h"

static cc_bool launcherMode;

class CWsEventReceiver;
class CWindow;

CWindow* window;

class CWindow : public CBase
{
public:
	static CWindow* NewLC();
	void HandleWsEvent(const TWsEvent& aEvent);
	void InitEventReceiver();
//	void AllocFrameBuffer(int width, int height);
//	void DrawFramebuffer(Rect2D r, struct Bitmap* bmp);
//	void FreeFrameBuffer();
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
    CFbsBitmap* iBitmap;
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

	WindowInfo.Focused = true;
	WindowInfo.Exists = true;
	WindowInfo.Handle.ptr = iWindow;
	
	TInt w = pixnrot.iPixelSize.iWidth,
		h = pixnrot.iPixelSize.iHeight;
	
	DisplayInfo.Width = w;
	DisplayInfo.Height = h;
	
	WindowInfo.Width = w;
	WindowInfo.Height = h;
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
	
	//iWindowGc = new (ELeave) CWindowGc(iWsScreenDevice);
	//iWindowGc->Construct();
	//iWindowGc->SetBrushStyle(CWindowGc::ESolidBrush);
}

void CWindow::InitEventReceiver() {
	if (!iWsEventReceiver){
		TRAPD(err,
			iWsEventReceiver = CWsEventReceiver::NewL(*this);
		);
		if (err) {
			User::Panic(_L("Failed to create CWsEventReceiver"), 0);
		}
	}
}

void CWindow::HandleWsEvent(const TWsEvent& aWsEvent) {
	TInt eventType = aWsEvent.Type();
	// TODO
	switch (eventType) {
	case EEventKeyDown: {
		MYLOG("EEventKeyDown\n");
		Input_Set(aWsEvent.Key()->iScanCode, true);
		break;
	}
	case EEventKeyUp: {
		MYLOG("EEventKeyUp\n");
		Input_Set(aWsEvent.Key()->iScanCode, false);
		break;
	}
	case EEventScreenDeviceChanged: {
		MYLOG("EEventScreenDeviceChanged\n");
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
		MYLOG("EEventFocusLost\n");
		WindowInfo.Focused = false;
		
		Event_RaiseVoid(&WindowEvents.FocusChanged);
		break;
	}
	case EEventFocusGained: {
		MYLOG("EEventFocusGained\n");
		WindowInfo.Focused = true;
		
		Event_RaiseVoid(&WindowEvents.FocusChanged);
		break;
	}
	case EEventWindowVisibilityChanged: {
		if (aWsEvent.Handle() == reinterpret_cast<TUint32>(this)) {
			MYLOG("EEventWindowVisibilityChanged\n");
			WindowInfo.Inactive = (aWsEvent.VisibilityChanged()->iFlags & TWsVisibilityChangedEvent::ECanBeSeen) == 0;
			
			Event_RaiseVoid(&WindowEvents.StateChanged);
		}
		break;
	}
	case EEventPointer: {
		MYLOG("EEventPointer\n");
		TAdvancedPointerEvent* pointer = aWsEvent.Pointer();
		
		long num = pointer->PointerNumber();
		TPoint pos = pointer->iPosition;
		if ((pointer->iModifiers & TPointerEvent::EButton1Down) != 0) {
			MYLOG("AddTouch\n");
			Input_AddTouch(num, pos.iX, pos.iY);
		} else {
			MYLOG("RemoveTouch\n");
			Input_RemoveTouch(num, pos.iX, pos.iY);
		}
		break;
	}
	}
}

//void CWindow::AllocFrameBuffer(int width, int height) {
//	FreeFrameBuffer();
//	iBitmap = new CFbsBitmap();
//	iBitmap->Create(TSize(width, height), EColor16MA);
//}
//
//void CWindow::FreeFrameBuffer() {
//	if (iBitmap != NULL) {
//		delete iBitmap;
//		iBitmap = NULL;
//	}
//}
//
//void CWindow::DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
//	iWindow->BeginRedraw();
//	iWindowGc->Activate(*iWindow);
	
//	iBitmap->BeginDataAccess();
//	TUint8* data = (TUint8*) iBitmap->DataAddress();
//	const TUint8* src = (TUint8*) bmp->scan0;
//	for (TInt row = bmp->height - 1; row >= 0; --row) {
//		memcpy(data, src, bmp->width * BITMAPCOLOR_SIZE);
//		src += bmp->width * BITMAPCOLOR_SIZE;
//		data += iBitmap->DataStride();
//	}
//	iBitmap->EndDataAccess();
	
//	iWindowGc->BitBlt(TPoint(r.x, r.y), iBitmap, TRect(r.x, r.y, r.width, r.height));
//	iWindowGc->Deactivate();
//	iWindow->EndRedraw();
//}

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

static GfxResourceID fb_tex, fb_vb;
static void AllocateVB(void) {
	MYLOG("+AllocateVB");
	fb_vb = Gfx_CreateVb(VERTEX_FORMAT_TEXTURED, 4);
	struct VertexTextured* data = (struct VertexTextured*)Gfx_LockVb(&fb_vb,
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
		window->InitEventReceiver();
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
/*
void* GLContext_GetAddress(const char* function) {
	MYLOG("GLContext_GetAddress");
	return (void*) eglGetProcAddress(function);
}
*/
#endif
