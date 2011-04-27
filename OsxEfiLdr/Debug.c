/*++

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  Debug.c

Abstract:

Revision History:

--*/
#include "EfiLdr.h"
#include "Debug.h"
//#include <Library/SerialPortLib.h>

UINT8 *mCursor;
UINT8 mHeaderIndex = 10;

VOID
PrintHeader (
  CHAR8 Char
  )
{
  *(UINT8 *)(UINTN)(0x000b8000 + mHeaderIndex) = Char;
  mHeaderIndex += 2;
}

VOID
ClearScreen (
  VOID
  )
{
  UINT32 Index;

  mCursor = (UINT8 *)(UINTN)(0x000b8000 + 160);
  for (Index = 0; Index < 80 * 49; Index++) {
    *mCursor = ' ';
    mCursor += 2;
  }
  mCursor = (UINT8 *)(UINTN)(0x000b8000 + 160);
}


VOID 
PrintU32Base10 (
  UINT32 Value
  )
{
  UINT32 Index;
  CHAR8  Char;
  CHAR8  String[11];
  UINTN  StringPos;
  UINT32 B10Div;

  B10Div = 1000000000;
  for (Index = 0, StringPos = 0; Index < 10; Index++) {
    Char = (UINT8) (((Value / B10Div) % 10) + '0');
    if ((StringPos > 0) || (Char != '0')) {
      String[StringPos] = Char;
      StringPos++;
    }
    B10Div = B10Div / 10;
  }

  if (StringPos == 0) {
      String[0] = '0';
      StringPos++;
  }

  String[StringPos] = '\0';

  PrintString (String);
}


VOID
PrintValue (
  UINT32 Value
  )
{
  UINT32 Index;
  CHAR8  Char;
  CHAR8  String[9];

  for (Index = 0; Index < 8; Index++) {
    Char = (UINT8)(((Value >> ((7 - Index) * 4)) & 0x0f) + '0');
    if (Char > '9') {
      Char = (UINT8) (Char - '0' - 10 + 'A');
    }
    String[Index] = Char;
  }

  String[sizeof (String) - 1] = '\0';

  PrintString (String);
}

VOID
PrintValue64 (
  UINT64 Value
  )
{
  PrintValue ((UINT32) RShiftU64 (Value, 32));
  PrintValue ((UINT32) Value);
}

VOID
PrintString (
  CHAR8 *String
  )
{
  UINT32 Index;

  for (Index = 0; String[Index] != 0; Index++) {
    if (String[Index] == '\n') {
      mCursor = (UINT8 *)(UINTN)(0xb8000 + (((((UINTN)mCursor - 0xb8000) + 160) / 160) * 160));
    } else {
      *mCursor = String[Index];
      mCursor += 2;
    }
  }

  //
  // All information also output to serial port.
  //
//SerialPortWrite ((UINT8*) String, Index);
}

