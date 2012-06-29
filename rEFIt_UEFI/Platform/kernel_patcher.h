/*
 * Copyright (c) 2009-2010 Frank peng. All rights reserved.
 *
 */
#include "Platform.h"
#include "loader.h"
#include "nlist.h"
#include "device_tree.h"

#ifndef __LIBSAIO_KERNEL_PATCHER_H
#define __LIBSAIO_KERNEL_PATCHER_H

#define CPUFAMILY_INTEL_6_13		0xaa33392b
#define CPUFAMILY_INTEL_YONAH		0x73d67300
#define CPUFAMILY_INTEL_MEROM		0x426f69ef
#define CPUFAMILY_INTEL_PENRYN		0x78ea4fbc
#define CPUFAMILY_INTEL_NEHALEM		0x6b5a4cd2
#define CPUFAMILY_INTEL_WESTMERE	0x573b5eec

#define CPUIDFAMILY_DEFAULT 6

#define CPUID_MODEL_6_13	 	    13
#define CPUID_MODEL_YONAH			14
#define CPUID_MODEL_MEROM			15
#define CPUID_MODEL_PENRYN			35

#define MACH_GET_MAGIC(hdr)        (((struct mach_header_64*)(hdr))->magic)
#define MACH_GET_NCMDS(hdr)        (((struct mach_header_64*)(hdr))->ncmds)
#define SC_GET_CMD(hdr)            (((struct segment_command_64*)(hdr))->cmd)


#define kPrelinkTextSegment                "__PRELINK_TEXT"
#define kPrelinkTextSection                "__text"

#define kPrelinkLinkStateSegment           "__PRELINK_STATE"
#define kPrelinkKernelLinkStateSection     "__kernel"
#define kPrelinkKextsLinkStateSection      "__kexts"

#define kPrelinkInfoSegment                "__PRELINK_INFO"
#define kPrelinkInfoSection                "__info"

#define kPrelinkBundlePathKey              "_PrelinkBundlePath"
#define kPrelinkExecutableRelativePathKey  "_PrelinkExecutableRelativePath"
#define kPrelinkExecutableLoadKey          "_PrelinkExecutableLoadAddr"
#define kPrelinkExecutableSourceKey        "_PrelinkExecutableSourceAddr"
#define kPrelinkExecutableSizeKey          "_PrelinkExecutableSize"
#define kPrelinkInfoDictionaryKey          "_PrelinkInfoDictionary"
#define kPrelinkInterfaceUUIDKey           "_PrelinkInterfaceUUID"
#define kPrelinkKmodInfoKey                "_PrelinkKmodInfo"
#define kPrelinkLinkStateKey               "_PrelinkLinkState"
#define kPrelinkLinkStateSizeKey           "_PrelinkLinkStateSize"

#define kPropCFBundleIdentifier ("CFBundleIdentifier")
#define kPropCFBundleExecutable ("CFBundleExecutable")
#define kPropOSBundleRequired   ("OSBundleRequired")
#define kPropOSBundleLibraries  ("OSBundleLibraries")
#define kPropIOKitPersonalities ("IOKitPersonalities")
#define kPropIONameMatch        ("IONameMatch")

typedef struct _BooterKextFileInfo {
    UINT32  infoDictPhysAddr;
    UINT32  infoDictLength;
    UINT32  executablePhysAddr;
    UINT32  executableLength;
    UINT32  bundlePathPhysAddr;
    UINT32  bundlePathLength;
} _BooterKextFileInfo;

typedef struct _DeviceTreeBuffer {
    uint32_t paddr;
    uint32_t length;
} _DeviceTreeBuffer;



extern EFI_PHYSICAL_ADDRESS KernelRelocBase;
extern BootArgs1    *bootArgs1;
extern BootArgs2    *bootArgs2;
extern CHAR8        *dtRoot;
extern VOID         *KernelData;
extern BOOLEAN      isKernelcache;
extern BOOLEAN      is64BitKernel;

// notes:
// - 64bit segCmd64->vmaddr is 0xffffff80xxxxxxxx and we are taking
//   only lower 32bit part into PrelinkTextAddr
// - PrelinkTextAddr is segCmd64->vmaddr + KernelRelocBase
extern UINT32       PrelinkTextLoadCmdAddr;
extern UINT32       PrelinkTextAddr;
extern UINT32       PrelinkTextSize;

// notes:
// - 64bit sect->addr is 0xffffff80xxxxxxxx and we are taking
//   only lower 32bit part into PrelinkInfoAddr
// - PrelinkInfoAddr is sect->addr + KernelRelocBase
extern UINT32       PrelinkInfoLoadCmdAddr;
extern UINT32       PrelinkInfoAddr;
extern UINT32       PrelinkInfoSize;

extern UINT32 DisplayVendor[2];
//VOID findCPUfamily();


//UINT64 kernelsize;

VOID Patcher_SSE3_5(VOID* kernelData);
VOID Patcher_SSE3_6(VOID* kernelData);
VOID Patcher_SSE3_7(VOID* kernelData);

VOID KernelAndKextsPatcherStart(VOID);

//VOID register_kernel_symbol(CONST CHAR8* name);
//UINT64 symbol_handler(CHAR8* symbolName, UINT64 addr);
//INTN locate_symbols(VOID* kernelData);

//
// kext_patcher.c
//
VOID KextPatcherStart();


#endif /* !__LIBSAIO_KERNEL_PATCHER_H */
