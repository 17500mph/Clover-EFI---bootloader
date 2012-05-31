/**

  Methods for finding, checking and fixing boot args

  by dmazar

**/

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>

#include "BootArgs.h"
#include "Lib.h"

// DBG_TO: 0=no debug, 1=serial, 2=console
// serial requires
// [PcdsFixedAtBuild]
//  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x07
//  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xFFFFFFFF
// in package DSC file
#define DBG_TO 0

#if DBG_TO == 3
// for debugging with NVRAM
	#define DBG(...) NVRAMDebugLog(__VA_ARGS__);
#elif DBG_TO == 2
	#define DBG(...) AsciiPrint(__VA_ARGS__);
#elif DBG_TO == 1
	#define DBG(...) DebugPrint(1, __VA_ARGS__);
#else
	#define DBG(...)
#endif


VOID
EFIAPI
BootArgsPrint(VOID *bootArgs)
{
#if DBG_TO > 0
	BootArgs1		*BA1 = bootArgs;
	BootArgs2		*BA2 = bootArgs;
	
	if (BA1->Version == kBootArgsVersion1) {
		// pre Lion
		DBG("BootArgs at 0x%p\n", BA1);
		DBG("Revision: 0x%x, Version: 0x%x, efiMode: 0x%x (%d), res10: %x, res11: %x, res12: %x, res20: %x\n",
			BA1->Revision, BA1->Version, BA1->efiMode, BA1->efiMode,
			BA1->__reserved1[0], BA1->__reserved1[1], BA1->__reserved1[2],
			BA1->__reserved2[0]
		);
		DBG("CommandLine: %a\n", BA1->CommandLine);
		DBG("MemoryMap: 0x%x, MMSize: 0x%x, MMDescSize: 0x%x, MMDescVersion: 0x%x\n",
			BA1->MemoryMap, BA1->MemoryMapSize, BA1->MemoryMapDescriptorSize, BA1->MemoryMapDescriptorVersion
		);
		DBG("Boot_Video v_baseAddr: 0x%x, v_display: 0x%x, v_rowBytes: 0x%x, v_width: %d, v_height: %d, v_depth: %d\n",
			BA1->Video.v_baseAddr, BA1->Video.v_display,
			BA1->Video.v_rowBytes, BA1->Video.v_width, BA1->Video.v_height, BA1->Video.v_depth
		);
		DBG("deviceTreeP: 0x%x, deviceTreeLength: 0x%x, kaddr: 0x%x, ksize: 0x%x\n",
			BA1->deviceTreeP, BA1->deviceTreeLength, BA1->kaddr, BA1->ksize
		);
		DBG("efiRTServPgStart: 0x%x, efiRTServPgCount: 0x%x, efiRTServVPgStart: 0x%lx\n",
			BA1->efiRuntimeServicesPageStart, BA1->efiRuntimeServicesPageCount, BA1->efiRuntimeServicesVirtualPageStart
		);
		DBG("efiSystemTable: 0x%x, res2: 0x%x, perfDataStart: 0x%x, perfDataSize: 0x%x\n",
			BA1->efiSystemTable, BA1->__reserved2, BA1->performanceDataStart, BA1->performanceDataSize
		);
		DBG("res30: %x, res31: %x\n", BA1->__reserved3[0], BA1->__reserved3[1]);
	} else {
		// Lion and up
		DBG("BootArgs at 0x%p\n", BA2);
		DBG("Revision: 0x%x, Version: 0x%x, efiMode: 0x%x (%d), debugMode: 0x%x, res10: %x, res11: %x\n",
			BA2->Revision, BA2->Version, BA2->efiMode, BA2->efiMode, BA2->debugMode, BA2->__reserved1[0], BA2->__reserved1[1]
		);
		DBG("CommandLine: %a\n", BA2->CommandLine);
		DBG("MemoryMap: 0x%x, MMSize: 0x%x, MMDescSize: 0x%x, MMDescVersion: 0x%x\n",
			BA2->MemoryMap, BA2->MemoryMapSize, BA2->MemoryMapDescriptorSize, BA2->MemoryMapDescriptorVersion
		);
		DBG("Boot_Video v_baseAddr: 0x%x, v_display: 0x%x, v_rowBytes: 0x%x, v_width: %d, v_height: %d, v_depth: %d\n",
			BA2->Video.v_baseAddr, BA2->Video.v_display,
			BA2->Video.v_rowBytes, BA2->Video.v_width, BA2->Video.v_height, BA2->Video.v_depth
		);
		DBG("deviceTreeP: 0x%x, deviceTreeLength: 0x%x, kaddr: 0x%x, ksize: 0x%x\n",
			BA2->deviceTreeP, BA2->deviceTreeLength, BA2->kaddr, BA2->ksize
		);
		DBG("efiRTServPgStart: 0x%x, efiRTServPgCount: 0x%x, efiRTServVPgStart: 0x%lx\n",
			BA2->efiRuntimeServicesPageStart, BA2->efiRuntimeServicesPageCount, BA2->efiRuntimeServicesVirtualPageStart
		);
		DBG("efiSystemTable: 0x%x, res2: 0x%x, perfDataStart: 0x%x, perfDataSize: 0x%x\n",
			BA2->efiSystemTable, BA2->__reserved2, BA2->performanceDataStart, BA2->performanceDataSize
		);
		DBG("keyStDtStart: 0x%x, keyStDtSize: 0x%x, bootMemStart: 0x%lx, bootMemSize: 0x%lx\n",
			BA2->keyStoreDataStart, BA2->keyStoreDataSize, BA2->bootMemStart, BA2->bootMemSize
		);
		DBG("PhysicalMemorySize: 0x%lx (%d GB), FSBFrequency: 0x%lx (%d MHz)\n",
			BA2->PhysicalMemorySize, BA2->PhysicalMemorySize / (1024 * 1024 * 1024),
			BA2->FSBFrequency, BA2->FSBFrequency / 1000000
		);
	}
	
#endif
}

VOID
EFIAPI
BootArgsFix(VOID *bootArgs, EFI_PHYSICAL_ADDRESS gRellocBase)
{
	BootArgs1		*BA1 = bootArgs;
	BootArgs2		*BA2 = bootArgs;
	
	if (BA1->Version == kBootArgsVersion1) {
		// pre Lion
		BA1->MemoryMap = BA1->MemoryMap - (UINT32)gRellocBase;
		BA1->deviceTreeP = BA1->deviceTreeP - (UINT32)gRellocBase;
		BA1->kaddr = BA1->kaddr - (UINT32)gRellocBase;
	} else {
		// Lion and up
		BA2->MemoryMap = BA2->MemoryMap - (UINT32)gRellocBase;
		BA2->deviceTreeP = BA2->deviceTreeP - (UINT32)gRellocBase;
		BA2->kaddr = BA2->kaddr - (UINT32)gRellocBase;
	}
	
}

