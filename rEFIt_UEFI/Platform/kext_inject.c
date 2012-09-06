#include "kext_inject.h"
#include "kext_patcher.c"


////////////////////
// globals
////////////////////
LIST_ENTRY gKextList = INITIALIZE_LIST_HEAD_VARIABLE (gKextList);


////////////////////
// before booting
////////////////////
EFI_STATUS EFIAPI ThinFatFile(IN OUT UINT8 **binary, IN OUT UINTN *length, IN cpu_type_t archCpuType) 
{
	UINT32 nfat, swapped, size = 0;
	FAT_HEADER *fhp = (FAT_HEADER *)*binary;
	FAT_ARCH   *fap = (FAT_ARCH *)(*binary + sizeof(FAT_HEADER));
	cpu_type_t fapcputype;
	UINT32 fapoffset;
	UINT32 fapsize;	

	if (fhp->magic == FAT_MAGIC) {
		nfat = fhp->nfat_arch;
		swapped = 0;
	} else if (fhp->magic == FAT_CIGAM) {
		nfat = SwapBytes32(fhp->nfat_arch);
		swapped = 1;
	} else {
		return EFI_NOT_FOUND;
	}

	for (; nfat > 0; nfat--, fap++) {
		if (swapped) {
			fapcputype = SwapBytes32(fap->cputype);
			fapoffset = SwapBytes32(fap->offset);
			fapsize = SwapBytes32(fap->size);
		} else {
			fapcputype = fap->cputype;
			fapoffset = fap->offset;
			fapsize = fap->size;
		}
		if (fapcputype == archCpuType) {
			*binary = (*binary + fapoffset);
			size = fapsize;
			break;
		}
	}

	if (length != 0) *length = size;

	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI LoadKext(IN CHAR16 *FileName, IN cpu_type_t archCpuType, IN OUT _DeviceTreeBuffer *kext)
{
	EFI_STATUS	Status;
	UINT8*		infoDictBuffer = 0;
	UINTN		infoDictBufferLength = 0;
	UINT8*		executableBuffer = 0;
	UINTN		executableBufferLength = 0;
	CHAR8*		bundlePathBuffer = 0;
	UINTN		bundlePathBufferLength = 0;
	CHAR16		TempName[256];
	CHAR16		Executable[256];

	TagPtr		dict = NULL;
	TagPtr		prop = NULL;

	UnicodeSPrint(TempName, 512, L"%s\\%s", FileName, L"Contents\\Info.plist");
	Status = egLoadFile(SelfVolume->RootDir, TempName, &infoDictBuffer, &infoDictBufferLength);
	if (EFI_ERROR(Status)) {
		MsgLog("Failed to load extra kext: %s\n", FileName);
		return EFI_NOT_FOUND;
	} else {
		if(ParseXML((CHAR8*)infoDictBuffer,&dict)!=0) {
			FreePool(infoDictBuffer);
			MsgLog("Failed to load extra kext: %s\n", FileName);
			return EFI_NOT_FOUND;
		}
		prop=GetProperty(dict,"CFBundleExecutable");
		if(prop!=0) {
			AsciiStrToUnicodeStr(prop->string,Executable);
			UnicodeSPrint(TempName, 512, L"%s\\%s\\%s", FileName, L"Contents\\MacOS",Executable);
			Status = egLoadFile(SelfVolume->RootDir, TempName, &executableBuffer, &executableBufferLength);
			if (EFI_ERROR(Status)) {
				FreePool(infoDictBuffer);
				MsgLog("Failed to load extra kext: %s\n", FileName);
				return EFI_NOT_FOUND;
			}
			if (ThinFatFile(&executableBuffer, &executableBufferLength, archCpuType)) {
				FreePool(infoDictBuffer);
				FreePool(executableBuffer);
				MsgLog("Thinning failed: %s\n", FileName);
				return EFI_NOT_FOUND;
			}
		}
		bundlePathBufferLength = StrLen(FileName) + 1;
		bundlePathBuffer = AllocateZeroPool(bundlePathBufferLength);
		UnicodeStrToAsciiStr(FileName, bundlePathBuffer);

		kext->length = sizeof(_BooterKextFileInfo) + infoDictBufferLength + executableBufferLength + bundlePathBufferLength;
		kext->paddr = (UINT32)(UINTN)AllocatePool(kext->length);
		((_BooterKextFileInfo*)(UINTN) kext->paddr)->infoDictPhysAddr = sizeof(_BooterKextFileInfo);
		((_BooterKextFileInfo*)(UINTN) kext->paddr)->infoDictLength = infoDictBufferLength;
		((_BooterKextFileInfo*)(UINTN) kext->paddr)->executablePhysAddr = sizeof(_BooterKextFileInfo) + infoDictBufferLength;
		((_BooterKextFileInfo*)(UINTN) kext->paddr)->executableLength = executableBufferLength;
		((_BooterKextFileInfo*)(UINTN) kext->paddr)->bundlePathPhysAddr = sizeof(_BooterKextFileInfo) + infoDictBufferLength + executableBufferLength;
		((_BooterKextFileInfo*)(UINTN) kext->paddr)->bundlePathLength = bundlePathBufferLength;
		CopyMem((VOID*)(UINTN)kext->paddr + sizeof(_BooterKextFileInfo), infoDictBuffer, infoDictBufferLength);
		CopyMem((VOID*)(UINTN)kext->paddr + sizeof(_BooterKextFileInfo) + infoDictBufferLength, executableBuffer, executableBufferLength);
		CopyMem((VOID*)(UINTN)kext->paddr + sizeof(_BooterKextFileInfo) + infoDictBufferLength + executableBufferLength, bundlePathBuffer, bundlePathBufferLength);
		FreePool(infoDictBuffer);
		FreePool(executableBuffer);
		FreePool(bundlePathBuffer);
	}
	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI AddKext(IN CHAR16 *FileName, IN cpu_type_t archCpuType)
{
	EFI_STATUS	Status;
	KEXT_ENTRY	*KextEntry;

	KextEntry = AllocatePool (sizeof(KEXT_ENTRY));
	KextEntry->Signature = KEXT_SIGNATURE;
	Status = LoadKext(FileName, archCpuType, &KextEntry->kext);
	if(EFI_ERROR(Status)) {
		FreePool(KextEntry);
	} else {
		InsertTailList (&gKextList, &KextEntry->Link);
	}

	return Status;
}

UINT32 GetListCount(LIST_ENTRY const* List)
{
	LIST_ENTRY		*Link;
	UINT32			Count=0;

	if(!IsListEmpty(List)) {
		for (Link = List->ForwardLink; Link != List; Link = Link->ForwardLink) 
			Count++;
	}

	return Count;
}

UINT32 GetKextCount()
{
	return (UINT32)GetListCount(&gKextList);
}

UINT32 GetKextsSize()
{
	LIST_ENTRY		*Link;
	KEXT_ENTRY		*KextEntry;
	UINT32			kextsSize=0;

	if(!IsListEmpty(&gKextList)) {
		for (Link = gKextList.ForwardLink; Link != &gKextList; Link = Link->ForwardLink) {
			KextEntry = CR(Link, KEXT_ENTRY, Link, KEXT_SIGNATURE);
			kextsSize += RoundPage(KextEntry->kext.length);
		}
	}
	return kextsSize;
}

EFI_STATUS LoadKexts(IN LOADER_ENTRY *Entry)
{
	EFI_STATUS              Status;
	REFIT_VOLUME            *Volume;
	CHAR16                  *SrcDir = NULL;
	REFIT_DIR_ITER          KextIter;
	EFI_FILE_INFO           *KextFile;
	REFIT_DIR_ITER          PlugInIter;
	EFI_FILE_INFO           *PlugInFile;
	CHAR16                  FileName[256];
	CHAR16                  PlugIns[256];
#if defined(MDE_CPU_X64)
	cpu_type_t archCpuType=CPU_TYPE_X86_64;
#else
	cpu_type_t archCpuType=CPU_TYPE_I386;
#endif
	UINTN					mm_extra_size;
	VOID					*mm_extra;
	UINTN					extra_size;
	VOID					*extra;

	// TODO: dmazar - archCpuType from detected kernel_patcher/is64BitKernel ?
	if     (AsciiStrStr(gSettings.BootArgs,"arch=x86_64")!=NULL)	archCpuType = CPU_TYPE_X86_64;
	else if(AsciiStrStr(gSettings.BootArgs,"arch=i386")!=NULL)		archCpuType = CPU_TYPE_I386;
	else if(AsciiStrnCmp(OSVersion,"10.8",4)==0)    	         	archCpuType = CPU_TYPE_X86_64;
	else if(AsciiStrnCmp(OSVersion,"10.7",4)!=0)					archCpuType = CPU_TYPE_I386;

	Volume = Entry->Volume;
	SrcDir = GetExtraKextsDir(Volume);
	if (SrcDir != NULL) {
		MsgLog("Injecting kexts from %s\n", SrcDir);
		// look through contents of the directory
		DirIterOpen(SelfVolume->RootDir, SrcDir, &KextIter);
		while (DirIterNext(&KextIter, 1, L"*.kext", &KextFile)) {
			if (KextFile->FileName[0] == '.' || StrStr(KextFile->FileName, L".kext") == NULL)
				continue;   // skip this
			
			UnicodeSPrint(FileName, 512, L"%s\\%s", SrcDir, KextFile->FileName);
			MsgLog("Extra kext: %s\n", FileName);
			AddKext(FileName, archCpuType);

			UnicodeSPrint(PlugIns, 512, L"%s\\%s", FileName, L"Contents\\PlugIns");
			DirIterOpen(SelfVolume->RootDir, PlugIns, &PlugInIter);
			while (DirIterNext(&PlugInIter, 1, L"*.kext", &PlugInFile)) {
				if (PlugInFile->FileName[0] == '.' || StrStr(PlugInFile->FileName, L".kext") == NULL)
					continue;   // skip this
				
				UnicodeSPrint(FileName, 512, L"%s\\%s", PlugIns, PlugInFile->FileName);
				MsgLog("Extra PlugIn kext: %s\n", FileName);
				AddKext(FileName, archCpuType);
			}
			DirIterClose(&PlugInIter);
		}
		DirIterClose(&KextIter);
	}

	// reserve space in the device tree
	if (GetKextCount() > 0) {
		mm_extra_size = GetKextCount() * (sizeof(DeviceTreeNodeProperty) + sizeof(_DeviceTreeBuffer));
		mm_extra = AllocateZeroPool(mm_extra_size - sizeof(DeviceTreeNodeProperty));
		Status =  LogDataHub(&gEfiMiscSubClassGuid, L"mm_extra", mm_extra, mm_extra_size - sizeof(DeviceTreeNodeProperty));
		extra_size = GetKextsSize();
		extra = AllocateZeroPool(extra_size - sizeof(DeviceTreeNodeProperty) + EFI_PAGE_SIZE);
		Status =  LogDataHub(&gEfiMiscSubClassGuid, L"extra", extra, extra_size - sizeof(DeviceTreeNodeProperty) + EFI_PAGE_SIZE);
		MsgLog("count: %d       \n", GetKextCount());
		MsgLog("mm_extra_size: %d       \n", mm_extra_size);
		MsgLog("extra_size: %d       \n", extra_size);
		MsgLog("offset: %d           \n", extra_size - sizeof(DeviceTreeNodeProperty) + EFI_PAGE_SIZE);
	}

	return EFI_SUCCESS;
}


////////////////////
// OnExitBootServices
////////////////////
EFI_STATUS InjectKexts(/*IN EFI_MEMORY_DESCRIPTOR *Desc*/ IN UINT32 deviceTreeP, IN UINT32* deviceTreeLength)
{
	UINT8					*dtEntry = (UINT8*)(UINTN) deviceTreeP;
	UINTN					dtLength = (UINTN) *deviceTreeLength;

	DTEntry					platformEntry;
	DTEntry					memmapEntry;
	CHAR8 					*ptr;
	struct OpaqueDTPropertyIterator OPropIter;
	DTPropertyIterator		iter = &OPropIter;
	DeviceTreeNodeProperty	*prop = NULL;

	UINT8					*infoPtr = 0;
	UINT8					*extraPtr = 0;
	UINT8					*drvPtr = 0;
	UINTN					offset = 0;

	LIST_ENTRY				*Link;
	KEXT_ENTRY				*KextEntry;
	UINTN					KextBase = 0;
	_DeviceTreeBuffer		*mm;
	_BooterKextFileInfo		*drvinfo;

	if (GetKextCount() == 0) return EFI_SUCCESS;

	// kextsBase = Desc->PhysicalStart + (((UINTN) Desc->NumberOfPages) * EFI_PAGE_SIZE);
	// kextsPages = EFI_SIZE_TO_PAGES(kext.length);
	// Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, kextsPages, &kextsBase);
	// if (EFI_ERROR(Status)) { MsgLog("Kext inject: could not allocate memory\n"); return Status; }
	// Desc->NumberOfPages += kextsPages;
	// CopyMem((VOID*)kextsBase, (VOID*)(UINTN)kext.paddr, kext.length);
	// drvinfo = (_BooterKextFileInfo*) kextsBase;
	// drvinfo->infoDictPhysAddr += (UINT32)kextsBase;
	// drvinfo->executablePhysAddr += (UINT32)kextsBase;
	// drvinfo->bundlePathPhysAddr += (UINT32)kextsBase;

	DTInit(dtEntry);
	if(DTLookupEntry(NULL,"/chosen/memory-map",&memmapEntry)==kSuccess) {
		if(DTCreatePropertyIteratorNoAlloc(memmapEntry,iter)==kSuccess) {
			while(DTIterateProperties(iter,&ptr)==kSuccess) {
				prop = iter->currentProperty;
				drvPtr = (UINT8*) prop;
				if(AsciiStrnCmp(prop->name, "Driver-", 7)==0 || AsciiStrnCmp(prop->name, "DriversPackage-", 15)==0) {
					break;
				}
			}
		}
	}

	if(DTLookupEntry(NULL,"/efi/platform",&platformEntry)==kSuccess) {
		if(DTCreatePropertyIteratorNoAlloc(platformEntry,iter)==kSuccess) {
			while(DTIterateProperties(iter,&ptr)==kSuccess) {
				prop = iter->currentProperty;
				if(AsciiStrCmp(prop->name,"mm_extra")==0) {
					infoPtr = (UINT8*) prop;
				}
				if(AsciiStrCmp(prop->name,"extra")==0) {
					extraPtr = (UINT8*) prop;
				}
			}
		}
	}

	if (drvPtr == 0 || infoPtr == 0 || extraPtr == 0 || drvPtr > infoPtr || drvPtr > extraPtr || infoPtr > extraPtr) {
		Print(L"\nInvalid device tree for kext injection");
		return EFI_INVALID_PARAMETER;
	}

	// make space for memory map entries
	platformEntry->nProperties -= 2;
	offset = sizeof(DeviceTreeNodeProperty) + ((DeviceTreeNodeProperty*) infoPtr)->length;
	CopyMem(drvPtr+offset, drvPtr, infoPtr-drvPtr);

	// make space behind device tree
	// platformEntry->nProperties--;
	offset = sizeof(DeviceTreeNodeProperty)+((DeviceTreeNodeProperty*) extraPtr)->length;
	CopyMem(extraPtr, extraPtr+offset, dtLength-(UINTN)(extraPtr-dtEntry)-offset);
	*deviceTreeLength -= offset; 

	KextBase = RoundPage(dtEntry + *deviceTreeLength);
	if(!IsListEmpty(&gKextList)) {
		for (Link = gKextList.ForwardLink; Link != &gKextList; Link = Link->ForwardLink) {
			KextEntry = CR(Link, KEXT_ENTRY, Link, KEXT_SIGNATURE);

			CopyMem((VOID*) KextBase, (VOID*)(UINTN) KextEntry->kext.paddr, KextEntry->kext.length);
			drvinfo = (_BooterKextFileInfo*) KextBase;
			drvinfo->infoDictPhysAddr += (UINT32) KextBase;
			drvinfo->executablePhysAddr += (UINT32) KextBase;
			drvinfo->bundlePathPhysAddr += (UINT32) KextBase;

			memmapEntry->nProperties++;
			prop = ((DeviceTreeNodeProperty*) drvPtr);
			prop->length = sizeof(_DeviceTreeBuffer);
			mm = (_DeviceTreeBuffer*) (((UINT8*)prop) + sizeof(DeviceTreeNodeProperty));
			mm->paddr = KextBase;
			mm->length = KextEntry->kext.length;
			AsciiSPrint(prop->name, 31, "Driver-%x", KextBase);

			drvPtr += sizeof(DeviceTreeNodeProperty) + sizeof(_DeviceTreeBuffer);
			KextBase = RoundPage(KextBase + KextEntry->kext.length);
		}
	}

	return EFI_SUCCESS;
}


////////////////////////////////////
//
// KernelBooterExtensionsPatch to load extra kexts besides kernelcache
//
// 

UINT8   KBELionSearch_X64[]  = { 0xE8, 0x0C, 0xFD, 0xFF, 0xFF, 0xEB, 0x08, 0x48, 0x89, 0xDF };
UINT8   KBELionReplace_X64[] = { 0xE8, 0x0C, 0xFD, 0xFF, 0xFF, 0x90, 0x90, 0x48, 0x89, 0xDF };

UINT8   KBELionSearch_i386[]  = { 0xE8, 0xAA, 0xFB, 0xFF, 0xFF, 0xEB, 0x08, 0x89, 0x34, 0x24 };
UINT8   KBELionReplace_i386[] = { 0xE8, 0xAA, 0xFB, 0xFF, 0xFF, 0x90, 0x90, 0x89, 0x34, 0x24 };

UINT8   KBEMLSearch[]  = { 0xE8, 0x30, 0x00, 0x00, 0x00, 0xEB, 0x08, 0x48, 0x89, 0xDF };
UINT8   KBEMLReplace[] = { 0xE8, 0x30, 0x00, 0x00, 0x00, 0x90, 0x90, 0x48, 0x89, 0xDF };

//
// We can not rely on OSVersion global variable for OS version detection,
// since in some cases it is not correct (install of ML from Lion, for example).
// So, we'll use "brute-force" method - just try to pacth.
// Actually, we'll at least check that if we can find only one instance of code that
// we are planning to patch.
//

#define KERNEL_MAX_SIZE 40000000
VOID EFIAPI KernelBooterExtensionsPatch(IN UINT8 *KernelData)
{

  UINTN   Num = 0;
  UINTN   NumLion_X64 = 0;
  UINTN   NumLion_i386 = 0;
  UINTN   NumML = 0;
  
  DBG(L"\nKernelBooterExtensionsPatch: kernelAddr = %x\nOSVersion (not reliable) = %a\n", KernelData, OSVersion);
  
  if (is64BitKernel) {
	NumLion_X64 = SearchAndCount(KernelData, KERNEL_MAX_SIZE, KBELionSearch_X64, sizeof(KBELionSearch_X64));
	NumML = SearchAndCount(KernelData, KERNEL_MAX_SIZE, KBEMLSearch, sizeof(KBEMLSearch));
  } else {
	NumLion_i386 = SearchAndCount(KernelData, KERNEL_MAX_SIZE, KBELionSearch_i386, sizeof(KBELionSearch_i386));
  }
  
  if (NumLion_X64 + NumLion_i386 + NumML > 1) {
	// more then one pattern found - we do not know what to do with it
	// and we'll skipp it
	DBG(L"==> ERROR: multiple patterns found (LionX64: %d, Lioni386: %d, ML: %d) - skipping patching!\n",
		NumLion_X64, NumLion_i386, NumML);
	//gBS->Stall(10000000);
	return;
  }
  
  if (NumLion_X64 == 1) {
	Num = SearchAndReplace(KernelData, KERNEL_MAX_SIZE, KBELionSearch_X64, sizeof(KBELionSearch_X64), KBELionReplace_X64, 1);
	DBG(L"==> Lion X64: %d replaces done.\n", Num);
  }
  else if (NumLion_i386 == 1) {
	Num = SearchAndReplace(KernelData, KERNEL_MAX_SIZE, KBELionSearch_i386, sizeof(KBELionSearch_i386), KBELionReplace_i386, 1);
	DBG(L"==> Lion i386: %d replaces done.\n", Num);
  }
  else if (NumML == 1) {
	Num = SearchAndReplace(KernelData, KERNEL_MAX_SIZE, KBEMLSearch, sizeof(KBEMLSearch), KBEMLReplace, 1);
	DBG(L"==> MountainLion X64: %d replaces done.\n", Num);
  }
  else {
	DBG(L"==> Patterns not found - no patching done.\n");
  }
  
  //gBS->Stall(5000000);
}
