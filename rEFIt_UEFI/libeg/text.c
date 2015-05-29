/*
 * libeg/text.c
 * Text drawing functions
 *
 * Copyright (c) 2006 Christoph Pfisterer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//Slice 2011 - 2015 numerous improvements

#include "libegint.h"

#include "egemb_font.h"
//#define FONT_CELL_WIDTH (7)
//#define FONT_CELL_HEIGHT (12)

#ifndef DEBUG_ALL
#define DEBUG_TEXT 0
#else
#define DEBUG_TEXT DEBUG_ALL
#endif

#if DEBUG_TEXT == 0
#define DBG(...)
#else
#define DBG(...) DebugLog(DEBUG_TEXT, __VA_ARGS__)
#endif


EG_IMAGE *FontImage = NULL;
INTN FontWidth = 7;
INTN FontHeight = 12;
INTN TextHeight = 16;

//
// Text rendering
//

VOID egMeasureText(IN CHAR16 *Text, OUT INTN *Width, OUT INTN *Height)
{
    if (Width != NULL)
        *Width = StrLen(Text) * FontWidth; //GlobalConfig.CharWidth;
    if (Height != NULL)
        *Height = FontHeight;
}

EG_IMAGE * egLoadFontImage(IN BOOLEAN FromTheme, IN INTN Rows, IN INTN Cols)
{
  EG_IMAGE            *NewImage;
  EG_IMAGE            *NewFontImage;
//  UINTN     FontWidth;  //using global variables
//  UINTN     FontHeight;
  INTN        ImageWidth, ImageHeight;
  INTN        x, y, Ypos, j;
  EG_PIXEL    *PixelPtr;
  EG_PIXEL    FirstPixel;
  BOOLEAN     WantAlpha = TRUE;
  
  if (!ThemeDir) {
    GlobalConfig.Font = FONT_GRAY;
    return NULL;
  }

  if (FromTheme) {
    NewImage = egLoadImage(ThemeDir, GlobalConfig.FontFileName, WantAlpha);
  } else {
    NewImage = egLoadImage(ThemeDir, L"FontKorean.png", WantAlpha);
  }

  if (NewImage) {
    if (FromTheme) {
      DBG("font %s loaded from themedir\n", GlobalConfig.FontFileName);
    } else {
      DBG("Korean font loaded from themedir\n");
    }
  } else {
    CHAR16 *commonFontDir = L"EFI\\CLOVER\\font";
    CHAR16 *fontFilePath = PoolPrint(L"%s\\%s", commonFontDir, GlobalConfig.FontFileName);
    NewImage = egLoadImage(SelfRootDir, fontFilePath, WantAlpha);
    if (!NewImage) {
      DBG("Font %s is not loaded, using default\n", fontFilePath);
      FreePool(fontFilePath);
      return NULL;
    }
    DBG("font %s loaded from common font dir %s\n", GlobalConfig.FontFileName, commonFontDir);
    FreePool(fontFilePath);
  }

  ImageWidth = NewImage->Width;
//  DBG("ImageWidth=%d\n", ImageWidth);
  ImageHeight = NewImage->Height;
//  DBG("ImageHeight=%d\n", ImageHeight);
  PixelPtr = NewImage->PixelData;
  DBG("Font loaded: ImageWidth=%d ImageHeight=%d\n", ImageWidth, ImageHeight);
  NewFontImage = egCreateImage(ImageWidth * Rows, ImageHeight / Rows, WantAlpha);
  if (NewFontImage == NULL) {
    DBG("Can't create new font image!\n");
    return NULL;
  }
  
  FontWidth = ImageWidth / Cols;
  FontHeight = ImageHeight / Rows;
  FirstPixel = *PixelPtr;
  for (y = 0; y < Rows; y++) {
    for (j = 0; j < FontHeight; j++) {
      Ypos = ((j * Rows) + y) * ImageWidth;
      for (x = 0; x < ImageWidth; x++) {
       if (WantAlpha && 
           (PixelPtr->b == FirstPixel.b) &&
           (PixelPtr->g == FirstPixel.g) &&
           (PixelPtr->r == FirstPixel.r)
           ) {
          PixelPtr->a = 0;
        }
        NewFontImage->PixelData[Ypos + x] = *PixelPtr++;
      }
    }    
  }
  egFreeImage(NewImage);
  
  return NewFontImage;  
} 

VOID PrepareFont(VOID)
{
  BOOLEAN ChangeFont = FALSE;
//  EG_PIXEL *FontPixelData;
  EG_PIXEL *p;
  INTN      Width;
  INTN      Height;
  if (gLanguage == korean) {
//    FontImage = egLoadImage(ThemeDir, L"FontKorean.png", TRUE);
    FontImage = egLoadFontImage(FALSE, 10, 28);
    if (FontImage) {
      FontHeight = 16;
 //     if (GlobalConfig.CharWidth == 0) {
        GlobalConfig.CharWidth = 16;
 //     }
      FontWidth = GlobalConfig.CharWidth;
      TextHeight = FontHeight + TEXT_YMARGIN * 2;
      DBG("Using Korean font matrix\n");
      return;
    } else {
      gLanguage = english;
    }
  }

  // load the font
  if (FontImage == NULL){
    switch (GlobalConfig.Font) {
      case FONT_ALFA:
        ChangeFont = TRUE;
        FontImage = egPrepareEmbeddedImage(&egemb_font, TRUE);        
        break;
      case FONT_GRAY:
        ChangeFont = TRUE;
        FontImage = egPrepareEmbeddedImage(&egemb_font_gray, TRUE);
        break;
      case FONT_LOAD:
        DBG("load font image\n");
        FontImage = egLoadFontImage(TRUE, 16, 16);
        if (!FontImage) {
          ChangeFont = TRUE;
          GlobalConfig.Font = FONT_ALFA;
          FontImage = egPrepareEmbeddedImage(&egemb_font, TRUE);
          //invert the font
          p = FontImage->PixelData;
          for (Height = 0; Height < FontImage->Height; Height++){
            for (Width = 0; Width < FontImage->Width; Width++, p++){
              p->b ^= 0xFF;
              p->g ^= 0xFF;
              p->r ^= 0xFF;
      //        p->a = 0xFF;    //huh!          
            }
          }
        }
        break;
      default:
        FontImage = egPrepareEmbeddedImage(&egemb_font, TRUE);
        break;
    }    
  }
  if (ChangeFont) {
    // set default values
    GlobalConfig.CharWidth = 7;
    FontWidth = GlobalConfig.CharWidth;
    FontHeight = 12;
  }
  TextHeight = FontHeight + TEXT_YMARGIN * 2;
  DBG("Font %d prepared WxH=%dx%d CharWidth=%d\n", GlobalConfig.Font, FontWidth, FontHeight, GlobalConfig.CharWidth);
}

static inline BOOLEAN EmptyPix(EG_PIXEL *Ptr, EG_PIXEL *FirstPixel)
{
  //compare with first pixel of the array top-left point [0][0]
   return ((Ptr->r >= FirstPixel->r - (FirstPixel->r >> 2)) && (Ptr->r <= FirstPixel->r + (FirstPixel->r >> 2)) &&
           (Ptr->g >= FirstPixel->g - (FirstPixel->g >> 2)) && (Ptr->g <= FirstPixel->g + (FirstPixel->g >> 2)) &&
           (Ptr->b >= FirstPixel->b - (FirstPixel->b >> 2)) && (Ptr->b <= FirstPixel->b + (FirstPixel->b >> 2)) &&
           (Ptr->a == FirstPixel->a)); //hack for transparent fonts
}

INTN GetEmpty(EG_PIXEL *Ptr, EG_PIXEL *FirstPixel, INTN MaxWidth, INTN Step, INTN Row)
{
  INTN i, j, m;
  EG_PIXEL *Ptr0, *Ptr1;

  Ptr1 = (Step > 0)?Ptr:Ptr - 1;
  DBG("Ptr=%x Ptr1=%x First=%x (%d, %d, %d, %d) W=%d Row=0x%x\n", Ptr, Ptr1, FirstPixel,
        FirstPixel->r, FirstPixel->g, FirstPixel->b, FirstPixel->b, MaxWidth, Row);
  m = MaxWidth;
  for (j = 0; j < FontHeight; j++) {
    Ptr0 = Ptr1 + j * Row;
    for (i = 0; i < MaxWidth; i++) {
      DBG("(%d, %d, %d, %d) at step %d\n", Ptr0->r, Ptr0->g, Ptr0->b, Ptr0->a, i);
      if (!EmptyPix(Ptr0, FirstPixel)) {
        break;
      }
      Ptr0 += Step;
    }
    m = (i > m)?m:i;
    DBG("choosen shift %d\n", m);
  }
  DBG("Empty %a %d\n", (Step > 0)?"right":"left", m);
  return m;
}

INTN egRenderText(IN CHAR16 *Text, IN OUT EG_IMAGE *CompImage,
                  IN INTN PosX, IN INTN PosY, IN INTN Cursor)
{
  EG_PIXEL        *BufferPtr;
  EG_PIXEL        *FontPixelData;
  EG_PIXEL        *FirstPixelBuf;
  INTN            BufferLineOffset, FontLineOffset;
  INTN            TextLength;
  INTN            i;
  UINT16          c, c1, c0;
  UINTN           Shift = 0;
  UINTN           Cho = 0, Jong = 0, Joong = 0;
  UINTN           LeftSpace, RightSpace;
  
  // clip the text
  TextLength = StrLen(Text);
  if ((TextLength * GlobalConfig.CharWidth + PosX) > CompImage->Width){
    if (GlobalConfig.CharWidth) {
      TextLength = (CompImage->Width - PosX + GlobalConfig.CharWidth - 1) / GlobalConfig.CharWidth;
    } else
      TextLength = (CompImage->Width - PosX + FontWidth - 1) / FontWidth;
  }
  if (!FontImage) {
    GlobalConfig.Font = FONT_LOAD;
    PrepareFont();
  }
  
  DBG("TextLength =%d PosX=%d PosY=%d\n", TextLength, PosX, PosY);
  // render it
  BufferPtr = CompImage->PixelData;
  BufferLineOffset = CompImage->Width;
  BufferPtr += PosX + PosY * BufferLineOffset;
  FirstPixelBuf = BufferPtr;
  FontPixelData = FontImage->PixelData;
  FontLineOffset = FontImage->Width;
  DBG("BufferLineOffset=%d  FontLineOffset=%d\n", BufferLineOffset, FontLineOffset);

  if (GlobalConfig.CharWidth < FontWidth) {
    Shift = (FontWidth - GlobalConfig.CharWidth) >> 1;
  }
  c0 = 0;
  for (i = 0; i < TextLength; i++) {
    c = Text[i];
    if (gLanguage != korean) {
      if (GlobalConfig.Font != FONT_LOAD) {
        if (c < 0x20 || c >= 0x7F)
          c = 0x5F;
        else
          c -= 0x20;
      } else {
        c1 = (((c >=0x410) ? (c -= 0x350) : c) & 0xff); //Russian letters
        c = c1;
      }

      if (GlobalConfig.Proportional) {
        if (c0 <= 0x20) {  // space before or buffer edge
          LeftSpace = 2;
        } else {
          LeftSpace = GetEmpty(BufferPtr, FirstPixelBuf, GlobalConfig.CharWidth, -1, BufferLineOffset);
        }
        if (c <= 0x20) { //new space will be half width
          RightSpace = GlobalConfig.CharWidth >> 1; 
        } else {
          RightSpace = GetEmpty(FontPixelData + c * FontWidth, FontPixelData, FontWidth, 1, FontLineOffset);
          if (RightSpace >= GlobalConfig.CharWidth + Shift) {
            RightSpace = 0; //empty place for invisible characters
          }
        }
      } else {
        LeftSpace = 2;
        RightSpace = Shift;
      }
      c0 = c; //old value
      egRawCompose(BufferPtr - LeftSpace + 2, FontPixelData + c * FontWidth + RightSpace,
                   GlobalConfig.CharWidth, FontHeight,
                   BufferLineOffset, FontLineOffset);
      if (i == Cursor) {
        c = (GlobalConfig.Font == FONT_LOAD)?0x5F:0x3F;
        egRawCompose(BufferPtr - LeftSpace + 1, FontPixelData + c * FontWidth + RightSpace,
                     GlobalConfig.CharWidth, FontHeight,
                     BufferLineOffset, FontLineOffset);
      }
      BufferPtr += GlobalConfig.CharWidth - LeftSpace + 2;
    } else {
      //
      if ((c >= 0x20) && (c <= 0x7F)) {
        c1 = ((c - 0x20) >> 4) * 28 + (c & 0x0F);
        Cho = c1;
        Shift = 12;
      } else if ((c < 0x20) || ((c > 0x7F) && (c < 0xAC00))) {
        Cho = 0x0E; //just a dot
        Shift = 8;
      } else if ((c >= 0xAC00) && (c <= 0xD638)) {
        //korean
        Shift = 18;
        c -= 0xAC00;
        c1 = c / 28;
        Jong = c % 28;
        Cho = c1 / 21;
        Joong = c1 % 21;
        Cho += 28 * 7;
        Joong += 28 * 8;
        Jong += 28 * 9;
      }
//        DBG("Cho=%d Joong=%d Jong=%d\n", Cho, Joong, Jong);
      if (Shift == 18) {
        egRawCompose(BufferPtr, FontPixelData + Cho * 28 + 4 + FontLineOffset,
                     GlobalConfig.CharWidth, FontHeight,
                     BufferLineOffset, FontLineOffset);
      } else {
        egRawCompose(BufferPtr + BufferLineOffset * 3, FontPixelData + Cho * 28 + 2,
                     GlobalConfig.CharWidth, FontHeight,
                     BufferLineOffset, FontLineOffset);
      }
      if (i == Cursor) {
        c = 99;
        egRawCompose(BufferPtr, FontPixelData + c * 28 + 2,
                     GlobalConfig.CharWidth, FontHeight,
                     BufferLineOffset, FontLineOffset);
      }
      if (Shift == 18) {
        egRawCompose(BufferPtr + 8, FontPixelData + Joong * 28 + 6, //9 , 4 are tunable
                     GlobalConfig.CharWidth - 8, FontHeight,
                     BufferLineOffset, FontLineOffset);
        egRawCompose(BufferPtr + BufferLineOffset * 10, FontPixelData + Jong * 28 + 5,
                     GlobalConfig.CharWidth, FontHeight - 10,
                     BufferLineOffset, FontLineOffset);

      }

      BufferPtr += Shift;
    }
  }
  return ((INTN)BufferPtr - (INTN)FirstPixelBuf) / sizeof(EG_PIXEL);
}

/* EOF */
