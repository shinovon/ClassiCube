#include "Window_Symbian.h"

extern "C" {
#include "main_impl.h"
}

CCContainer* container;

TInt CCContainer::LoopCallBack(TAny* ptr) {
	if (crashed) {
		return EFalse;
	}
	CCContainer* container = (CCContainer*) ptr;
	cc_bool run = false;
	for (;;) {
		if (!WindowInfo.Exists) {
			Window_RequestClose();
			container->iAppUi->Exit();
			return EFalse;
		}
		
		if (run) {
			run = false;
			container->gameRunning = true;
			ProcessProgramArgs(0, 0);
			Game_Setup();
			container->RestartTimerL(100);
		}
		
		if (!container->gameRunning) {
			if (Launcher_Tick()) break;
			Launcher_Finish();
			run = true;
			continue;
		}
		
		if (!Game_Running) {
			container->gameRunning = false;
			Game_Free();
//			Launcher_Setup();
//			container->RestartTimerL(10000);
			WindowInfo.Exists = false;
			continue;
		}
		
		Game_RenderFrame();
		break;
	}
	return ETrue;
}

void CCContainer::RestartTimerL(TInt aInterval) {
	if (iPeriodic) {
		iPeriodic->Cancel();
	} else {
		iPeriodic = CPeriodic::NewL(CActive::EPriorityIdle);
	}
	iPeriodic->Start(aInterval, aInterval, TCallBack(CCContainer::LoopCallBack, this));
}

void CCContainer::ConstructL(const TRect& aRect, CCAppUi* aAppUi) {
	iAppUi = aAppUi;
	
	// create window
	CreateWindowL();
	SetExtentToWholeScreen();
	
	// enable multi-touch and drag events
#ifdef CC_BUILD_SYMBIAN_3
	Window().EnableAdvancedPointers();
#endif
	EnableDragEvents();
	
	ActivateL();
	container = this;
	
	SetupProgram(0, 0);

	TSize size = Size();
	WindowInfo.Focused = true;
	WindowInfo.Exists = true;
	WindowInfo.Handle.ptr = (void*) &Window();
#ifdef CC_BUILD_TOUCH
	WindowInfo.SoftKeyboard = SOFT_KEYBOARD_VIRTUAL;
#endif

	DisplayInfo.Width = size.iWidth;
	DisplayInfo.Height = size.iHeight;

	WindowInfo.Width = size.iWidth;
	WindowInfo.Height = size.iHeight;

	WindowInfo.UIScaleX = DEFAULT_UI_SCALE_X;
	WindowInfo.UIScaleY = DEFAULT_UI_SCALE_Y;
	if (size.iWidth <= 360) {
		DisplayInfo.ScaleX = 0.5f;
		DisplayInfo.ScaleY = 0.5f;
	} else {
		DisplayInfo.ScaleX = 1;
		DisplayInfo.ScaleY = 1;
	}
	
	TDisplayMode displayMode = Window().DisplayMode();
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
#ifdef EKA2
	case EColor16MA:
#else
	case EColor16MU + 1:
#endif
		bufferSize = 32;
		break;
	default:
		break;
	}
	DisplayInfo.Depth = bufferSize;

	Launcher_Setup();
	// reduced tickrate for launcher
	RestartTimerL(10000);
}

CCContainer::~CCContainer() {
	delete iPeriodic;
}

void CCContainer::SizeChanged() {
	TSize size = Size();
	if (iBitmap) {
		delete iBitmap;
		iBitmap = NULL;
	}
	iBitmap = new CFbsBitmap();
#ifdef EKA2
	TInt err = iBitmap->Create(size, EColor16MA);
#else
	TInt err = iBitmap->Create(size, EColor16MU);
#endif
	if (err) {
		Process_Abort("Failed to create bitmap");
		return;
	}
	
	DisplayInfo.Width  = size.iWidth;
	DisplayInfo.Height = size.iHeight;

	if (Window_Main.Is3D) {
		if (size.iWidth <= 360) {
			DisplayInfo.ScaleX = 0.5f;
			DisplayInfo.ScaleY = 0.5f;
		} else {
			DisplayInfo.ScaleX = 1;
			DisplayInfo.ScaleY = 1;
		}
	}

	WindowInfo.Width  = size.iWidth;
	WindowInfo.Height = size.iHeight;
	Event_RaiseVoid(&WindowEvents.Resized);
	DrawNow();
}

void CCContainer::HandleResourceChange(TInt aType) {
	const TInt KEikDynamicLayoutVariantSwitch = 0x101F8121;
	switch (aType) {
	case KEikDynamicLayoutVariantSwitch:
		SetExtentToWholeScreen();
		break;
	}
}

TInt CCContainer::CountComponentControls() const {
	return 0;
}

CCoeControl* CCContainer::ComponentControl(TInt) const {
	return NULL;
}

void CCContainer::Draw(const TRect& aRect) const {
#if CC_GFX_BACKEND_IS_GL()
	if (Window_Main.Is3D) return;
#endif
	if (iBitmap) {
		SystemGc().BitBlt(TPoint(0, 0), iBitmap);
	}
}

void CCContainer::DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	if (iBitmap) {
		iBitmap->LockHeap();
		TUint8* data = (TUint8*) iBitmap->DataAddress();
		if (!data) {
			Process_Abort("Bitmap data address is null");
			return;
		}
		const TUint8* src = (TUint8*) bmp->scan0;
		for (TInt row = bmp->height - 1; row >= 0; --row) {
			memcpy(data, src, bmp->width * BITMAPCOLOR_SIZE);
			src += bmp->width * BITMAPCOLOR_SIZE;
#ifdef EKA2
			data += iBitmap->DataStride();
#else
			data += bmp->width * BITMAPCOLOR_SIZE;
#endif
		}
		iBitmap->UnlockHeap();
	}
	DrawDeferred();
}

void CCContainer::HandleControlEventL(CCoeControl*, TCoeEvent) {
}

void CCContainer::HandlePointerEventL(const TPointerEvent& aPointerEvent) {
#ifdef CC_BUILD_TOUCH
	CCEvent event = { 0 };
#ifdef CC_BUILD_SYMBIAN_3
	const TAdvancedPointerEvent* advpointer = aPointerEvent.AdvancedPointerEvent();
	event.i1 = advpointer != NULL ? advpointer->PointerNumber() : 0;
#else
	event.i1 = 0;
#endif
	TPoint pos = aPointerEvent.iPosition;
	event.i2 = pos.iX;
	event.i3 = pos.iY;
	switch (aPointerEvent.iType) {
	case TPointerEvent::EButton1Down:
		event.type = CC_TOUCH_ADD;
		break;
	case TPointerEvent::EDrag:
		event.type = CC_TOUCH_ADD;
		break;
	case TPointerEvent::EButton1Up:
		event.type = CC_TOUCH_REMOVE;
		break;
	default:
		break;
	}
	if (event.type) Events_Push(&event);
#endif
	CCoeControl::HandlePointerEventL(aPointerEvent);
}

void CCContainer::InitEvents() {
	events_mutex    = Mutex_Create("Symbian events");
	events_capacity = EVENTS_DEFAULT_MAX;
	events_list     = events_default;
}

void CCContainer::PushEvent(const CCEvent* event) {
	if (!events_mutex) return;
	Mutex_Lock(events_mutex);
	{
		if (events_count >= events_capacity) {
			Utils_Resize((void**)&events_list, &events_capacity,
						sizeof(CCEvent), EVENTS_DEFAULT_MAX, 20);
		}
		events_list[events_count++] = *event;
	}
	Mutex_Unlock(events_mutex);
}

cc_bool CCContainer::PullEvent(CCEvent* event) {
	cc_bool found = false;
	
	Mutex_Lock(events_mutex);
	{
		if (events_count) {
			*event = events_list[0];
			for (int i = 1; i < events_count; i++) {
				events_list[i - 1] = events_list[i];
			}
			events_count--;
			found = true;
		}
	}
	Mutex_Unlock(events_mutex);
	return found;
}
