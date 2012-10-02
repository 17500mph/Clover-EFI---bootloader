//
//  Pointer.c
//  
//
//  Created by Slice on 23.09.12.
//
// Initial idea comes from iBoot project by OS_Ninja and Ujen
// their sources are under GNU License but I don't know what is the subject for licensing here.
// my sources are quite different while Mouse/Events interfaces comes from Tiano,
// for example ConSplitterDxe or BdsDxe/FrontPage
// anyway thanks for good tutorial how to do and how not to do
// 
// Any usage for SMBIOS here?
/// Built-in Pointing Device (Type 21).

//#include "Platform.h"
#include "libegint.h"   //this includes platform.h 

#ifndef DEBUG_ALL
#define DEBUG_MOUSE 1
#else
#define DEBUG_MOUSE DEBUG_ALL
#endif

#if DEBUG_MOUSE == 0
#define DBG(...)
#else
#define DBG(...) DebugLog(DEBUG_MOUSE, __VA_ARGS__)
#endif


//TODO - make them theme dependent?
UINTN  PointerWidth = 32;
UINTN  PointerHeight = 32;
ACTION gAction;
UINTN  gItemID;

POINTERS gPointer = {NULL, NULL, NULL, NULL, {0, 0, 32, 32}, {0, 0, 32, 32}, 0,
  {0, 0, 0, FALSE, FALSE}, NoEvents};

VOID HidePointer()
{
  egDrawImage(gPointer.oldImage, gPointer.oldPlace.XPos, gPointer.oldPlace.YPos);
}

VOID DrawPointer()
{
  // take background image
  egTakeImage(gPointer.oldImage, gPointer.newPlace.XPos, gPointer.newPlace.YPos,
              PointerWidth, PointerHeight);
  CopyMem(&gPointer.oldPlace, &gPointer.newPlace, sizeof(EG_RECT));
  egRawCopy(gPointer.newImage->PixelData, gPointer.oldImage->PixelData,
            PointerWidth, PointerHeight,
            gPointer.newImage->Width,
            gPointer.oldImage->Width);
  egComposeImage(gPointer.newImage, gPointer.Pointer, 0, 0);
  egDrawImage(gPointer.newImage, gPointer.oldPlace.XPos, gPointer.oldPlace.YPos);
  
}

VOID RedrawPointer()
{
  //always assumed
  /*  if (!gPointer.SimplePointerProtocol) {
   return;
   }*/
  HidePointer();
  DrawPointer();
}

EFI_STATUS MouseBirth()
{
  EFI_STATUS Status = EFI_UNSUPPORTED;
  EFI_SIMPLE_POINTER_MODE  *CurrentMode;
//  EG_PIXEL pi;
  if (gPointer.SimplePointerProtocol) { //do not double
    DrawPointer();
    return EFI_SUCCESS;
  }
  Status = gBS->LocateProtocol (&gEfiSimplePointerProtocolGuid, NULL, (VOID**)&gPointer.SimplePointerProtocol);
	if(EFI_ERROR(Status)) {
		MsgLog("No mouse!\n");
    return Status;
	}
  CurrentMode = gPointer.SimplePointerProtocol->Mode;
  DBG("Found Mouse device:\n");
  DBG(" - ResolutionX=%d\n", CurrentMode->ResolutionX);
  DBG(" - ResolutionY=%d\n", CurrentMode->ResolutionY);
  DBG(" - ResolutionZ=%d\n", CurrentMode->ResolutionZ);
  DBG(" - Left button %a present\n", CurrentMode->LeftButton?" ":"not");
  DBG(" - Right button %a present\n\n", CurrentMode->RightButton?" ":"not");
  //TODO - config and menu?
  CurrentMode->ResolutionX = gSettings.PointerSpeed;
  CurrentMode->ResolutionY = gSettings.PointerSpeed;
  CurrentMode->ResolutionZ = 0;
  
  //there may be also trackpad protocol but afaik it is not properly work and
  // trackpad is usually controlled by simple mouse driver
  
//  gPointer.Pointer = egLoadIcon(ThemeDir, L"icons\\pointer.icns", PointerWidth);
  gPointer.Pointer = BuiltinIcon(BUILTIN_ICON_POINTER);
	if(!gPointer.Pointer) {
    
		DBG("No pointer image!\n");
    gPointer.SimplePointerProtocol = NULL;
    return EFI_NOT_FOUND;
	}
  gPointer.LastClickTime = AsmReadTsc();
  gPointer.oldPlace.XPos = UGAWidth >> 2;
  gPointer.oldPlace.YPos = UGAHeight >> 2;
  gPointer.oldPlace.Width = PointerWidth;
  gPointer.oldPlace.Height = PointerHeight;
  CopyMem(&gPointer.newPlace, &gPointer.oldPlace, sizeof(EG_RECT));
  
  gPointer.oldImage = egCreateImage(PointerWidth, PointerHeight, FALSE);
  gPointer.newImage = egCreateFilledImage(PointerWidth, PointerHeight, FALSE, &MenuBackgroundPixel);
  egTakeImage(gPointer.oldImage, gPointer.oldPlace.XPos, gPointer.oldPlace.YPos,
              PointerWidth, PointerHeight);
/*
  pi = gPointer.oldImage->PixelData[0];
  DBG("Pixel data at start\n");
  DBG(" Blue=%x Green=%x Red=%x Alfa=%x\n\n", pi.b, pi.g, pi.r, pi.a);
 */
  DrawPointer();
  gPointer.MouseEvent = NoEvents;
  return Status;
}

VOID KillMouse()
{
//  EG_PIXEL pi;
  
  if (!gPointer.SimplePointerProtocol) {
    return;
  }
//  pi = gPointer.oldImage->PixelData[0];
  DBG("Mouse death\n");
//  DBG(" Blue=%x Green=%x Red=%x Alfa=%x\n\n", pi.b, pi.g, pi.r, pi.a);

  egFreeImage(gPointer.newImage);
  egFreeImage(gPointer.oldImage);
  egFreeImage(gPointer.Pointer);
  gPointer.MouseEvent = NoEvents;
  gPointer.SimplePointerProtocol = NULL;
}

// input - tsc
// output - milliseconds
UINT64 TimeDiff(UINT64 t0, UINT64 t1) 
{
  return DivU64x64Remainder((t1 - t0), DivU64x32(gCPUStructure.TSCFrequency, 1000), 0);
}

VOID UpdatePointer()
{
//  EFI_TIME Now;
  UINT64   Now;
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_SIMPLE_POINTER_STATE	tmpState;
 
  //always assumed
/*  if (!gPointer.SimplePointerProtocol) {
    return;
  }*/

//  Now = gRS->GetTime(&Now, NULL);
  Now = AsmReadTsc();
  Status = gPointer.SimplePointerProtocol->GetState(gPointer.SimplePointerProtocol, &tmpState);
  if (!EFI_ERROR(Status)) {
    if (gPointer.State.LeftButton && !tmpState.LeftButton) { //release left
      //TODO - time for double click 500ms into menu
      if (TimeDiff(gPointer.LastClickTime, Now) < gSettings.DoubleClickTime) {
        gPointer.MouseEvent = DoubleClick;
      } else {
        gPointer.MouseEvent = LeftClick;
      }
 //     CopyMem(&gPointer.LastClickTime, &Now, sizeof(EFI_TIME));
      gPointer.LastClickTime = Now;
    } else if (gPointer.State.RightButton && !tmpState.RightButton) { //release right
      gPointer.MouseEvent = RightClick;
    } else
      gPointer.MouseEvent = NoEvents;
    
    CopyMem(&gPointer.State, &tmpState, sizeof(EFI_SIMPLE_POINTER_STATE));
    gPointer.newPlace.XPos += gPointer.State.RelativeMovementX;
    if (gPointer.newPlace.XPos < 0) gPointer.newPlace.XPos = 0;
    if (gPointer.newPlace.XPos > UGAWidth) gPointer.newPlace.XPos = UGAWidth;
    
    gPointer.newPlace.YPos += gPointer.State.RelativeMovementY;
    if (gPointer.newPlace.YPos < 0) gPointer.newPlace.YPos = 0;
    if (gPointer.newPlace.YPos > UGAHeight) gPointer.newPlace.YPos = UGAHeight;
  
    RedrawPointer();
  }
//  return Status;
}

BOOLEAN MouseInRect(EG_RECT Place)
{
  return  ((gPointer.newPlace.XPos > Place.XPos) &&
           (gPointer.newPlace.XPos < Place.XPos + Place.Width) &&
           (gPointer.newPlace.YPos > Place.YPos) &&
           (gPointer.newPlace.YPos < Place.YPos + Place.Height));
}

EFI_STATUS CheckMouseEvent(REFIT_MENU_SCREEN *Screen)
{
  EFI_STATUS Status = EFI_TIMEOUT;
  UINTN EntryId;
  
  gAction = ActionNone;
  
  if (!Screen) {
    return EFI_TIMEOUT;
  }
  if (gPointer.MouseEvent != NoEvents){
    for (EntryId = 0; EntryId < Screen->EntryCount; EntryId++) {
      if (MouseInRect(Screen->Entries[EntryId]->Place)) {
        switch (gPointer.MouseEvent) {
          case LeftClick:
            gAction = Screen->Entries[EntryId]->AtClick;
            break;
          case RightClick:
            gAction = Screen->Entries[EntryId]->AtRightClick;
            break;
          case DoubleClick:
            gAction = Screen->Entries[EntryId]->AtDoubleClick;
            break;
          default:
            gAction = ActionNone;
            break;
        }
        gItemID = EntryId;
        break;
      }
    }
  }
  if (gAction != ActionNone) {
    Status = EFI_SUCCESS;
    gPointer.MouseEvent = NoEvents; //clear event as set action
  }
  return Status;
}

#define ONE_SECOND  10000000
#define ONE_MSECOND    10000

// mouse events depends on Screen
// TimeoutDefault for a wait in 0.1 seconds
// return EFI_TIMEOUT if no inputs
EFI_STATUS WaitForInputEvent(REFIT_MENU_SCREEN *Screen, UINTN TimeoutDefault)
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN TimeoutRemain = TimeoutDefault * 10;
  while (TimeoutRemain != 0) {
    
    Status = WaitForSingleEvent (gST->ConIn->WaitForKey, ONE_MSECOND * 10);
    if (Status != EFI_TIMEOUT) {
      break;
    }
    TimeoutRemain--;
    if (gPointer.SimplePointerProtocol) {
      UpdatePointer();
      Status = CheckMouseEvent(Screen); //out: gItemID, gAction
      if (Status != EFI_TIMEOUT) { //this check should return timeout if no mouse events occured
        break;
      }
    }
  }
  return Status;
}
