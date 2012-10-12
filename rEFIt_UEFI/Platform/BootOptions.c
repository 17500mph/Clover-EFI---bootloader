/*
 *  BootOptions.c
 *  
 *  Created by dmazar, 10.2012.
 *  
 *  Functions for modifying UEFI boot options.
 *
 */


#include "Platform.h"


#ifndef DEBUG_ALL
#define DEBUG_BO 1
#else
#define DEBUG_BO DEBUG_ALL
#endif

#if DEBUG_BO == 0
#define DBG(...)
#else
#define DBG(...) DebugLog(DEBUG_BO, __VA_ARGS__)
#endif


#define BOOT_ORDER_VAR  L"BootOrder"


///
/// Internal load option struct.
///
typedef struct {
    ///
    /// XXXX in BootXXXX.
    ///
    UINT16    BootNum;
    ///
    /// Pointer to raw EFI_LOAD_OPTION (BootXXXX) variable content.
    ///
    VOID    *Variable;
    ///
    /// Variable size in bytes.
    ///
    UINTN   VariableSize;
    ///
    /// BootOption Attributes (first 4 bytes from Variable).
    ///
    UINT32  Attributes;
    ///
    /// BootOption FilePathListLength (next 2 bytes from Variable).
    ///
    UINT16  FilePathListLength;
    ///
    /// Null terminated BootOption Description (pointer to 6th byte of Variable).
    ///
    CHAR16  *Description;
    ///
    /// Size in bytes of BootOption Description.
    ///
    UINTN   DescriptionSize;
    ///
    /// Pointer to BootOption FilePathList.
    ///
    EFI_DEVICE_PATH_PROTOCOL    *FilePathList;
    ///
    /// Pointer to BootOption OptionalData.
    ///
    UINT8   *OptionalData;
    ///
    /// BootOption OptionalData size in bytes.
    ///
    UINTN   OptionalDataSize;
} BO_BOOT_OPTION;



VOID
WaitForKeyPress(
    CHAR16          *Message
    )
{
    EFI_STATUS      Status;
    UINTN           index;
    EFI_INPUT_KEY   key;
    
    Print(Message);
    do {
        Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &key);
    } while(Status == EFI_SUCCESS);
    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &index);
    do {
        Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &key);
    } while(Status == EFI_SUCCESS);
}


/** Returns upper case version of char - valid only for ASCII chars in unicode. */
CHAR16
EFIAPI
ToUpperChar(
    IN  CHAR16          Chr
    )
{
    CHAR8               C;
    
    
    if (Chr > 0xFF) return Chr;
    C = (CHAR8)Chr;
    return ((C >= 'a' && C <= 'z') ? C - ('a' - 'A') : C);
}


/** Returns 0 if two strings are equal, !=0 otherwise. Compares just first 8 bits of chars (valid for ASCII), case insensitive. */
UINTN
EFIAPI
StrCmpiBasic(
    IN  CHAR16          *String1,
    IN  CHAR16          *String2
    )
{
    CHAR16              Chr1;
    CHAR16              Chr2;
    
    
    if (String1 == NULL || String2 == NULL) {
        return 1;
    }
    if (*String1 == L'\0' && *String2 == L'\0') {
        return 0;
    }
    if (*String1 == L'\0' || *String2 == L'\0') {
        return 1;
    }
    
    Chr1 = ToUpperChar(*String1);
    Chr2 = ToUpperChar(*String2);
    while ((*String1 != L'\0') && (Chr1 == Chr2)) {
        String1++;
        String2++;
        Chr1 = ToUpperChar(*String1);
        Chr2 = ToUpperChar(*String2);
    }
    
    return Chr1 - Chr2;
}


/** Creates device path for boot option: device path for file system volume + file name.
 *  Caller is responsible for releasing DevicePath with FreePool().
 */
EFI_STATUS
CreateBootOptionDevicePath (
    IN  EFI_HANDLE      FileDeviceHandle,
    IN  CHAR16          *FileName,
    OUT EFI_DEVICE_PATH_PROTOCOL    **DevicePath
    )
{
    EFI_STATUS          Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;    
    
    //
    // Check that FileDeviceHandle is file system volume
    //
    Status = gBS->HandleProtocol (FileDeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID**)&Volume);
    if (EFI_ERROR(Status)) {
        DBG("CreateBootOptionDevicePath: FileDeviceHandle %p is not fs volume", FileDeviceHandle);
        return EFI_INVALID_PARAMETER;
    }
    
    //
    // Create file path node with FileName
    //
    *DevicePath = FileDevicePath(FileDeviceHandle, FileName);
    if (*DevicePath == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }
    
    return EFI_SUCCESS;
}


/** Returns TRUE if dev paths are equal. Has special ascii case insensitive compare for file dev paths. */
BOOLEAN
DevicePathEqual (
    IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath1,
    IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath2
    )
{
    BOOLEAN         Equal;
    UINT8           Type1;
    UINT8           SubType1;
    UINTN           Len1;
    CHAR16          *FPath1;
    CHAR16          *FPath2;
    
    
    Equal = FALSE;
    
    while (TRUE) {
        Type1 = DevicePathType (DevicePath1);
        SubType1 = DevicePathSubType (DevicePath1);
        Len1 = DevicePathNodeLength (DevicePath1);
        
        /*
         DBG("Type: %d, %d\n", Type1, DevicePathType (DevicePath2));
         DBG("SubType: %d, %d\n", SubType1, DevicePathSubType (DevicePath2));
         DBG("Len: %d, %d\n", Len1, DevicePathNodeLength (DevicePath2));
         DBG("%s\n", DevicePathToStr(DevicePath1));
         DBG("%s\n", DevicePathToStr(DevicePath2));
         */
        
        if (Type1 != DevicePathType (DevicePath2)
            || SubType1 != DevicePathSubType (DevicePath2)
            || Len1 != DevicePathNodeLength (DevicePath2)
           )
        {
            // Not equal
            //DBG("Not equal type/subtype/len\n");
            break;
        }
        
        //
        // Same type/subtype/len ...
        //
        
        if (IsDevicePathEnd (DevicePath1)) {
            // END node - they are the same
            Equal = TRUE;
            break;
        }
        
        //
        // Do mem compare of nodes or special compare for selected types/subtypes
        //
        if (Type1 == MEDIA_DEVICE_PATH && SubType1 == MEDIA_FILEPATH_DP) {
            //
            // Special compare: case insensitive file path compare + skip leading \ char
            //
            FPath1 = &((FILEPATH_DEVICE_PATH *)DevicePath1)->PathName[0];
            if (FPath1[0] == L'\\') {
                FPath1++;
            }
            FPath2 = &((FILEPATH_DEVICE_PATH *)DevicePath2)->PathName[0];
            if (FPath2[0] == L'\\') {
                FPath2++;
            }
            //DBG("COMPARING: '%s' and '%s'\n", FPath1, FPath2);
            if (StrCmpiBasic(FPath1, FPath2) != 0) {
                // Not equal
                //DBG("Not equal fpaths\n");
                break;
            }
        } else {
            if (CompareMem(DevicePath1, DevicePath2, DevicePathNodeLength (DevicePath1)) != 0) {
                // Not equal
                //DBG("Not equal nodes\n");
                break;
            }
        }
        
        //
        // Advance to next node
        //
        DevicePath1 =  NextDevicePathNode (DevicePath1);
        DevicePath2 =  NextDevicePathNode (DevicePath2);
    }
    
    return Equal;
}


/** Prints BootOrder with DBG. */
VOID
PrintBootOrder (
    IN  UINT16          BootOrder[],
    IN  UINTN           BootOrderLen
    )
{
    UINTN       Index;
    
    
    DBG(" %d: ", BootOrderLen);
    for (Index = 0; Index < BootOrderLen; Index++) {
        if (Index > 0) {
            DBG(", ");
        }
        DBG("Boot%04X", BootOrder[Index]);
    }
    DBG("\n");
    //WaitForKeyPress(L"press a key to continue\n");
}


/** Returns gEfiGlobalVariableGuid:BootOrder as UINT16 array and it's length (num of elements).
 *  Caller is responsible for releasing BootOrder mem (FreePool()).
 */
EFI_STATUS
GetBootOrder (
    OUT UINT16          *BootOrder[],
    OUT UINTN           *BootOrderLen
    )
{
    UINTN               BootOrderSize;
    
    
    DBG("BootOrder:");
    //
    // Basic checks
    //
    if (BootOrder == NULL || BootOrderLen == NULL) {
        DBG(" EFI_INVALID_PARAMETER\n");
        return EFI_INVALID_PARAMETER;
    }
    
    //
    // Get gEfiGlobalVariableGuid:BootOrder and it's length
    //
    *BootOrder = GetNVRAMVariable (BOOT_ORDER_VAR, &gEfiGlobalVariableGuid, &BootOrderSize);
    if (*BootOrder == NULL) {
        DBG(" EFI_NOT_FOUND\n");
        return EFI_NOT_FOUND;
    }
    *BootOrderLen = BootOrderSize / sizeof(UINT16);
    
    PrintBootOrder(*BootOrder, *BootOrderLen);
    
    return EFI_SUCCESS;
}


/** Updates BootOrder by adding new boot option BootNumNew at index BootIndexNew. */
EFI_STATUS
AddToBootOrder (
    IN  UINT16          BootNumNew,
    IN  UINTN           BootIndexNew
    )
{
    EFI_STATUS          Status;
    UINT16              *BootOrder;
    UINT16              *BootOrderNew;
    UINTN               BootOrderLen;
    UINTN               Index;
    
    
    DBG("AddToBootOrder: Boot%04X at index %d\n", BootNumNew, BootIndexNew);
    Status = GetBootOrder (&BootOrder, &BootOrderLen);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    
    if (BootIndexNew > BootOrderLen) {
        BootIndexNew = BootOrderLen;
        DBG("AddToBootOrder: Index too big. Setting to: %d\n", BootIndexNew);
    }
    
    //
    // Make new order buffer with space for our option
    //
    BootOrderNew = AllocateZeroPool ((BootOrderLen + 1) * sizeof(UINT16));
    if (BootOrderNew == NULL) {
        DBG("AddToBootOrder: EFI_OUT_OF_RESOURCES\n");
        return EFI_OUT_OF_RESOURCES;
    }
    BootOrderLen += 1;
    
    
    //
    // Make BootOrderNew array
    //
    
    // copy all before BootIndex first
    for (Index = 0; Index < BootIndexNew; Index++) {
        BootOrderNew[Index] = BootOrder[Index];
    }
    
    // then add our new BootNumNew
    BootOrderNew[BootIndexNew] = BootNumNew;
    
    // then add the rest of previous indexes
    for (Index = BootIndexNew + 1; Index < BootOrderLen; Index++) {
        BootOrderNew[Index] = BootOrder[Index - 1];
    }
    
    //
    // Save it
    //
    Status = gRT->SetVariable (BOOT_ORDER_VAR,
                               &gEfiGlobalVariableGuid,
                               EFI_VARIABLE_NON_VOLATILE
                               | EFI_VARIABLE_BOOTSERVICE_ACCESS
                               | EFI_VARIABLE_RUNTIME_ACCESS,
                               BootOrderLen * sizeof(UINT16),
                               BootOrderNew
                               );
    DBG("SetVariable: %s = %r\n", BOOT_ORDER_VAR, Status);
    
    FreePool (BootOrder);
    FreePool (BootOrderNew);
    
    // Debug: Get and print new BootOrder value
    //GetBootOrder (&BootOrder, &BootOrderLen);
    
    if (EFI_ERROR(Status)) {
        return Status;
    }
    
    return EFI_SUCCESS;
}


/** Updates BootOrder by deleting boot option BootNum. */
EFI_STATUS
DeleteFromBootOrder (
    IN  UINT16          BootNum
    )
{
    EFI_STATUS          Status;
    UINT16              *BootOrder;
    UINTN               BootOrderLen;
    UINTN               Index;
    
    
    DBG("DeleteFromBootOrder: %04X\n", BootNum);
    
    Status = GetBootOrder (&BootOrder, &BootOrderLen);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    
    //
    // Find BootNum and remove it by sliding others to it's place
    //
    for (Index = 0; Index < BootOrderLen; Index++) {
        if (BootOrder[Index] == BootNum) {
            break;
        }
    }
    
    if (Index >= BootOrderLen) {
        DBG("Not found\n");
        return EFI_NOT_FOUND;
    }
    DBG(" found at index %d\n", Index);
    
    //
    // BootNum found at Index - copy the rest over it
    //
    if (Index < BootOrderLen - 1) {
        CopyMem (&BootOrder[Index],
                 &BootOrder[Index + 1],
                 (BootOrderLen - (Index + 1)) * sizeof(UINT16)
                 );
    }
    BootOrderLen -= 1;
    
    //
    // Save it
    //
    Status = gRT->SetVariable (BOOT_ORDER_VAR,
                               &gEfiGlobalVariableGuid,
                               EFI_VARIABLE_NON_VOLATILE
                               | EFI_VARIABLE_BOOTSERVICE_ACCESS
                               | EFI_VARIABLE_RUNTIME_ACCESS,
                               BootOrderLen * sizeof(UINT16),
                               BootOrder
                               );
    DBG("SetVariable: %s = %r\n", BOOT_ORDER_VAR, Status);
    
    FreePool (BootOrder);
    
    // Debug: Get and print new BootOrder value
    //GetBootOrder (&BootOrder, &BootOrderLen);
    
    if (EFI_ERROR(Status)) {
        return Status;
    }
    
    return EFI_SUCCESS;
}


/** Prints BootOption with DBG(). */
VOID
PrintBootOption (
    IN  BO_BOOT_OPTION  *BootOption,
    IN  UINTN           Index
    )
{
    UINTN               VarSizeTmp;
    CHAR16              *FPStr;
    
    
    DBG("%2d) Boot%04X: %s, Attr: 0x%x\n",
        Index, BootOption->BootNum, BootOption->Description, BootOption->Attributes);
    FPStr = DevicePathToStr(BootOption->FilePathList);
    DBG("    %s\n", DevicePathToStr(BootOption->FilePathList));
    FreePool (FPStr);
    
    VarSizeTmp = sizeof(BootOption->Attributes)
                        + sizeof(BootOption->FilePathListLength)
                        + BootOption->DescriptionSize
                        + BootOption->FilePathListLength
                        + BootOption->OptionalDataSize;
    DBG("    Size: %d (Attr:%d + FPl:%d + Desc:%d + FP:%d + Opt:%d = %d -> %s)\n",
        BootOption->VariableSize,
        sizeof(BootOption->Attributes),
        sizeof(BootOption->FilePathListLength),
        BootOption->DescriptionSize,
        BootOption->FilePathListLength,
        BootOption->OptionalDataSize,
        VarSizeTmp,
        VarSizeTmp == BootOption->VariableSize ? L"OK" : L"ERROR"
        );
    //DBG(" FilePathList: %p, %d", BootOption->FilePathList, BootOption->FilePathListLength);
    //DBG(" FP F: %s\n", FileDevicePathToStr(BootOption->FilePathList));
    //DBG(" Description: %p, %d, %s\n", BootOption->Description, BootOption->DescriptionSize, BootOption->Description);
    //DBG("OptionalData: %p, %d\n", BootOption->OptionalData, BootOption->OptionalDataSize);
    
}


/** Parses BootXXXX (XXXX = BootNum) var (from BootOption->Variable) and returns it in BootOption.
 *  BootOption->Variable, BootOption->VariableSize and BootOption->BootNum must be set.
 */
EFI_STATUS
ParseBootOption (
    OUT BO_BOOT_OPTION  *BootOption
    )
{
    UINT8               *Ptr8;
    UINT8               *VarStart;
    UINT8               *VarEnd;
    
    
    if (BootOption->Variable == NULL
        || BootOption->VariableSize <= sizeof(BootOption->Attributes) + sizeof(BootOption->FilePathListLength)
        )
    {
        DBG("ParseBootOption: invalid input params\n");
        return EFI_INVALID_PARAMETER;
    }
    
    VarStart = (UINT8*)BootOption->Variable;
    VarEnd = VarStart + BootOption->VariableSize;
    Ptr8 = VarStart;
    
    // Attributes
    BootOption->Attributes = *((UINT32*)Ptr8);
    Ptr8 += sizeof(BootOption->Attributes);
    
    // FilePathListLength
    BootOption->FilePathListLength = *((UINT16*)Ptr8);
    Ptr8 += sizeof(BootOption->FilePathListLength);
    
    // Description and it's size
    BootOption->Description = (CHAR16*)Ptr8;
    BootOption->DescriptionSize = StrSize(BootOption->Description);
    Ptr8 += BootOption->DescriptionSize;
    if (Ptr8 > VarEnd) {
        DBG("ParseBootOption: invalid boot variable\n");
        return EFI_INVALID_PARAMETER;
    }
    
    // FilePathList
    BootOption->FilePathList = (EFI_DEVICE_PATH_PROTOCOL*)Ptr8;
    Ptr8 += BootOption->FilePathListLength;
    if (Ptr8 > VarEnd) {
        DBG("ParseBootOption: invalid boot variable\n");
        return EFI_INVALID_PARAMETER;
    }
    
    // OptionalData and it's size
    if (Ptr8 < VarEnd) {
        BootOption->OptionalData = Ptr8;
        BootOption->OptionalDataSize = VarEnd - Ptr8;
    } else {
        BootOption->OptionalData = NULL;
        BootOption->OptionalDataSize = 0;
    }
    Ptr8 += BootOption->OptionalDataSize;
    
    return EFI_SUCCESS;
}


/** Compiles boot option params set in BootOption to valid BootXXXX var content (BootOption->Variable).
 *  BootOption fields Attributes, FilePathListLength, FilePathList, Description, OptionalData
 * and OptionalDataSize must be set.
 */
EFI_STATUS
CompileBootOption (
    BO_BOOT_OPTION      *BootOption
    )
{
    UINT8               *Ptr8;
    
    
    if (BootOption->Description == NULL
        || BootOption->FilePathList == NULL
        || BootOption->FilePathListLength == 0
        || (BootOption->OptionalData != NULL && BootOption->OptionalDataSize == 0)
        )
    {
        DBG("CompileBootOption: invalid input params\n");
        return EFI_INVALID_PARAMETER;
    }
    
    BootOption->DescriptionSize = StrSize(BootOption->Description);
    BootOption->VariableSize = sizeof(BootOption->Attributes)
                                + sizeof(BootOption->FilePathListLength)
                                + BootOption->DescriptionSize
                                + BootOption->FilePathListLength
                                + BootOption->OptionalDataSize;
    BootOption->Variable = AllocateZeroPool (BootOption->VariableSize);
    if (BootOption->Variable == NULL) {
        DBG("CompileBootOption: EFI_OUT_OF_RESOURCES\n");
        return EFI_OUT_OF_RESOURCES;
    }
    Ptr8 = (UINT8*)BootOption->Variable;
    
    // Attributes
    *((UINT32*)Ptr8) = BootOption->Attributes;
    Ptr8 += sizeof(BootOption->Attributes);
    
    // FilePathListLength;
    *((UINT16*)Ptr8) = BootOption->FilePathListLength;
    Ptr8 += sizeof(BootOption->FilePathListLength);
    
    // Description
    CopyMem ((CHAR16*)Ptr8, BootOption->Description, BootOption->DescriptionSize);
    Ptr8 += BootOption->DescriptionSize;
    
    // FilePathList
    CopyMem (Ptr8, BootOption->FilePathList, BootOption->FilePathListLength);
    Ptr8 += BootOption->FilePathListLength;
    
    // OptionalData
    if (BootOption->OptionalDataSize > 0) {
        CopyMem (Ptr8, BootOption->OptionalData, BootOption->OptionalDataSize);
        Ptr8 += BootOption->OptionalDataSize;
    }
    
    return EFI_SUCCESS;
}


/** Reads BootXXXX (XXXX = BootNum) var, parses it and returns in BootOption.
 *  Caller is responsible for releasing BootOption->Variable with FreePool().
 */
EFI_STATUS
GetBootOption (
    IN  UINT16          BootNum,
    OUT BO_BOOT_OPTION  *BootOption
    )
{
    CHAR16              VarName[16];
    
    //
    // Get BootXXXX var.
    //
    BootOption->BootNum = BootNum;
    UnicodeSPrint (VarName, sizeof(VarName), L"Boot%04X", BootNum);
    
    BootOption->Variable = GetNVRAMVariable (VarName, &gEfiGlobalVariableGuid, &BootOption->VariableSize);
    if (BootOption->Variable == NULL) {
        return EFI_NOT_FOUND;
    }
    
    //
    // Parse it.
    //
    return ParseBootOption (BootOption);
}


/** Returns BootNum: XXXX of first unoccupied BootXXXX var slot. */
EFI_STATUS
FindFreeBootNum (
    UINT16              *BootNum
    )
{
    EFI_STATUS          Status;
    UINTN               Index;
    CHAR16              VarName[16];
    UINTN               VarSize;
    
    
    for (Index = 0; Index <= 0xFFFF; Index++) {
        UnicodeSPrint (VarName, sizeof(VarName), L"Boot%04X", Index);
        VarSize = 0;
        Status = gRT->GetVariable (VarName, &gEfiGlobalVariableGuid, NULL, &VarSize, NULL);
        if (Status == EFI_NOT_FOUND) {
            *BootNum = (UINT16)Index;
            return EFI_SUCCESS;
        }
    }
    
    return EFI_NOT_FOUND;
}


/** Searches BootXXXX vars for entry that points to given FileDeviceHandle/FileName
 *  and returns BootNum (XXXX in BootXXXX variable name) and BootIndex (index in BootOrder)
 *  if found.
 */
EFI_STATUS
FindBootOptionForFile (
    IN  EFI_HANDLE      FileDeviceHandle,
    IN  CHAR16          *FileName,
    OUT UINT16          *BootNum,
    OUT UINTN           *BootIndex
    )
{
    EFI_STATUS          Status;
    UINT16              *BootOrder;
    UINTN               BootOrderLen;
    UINTN               Index;
    BO_BOOT_OPTION      BootOption;
    EFI_DEVICE_PATH_PROTOCOL    *SearchedDevicePath;
    UINTN               SearchedDevicePathSize;
    
    
    DBG("\nFindBootOptionForFile: %p, %s\n", FileDeviceHandle, FileName);
    
    //
    // Get BootOrder - we will search only options listed in BootOrder.
    //
    Status = GetBootOrder (&BootOrder, &BootOrderLen);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    
    //
    // Create FileDeviceHandle/FileName device path - we will search boot options for that.
    //
    Status = CreateBootOptionDevicePath (FileDeviceHandle, FileName, &SearchedDevicePath);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    SearchedDevicePathSize = GetDevicePathSize (SearchedDevicePath);
    DBG(" Searching for: %d, %s\n", SearchedDevicePathSize, DevicePathToStr(SearchedDevicePath));
    
    //
    // Iterate over all BootXXXX vars (actually, only ones that are in BootOrder list)
    //
    BootOption.Variable = NULL;
    for (Index = 0; Index < BootOrderLen; Index++) {
        if (BootOption.Variable != NULL) {
            FreePool (BootOption.Variable);
            BootOption.Variable = NULL;
        }
        //
        // Get boot option
        //
        Status = GetBootOption (BootOrder[Index], &BootOption);
        if (EFI_ERROR(Status)) {
            DBG("FindBootOptionForFile: Boot%04X: ERROR: %r\n", BootOrder[Index], Status);
            //WaitForKeyPress(L"press a key to continue\n\n");
            continue;
        }
        
        //PrintBootOption (&BootOption, Index);
        
        if (DevicePathEqual (SearchedDevicePath, BootOption.FilePathList)) {
            DBG("FindBootOptionForFile: Found Boot%04X, at index %d\n", BootOrder[Index], Index);
            if (BootNum != NULL) {
                *BootNum = BootOrder[Index];
            }
            if (BootIndex != NULL) {
                *BootIndex = Index;
            }
            FreePool (BootOption.Variable);
            //WaitForKeyPress(L"press a key to continue\n\n");
            return EFI_SUCCESS;
        }
        //WaitForKeyPress(L"press a key to continue\n\n");
    }
    
    if (BootOption.Variable != NULL) {
        FreePool (BootOption.Variable);
    }
    
    DBG("FindBootOptionForFile: Not found.\n");
    return EFI_NOT_FOUND;
}

/** Prints BootXXXX vars found listed in BootOrder, plus print others if AllBootOptions == TRUE. */
VOID
PrintBootOptions (
    IN  BOOLEAN         AllBootOptions
    )
{
    EFI_STATUS          Status;
    UINT16              *BootOrder;
    UINTN               BootOrderLen;
    UINTN               Index;
    UINTN               BootNum;
    BO_BOOT_OPTION      BootOption;
    BOOLEAN             FoundOthers;
    
    
    DBG("\nBoot options:\n-------------\n");
    
    //
    // Get BootOrder - we will search only options listed in BootOrder.
    //
    Status = GetBootOrder (&BootOrder, &BootOrderLen);
    if (EFI_ERROR(Status)) {
        return;
    }
    
    //
    // Iterate over all BootXXXX vars (actually, only ones that are in BootOrder list)
    //
    BootOption.Variable = NULL;
    for (Index = 0; Index < BootOrderLen; Index++) {
        //
        // Get boot option
        //
        Status = GetBootOption (BootOrder[Index], &BootOption);
        if (EFI_ERROR(Status)) {
            DBG("%2d) Boot%04X: ERROR, not found: %r\n", Index, BootOrder[Index], Status);
            continue;
        }
        
        PrintBootOption (&BootOption, Index);
        FreePool (BootOption.Variable);
    }
    
    if (AllBootOptions) {
        DBG("\nBoot options not in BootOrder list:\n");
        FoundOthers = FALSE;
        //
        // Additionally print BootXXXX vars which are not in BootOrder
        //
        BootOption.Variable = NULL;
        for (BootNum = 0; BootNum <= 0xFFFF; BootNum++) {
            //
            // Check if it is in BootOrder
            //
            for (Index = 0; Index < BootOrderLen; Index++) {
                if (BootNum == (UINTN)BootOrder[Index]) {
                    break;
                }
            }
            if (Index < BootOrderLen) {
                // exists in BootOrder - skip it
                continue;
            }
            
            //
            // Get boot option
            //
            Status = GetBootOption ((UINT16)BootNum, &BootOption);
            if (EFI_ERROR(Status)) {
                continue;
            }
            
            PrintBootOption (&BootOption, 0);
            FreePool (BootOption.Variable);
            FoundOthers = TRUE;
        }
        if (!FoundOthers) {
            DBG(" not found\n");
        }
    }
    
    DBG("-------------\n");
    //WaitForKeyPress(L"press a key to continue\n\n");
}



/** Adds new boot option with data specified in BootOption,
 *  to be at BootIndex in the list of options (0 based).
 *  BootOption should be populated with: Attributes, FilePathListLength,
 *  FilePathList, Description, OptionalData and OptionalDataSize.
 *  BootOption->BootNum will contain XXXX from added BootXXXX var
 *  on success.
 */
EFI_STATUS
AddBootOption (
    IN  BO_BOOT_OPTION  *BootOption,
    IN  UINTN           BootIndex
    )
{
    EFI_STATUS          Status;
    CHAR16              VarName[16];
    
    
    DBG("AddBootOption: %s\n", BootOption->Description);
    DBG(" FP: %s\n", DevicePathToStr(BootOption->FilePathList));
    DBG(" BootIndex: %d\n", BootIndex);
    
    //
    // Find free BootXXXX var slot.
    //
    Status = FindFreeBootNum (&BootOption->BootNum);
    if (EFI_ERROR(Status)) {
        DBG("FindFreeBootNum: %r\n", Status);
        return Status;
    }
    DBG(" Found BootNum: %04X\n", BootOption->BootNum);
    UnicodeSPrint (VarName, sizeof(VarName), L"Boot%04X", BootOption->BootNum);
    
    //
    // Prepare BootOption variable
    //
    Status = CompileBootOption (BootOption);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    
    //
    // Save BootXXXX var
    //
    Status = gRT->SetVariable (VarName,
                               &gEfiGlobalVariableGuid,
                               EFI_VARIABLE_NON_VOLATILE
                               | EFI_VARIABLE_BOOTSERVICE_ACCESS
                               | EFI_VARIABLE_RUNTIME_ACCESS,
                               BootOption->VariableSize,
                               BootOption->Variable
                               );
    if (EFI_ERROR(Status)) {
        DBG("SetVariable: %s = %r\n", VarName, Status);
        return Status;
    }
    DBG(" %s saved\n", VarName);
    
    //
    // Free allocated space
    //
    FreePool (BootOption->Variable);
    
    //
    // Update BootOrder - add our new boot option as BootIndex in the list
    //
    Status = AddToBootOrder (BootOption->BootNum, BootIndex);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    
    return EFI_SUCCESS;
}


/** Adds new boot option for given file system device handle FileDeviceHandle, file path FileName
 *  and Description, to be BootIndex in the list of options (0 based).
 */
EFI_STATUS
AddBootOptionForFile (
    IN  EFI_HANDLE      FileDeviceHandle,
    IN  CHAR16          *FileName,
    IN  CHAR16          *Description,
    IN  UINTN           BootIndex,
    OUT UINT16          *BootNum
    )
{
    EFI_STATUS          Status;
    BO_BOOT_OPTION      BootOption;
    
    
    DBG("\nAddBootOptionForFile: %p, %s\n %s, %d\n", FileDeviceHandle, FileName, Description, BootIndex);
    
    //
    // Prepare BootOption FilePath from FileDeviceHandle and FileName
    //
    Status = CreateBootOptionDevicePath (FileDeviceHandle, FileName, &BootOption.FilePathList);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    
    //
    // Prepare BootOption variable
    //
    BootOption.Attributes = LOAD_OPTION_ACTIVE;
    BootOption.FilePathListLength = GetDevicePathSize (BootOption.FilePathList);
    BootOption.Description = Description;
    BootOption.OptionalData = NULL;
    BootOption.OptionalDataSize = 0;
    
    Status = AddBootOption (&BootOption, BootIndex);
    if (EFI_ERROR(Status)) {
        FreePool (BootOption.FilePathList);
        DBG("AddBootOptionForFile: Error: %r\n", Status);
        return Status;
    }
    
    //
    // Free allocated space
    //
    FreePool (BootOption.FilePathList);
    
    //
    // Output vars
    //
    if (BootNum != NULL) {
        *BootNum = BootOption.BootNum;
    }
    
    DBG("AddBootOptionForFile: done.\n");
    //WaitForKeyPress(L"press a key to continue\n\n");
    return EFI_SUCCESS;
}


/** Deletes boot option specified with BootNum (XXXX in BootXXXX var name). */
EFI_STATUS
DeleteBootOption (
    IN  UINT16          BootNum
    )
{
    EFI_STATUS          Status;
    CHAR16              VarName[16];
    
    
    DBG("DeleteBootOption: Boot%04X\n", BootNum);
    
    UnicodeSPrint (VarName, sizeof(VarName), L"Boot%04X", BootNum);
    
    //
    // Delete BootXXXX var
    //
    Status = gRT->SetVariable (VarName,
                               &gEfiGlobalVariableGuid,
                               0,
                               0,
                               NULL
                               );
    if (EFI_ERROR(Status)) {
        DBG(" Error del. variable: %s = %r\n", VarName, Status);
        return Status;
    }
    DBG(" %s deleted\n", VarName);
    
    //
    // Update BootOrder - delete our boot option from the list
    //
    Status = DeleteFromBootOrder (BootNum);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    
    return EFI_SUCCESS;
}


/** Deletes boot option for file specified with FileDeviceHandle and FileName. */
EFI_STATUS
DeleteBootOptionForFile (
    IN  EFI_HANDLE      FileDeviceHandle,
    IN  CHAR16          *FileName
    )
{
    EFI_STATUS          Status;
    IN  UINT16          BootNum;
    
    
    DBG("\nDeleteBootOptionForFile: %p, %s\n", FileDeviceHandle, FileName);
    
    Status = FindBootOptionForFile (FileDeviceHandle, FileName, &BootNum, NULL);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    
    Status = DeleteBootOption (BootNum);
    
    DBG("DeleteBootOptionForFile: done.\n");
    //WaitForKeyPress(L"press a key to continue\n\n");
    return Status;
}

