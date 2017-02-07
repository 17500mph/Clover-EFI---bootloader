/** @file
  Safe String functions.

  Copyright (c) 2014 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BaseLibInternals.h"
// this is 1.000.000
#define RSIZE_MAX  100000000ull 
//(PcdGet32 (PcdMaximumUnicodeStringLength))

#define ASCII_RSIZE_MAX  100000000ull
//(PcdGet32 (PcdMaximumAsciiStringLength))

#define SAFE_STRING_CONSTRAINT_CHECK(Expression, Status)  \
  do { \
    ASSERT (Expression); \
    if (!(Expression)) { \
      return Status; \
    } \
  } while (FALSE)

/**
  Returns if 2 memory blocks are overlapped.

  @param  Base1  Base address of 1st memory block.
  @param  Size1  Size of 1st memory block.
  @param  Base2  Base address of 2nd memory block.
  @param  Size2  Size of 2nd memory block.

  @retval TRUE  2 memory blocks are overlapped.
  @retval FALSE 2 memory blocks are not overlapped.
**/
BOOLEAN
InternalSafeStringIsOverlap (
  IN VOID    *Base1,
  IN UINTN   Size1,
  IN VOID    *Base2,
  IN UINTN   Size2
  )
{
  if ((((UINTN)Base1 >= (UINTN)Base2) && ((UINTN)Base1 < (UINTN)Base2 + Size2)) ||
      (((UINTN)Base2 >= (UINTN)Base1) && ((UINTN)Base2 < (UINTN)Base1 + Size1))) {
    return TRUE;
  }
  return FALSE;
}

/**
  Returns if 2 Unicode strings are not overlapped.

  @param  Str1   Start address of 1st Unicode string.
  @param  Size1  The number of char in 1st Unicode string,
                 including terminating null char.
  @param  Str2   Start address of 2nd Unicode string.
  @param  Size2  The number of char in 2nd Unicode string,
                 including terminating null char.

  @retval TRUE  2 Unicode strings are NOT overlapped.
  @retval FALSE 2 Unicode strings are overlapped.
**/
BOOLEAN
InternalSafeStringNoStrOverlap (
  IN CHAR16  *Str1,
  IN UINTN   Size1,
  IN CHAR16  *Str2,
  IN UINTN   Size2
  )
{
  return !InternalSafeStringIsOverlap (Str1, Size1 * sizeof(CHAR16), Str2, Size2 * sizeof(CHAR16));
}

/**
  Returns if 2 Ascii strings are not overlapped.

  @param  Str1   Start address of 1st Ascii string.
  @param  Size1  The number of char in 1st Ascii string,
                 including terminating null char.
  @param  Str2   Start address of 2nd Ascii string.
  @param  Size2  The number of char in 2nd Ascii string,
                 including terminating null char.

  @retval TRUE  2 Ascii strings are NOT overlapped.
  @retval FALSE 2 Ascii strings are overlapped.
**/
BOOLEAN
InternalSafeStringNoAsciiStrOverlap (
  IN CHAR8   *Str1,
  IN UINTN   Size1,
  IN CHAR8   *Str2,
  IN UINTN   Size2
  )
{
  return !InternalSafeStringIsOverlap (Str1, Size1, Str2, Size2);
}

/**
  Returns the length of a Null-terminated Unicode string.

  This function is similar as strlen_s defined in C11.

  If String is not aligned on a 16-bit boundary, then ASSERT().

  @param  String   A pointer to a Null-terminated Unicode string.
  @param  MaxSize  The maximum number of Destination Unicode
                   char, including terminating null char.
Slice: NOT including

  @retval 0        If String is NULL.
  @retval MaxSize  If there is no null character in the first MaxSize characters of String.
  @return The number of characters that percede the terminating null character.
Slice:
 for example string = L"012345"
  StrnLenS(string, 4) = 4;
  StrnLenS(string, 6) = 6;
  StrnLenS(string, 9) = 6;
**/
UINTN
EFIAPI
StrnLenS (
  IN CONST CHAR16              *String,
  IN UINTN                     MaxSize
  )
{
  UINTN                             Length;

  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // If String is a null pointer, then the StrnLenS function returns zero.
  //
  if (String == NULL) {
    return 0;
  }

  //
  // Otherwise, the StrnLenS function returns the number of characters that precede the
  // terminating null character. If there is no null character in the first MaxSize characters of
  // String then StrnLenS returns MaxSize. At most the first MaxSize characters of String shall
  // be accessed by StrnLenS.
  //
  Length = 0;
  while (String[Length] != 0) {
    if (Length >= MaxSize - 1) {
      return MaxSize;
    }
    Length++;
  }
  return Length;
}

/**
  Returns the size of a Null-terminated Unicode string in bytes, including the
  Null terminator.

  This function returns the size of the Null-terminated Unicode string
  specified by String in bytes, including the Null terminator.

  If String is not aligned on a 16-bit boundary, then ASSERT().

  @param  String   A pointer to a Null-terminated Unicode string.
  @param  MaxSize  The maximum number of Destination Unicode
                   char, including the Null terminator.

  @retval 0  If String is NULL.
  @retval (sizeof (CHAR16) * (MaxSize + 1))
             If there is no Null terminator in the first MaxSize characters of
             String.
  @return The size of the Null-terminated Unicode string in bytes, including
          the Null terminator.

**/
UINTN
EFIAPI
StrnSizeS (
  IN CONST CHAR16              *String,
  IN UINTN                     MaxSize
  )
{
  //
  // If String is a null pointer, then the StrnSizeS function returns zero.
  //
  if (String == NULL) {
    return 0;
  }

  //
  // Otherwise, the StrnSizeS function returns the size of the Null-terminated
  // Unicode string in bytes, including the Null terminator. If there is no
  // Null terminator in the first MaxSize characters of String, then StrnSizeS
  // returns (sizeof (CHAR16) * (MaxSize + 1)) to keep a consistent map with
  // the StrnLenS function.
  //
  return (StrnLenS (String, MaxSize) + 1) * sizeof (*String);
}

/**
  Copies the string pointed to by Source (including the terminating null char)
  to the array pointed to by Destination.

  This function is similar as strcpy_s defined in C11.

  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Unicode string.
  @param  DestMax                  The maximum number of Destination Unicode
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Unicode string.

  @retval RETURN_SUCCESS           String is copied.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumUnicodeStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
StrCpyS (
  OUT CHAR16       *Destination,
  IN  UINTN        DestMax,
  IN  CONST CHAR16 *Source
  )
{
  UINTN            SourceLen;
  
  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than RSIZE_MAX.
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. DestMax shall be greater than StrnLenS(Source, DestMax).
  //
  SourceLen = StrnLenS (Source, DestMax);
//  SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);
//Slice: nonsense because of StrnLenS result always <= DestMax

  //
  // 5. Copying shall not take place between objects that overlap.
  //
//  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);
  //Slice: relax

  //
  // The StrCpyS function copies the string pointed to by Source (including the terminating
  // null character) into the array pointed to by Destination.
  //
  while ((*Source != 0) && (--DestMax > 0)) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Copies not more than Length successive char from the string pointed to by
  Source to the array pointed to by Destination. If no null char is copied from
  Source, then Destination[Length] is always set to null.

  This function is similar as strncpy_s defined in C11.

  If Length > 0 and Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Length > 0 and Source is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Unicode string.
  @param  DestMax                  The maximum number of Destination Unicode
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Unicode string.
  @param  Length                   The maximum number of Unicode characters to copy.

  @retval RETURN_SUCCESS           String is copied.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than 
                                   MIN(StrLen(Source), Length).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumUnicodeStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
StrnCpyS (
  OUT CHAR16       *Destination,
  IN  UINTN        DestMax,
  IN  CONST CHAR16 *Source,
  IN  UINTN        Length
  )
{
  UINTN            SourceLen;

  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither DestMax nor Length shall be greater than RSIZE_MAX
  //
//  if (RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
//    SAFE_STRING_CONSTRAINT_CHECK ((Length <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. If Length is not less than DestMax, then DestMax shall be greater than StrnLenS(Source, DestMax).
  //
  SourceLen = StrnLenS (Source, DestMax); //Slice: SourceLen <= DestMax
//  if (Length >= DestMax) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);
//  }

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
//  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The StrnCpyS function copies not more than Length successive characters (characters that
  // follow a null character are not copied) from the array pointed to by Source to the array
  // pointed to by Destination. If no null character was copied from Source, then Destination[Length] is set to a null
  // character.
  //
  while ((*Source != 0) && (SourceLen > 0) && (--DestMax > 0)) {
    *(Destination++) = *(Source++);
    SourceLen--;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Appends a copy of the string pointed to by Source (including the terminating
  null char) to the end of the string pointed to by Destination.

  This function is similar as strcat_s defined in C11.

  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Unicode string.
  @param  DestMax                  The maximum number of Destination Unicode
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Unicode string.

  @retval RETURN_SUCCESS           String is appended.
  @retval RETURN_BAD_BUFFER_SIZE   If DestMax is NOT greater than 
                                   StrLen(Destination).
  @retval RETURN_BUFFER_TOO_SMALL  If (DestMax - StrLen(Destination)) is NOT
                                   greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumUnicodeStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
StrCatS (
  IN OUT CHAR16       *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR16 *Source
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;
  
  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // Let CopyLen denote the value DestMax - StrnLenS(Destination, DestMax) upon entry to StrCatS.
  //
  DestLen = StrnLenS (Destination, DestMax);  //Slice: DestLen <= DestMax
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than RSIZE_MAX.
  //
//  if (RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
//    if (DestMax > RSIZE_MAX) {
//      DestMax = RSIZE_MAX;
//    }
//  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. CopyLen shall be greater than StrnLenS(Source, CopyLen).
  //
  SourceLen = StrnLenS (Source, CopyLen);  //Slice:  SourceLen <= CopyLen
//  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 6. Copying shall not take place between objects that overlap.
  //
//  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The StrCatS function appends a copy of the string pointed to by Source (including the
  // terminating null character) to the end of the string pointed to by Destination. The initial character
  // from Source overwrites the null character at the end of Destination.
  //
  Destination = Destination + DestLen;
  while ((*Source != 0) && (--CopyLen > 0)) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Appends not more than Length successive char from the string pointed to by
  Source to the end of the string pointed to by Destination. If no null char is
  copied from Source, then Destination[StrLen(Destination) + Length] is always
  set to null.

  This function is similar as strncat_s defined in C11.

  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Unicode string.
  @param  DestMax                  The maximum number of Destination Unicode
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Unicode string.
  @param  Length                   The maximum number of Unicode characters to copy.

  @retval RETURN_SUCCESS           String is appended.
  @retval RETURN_BAD_BUFFER_SIZE   If DestMax is NOT greater than
                                   StrLen(Destination).
  @retval RETURN_BUFFER_TOO_SMALL  If (DestMax - StrLen(Destination)) is NOT
                                   greater than MIN(StrLen(Source), Length).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumUnicodeStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
StrnCatS (
  IN OUT CHAR16       *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR16 *Source,
  IN     UINTN        Length
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;
  
  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // Let CopyLen denote the value DestMax - StrnLenS(Destination, DestMax) upon entry to StrnCatS.
  //
  DestLen = StrnLenS (Destination, DestMax);
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither DestMax nor Length shall be greater than RSIZE_MAX.
  //
//  if (RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
//    SAFE_STRING_CONSTRAINT_CHECK ((Length <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. If Length is not less than CopyLen, then CopyLen shall be greater than StrnLenS(Source, CopyLen).
  //
  SourceLen = StrnLenS (Source, CopyLen);
//  if (Length >= CopyLen) {
//    SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);
//  }

  //
  // 6. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
//  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The StrnCatS function appends not more than Length successive characters (characters
  // that follow a null character are not copied) from the array pointed to by Source to the end of
  // the string pointed to by Destination. The initial character from Source overwrites the null character at
  // the end of Destination. If no null character was copied from Source, then Destination[DestMax-CopyLen+Length] is set to
  // a null character.
  //
  Destination = Destination + DestLen;
  while ((*Source != 0) && (SourceLen > 0) && (--CopyLen > 0)) {
    *(Destination++) = *(Source++);
    SourceLen--;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Unicode decimal string to a value of type UINTN.

  This function outputs a value of type UINTN by interpreting the contents of
  the Unicode string specified by String as a decimal number. The format of the
  input Unicode string String is:

                  [spaces] [decimal digits].

  The valid decimal digit character is in the range [0-9]. The function will
  ignore the pad space, which includes spaces or tab characters, before
  [decimal digits]. The running zero in the beginning of [decimal digits] will
  be ignored. Then, the function stops at the first character that is a not a
  valid decimal character or a Null-terminator, whichever one comes first.

  If String is NULL, then ASSERT().
  If Data is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  If String has no valid decimal digits in the above format, then 0 is stored
  at the location pointed to by Data.
  If the number represented by String exceeds the range defined by UINTN, then
  MAX_UINTN is stored at the location pointed to by Data.

  If EndPointer is not NULL, a pointer to the character that stopped the scan
  is stored at the location pointed to by EndPointer. If String has no valid
  decimal digits right after the optional pad spaces, the value of String is
  stored at the location pointed to by EndPointer.

  @param  String                   Pointer to a Null-terminated Unicode string.
  @param  EndPointer               Pointer to character that stops scan.
  @param  Data                     Pointer to the converted value.

  @retval RETURN_SUCCESS           Value is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If PcdMaximumUnicodeStringLength is not
                                   zero, and String contains more than
                                   PcdMaximumUnicodeStringLength Unicode
                                   characters, not including the
                                   Null-terminator.
  @retval RETURN_UNSUPPORTED       If the number represented by String exceeds
                                   the range defined by UINTN.

**/
RETURN_STATUS
EFIAPI
StrDecimalToUintnS (
  IN  CONST CHAR16             *String,
  OUT       CHAR16             **EndPointer,  OPTIONAL
  OUT       UINTN              *Data
  )
{
  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // 1. Neither String nor Data shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Data != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. The length of String shall not be greater than RSIZE_MAX.
  //
//  if (RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((StrnLenS (String, RSIZE_MAX + 1) <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  *Data = 0;

  while (InternalIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according to the range
    // defined by UINTN, then MAX_UINTN is stored in *Data and
    // RETURN_UNSUPPORTED is returned.
    //
//    if (*Data > ((MAX_UINTN - (*String - L'0')) / 10)) {
//      *Data = MAX_UINTN;
//      if (EndPointer != NULL) {
//        *EndPointer = (CHAR16 *) String;
//      }
//      return RETURN_UNSUPPORTED;
//      ASSERT (*Data <= ((((UINTN) ~0) - (*String - L'0')) / 10));
//    }

    *Data = *Data * 10 + (*String - L'0');
    String++;
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }
  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Unicode decimal string to a value of type UINT64.

  This function outputs a value of type UINT64 by interpreting the contents of
  the Unicode string specified by String as a decimal number. The format of the
  input Unicode string String is:

                  [spaces] [decimal digits].

  The valid decimal digit character is in the range [0-9]. The function will
  ignore the pad space, which includes spaces or tab characters, before
  [decimal digits]. The running zero in the beginning of [decimal digits] will
  be ignored. Then, the function stops at the first character that is a not a
  valid decimal character or a Null-terminator, whichever one comes first.

  If String is NULL, then ASSERT().
  If Data is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  If String has no valid decimal digits in the above format, then 0 is stored
  at the location pointed to by Data.
  If the number represented by String exceeds the range defined by UINT64, then
  MAX_UINT64 is stored at the location pointed to by Data.

  If EndPointer is not NULL, a pointer to the character that stopped the scan
  is stored at the location pointed to by EndPointer. If String has no valid
  decimal digits right after the optional pad spaces, the value of String is
  stored at the location pointed to by EndPointer.

  @param  String                   Pointer to a Null-terminated Unicode string.
  @param  EndPointer               Pointer to character that stops scan.
  @param  Data                     Pointer to the converted value.

  @retval RETURN_SUCCESS           Value is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If PcdMaximumUnicodeStringLength is not
                                   zero, and String contains more than
                                   PcdMaximumUnicodeStringLength Unicode
                                   characters, not including the
                                   Null-terminator.
  @retval RETURN_UNSUPPORTED       If the number represented by String exceeds
                                   the range defined by UINT64.

**/
RETURN_STATUS
EFIAPI
StrDecimalToUint64S (
  IN  CONST CHAR16             *String,
  OUT       CHAR16             **EndPointer,  OPTIONAL
  OUT       UINT64             *Data
  )
{
  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // 1. Neither String nor Data shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Data != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. The length of String shall not be greater than RSIZE_MAX.
  //
//  if (RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((StrnLenS (String, RSIZE_MAX + 1) <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  *Data = 0;

  while (InternalIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according to the range
    // defined by UINT64, then MAX_UINT64 is stored in *Data and
    // RETURN_UNSUPPORTED is returned.
    //
//    if (*Data > DivU64x32 (MAX_UINT64 - (*String - L'0'), 10)) {
//      *Data = MAX_UINT64;
//      if (EndPointer != NULL) {
//        *EndPointer = (CHAR16 *) String;
//      }
//      return RETURN_UNSUPPORTED;
//    }
//    ASSERT (*Data <= DivU64x32 (((UINT64) ~0) - (*String - L'0') , 10));

    *Data = MultU64x32 (*Data, 10) + (*String - L'0');
    String++;
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }
  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Unicode hexadecimal string to a value of type
  UINTN.

  This function outputs a value of type UINTN by interpreting the contents of
  the Unicode string specified by String as a hexadecimal number. The format of
  the input Unicode string String is:

                  [spaces][zeros][x][hexadecimal digits].

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F].
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix.
  If "x" appears in the input string, it must be prefixed with at least one 0.
  The function will ignore the pad space, which includes spaces or tab
  characters, before [zeros], [x] or [hexadecimal digit]. The running zero
  before [x] or [hexadecimal digit] will be ignored. Then, the decoding starts
  after [x] or the first valid hexadecimal digit. Then, the function stops at
  the first character that is a not a valid hexadecimal character or NULL,
  whichever one comes first.

  If String is NULL, then ASSERT().
  If Data is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  If String has no valid hexadecimal digits in the above format, then 0 is
  stored at the location pointed to by Data.
  If the number represented by String exceeds the range defined by UINTN, then
  MAX_UINTN is stored at the location pointed to by Data.

  If EndPointer is not NULL, a pointer to the character that stopped the scan
  is stored at the location pointed to by EndPointer. If String has no valid
  hexadecimal digits right after the optional pad spaces, the value of String
  is stored at the location pointed to by EndPointer.

  @param  String                   Pointer to a Null-terminated Unicode string.
  @param  EndPointer               Pointer to character that stops scan.
  @param  Data                     Pointer to the converted value.

  @retval RETURN_SUCCESS           Value is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If PcdMaximumUnicodeStringLength is not
                                   zero, and String contains more than
                                   PcdMaximumUnicodeStringLength Unicode
                                   characters, not including the
                                   Null-terminator.
  @retval RETURN_UNSUPPORTED       If the number represented by String exceeds
                                   the range defined by UINTN.

**/
RETURN_STATUS
EFIAPI
StrHexToUintnS (
  IN  CONST CHAR16             *String,
  OUT       CHAR16             **EndPointer,  OPTIONAL
  OUT       UINTN              *Data
  )
{
  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // 1. Neither String nor Data shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Data != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. The length of String shall not be greater than RSIZE_MAX.
  //
//  if (RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((StrnLenS (String, RSIZE_MAX + 1) <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  if (InternalCharToUpper (*String) == L'X') {
    if (*(String - 1) != L'0') {
      *Data = 0;
      return RETURN_SUCCESS;
    }
    //
    // Skip the 'X'
    //
    String++;
  }

  *Data = 0;

  while (InternalIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according to the range
    // defined by UINTN, then MAX_UINTN is stored in *Data and
    // RETURN_UNSUPPORTED is returned.
    //
//    if (*Data > ((MAX_UINTN - InternalHexCharToUintn (*String)) >> 4)) {
//      *Data = MAX_UINTN;
//      if (EndPointer != NULL) {
//        *EndPointer = (CHAR16 *) String;
//      }
//      return RETURN_UNSUPPORTED;
//    }
//      ASSERT (*Data <= ((((UINTN) ~0) - InternalHexCharToUintn (*String)) >> 4));

    *Data = (*Data << 4) + InternalHexCharToUintn (*String);
    String++;
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }
  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Unicode hexadecimal string to a value of type
  UINT64.

  This function outputs a value of type UINT64 by interpreting the contents of
  the Unicode string specified by String as a hexadecimal number. The format of
  the input Unicode string String is:

                  [spaces][zeros][x][hexadecimal digits].

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F].
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix.
  If "x" appears in the input string, it must be prefixed with at least one 0.
  The function will ignore the pad space, which includes spaces or tab
  characters, before [zeros], [x] or [hexadecimal digit]. The running zero
  before [x] or [hexadecimal digit] will be ignored. Then, the decoding starts
  after [x] or the first valid hexadecimal digit. Then, the function stops at
  the first character that is a not a valid hexadecimal character or NULL,
  whichever one comes first.

  If String is NULL, then ASSERT().
  If Data is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  If String has no valid hexadecimal digits in the above format, then 0 is
  stored at the location pointed to by Data.
  If the number represented by String exceeds the range defined by UINT64, then
  MAX_UINT64 is stored at the location pointed to by Data.

  If EndPointer is not NULL, a pointer to the character that stopped the scan
  is stored at the location pointed to by EndPointer. If String has no valid
  hexadecimal digits right after the optional pad spaces, the value of String
  is stored at the location pointed to by EndPointer.

  @param  String                   Pointer to a Null-terminated Unicode string.
  @param  EndPointer               Pointer to character that stops scan.
  @param  Data                     Pointer to the converted value.

  @retval RETURN_SUCCESS           Value is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If PcdMaximumUnicodeStringLength is not
                                   zero, and String contains more than
                                   PcdMaximumUnicodeStringLength Unicode
                                   characters, not including the
                                   Null-terminator.
  @retval RETURN_UNSUPPORTED       If the number represented by String exceeds
                                   the range defined by UINT64.

**/
RETURN_STATUS
EFIAPI
StrHexToUint64S (
  IN  CONST CHAR16             *String,
  OUT       CHAR16             **EndPointer,  OPTIONAL
  OUT       UINT64             *Data
  )
{
  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // 1. Neither String nor Data shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Data != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. The length of String shall not be greater than RSIZE_MAX.
  //
//  if (RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((StrnLenS (String, RSIZE_MAX + 1) <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  if (InternalCharToUpper (*String) == L'X') {
    if (*(String - 1) != L'0') {
      *Data = 0;
      return RETURN_SUCCESS;
    }
    //
    // Skip the 'X'
    //
    String++;
  }

  *Data = 0;

  while (InternalIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according to the range
    // defined by UINT64, then MAX_UINT64 is stored in *Data and
    // RETURN_UNSUPPORTED is returned.
    //
//    if (*Data > RShiftU64 (MAX_UINT64 - InternalHexCharToUintn (*String), 4)) {
//      *Data = MAX_UINT64;
//      if (EndPointer != NULL) {
//        *EndPointer = (CHAR16 *) String;
//      }
//      return RETURN_UNSUPPORTED;
//    }
//    ASSERT (*Data <= RShiftU64 (((UINT64) ~0) - InternalHexCharToUintn (*String) , 4));

    *Data = LShiftU64 (*Data, 4) + InternalHexCharToUintn (*String);
    String++;
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }
  return RETURN_SUCCESS;
}

/**
  Returns the length of a Null-terminated Ascii string.

  This function is similar as strlen_s defined in C11.

  @param  String   A pointer to a Null-terminated Ascii string.
  @param  MaxSize  The maximum number of Destination Ascii
                   char, including terminating null char.

  @retval 0        If String is NULL.
  @retval MaxSize  If there is no null character in the first MaxSize characters of String.
  @return The number of characters that percede the terminating null character.

**/
UINTN
EFIAPI
AsciiStrnLenS (
  IN CONST CHAR8               *String,
  IN UINTN                     MaxSize
  )
{
  UINTN                             Length;

  //
  // If String is a null pointer, then the AsciiStrnLenS function returns zero.
  //
  if (String == NULL) {
    return 0;
  }

  //
  // Otherwise, the AsciiStrnLenS function returns the number of characters that precede the
  // terminating null character. If there is no null character in the first MaxSize characters of
  // String then AsciiStrnLenS returns MaxSize. At most the first MaxSize characters of String shall
  // be accessed by AsciiStrnLenS.
  //
  Length = 0;
  while (String[Length] != 0) {
    if (Length >= MaxSize - 1) {
      return MaxSize;
    }
    Length++;
  }
  return Length;
}

/**
  Returns the size of a Null-terminated Ascii string in bytes, including the
  Null terminator.

  This function returns the size of the Null-terminated Ascii string specified
  by String in bytes, including the Null terminator.

  @param  String   A pointer to a Null-terminated Ascii string.
  @param  MaxSize  The maximum number of Destination Ascii
                   char, including the Null terminator.

  @retval 0  If String is NULL.
  @retval (sizeof (CHAR8) * (MaxSize + 1))
             If there is no Null terminator in the first MaxSize characters of
             String.
  @return The size of the Null-terminated Ascii string in bytes, including the
          Null terminator.

**/
UINTN
EFIAPI
AsciiStrnSizeS (
  IN CONST CHAR8               *String,
  IN UINTN                     MaxSize
  )
{
  //
  // If String is a null pointer, then the AsciiStrnSizeS function returns
  // zero.
  //
  if (String == NULL) {
    return 0;
  }

  //
  // Otherwise, the AsciiStrnSizeS function returns the size of the
  // Null-terminated Ascii string in bytes, including the Null terminator. If
  // there is no Null terminator in the first MaxSize characters of String,
  // then AsciiStrnSizeS returns (sizeof (CHAR8) * (MaxSize + 1)) to keep a
  // consistent map with the AsciiStrnLenS function.
  //
  return (AsciiStrnLenS (String, MaxSize) + 1);
}

/**
  Copies the string pointed to by Source (including the terminating null char)
  to the array pointed to by Destination.

  This function is similar as strcpy_s defined in C11.

  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.

  @retval RETURN_SUCCESS           String is copied.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
AsciiStrCpyS (
  OUT CHAR8        *Destination,
  IN  UINTN        DestMax,
  IN  CONST CHAR8  *Source
  )
{
  UINTN            SourceLen;
  
  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than ASCII_RSIZE_MAX.
  //
//  if (ASCII_RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. DestMax shall be greater than AsciiStrnLenS(Source, DestMax).
  //
  SourceLen = AsciiStrnLenS (Source, DestMax);
//  SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 5. Copying shall not take place between objects that overlap.
  //
//  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrCpyS function copies the string pointed to by Source (including the terminating
  // null character) into the array pointed to by Destination.
  //
  while ((*Source != 0) && (--DestMax > 0)) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Copies not more than Length successive char from the string pointed to by
  Source to the array pointed to by Destination. If no null char is copied from
  Source, then Destination[Length] is always set to null.

  This function is similar as strncpy_s defined in C11.

  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.
  @param  Length                   The maximum number of Ascii characters to copy.

  @retval RETURN_SUCCESS           String is copied.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than 
                                   MIN(StrLen(Source), Length).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
AsciiStrnCpyS (
  OUT CHAR8        *Destination,
  IN  UINTN        DestMax,
  IN  CONST CHAR8  *Source,
  IN  UINTN        Length
  )
{
  UINTN            SourceLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither DestMax nor Length shall be greater than ASCII_RSIZE_MAX
  //
//  if (ASCII_RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
//    SAFE_STRING_CONSTRAINT_CHECK ((Length <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. If Length is not less than DestMax, then DestMax shall be greater than AsciiStrnLenS(Source, DestMax).
  //
  SourceLen = AsciiStrnLenS (Source, DestMax);
//  if (Length >= DestMax) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);
//  }

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
//  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrnCpyS function copies not more than Length successive characters (characters that
  // follow a null character are not copied) from the array pointed to by Source to the array
  // pointed to by Destination. If no null character was copied from Source, then Destination[Length] is set to a null
  // character.
  //
  while ((*Source != 0) && (SourceLen > 0) && (--DestMax > 0)) {
    *(Destination++) = *(Source++);
    SourceLen--;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Appends a copy of the string pointed to by Source (including the terminating
  null char) to the end of the string pointed to by Destination.

  This function is similar as strcat_s defined in C11.

  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.

  @retval RETURN_SUCCESS           String is appended.
  @retval RETURN_BAD_BUFFER_SIZE   If DestMax is NOT greater than 
                                   StrLen(Destination).
  @retval RETURN_BUFFER_TOO_SMALL  If (DestMax - StrLen(Destination)) is NOT
                                   greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
AsciiStrCatS (
  IN OUT CHAR8        *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR8  *Source
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;
  
  //
  // Let CopyLen denote the value DestMax - AsciiStrnLenS(Destination, DestMax) upon entry to AsciiStrCatS.
  //
  DestLen = AsciiStrnLenS (Destination, DestMax);
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than ASCII_RSIZE_MAX.
  //
//  if (ASCII_RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. CopyLen shall be greater than AsciiStrnLenS(Source, CopyLen).
  //
  SourceLen = AsciiStrnLenS (Source, CopyLen);
//  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 6. Copying shall not take place between objects that overlap.
  //
//  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrCatS function appends a copy of the string pointed to by Source (including the
  // terminating null character) to the end of the string pointed to by Destination. The initial character
  // from Source overwrites the null character at the end of Destination.
  //
  Destination = Destination + DestLen;
  while ((*Source != 0) && (--CopyLen > 0)) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Appends not more than Length successive char from the string pointed to by
  Source to the end of the string pointed to by Destination. If no null char is
  copied from Source, then Destination[StrLen(Destination) + Length] is always
  set to null.

  This function is similar as strncat_s defined in C11.

  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.
  @param  Length                   The maximum number of Ascii characters to copy.

  @retval RETURN_SUCCESS           String is appended.
  @retval RETURN_BAD_BUFFER_SIZE   If DestMax is NOT greater than
                                   StrLen(Destination).
  @retval RETURN_BUFFER_TOO_SMALL  If (DestMax - StrLen(Destination)) is NOT
                                   greater than MIN(StrLen(Source), Length).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
AsciiStrnCatS (
  IN OUT CHAR8        *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR8  *Source,
  IN     UINTN        Length
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;
  
  //
  // Let CopyLen denote the value DestMax - AsciiStrnLenS(Destination, DestMax) upon entry to AsciiStrnCatS.
  //
  DestLen = AsciiStrnLenS (Destination, DestMax);
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither DestMax nor Length shall be greater than ASCII_RSIZE_MAX.
  //
//  if (ASCII_RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
//    SAFE_STRING_CONSTRAINT_CHECK ((Length <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. If Length is not less than CopyLen, then CopyLen shall be greater than AsciiStrnLenS(Source, CopyLen).
  //
  SourceLen = AsciiStrnLenS (Source, CopyLen);
//  if (Length >= CopyLen) {
//    SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);
//  }

  //
  // 6. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
//  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrnCatS function appends not more than Length successive characters (characters
  // that follow a null character are not copied) from the array pointed to by Source to the end of
  // the string pointed to by Destination. The initial character from Source overwrites the null character at
  // the end of Destination. If no null character was copied from Source, then Destination[DestMax-CopyLen+Length] is set to
  // a null character.
  //
  Destination = Destination + DestLen;
  while ((*Source != 0) && (SourceLen > 0) && (--CopyLen > 0)) {
    *(Destination++) = *(Source++);
    SourceLen--;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Ascii decimal string to a value of type UINTN.

  This function outputs a value of type UINTN by interpreting the contents of
  the Ascii string specified by String as a decimal number. The format of the
  input Ascii string String is:

                  [spaces] [decimal digits].

  The valid decimal digit character is in the range [0-9]. The function will
  ignore the pad space, which includes spaces or tab characters, before
  [decimal digits]. The running zero in the beginning of [decimal digits] will
  be ignored. Then, the function stops at the first character that is a not a
  valid decimal character or a Null-terminator, whichever one comes first.

  If String is NULL, then ASSERT().
  If Data is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and String contains more than
  PcdMaximumAsciiStringLength Ascii characters, not including the
  Null-terminator, then ASSERT().

  If String has no valid decimal digits in the above format, then 0 is stored
  at the location pointed to by Data.
  If the number represented by String exceeds the range defined by UINTN, then
  MAX_UINTN is stored at the location pointed to by Data.

  If EndPointer is not NULL, a pointer to the character that stopped the scan
  is stored at the location pointed to by EndPointer. If String has no valid
  decimal digits right after the optional pad spaces, the value of String is
  stored at the location pointed to by EndPointer.

  @param  String                   Pointer to a Null-terminated Ascii string.
  @param  EndPointer               Pointer to character that stops scan.
  @param  Data                     Pointer to the converted value.

  @retval RETURN_SUCCESS           Value is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                   and String contains more than
                                   PcdMaximumAsciiStringLength Ascii
                                   characters, not including the
                                   Null-terminator.
  @retval RETURN_UNSUPPORTED       If the number represented by String exceeds
                                   the range defined by UINTN.

**/
RETURN_STATUS
EFIAPI
AsciiStrDecimalToUintnS (
  IN  CONST CHAR8              *String,
  OUT       CHAR8              **EndPointer,  OPTIONAL
  OUT       UINTN              *Data
  )
{
  //
  // 1. Neither String nor Data shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Data != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. The length of String shall not be greater than ASCII_RSIZE_MAX.
  //
//  if (ASCII_RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((AsciiStrnLenS (String, ASCII_RSIZE_MAX + 1) <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR8 *) String;
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == ' ') || (*String == '\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == '0') {
    String++;
  }

  *Data = 0;

  while (InternalAsciiIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according to the range
    // defined by UINTN, then MAX_UINTN is stored in *Data and
    // RETURN_UNSUPPORTED is returned.
    //
//    if (*Data > ((MAX_UINTN - (*String - '0')) / 10)) {
//      *Data = MAX_UINTN;
//      if (EndPointer != NULL) {
//        *EndPointer = (CHAR8 *) String;
//      }
//     return RETURN_UNSUPPORTED;
//    }
//    ASSERT (*Data <= ((((UINTN) ~0) - (*String - L'0')) / 10));

    *Data = *Data * 10 + (*String - '0');
    String++;
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR8 *) String;
  }
  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Ascii decimal string to a value of type UINT64.

  This function outputs a value of type UINT64 by interpreting the contents of
  the Ascii string specified by String as a decimal number. The format of the
  input Ascii string String is:

                  [spaces] [decimal digits].

  The valid decimal digit character is in the range [0-9]. The function will
  ignore the pad space, which includes spaces or tab characters, before
  [decimal digits]. The running zero in the beginning of [decimal digits] will
  be ignored. Then, the function stops at the first character that is a not a
  valid decimal character or a Null-terminator, whichever one comes first.

  If String is NULL, then ASSERT().
  If Data is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and String contains more than
  PcdMaximumAsciiStringLength Ascii characters, not including the
  Null-terminator, then ASSERT().

  If String has no valid decimal digits in the above format, then 0 is stored
  at the location pointed to by Data.
  If the number represented by String exceeds the range defined by UINT64, then
  MAX_UINT64 is stored at the location pointed to by Data.

  If EndPointer is not NULL, a pointer to the character that stopped the scan
  is stored at the location pointed to by EndPointer. If String has no valid
  decimal digits right after the optional pad spaces, the value of String is
  stored at the location pointed to by EndPointer.

  @param  String                   Pointer to a Null-terminated Ascii string.
  @param  EndPointer               Pointer to character that stops scan.
  @param  Data                     Pointer to the converted value.

  @retval RETURN_SUCCESS           Value is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                   and String contains more than
                                   PcdMaximumAsciiStringLength Ascii
                                   characters, not including the
                                   Null-terminator.
  @retval RETURN_UNSUPPORTED       If the number represented by String exceeds
                                   the range defined by UINT64.

**/
RETURN_STATUS
EFIAPI
AsciiStrDecimalToUint64S (
  IN  CONST CHAR8              *String,
  OUT       CHAR8              **EndPointer,  OPTIONAL
  OUT       UINT64             *Data
  )
{
  //
  // 1. Neither String nor Data shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Data != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. The length of String shall not be greater than ASCII_RSIZE_MAX.
  //
//  if (ASCII_RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((AsciiStrnLenS (String, ASCII_RSIZE_MAX + 1) <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
 // }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR8 *) String;
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == ' ') || (*String == '\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == '0') {
    String++;
  }

  *Data = 0;

  while (InternalAsciiIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according to the range
    // defined by UINT64, then MAX_UINT64 is stored in *Data and
    // RETURN_UNSUPPORTED is returned.
    //
//    if (*Data > DivU64x32 (MAX_UINT64 - (*String - '0'), 10)) {
//     *Data = MAX_UINT64;
//      if (EndPointer != NULL) {
//        *EndPointer = (CHAR8 *) String;
//      }
//      return RETURN_UNSUPPORTED;
//    }
//    ASSERT (*Data <= DivU64x32 (((UINT64) ~0) - (*String - L'0') , 10));

    *Data = MultU64x32 (*Data, 10) + (*String - '0');
    String++;
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR8 *) String;
  }
  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Ascii hexadecimal string to a value of type UINTN.

  This function outputs a value of type UINTN by interpreting the contents of
  the Ascii string specified by String as a hexadecimal number. The format of
  the input Ascii string String is:

                  [spaces][zeros][x][hexadecimal digits].

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F].
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix. If
  "x" appears in the input string, it must be prefixed with at least one 0. The
  function will ignore the pad space, which includes spaces or tab characters,
  before [zeros], [x] or [hexadecimal digits]. The running zero before [x] or
  [hexadecimal digits] will be ignored. Then, the decoding starts after [x] or
  the first valid hexadecimal digit. Then, the function stops at the first
  character that is a not a valid hexadecimal character or Null-terminator,
  whichever on comes first.

  If String is NULL, then ASSERT().
  If Data is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and String contains more than
  PcdMaximumAsciiStringLength Ascii characters, not including the
  Null-terminator, then ASSERT().

  If String has no valid hexadecimal digits in the above format, then 0 is
  stored at the location pointed to by Data.
  If the number represented by String exceeds the range defined by UINTN, then
  MAX_UINTN is stored at the location pointed to by Data.

  If EndPointer is not NULL, a pointer to the character that stopped the scan
  is stored at the location pointed to by EndPointer. If String has no valid
  hexadecimal digits right after the optional pad spaces, the value of String
  is stored at the location pointed to by EndPointer.

  @param  String                   Pointer to a Null-terminated Ascii string.
  @param  EndPointer               Pointer to character that stops scan.
  @param  Data                     Pointer to the converted value.

  @retval RETURN_SUCCESS           Value is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                   and String contains more than
                                   PcdMaximumAsciiStringLength Ascii
                                   characters, not including the
                                   Null-terminator.
  @retval RETURN_UNSUPPORTED       If the number represented by String exceeds
                                   the range defined by UINTN.

**/
RETURN_STATUS
EFIAPI
AsciiStrHexToUintnS (
  IN  CONST CHAR8              *String,
  OUT       CHAR8              **EndPointer,  OPTIONAL
  OUT       UINTN              *Data
  )
{
  //
  // 1. Neither String nor Data shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Data != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. The length of String shall not be greater than ASCII_RSIZE_MAX.
  //
//  if (ASCII_RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((AsciiStrnLenS (String, ASCII_RSIZE_MAX + 1) <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR8 *) String;
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == ' ') || (*String == '\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == '0') {
    String++;
  }

  if (InternalBaseLibAsciiToUpper (*String) == 'X') {
    if (*(String - 1) != '0') {
      *Data = 0;
      return RETURN_SUCCESS;
    }
    //
    // Skip the 'X'
    //
    String++;
  }

  *Data = 0;

  while (InternalAsciiIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according to the range
    // defined by UINTN, then MAX_UINTN is stored in *Data and
    // RETURN_UNSUPPORTED is returned.
    //
//    if (*Data > ((MAX_UINTN - InternalAsciiHexCharToUintn (*String)) >> 4)) {
//      *Data = MAX_UINTN;
//      if (EndPointer != NULL) {
//        *EndPointer = (CHAR8 *) String;
//      }
//     return RETURN_UNSUPPORTED;
//    }
//    ASSERT (*Data <= ((((UINTN) ~0) - InternalHexCharToUintn (*String)) >> 4));

    *Data = (*Data << 4) + InternalAsciiHexCharToUintn (*String);
    String++;
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR8 *) String;
  }
  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Ascii hexadecimal string to a value of type UINT64.

  This function outputs a value of type UINT64 by interpreting the contents of
  the Ascii string specified by String as a hexadecimal number. The format of
  the input Ascii string String is:

                  [spaces][zeros][x][hexadecimal digits].

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F].
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix. If
  "x" appears in the input string, it must be prefixed with at least one 0. The
  function will ignore the pad space, which includes spaces or tab characters,
  before [zeros], [x] or [hexadecimal digits]. The running zero before [x] or
  [hexadecimal digits] will be ignored. Then, the decoding starts after [x] or
  the first valid hexadecimal digit. Then, the function stops at the first
  character that is a not a valid hexadecimal character or Null-terminator,
  whichever on comes first.

  If String is NULL, then ASSERT().
  If Data is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and String contains more than
  PcdMaximumAsciiStringLength Ascii characters, not including the
  Null-terminator, then ASSERT().

  If String has no valid hexadecimal digits in the above format, then 0 is
  stored at the location pointed to by Data.
  If the number represented by String exceeds the range defined by UINT64, then
  MAX_UINT64 is stored at the location pointed to by Data.

  If EndPointer is not NULL, a pointer to the character that stopped the scan
  is stored at the location pointed to by EndPointer. If String has no valid
  hexadecimal digits right after the optional pad spaces, the value of String
  is stored at the location pointed to by EndPointer.

  @param  String                   Pointer to a Null-terminated Ascii string.
  @param  EndPointer               Pointer to character that stops scan.
  @param  Data                     Pointer to the converted value.

  @retval RETURN_SUCCESS           Value is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                   and String contains more than
                                   PcdMaximumAsciiStringLength Ascii
                                   characters, not including the
                                   Null-terminator.
  @retval RETURN_UNSUPPORTED       If the number represented by String exceeds
                                   the range defined by UINT64.

**/
RETURN_STATUS
EFIAPI
AsciiStrHexToUint64S (
  IN  CONST CHAR8              *String,
  OUT       CHAR8              **EndPointer,  OPTIONAL
  OUT       UINT64             *Data
  )
{
  //
  // 1. Neither String nor Data shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Data != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. The length of String shall not be greater than ASCII_RSIZE_MAX.
  //
//  if (ASCII_RSIZE_MAX != 0) {
 //   SAFE_STRING_CONSTRAINT_CHECK ((AsciiStrnLenS (String, ASCII_RSIZE_MAX + 1) <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR8 *) String;
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == ' ') || (*String == '\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == '0') {
    String++;
  }

  if (InternalBaseLibAsciiToUpper (*String) == 'X') {
    if (*(String - 1) != '0') {
      *Data = 0;
      return RETURN_SUCCESS;
    }
    //
    // Skip the 'X'
    //
    String++;
  }

  *Data = 0;

  while (InternalAsciiIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according to the range
    // defined by UINT64, then MAX_UINT64 is stored in *Data and
    // RETURN_UNSUPPORTED is returned.
    //
//    if (*Data > RShiftU64 (MAX_UINT64 - InternalAsciiHexCharToUintn (*String), 4)) {
//      *Data = MAX_UINT64;
//      if (EndPointer != NULL) {
//        *EndPointer = (CHAR8 *) String;
//      }
//      return RETURN_UNSUPPORTED;
//    }
//      ASSERT (*Data <= RShiftU64 (((UINT64) ~0) - InternalHexCharToUintn (*String) , 4));

    *Data = LShiftU64 (*Data, 4) + InternalAsciiHexCharToUintn (*String);
    String++;
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR8 *) String;
  }
  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Unicode string to a Null-terminated
  ASCII string.

  This function is similar to AsciiStrCpyS.

  This function converts the content of the Unicode string Source
  to the ASCII string Destination by copying the lower 8 bits of
  each Unicode character. The function terminates the ASCII string
  Destination by appending a Null-terminator character at the end.

  The caller is responsible to make sure Destination points to a buffer with size
  equal or greater than ((StrLen (Source) + 1) * sizeof (CHAR8)) in bytes.

  If any Unicode characters in Source contain non-zero value in
  the upper 8 bits, then ASSERT().

  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Source        The pointer to a Null-terminated Unicode string.
  @param  Destination   The pointer to a Null-terminated ASCII string.
  @param  DestMax       The maximum number of Destination Ascii
                        char, including terminating null char.

  @retval RETURN_SUCCESS           String is converted.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than
                                    PcdMaximumAsciiStringLength.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and DestMax is greater than
                                    PcdMaximumUnicodeStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.

**/
RETURN_STATUS
EFIAPI
UnicodeStrToAsciiStrS (
  IN      CONST CHAR16              *Source,
  OUT     CHAR8                     *Destination,
  IN      UINTN                     DestMax
  )
{
  UINTN            SourceLen;

  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than ASCII_RSIZE_MAX or RSIZE_MAX.
  //
//  if (ASCII_RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }
//  if (RSIZE_MAX != 0) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
//  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. DestMax shall be greater than StrnLenS (Source, DestMax).
  //
  SourceLen = StrnLenS (Source, DestMax);
//  SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 5. Copying shall not take place between objects that overlap.
  //
//  SAFE_STRING_CONSTRAINT_CHECK (!InternalSafeStringIsOverlap (Destination, DestMax, (VOID *)Source, (SourceLen + 1) * sizeof(CHAR16)), RETURN_ACCESS_DENIED);

  //
  // convert string
  //
  while ((*Source != '\0') && (--DestMax > 0)) {
    //
    // If any Unicode characters in Source contain
    // non-zero value in the upper 8 bits, then ASSERT().
    //
//    ASSERT (*Source < 0x100);
    *(Destination++) = (CHAR8) (*(Source++) & 0xFF);
  }
  *Destination = '\0';

  return RETURN_SUCCESS;
}

/**
  Convert not more than Length successive characters from a Null-terminated
  Unicode string to a Null-terminated Ascii string. If no null char is copied
  from Source, then Destination[Length] is always set to null.

  This function converts not more than Length successive characters from the
  Unicode string Source to the Ascii string Destination by copying the lower 8
  bits of each Unicode character. The function terminates the Ascii string
  Destination by appending a Null-terminator character at the end.

  The caller is responsible to make sure Destination points to a buffer with
  size not smaller than ((MIN(StrLen(Source), Length) + 1) * sizeof (CHAR8))
  in bytes.

  If any Unicode characters in Source contain non-zero value in the upper 8
  bits, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then Destination and DestinationLength are
  unmodified.

  @param  Source             The pointer to a Null-terminated Unicode string.
  @param  Length             The maximum number of Unicode characters to
                             convert.
  @param  Destination        The pointer to a Null-terminated Ascii string.
  @param  DestMax            The maximum number of Destination Ascii char,
                             including terminating null char.
  @param  DestinationLength  The number of Unicode characters converted.

  @retval RETURN_SUCCESS            String is converted.
  @retval RETURN_INVALID_PARAMETER  If Destination is NULL.
                                    If Source is NULL.
                                    If DestinationLength is NULL.
                                    If PcdMaximumAsciiStringLength is not zero,
                                    and Length or DestMax is greater than
                                    PcdMaximumAsciiStringLength.
                                    If PcdMaximumUnicodeStringLength is not
                                    zero, and Length or DestMax is greater than
                                    PcdMaximumUnicodeStringLength.
                                    If DestMax is 0.
  @retval RETURN_BUFFER_TOO_SMALL   If DestMax is NOT greater than
                                    MIN(StrLen(Source), Length).
  @retval RETURN_ACCESS_DENIED      If Source and Destination overlap.

**/
RETURN_STATUS
EFIAPI
UnicodeStrnToAsciiStrS (
  IN      CONST CHAR16              *Source,
  IN      UINTN                     Length,
  OUT     CHAR8                     *Destination,
  IN      UINTN                     DestMax,
  OUT     UINTN                     *DestinationLength
  )
{
  UINTN            SourceLen;

  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // 1. None of Destination, Source or DestinationLength shall be a null
  // pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((DestinationLength != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither Length nor DestMax shall be greater than ASCII_RSIZE_MAX or
  // RSIZE_MAX.
  //
/*  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }
*/
  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. If Length is not less than DestMax, then DestMax shall be greater than
  // StrnLenS(Source, DestMax).
  //
  SourceLen = StrnLenS (Source, DestMax);
  if (Length >= DestMax) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);
  }

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
  SAFE_STRING_CONSTRAINT_CHECK (!InternalSafeStringIsOverlap (Destination, DestMax, (VOID *)Source, (SourceLen + 1) * sizeof(CHAR16)), RETURN_ACCESS_DENIED);

  *DestinationLength = 0;

  //
  // Convert string
  //
  while ((*Source != 0) && (SourceLen > 0) && (--DestMax > 0)) {
    //
    // If any Unicode characters in Source contain non-zero value in the upper
    // 8 bits, then ASSERT().
    //
//    ASSERT (*Source < 0x100);
    *(Destination++) = (CHAR8) (*(Source++)  & 0xFF);
    SourceLen--;
    (*DestinationLength)++;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Convert one Null-terminated ASCII string to a Null-terminated
  Unicode string.

  This function is similar to StrCpyS.

  This function converts the contents of the ASCII string Source to the Unicode
  string Destination. The function terminates the Unicode string Destination by
  appending a Null-terminator character at the end.

  The caller is responsible to make sure Destination points to a buffer with size
  equal or greater than ((AsciiStrLen (Source) + 1) * sizeof (CHAR16)) in bytes.

  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Source        The pointer to a Null-terminated ASCII string.
  @param  Destination   The pointer to a Null-terminated Unicode string.
  @param  DestMax       The maximum number of Destination Unicode
                        char, including terminating null char.

  @retval RETURN_SUCCESS           String is converted.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and DestMax is greater than
                                    PcdMaximumUnicodeStringLength.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.

**/
RETURN_STATUS
EFIAPI
AsciiStrToUnicodeStrS (
  IN      CONST CHAR8               *Source,
  OUT     CHAR16                    *Destination,
  IN      UINTN                     DestMax
  )
{
  UINTN            SourceLen;

  ASSERT (((UINTN) Destination & BIT0) == 0);

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than RSIZE_MAX or ASCII_RSIZE_MAX.
  //
/*  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }
*/
  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. DestMax shall be greater than AsciiStrnLenS(Source, DestMax).
  //
  SourceLen = AsciiStrnLenS (Source, DestMax);
//  SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 5. Copying shall not take place between objects that overlap.
  //
//  SAFE_STRING_CONSTRAINT_CHECK (!InternalSafeStringIsOverlap (Destination, DestMax * sizeof(CHAR16), (VOID *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // Convert string
  //
  while ((*Source != '\0') && (--DestMax > 0)) {
    *(Destination++) = (CHAR16)*(Source++);
  }
  *Destination = L'\0';

  return RETURN_SUCCESS;
}

/**
  Convert not more than Length successive characters from a Null-terminated
  Ascii string to a Null-terminated Unicode string. If no null char is copied
  from Source, then Destination[Length] is always set to null.

  This function converts not more than Length successive characters from the
  Ascii string Source to the Unicode string Destination. The function
  terminates the Unicode string Destination by appending a Null-terminator
  character at the end.

  The caller is responsible to make sure Destination points to a buffer with
  size not smaller than
  ((MIN(AsciiStrLen(Source), Length) + 1) * sizeof (CHAR8)) in bytes.

  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then Destination and DestinationLength are
  unmodified.

  @param  Source             The pointer to a Null-terminated Ascii string.
  @param  Length             The maximum number of Ascii characters to convert.
  @param  Destination        The pointer to a Null-terminated Unicode string.
  @param  DestMax            The maximum number of Destination Unicode char,
                             including terminating null char.
  @param  DestinationLength  The number of Ascii characters converted.

  @retval RETURN_SUCCESS            String is converted.
  @retval RETURN_INVALID_PARAMETER  If Destination is NULL.
                                    If Source is NULL.
                                    If DestinationLength is NULL.
                                    If PcdMaximumUnicodeStringLength is not
                                    zero, and Length or DestMax is greater than
                                    PcdMaximumUnicodeStringLength.
                                    If PcdMaximumAsciiStringLength is not zero,
                                    and Length or DestMax is greater than
                                    PcdMaximumAsciiStringLength.
                                    If DestMax is 0.
  @retval RETURN_BUFFER_TOO_SMALL   If DestMax is NOT greater than
                                    MIN(AsciiStrLen(Source), Length).
  @retval RETURN_ACCESS_DENIED      If Source and Destination overlap.

**/
RETURN_STATUS
EFIAPI
AsciiStrnToUnicodeStrS (
  IN      CONST CHAR8               *Source,
  IN      UINTN                     Length,
  OUT     CHAR16                    *Destination,
  IN      UINTN                     DestMax,
  OUT     UINTN                     *DestinationLength
  )
{
  UINTN            SourceLen;

  ASSERT (((UINTN) Destination & BIT0) == 0);

  //
  // 1. None of Destination, Source or DestinationLength shall be a null
  // pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((DestinationLength != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither Length nor DestMax shall be greater than ASCII_RSIZE_MAX or
  // RSIZE_MAX.
  //
/*  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }
*/
  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. If Length is not less than DestMax, then DestMax shall be greater than
  // AsciiStrnLenS(Source, DestMax).
  //
  SourceLen = AsciiStrnLenS (Source, DestMax);
//  if (Length >= DestMax) {
//    SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);
//  }

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
//  SAFE_STRING_CONSTRAINT_CHECK (!InternalSafeStringIsOverlap (Destination, DestMax * sizeof(CHAR16), (VOID *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  *DestinationLength = 0;

  //
  // Convert string
  //
  while ((*Source != 0) && (SourceLen > 0)  && (--DestMax > 0)) {
    *(Destination++) = (CHAR16)*(Source++);
    SourceLen--;
    (*DestinationLength)++;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}
