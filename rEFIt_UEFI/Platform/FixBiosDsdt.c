/*
 * Copyright (c) 2011-2012 Frank Peng. All rights reserved.
 *
 */

#include "StateGenerator.h"
#include "Display.h"
#include <OsxPciBusNoEnumerationDxe/PciCommand.h>

#ifdef DBG
#undef DBG
#endif

#ifndef DEBUG_FIX
#define DEBUG_FIX 1
#endif

#if DEBUG_FIX==2
#define DBG(x...)  AsciiPrint(x)
#elif DEBUG_FIX==1
#define DBG(x...)  MsgLog(x)
#else
#define DBG(x...)
#endif



CHAR8*  device_name[10];  // 0=>Display  1=>network  2=>firewire 3=>LPCB 4=>HDAAudio 5=>RTC 6=>TMR 7=>SBUS 8=>PIC 9=>Airport
CHAR8*  UsbName[10];
CHAR8*  Netmodel;

BOOLEAN HDAFIX;
BOOLEAN GFXHDAFIX;
BOOLEAN DisplayName1;
BOOLEAN DisplayName2;
BOOLEAN NetworkName;
BOOLEAN ArptName;
BOOLEAN ArptBCM;
BOOLEAN LPCBFIX;
BOOLEAN IDEFIX;
BOOLEAN SATAFIX;
BOOLEAN ASUSFIX;
BOOLEAN USBIDFIX = TRUE;
BOOLEAN Display1PCIE;
BOOLEAN Display2PCIE;
BOOLEAN FirewireName;

// for Fix DSDT, we need record Scope(\_SB), Device(PCI), Device(LPCB) address and size
UINT32   SBADR;
UINT32   SBSIZE;
UINT32   PCIADR;
UINT32   PCISIZE;
UINT32   LPCBADR;
UINT32   LPCBADR1;
UINT32   LPCBSIZE;
UINT32   IDEADR;
UINT32   SATAADR;
UINT32   SATAAHCIADR;

UINT32  RTCADR;
UINT32  TMRADR;
UINT32  PICADR;
UINT32  HPETADR;
UINT32  DisplayADR[2];
UINT32  FirewireADR;
UINT32  NetworkADR;
UINT32  ArptADR;
UINT32  SBUSADR;
INT32   sizeoffset;

// for read computer data
UINT32 DisplayADR1[2];
UINT32 DisplayADR2[2];
UINT32 NetworkADR1;
UINT32 NetworkADR2;
UINT32 ArptADR1;
UINT32 ArptADR2;
UINT32 FirewireADR1;
UINT32 FirewireADR2;
UINT32 SBUSADR1;
UINT32 SBUSADR2;
UINT32 IDEADR1;
UINT32 IDEADR2;
UINT32 SATAADR1;
UINT32 SATAADR2;
UINT32 SATAAHCIADR1;
UINT32 SATAAHCIADR2;
UINT32 IDEVENDOR;
UINT32 SATAVENDOR;
UINT32 SATAAHCIVENDOR;
UINT32 DisplayVendor[2];
UINT16 DisplayID[2];
UINT32 DisplaySubID[2];


UINT32 HDAADR;
UINT32 USBADR[12];
UINT32 USBID[12];
UINT32 USB20[12];

UINT32 HDAcodecId=0;
UINT32 HDAlayoutId=0;
UINT32 GfxcodecId[2];
UINT32 GfxlayoutId[2];

pci_dt_t              Displaydevice[2];


INTN usb;

struct lpc_device_t 
{
	UINT32		id;
};

static CHAR8 data[] = {0x00};

static struct lpc_device_t lpc_chipset[] =
{
    {0x00000000},
    //
    {0x80862811},
    {0x80862815},
    {0x808627b9},
    {0x808627bd},
    {0x80862670},
    {0x80868119},
    {0x80862916},
    {0x80863a18},
    {0x80863b00},
    {0x80863b01},
    {0x80863b02},
    {0x80863b09},
    {0x10de0aac},
    {0x10de0aae},
    {0x10de0aaf},
    {0x10de0d80},
    {0x10de0d81},
    {0x10de0d82},
    {0x10de0d83},
    //SB
    {0x80861c42},
    {0x80861c44},
    {0x80861c4e},
    {0x80861c4c},
    {0x80861c50},
    {0x80861c4a},
    {0x80861c46},
    {0x80861c5c},
    {0x80861c52},
    {0x80861c54},
    {0x80861c56}, 
    {0x80861c43},
    {0x80861c4f},
    {0x80861c47},
    {0x80861c4b},
    {0x80861c49},
    {0x80861c41},
    {0x80861c4d},
};

struct net_chipsets_t {
	UINT32 id;
	char *name;
};

static struct net_chipsets_t NetChipsets[] = {
	{ 0x00000000, "Unknown" },
	// 8169
	{ 0x10EC8169, "Realtek 8169/8110 Gigabit Ethernet" },
	{ 0x10EC8168, "Realtek 8168/8101E Gigabit Ethernet" },
	{ 0x10EC8167, "Realtek 8169/8110 Gigabit Ethernet" },
    { 0x10EC8136, "Realtek 8168/8101E Gigabit Ethernet" },
    // 8139
    { 0x10EC8139, "Realtek RTL8139/810x Family Fast Ethernet" },
    { 0x11861300, "Realtek RTL8139/810x Family Fast Ethernet" },
    { 0x11131211, "Realtek RTL8139/810x Family Fast Ethernet" },
    // Broadcom 57XX 
  { 0x14e41600, "Broadcom 5751 Ethernet" },
    { 0x14e41659, "Broadcom 57XX Ethernet" },
    { 0x14e4166A, "Broadcom 57XX Ethernet" },
    { 0x14e41684, "Broadcom 57XX Ethernet" },
    { 0x14e416B4, "Broadcom 57XX Ethernet" },
    // Intel 8255x Ethernet
    { 0x80861051, "Intel 8255x Ethernet" },
    { 0x80861050, "Intel 8255x Ethernet" },
    { 0x80861029, "Intel 8255x Ethernet" },
    { 0x80861030, "Intel 8255x Ethernet" },    
    { 0x80861209, "Intel 8255x Ethernet" },
    { 0x80861227, "Intel 8255x Ethernet" },
    { 0x80861228, "Intel 8255x Ethernet" },
    { 0x80861229, "Intel 8255x Ethernet" },   
    { 0x80862449, "Intel 8255x Ethernet" },
    { 0x80862459, "Intel 8255x Ethernet" },
    { 0x8086245D, "Intel 8255x Ethernet" },
    { 0x80861091, "Intel 8255x Ethernet" }, 
    { 0x80861060, "Intel 8255x Ethernet" },
};

struct ide_chipsets_t {
    UINT32 id;
};

static struct ide_chipsets_t ide_chipset[] =
{
    // IDE
    {0x00000000},
    //
    {0x8086269e},
    {0x808627df},
    {0x80862850},

    //SATA
    {0x80862680},
    {0x808627c0},
    {0x808627c4},
    {0x80862828},
};
//2820? 2825?
struct ahci_chipsets_t {
    UINT32 id;
};

static struct ahci_chipsets_t ahci_chipset[] =
{
    // SATA AHCI
    {0x00000000},
    //
    {0x80863a22},
    {0x80862681},
    {0x808627c5},
    {0x80862829},
    {0x80863b29},
    {0x80863b22},
    {0x80863b2f},
    {0x80861c02},
    {0x80861c03},
    {0x10de0ab9},
    {0x10de0b88},
};

CHAR8 dtgp[] = // Method (DTGP, 5, NotSerialized) ......
{
   0x14, 0x3F, 0x44, 0x54, 0x47, 0x50, 0x05, 0xA0, 
   0x30, 0x93, 0x68, 0x11, 0x13, 0x0A, 0x10, 0xC6,
   0xB7, 0xB5, 0xA0, 0x18, 0x13, 0x1C, 0x44, 0xB0,
   0xC9, 0xFE, 0x69, 0x5E, 0xAF, 0x94, 0x9B, 0xA0,
   0x18, 0x93, 0x69, 0x01, 0xA0, 0x0C, 0x93, 0x6A,
   0x00, 0x70, 0x11, 0x03, 0x01, 0x03, 0x6C, 0xA4,
   0x01, 0xA0, 0x06, 0x93, 0x6A, 0x01, 0xA4, 0x01,
   0x70, 0x11, 0x03, 0x01, 0x00, 0x6C, 0xA4, 0x00
};

CHAR8 sbus[] = 
{              //  Device (SBUS) ......
   0x5B, 0x82, 0x47, 0x09, 0x53, 0x42, 0x55, 0x53, 
   0x08, 0x5F, 0x41, 0x44, 0x52, 
   0x0C, 0x03, 0x00, 0x1F, 0x00, 0x5B,
   0x80, 0x50, 0x42, 0x41, 0x53, 0x02, 0x0A, 0x20,
   0x0A, 0x02, 0x5B, 0x81, 0x0B, 0x50, 0x42, 0x41,
   0x53, 0x01, 0x42, 0x41, 0x53, 0x30, 0x10, 0x14,
   0x11, 0x53, 0x4D, 0x42, 0x42, 0x00, 0x7B, 0x42,
   0x41, 0x53, 0x30, 0x0B, 0xFE, 0xFF, 0x60, 0xA4,
   0x60, 0x5B, 0x82, 0x4B, 0x05, 0x42, 0x55, 0x53,
   0x30, 0x08, 0x5F, 0x43, 0x49, 0x44, 0x0D, 0x73,
   0x6D, 0x62, 0x75, 0x73, 0x00, 0x08, 0x5F, 0x41,
   0x44, 0x52, 0x00, 0x5B, 0x82, 0x41, 0x04, 0x44,
   0x56, 0x4C, 0x30, 0x08, 0x5F, 0x41, 0x44, 0x52,
   0x0A, 0x57, 0x08, 0x5F, 0x43, 0x49, 0x44, 0x0D,
   0x64, 0x69, 0x61, 0x67, 0x73, 0x76, 0x61, 0x75,
   0x6C, 0x74, 0x00, 0x14, 0x22, 0x5F, 0x44, 0x53,
   0x4D, 0x04, 0x70, 0x12, 0x0D, 0x02, 0x0D, 0x61,
   0x64, 0x64, 0x72, 0x65, 0x73, 0x73, 0x00, 0x0A,
   0x57, 0x60, 0x44, 0x54, 0x47, 0x50, 0x68, 0x69,
   0x6A, 0x6B, 0x71, 0x60, 0xA4, 0x60
};

CHAR8 bus0[] =
{
   0x5B, 0x82, 0x4B, 0x05, 0x42, 0x55,
   0x53, 0x30, 0x08, 0x5F, 0x43, 0x49, 0x44, 0x0D,
   0x73, 0x6D, 0x62, 0x75, 0x73, 0x00, 0x08, 0x5F,
   0x41, 0x44, 0x52, 0x00, 0x5B, 0x82, 0x41, 0x04,
   0x44, 0x56, 0x4C, 0x30, 0x08, 0x5F, 0x41, 0x44,
   0x52, 0x0A, 0x57, 0x08, 0x5F, 0x43, 0x49, 0x44,
   0x0D, 0x64, 0x69, 0x61, 0x67, 0x73, 0x76, 0x61,
   0x75, 0x6C, 0x74, 0x00, 0x14, 0x22, 0x5F, 0x44,
   0x53, 0x4D, 0x04, 0x70, 0x12, 0x0D, 0x02, 0x0D,
   0x61, 0x64, 0x64, 0x72, 0x65, 0x73, 0x73, 0x00,
   0x0A, 0x57, 0x60, 0x44, 0x54, 0x47, 0x50, 0x68,
   0x69, 0x6A, 0x6B, 0x71, 0x60, 0xA4, 0x60
};
/*
CHAR8 hpet[] =
{
    0x5B, 0x82, 0x3C, 0x48, 0x50, 0x45, 0x54, 0x08, 0x5F, 0x48, 0x49, 0x44,
    0x0C, 0x41, 0xD0, 0x01, 0x03, 0x08, 0x5F, 0x43, 0x52, 0x53, 0x11, 0x1D,
    0x0A, 0x1A, 0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0xD0, 0xFE, 0x00, 0x04,
    0x00, 0x00, 0x22, 0x01, 0x00, 0x22, 0x00, 0x01, 0x22, 0x00, 0x08, 0x22,
    0x00, 0x10, 0x79, 0x00, 0x14, 0x09, 0x5F, 0x53, 0x54, 0x41, 0x00, 0xA4,
    0x0A, 0x0F
};
*/
CHAR8 hpet0[] =
{
  0x5B, 0x82, 0x4A, 0x04, 0x48, 0x50, 0x45, 0x54,                 //Device (HPET)
  0x08, 0x5F, 0x48, 0x49, 0x44, 0x0C, 0x41, 0xD0, 0x01, 0x03,     //Name (_HID, EisaId ("PNP0103"))
  0x08, 0x5F, 0x43, 0x49, 0x44, 0x0C, 0x41, 0xD0, 0x0C, 0x01,     //Name (_CID, EisaId ("PNP0C01"))
  0x08, 0x41, 0x54, 0x54, 0x30, 0x11, 0x14, 0x0A, 0x11,           //Name (ATT0, ResourceTemplate ()
  0x86, 0x09, 0x00, 0x01,                                         //  Memory32Fixed (ReadWrite,
  0x00, 0x00, 0xD0, 0xFE, 0x00, 0x04, 0x00, 0x00,                 //    0xFED00000, 0x00000400, )
  0x22, 0x01, 0x09, 0x79, 0x00,                                   //  IRQNoFlags () {0,8,11}
//  0x14, 0x09, 0x5F, 0x53, 0x54, 0x41, 0x00,                       //Method (_STA, 0, NotSerialized)
//  0xA4, 0x0A, 0x0F,                                               //  Return (0x0F)
  0x08, 0x5F, 0x53, 0x54, 0x41, 0x0A, 0x0F,                       //  Name (_STA, 0x0F)
  0x14, 0x0B, 0x5F, 0x43, 0x52, 0x53, 0x00,                       //Method (_CRS, 0, NotSerialized)
  0xA4, 0x41, 0x54, 0x54, 0x30                                    //  Return (ATT0)
};

CHAR8 hpet1[] =  // Name (_CID, EisaId ("PNP0C01"))
{
    0x08, 0x5F, 0x43, 0x49, 0x44, 0x0C, 0x41, 0xD0, 0x0C, 0x01
};

CHAR8 hpet2[] =  
{
//   0x22, 0x01, 0x00, 0x22, 0x00, 0x01, 0x22, 0x00, 0x08, 0x22, 0x00, 0x10
  0x22, 0x01, 0x09 //  IRQNoFlags () {0,8,11}
};

CHAR8 wakret[] =
{
    0xA4, 0x12, 0x04, 0x02, 0x00, 0x00
};

CHAR8 pwrb[] =
{
  0x86, 0x5C, 0x2E, 0x5F, 0x53, 0x42, 0x5F, 0x50, 0x57, 0x52, 0x42, 0x0A, 0x02
};

//08 5F 50 52 57 12 06 02 0A 1C 0A 03  //Name (_PRW, Package (0x02){0x1C, 0x03}

CHAR8 dtgp_1[] =    // DTGP (Arg0, Arg1, Arg2, Arg3, RefOf (Local0))  
{                   // Return (Local0)
   0x44, 0x54, 0x47, 0x50, 0x68, 0x69, 0x6A, 0x6B,
   0x71, 0x60, 0xA4, 0x60
};

CHAR8 pwrbcid[] =
{
    0x08, 0x5F, 0x43, 0x49, 0x44, 0x0C, 0x41, 0xD0, 0x0C, 0x0E, 0x14,
    0x0E, 0x5F, 0x50, 0x52, 0x57, 0x00, 0xA4, 0x12, 0x06, 0x02, 0x0A,
    0x0B, 0x0A, 0x04
};

CHAR8 shutdown[] =
{
    0xA0, 0x05, 0x93, 0x68, 0x0A, 0x05, 0xA1, 0x01
};

CHAR8 pnlf[] =
{
  0x5B, 0x82, 0x2D, 0x50, 0x4E, 0x4C, 0x46,                         //Device (PNLF)
  0x08, 0x5F, 0x48, 0x49, 0x44, 0x0C, 0x06, 0x10, 0x00, 0x02,       //  Name (_HID, EisaId ("APP0002"))
  0x08, 0x5F, 0x43, 0x49, 0x44,                                     //  Name (_CID, 
  0x0D, 0x62, 0x61, 0x63, 0x6B, 0x6C, 0x69, 0x67, 0x68, 0x74, 0x00, //              "backlight")
  0x08, 0x5F, 0x55, 0x49, 0x44, 0x0A, 0x0A,                         //  Name (_UID, 0x0A)
  0x08, 0x5F, 0x53, 0x54, 0x41, 0x0A, 0x0B                          //  Name (_STA, 0x0B)
};

// for HDA from device_inject.c and mark device_inject function
extern UINT32 HDA_IC_sendVerb(EFI_PCI_IO_PROTOCOL *PciIo, UINT32 codecAdr, UINT32 nodeId, UINT32 verb);
extern UINT32 HDA_getCodecVendorAndDeviceIds(EFI_PCI_IO_PROTOCOL *PciIo);
extern UINT32 getLayoutIdFromVendorAndDeviceId(UINT32 vendorDeviceId);

BOOLEAN get_lpc_model(UINT32 id) {
	int	i;

	for (i=1; i< (sizeof(lpc_chipset) / sizeof(lpc_chipset[0])); i++) {
		if (lpc_chipset[i].id == id) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOLEAN get_ide_model(UINT32 id) {
	int	i;

	for (i=1; i< (sizeof(ide_chipset) / sizeof(ide_chipset[0])); i++) {
		if (ide_chipset[i].id == id) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOLEAN get_ahci_model(UINT32 id) {
	int	i;

	for (i=1; i< (sizeof(ahci_chipset) / sizeof(ahci_chipset[0])); i++) {
		if (ahci_chipset[i].id == id) {
			return FALSE;
		}
	}
	return TRUE;
}

CHAR8* get_net_model(UINT32 id) {
	int	i;

	for (i=1; i< (sizeof(NetChipsets) / sizeof(NetChipsets[0])); i++) {
		if (NetChipsets[i].id == id) {
			return NetChipsets[i].name;
		}
	}
	return NetChipsets[0].name;
}

VOID GetPciADR(IN EFI_DEVICE_PATH_PROTOCOL *DevicePath, OUT UINT32 *Addr1, OUT UINT32 *Addr2)
{
  PCI_DEVICE_PATH *PciNode;
  UINTN           PciNodeCount;
  // default to 0
  if (Addr1 != NULL) *Addr1 = 0;
  if (Addr2 != NULL) *Addr2 = 0xFFFE; //some code we will consider as "non-exists" b/c 0 is meaningful value
  
  EFI_DEVICE_PATH_PROTOCOL *TmpDevicePath = DuplicateDevicePath(DevicePath);
  if (!TmpDevicePath) {
    return;
  }
  
  // sanity check - expecting ACPI path for PciRoot
  if (TmpDevicePath->Type != ACPI_DEVICE_PATH && TmpDevicePath->SubType != ACPI_DP) {
    return;
  }
  
  PciNodeCount = 0;
  while (TmpDevicePath && !IsDevicePathEndType(TmpDevicePath)) {
    if (TmpDevicePath->Type == HARDWARE_DEVICE_PATH && TmpDevicePath->SubType == HW_PCI_DP) {
      PciNodeCount++;
      PciNode = (PCI_DEVICE_PATH *)TmpDevicePath;
      if (PciNodeCount == 1 && Addr1 != NULL) {
        *Addr1 = (PciNode->Device << 16) | PciNode->Function;
      } else if (PciNodeCount == 2 && Addr2 != NULL) {
        *Addr2 = (PciNode->Device << 16) | PciNode->Function;
      } else {
        break;
      }
    }
    TmpDevicePath = NextDevicePathNode(TmpDevicePath);
  }
  return;
}

VOID CheckHardware()
{
  EFI_STATUS			Status;
	EFI_HANDLE			*HandleBuffer = NULL;
  EFI_HANDLE      Handle;
//	EFI_GUID        **ProtocolGuidArray;
	EFI_PCI_IO_PROTOCOL *PciIo;
	PCI_TYPE00          Pci;
	UINTN         HandleCount = 0;
//	UINTN         ArrayCount;
	UINTN         HandleIndex;
//	UINTN         ProtocolIndex;
  
  //	UINT16		  did, vid;
	UINTN         Segment;
	UINTN         Bus;
	UINTN         Device;
	UINTN         Function;
  
	pci_dt_t      PCIdevice;
	EFI_DEVICE_PATH_PROTOCOL *DevicePath = NULL;
  
	usb=0;
	UINTN display=0;
	UINTN gfxid=0;
	
	/* Scan PCI BUS */
  /*	Status = gBS->LocateHandleBuffer(AllHandles,NULL,NULL,&HandleCount,&HandleBuffer);
   if (!EFI_ERROR(Status))
   {	
   for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++)
   {
   Status = gBS->ProtocolsPerHandle(HandleBuffer[HandleIndex],&ProtocolGuidArray,&ArrayCount);
   if (!EFI_ERROR(Status))
   {			
   for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++)
   {
   if (CompareGuid(&gEfiPciIoProtocolGuid, ProtocolGuidArray[ProtocolIndex]))
   {
   Status = gBS->OpenProtocol(HandleBuffer[HandleIndex],&gEfiPciIoProtocolGuid,(VOID **)&PciIo,gImageHandle,NULL,EFI_OPEN_PROTOCOL_GET_PROTOCOL);
   if (!EFI_ERROR(Status))
   {
   */              
  // Scan PCI handles 
  Status = gBS->LocateHandleBuffer (
                                    ByProtocol,
                                    &gEfiPciIoProtocolGuid,
                                    NULL,
                                    &HandleCount,
                                    &HandleBuffer
                                    );
  if (!EFI_ERROR (Status)) {
//    DBG("PciIo handles count=%d\n", HandleCount);
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      Handle = HandleBuffer[HandleIndex];
      Status = gBS->HandleProtocol (
                                    Handle,
                                    &gEfiPciIoProtocolGuid,
                                    (VOID **)&PciIo
                                    );
      if (!EFI_ERROR (Status)) {
        
        /* Read PCI BUS */
        Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
        Status = PciIo->Pci.Read (
                                  PciIo,
                                  EfiPciIoWidthUint32,
                                  0,
                                  sizeof (Pci) / sizeof (UINT32),
                                  &Pci
                                  );
        
        UINT32 deviceid = Pci.Hdr.DeviceId | (Pci.Hdr.VendorId << 16);
        
        // add for auto patch dsdt get DSDT Device _ADR
        PCIdevice.DeviceHandle = Handle;
        DevicePath = DevicePathFromHandle (Handle);
        if (DevicePath)
        {
  //        DBG("Device patch = %s \n", DevicePathToStr(DevicePath));
          
          //Display ADR
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_DISPLAY) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_DISPLAY_VGA)) {
            GetPciADR(DevicePath, &DisplayADR1[display], &DisplayADR2[display]);
   //         DBG("Display PciAdr=0x%x\n", ((Device << 16) | Function));
   //         UINT32 dadr1 = DisplayADR1[display];
   //         UINT32 dadr2 = DisplayADR2[display];
  //          DBG("DisplayADR1[%d] = 0x%x, DisplayADR2[%d] = 0x%x\n", display, dadr1, display, dadr2);
  //          dadr2 = dadr1; //to avoid warning "unused variable" :(
            DisplayVendor[display] = Pci.Hdr.VendorId;
            DisplayID[display] = Pci.Hdr.DeviceId;
            DisplaySubID[display] = (Pci.Device.SubsystemID << 16) | (Pci.Device.SubsystemVendorID << 0);
            // for get display data
            Displaydevice[display].DeviceHandle = HandleBuffer[HandleIndex];
            Displaydevice[display].dev.addr = PCIADDR(Bus, Device, Function);
            Displaydevice[display].vendor_id = Pci.Hdr.VendorId;
            Displaydevice[display].device_id = Pci.Hdr.DeviceId;
            Displaydevice[display].revision = Pci.Hdr.RevisionID;
            Displaydevice[display].subclass = Pci.Hdr.ClassCode[0];
            Displaydevice[display].class_id = *((UINT16*)(Pci.Hdr.ClassCode+1));
            Displaydevice[display].subsys_id.subsys.vendor_id = Pci.Device.SubsystemVendorID;
            Displaydevice[display].subsys_id.subsys.device_id = Pci.Device.SubsystemID;
            //
            // Detect if PCI Express Device
            //
            //
            // Go through the Capability list
            //
            PCI_IO_DEVICE *PciIoDevice;
            PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (PciIo);
            if (PciIoDevice->IsPciExp)
            {
              if (display==0)
                Display1PCIE = TRUE;
              else
                Display2PCIE = TRUE;
            }            
            display++;
          }
          
          //Network ADR
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_NETWORK) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_NETWORK_ETHERNET))
          {
            GetPciADR(DevicePath, &NetworkADR1, &NetworkADR2);
 //           DBG("NetworkADR1 = 0x%x, NetworkADR2 = 0x%x\n", NetworkADR1, NetworkADR2);
            Netmodel = get_net_model(deviceid);            
          }
          
          //Network WiFi ADR
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_NETWORK) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_NETWORK_OTHER))
          {
            GetPciADR(DevicePath, &ArptADR1, &ArptADR2);
   //         DBG("ArptADR1 = 0x%x, ArptADR2 = 0x%x\n", ArptADR1, ArptADR2);
   //         Netmodel = get_arpt_model(deviceid);  
            ArptBCM = (deviceid == 0x14e44315);
          }
          
          //Firewire ADR
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_SERIAL) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_SERIAL_FIREWIRE))
          {
            GetPciADR(DevicePath, &FirewireADR1, &FirewireADR2);
 //           DBG("FirewireADR1 = 0x%x, FirewireADR2 = 0x%x\n", FirewireADR1, FirewireADR2);
          }
          
          //SBUS ADR
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_SERIAL) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_SERIAL_SMB))
          {
            GetPciADR(DevicePath, &SBUSADR1, &SBUSADR2);
//            DBG("SBUSADR1 = 0x%x, SBUSADR2 = 0x%x\n", SBUSADR1, SBUSADR2);
          }
          
          //USB
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_SERIAL) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_SERIAL_USB)) 
          {
            UINT16 DID = Pci.Hdr.DeviceId;
            GetPciADR(DevicePath, &USBADR[usb], NULL);
  //          DBG("USBADR[%d] = 0x%x\n", usb, USBADR[usb]);
            if (USBIDFIX)
            {
              if (USBADR[usb] == 0x001D0000 && DID != 0x3a34) DID = 0x3a34;
              if (USBADR[usb] == 0x001D0001 && DID != 0x3a35) DID = 0x3a35;
              if (USBADR[usb] == 0x001D0002 && DID != 0x3a36) DID = 0x3a36;
              if (USBADR[usb] == 0x001D0003 && DID != 0x3a37) DID = 0x3a37;
              if (USBADR[usb] == 0x001A0000 && DID != 0x3a37) DID = 0x3a37;
              if (USBADR[usb] == 0x001A0001 && DID != 0x3a38) DID = 0x3a38;
              if (USBADR[usb] == 0x001A0002 && DID != 0x3a39) DID = 0x3a39;
              if (USBADR[usb] == 0x001D0007 && DID != 0x3a3a) DID = 0x3a3a;
              if (USBADR[usb] == 0x001A0007 && DID != 0x3a3c) DID = 0x3a3c;
            }       
            USBID[usb] = DID;
            USB20[usb] = (Pci.Hdr.ClassCode[0] == 0x20)?1:0;
            usb++;
          }
          
          // HDA Auido
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_MEDIA) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_MEDIA_HDA))
          {
            GetPciADR(DevicePath, &HDAADR, NULL);
  //          DBG("HDAADR = 0x%x\n", HDAADR);
            UINT32 codecId = 0, layoutId = 0;
            codecId = HDA_getCodecVendorAndDeviceIds(PciIo);
            if (codecId >0)
            {
              layoutId = getLayoutIdFromVendorAndDeviceId(codecId);
              if (layoutId == 0) {
                layoutId = 12;
              }
            }
            if (layoutId > 0)
            {
              HDAFIX = TRUE;
              HDAcodecId = codecId;
              HDAlayoutId = layoutId;
            } 
            else
            {
              GFXHDAFIX = TRUE;
              GfxcodecId[gfxid] = codecId;
              GfxlayoutId[gfxid] = layoutId;
              gfxid++;
            }                                   
          }
          
          // LPC
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_BRIDGE) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_ISA))
          {
            LPCBFIX = get_lpc_model(deviceid);
          }
          
          // IDE device
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_MASS_STORAGE) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_MASS_STORAGE_IDE))
          {
            GetPciADR(DevicePath, &IDEADR1, &IDEADR2);
  //          DBG("IDEADR1 = 0x%x, IDEADR2 = 0x%x\n", IDEADR1, IDEADR2);
            IDEFIX = get_ide_model(deviceid);
            IDEVENDOR = Pci.Hdr.VendorId;
          }
          
          // SATA 
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_MASS_STORAGE) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_MASS_STORAGE_SATADPA) &&
              (Pci.Hdr.ClassCode[0] == 0x00))
          {
            GetPciADR(DevicePath, &SATAADR1, &SATAADR2);
 //           DBG("SATAADR1 = 0x%x, SATAADR2 = 0x%x\n", SATAADR1, SATAADR2);
            SATAFIX = get_ide_model(deviceid);
            SATAVENDOR = Pci.Hdr.VendorId;
          }
          
          // SATA AHCI
          if ((Pci.Hdr.ClassCode[2] == PCI_CLASS_MASS_STORAGE) &&
              (Pci.Hdr.ClassCode[1] == PCI_CLASS_MASS_STORAGE_SATADPA) &&
              (Pci.Hdr.ClassCode[0] == 0x01))
          {
            GetPciADR(DevicePath, &SATAAHCIADR1, &SATAAHCIADR2);
  //          DBG("SATAAHCIADR1 = 0x%x, SATAAHCIADR2 = 0x%x\n", SATAAHCIADR1, SATAAHCIADR2);
            //AHCIFIX = get_ahci_model(deviceid);
            SATAAHCIVENDOR = Pci.Hdr.VendorId;
          }
        }
        // detected finish						
      }
    }
  }
  //			}
  //		}
  //	}
}

VOID findCPU(UINT8* dsdt, UINT32 length)
{
	UINT32 i;
	
	acpi_cpu_count = 0;
	
	for (i=0; i<length-20; i++) 
	{
		if (dsdt[i] == 0x5B && dsdt[i+1] == 0x83) // ProcessorOP
		{
			UINT32 offset = i + 3 + (dsdt[i+2] >> 6);
			
			BOOLEAN add_name = TRUE;

			UINT8 j;
			
			for (j=0; j<4; j++) 
			{
				char c = dsdt[offset+j];
				if( c == '\\') {
				    offset = i + 8 + (dsdt[i+7] >> 6);
				    c = dsdt[offset+j];
				}
				
				if (!aml_isvalidchar(c)) 
				{
					add_name = FALSE;
					DBG("Invalid character found in ProcessorOP 0x%x!\n", c);
					break;
				}
			}
			
			if (add_name) 
			{
				acpi_cpu_name[acpi_cpu_count] = AllocateZeroPool(5);
				CopyMem(acpi_cpu_name[acpi_cpu_count], dsdt+offset, 4);
				i = offset + 5;
                
                //if (acpi_cpu_count == 0)
                //    acpi_cpu_p_blk = dsdt[i] | (dsdt[i+1] << 8);
				
				if (acpi_cpu_count == 0) {
				    DBG("Found ACPI CPU: %a ", acpi_cpu_name[acpi_cpu_count]);
				} else { 
				    DBG("And %a ", acpi_cpu_name[acpi_cpu_count]);
				}    
				if (++acpi_cpu_count == 32)
				    break;
			}
		}
	}
	DBG("\n");
	return;
}

UINT32 get_size(UINT8* Buffer, UINT32 adr)
{
	UINT32 temp;
	
	temp = Buffer[adr] & 0xF0; //keep bits 0x30 to check if this is valid size field
	
	if(temp <= 0x30)			    // 0
	{
		temp = Buffer[adr];		
	}
	else if(temp == 0x40)		// 4
	{
		temp =  (Buffer[adr]   - 0x40)  << 0|
		         Buffer[adr+1]          << 4;
	}
	else if(temp == 0x80)		// 8
	{
		temp = (Buffer[adr]   - 0x80)  <<  0|
		        Buffer[adr+1]          <<  4|
		        Buffer[adr+2]          << 12;	
	}	
	else if(temp == 0xC0)		// C
	{						   // C
		temp = (Buffer[adr]   - 0xC0) <<  0|
		        Buffer[adr+1]         <<  4|
		        Buffer[adr+2]         << 12|
		        Buffer[adr+3]         << 20;	
	} 
  else {
    DBG("wrong pointer to size field at %x\n", adr);
    return 0;  //this means wrong pointer to size field
  }

	return temp;
}

UINT32 write_offset(UINT32 adr, UINT8* buffer, UINT32 len, UINT32 offset)
{
  UINT32 i;
  UINT32 size = offset + 1;
  if (size > 0x3F) {
    for (i=len; i>adr; i--) {
      buffer[i+1] = buffer[i];
    }
    len += 1;
    size += 1;
    sizeoffset += 1;    
  }
  aml_write_size(size, (CHAR8 *)buffer, adr);
  return len;
}

//return final buffer len +/- 1 from original
//this procedure assumes hidden parameter - global valiable sizeoffset
UINT32 write_size(UINT32 adr, UINT8* buffer, UINT32 len, UINT32 oldsize)
{
    UINT32 i;
    UINT32 size;
    size = (INT32)oldsize + sizeoffset;
    // data move to back
    if ( (oldsize <= 0x3f && size > 0x3f) || (oldsize<=0x0fff && size > 0x0fff) ||
         (oldsize <= 0x0fffff && size > 0x0fffff) ) 
    {
        //DBG("size different, data will move to back 1 byte\n"); 
      for (i=len; i>adr; i--) {
        buffer[i+1] = buffer[i];
      }
      len += 1;
      size += 1;
      sizeoffset += 1;
    }  // data move to front
    else if ((size <= 0x3f && oldsize > 0x3f) || (size<=0x0fff && oldsize > 0x0fff) ||
             (size <= 0x0fffff && oldsize > 0x0fffff)) 
    {
        //DBG("size different, data will move to front 1 byte\n"); 
        for (i=adr; i<len-1; i++) {
            buffer[i] = buffer[i+1];
        }
        len -= 1;
        size -= 1;
        sizeoffset -= 1;
    }
    
//    DBG("write size =0x%08x, adr = 0x%08x, offset = 0x%08x\n", size, adr, sizeoffset);
    //offset = size;
  aml_write_size(size, (CHAR8 *)buffer, adr); //reuse existing codes  
	return len;
}

//                start => move data start address
//                offset => data move how many byte 
//                len => initial length of the buffer
// return final length of the buffer
// we suppose that buffer allocation is more then len+offset
UINT32 move_data(UINT32 start, UINT8* buffer, UINT32 len, INT32 offset)
{
  UINT32 i;
  
  if (offset<0) {
    for (i=start; i<len+offset; i++) {
      buffer[i] = buffer[i-offset];
    }
  }
  else  { // data move to back        
    for (i=len-1; i>=start; i--) {
      buffer[i+offset] = buffer[i];
    }
  }        
  
  return len + offset;
}

//if (!FindMethod(dsdt, len, "DTGP")) 
// return address of size field. Assume size not more then 0x0FFF = 4095 bytes
UINTN FindMethod(UINT8 *dsdt, UINT32 len, CONST CHAR8* Name)
{
  UINTN i;
  for (i=20; i<len; i++) {
    if (((dsdt[i] == 0x14) || (dsdt[i+1] == 0x14)) &&
        (dsdt[i+3] == Name[0]) && (dsdt[i+4] == Name[1]) &&
        (dsdt[i+5] == Name[2]) && (dsdt[i+6] == Name[3])){
      return (dsdt[i+1] == 0x14)?(i+2):(i+1);
    }
  }
  return 0;
}

UINTN findSB(UINT8 *dsdt, UINT32 len, UINT32 maxAdr) //return address of size field of Scope=0x10
{
  INTN    i, j;
  UINTN   SBadr = 0;
  for (i=maxAdr; i>20; i--) { //there is an ACPI header so no sense to search lower
    if (dsdt[i] == '_' && dsdt[i+1] == 'S' && dsdt[i+2] == 'B' && dsdt[i+3] == '_') {
      for (j=0; j<10; j++) {
        if ((dsdt[i-j] == 0x10) && (dsdt[i-j-1] != 0x0A)) {
          SBADR = i-j+1;
          SBSIZE = get_size(dsdt, SBADR);
          //DBG("found Scope(\\_SB) address = 0x%08x size = 0x%08x\n", SBADR, SBSIZE);
          if ((SBSIZE != 0) && (SBSIZE < len)) {  //if zero or too large then search more
            SBadr = SBADR;
            break;
          }          
        }
      }
      if (SBadr) { //if found
        break;
      }
    }
  } 
  return SBadr;
}

UINTN findOuterDevice (UINT8 *dsdt, UINT32 maxAdr) //return address of size field 
{
  INTN    i;
  UINTN   Size = 0;
  for (i=maxAdr; i>20; i--) { 
    if ((dsdt[i] == 0x5B) && (dsdt[i+1] == 0x82) && (dsdt[i-1] != 0x0A)) { //device candidate
      Size = get_size(dsdt, i+2);
      if (Size) {
        return i+2;
      }
    }
  }
  return 0;
}

UINTN CorrectOuters (UINT8 *dsdt, UINT32 len, UINT32 adr) //return final length of dsdt
{
  INTN    i, j;
  UINTN   size = 0;
  
  i = adr;
  while (i>20) {  //find devices that previous to adr
    j = findOuterDevice(dsdt, i);
//    DBG("  Found dev at %d\n", i);
    if (!j) {
      break;
    }
    size = get_size(dsdt, j);
    if ((j+size) > adr+4) {  //Yes - it is outer
//      DBG("found outer device begin=%x end=%x\n", j, j+size);
      len = write_size(j, dsdt, len, size);  //size corrected to sizeoffset at address j
    }    
    i = j - 3;  //step over Device()
  }
  i = adr;
  while (i>20) { //find scopes
    j = findSB(dsdt, len, i);
//    DBG("  Found _SB at %d\n", i);
    if (!j) {
      break;
    }
    size = get_size(dsdt, j);
    if ((j+size) > adr+4) {  //Yes - it is outer
//      DBG("found outer scope begin=%x end=%x\n", j, j+size);
      len = write_size(j, dsdt, len, size);
    }    
    i = j - 3;    //if found then search again from found 
  }  
  return len;
}

//ReplaceName(dsdt, "AZAL", "HDEF");
VOID ReplaceName(UINT8 *dsdt, UINT32 len, CONST CHAR8 *OldName, CONST CHAR8 *NewName)
{
  UINTN i;
  for (i=30; i<len; i++) {
    if ((dsdt[i+0] == OldName[0]) && (dsdt[i+1] == OldName[1]) &&
        (dsdt[i+2] == OldName[2]) && (dsdt[i+3] == OldName[3])) {
      dsdt[i+0] = NewName[0];
      dsdt[i+1] = NewName[1];
      dsdt[i+2] = NewName[2];
      dsdt[i+3] = NewName[3];
    }       
  }
}

// if (CmpAdr(dsdt, j, NetworkADR1))
BOOLEAN CmpAdr (UINT8 *dsdt, UINT32 j, UINT32 PciAdr)
{ 
  // Name (_ADR, 0x001f0001)
  return (BOOLEAN)
         ((dsdt[j + 4] == 0x08) &&
          (dsdt[j + 5] == 0x5F) &&
          (dsdt[j + 6] == 0x41) &&
          (dsdt[j + 7] == 0x44) &&
          (dsdt[j + 8] == 0x52) &&
          (//--------------------
           ((dsdt[j +  9] == 0x0C) &&
            (dsdt[j + 10] == ((PciAdr & 0x000000ff) >> 0)) &&
            (dsdt[j + 11] == ((PciAdr & 0x0000ff00) >> 8)) &&
            (dsdt[j + 12] == ((PciAdr & 0x00ff0000) >> 16)) &&
            (dsdt[j + 13] == ((PciAdr & 0xff000000) >> 24))
           ) ||
           //--------------------
           ((dsdt[j +  9] == 0x0B) &&
            (dsdt[j + 10] == ((PciAdr & 0x000000ff) >> 0)) &&
            (dsdt[j + 11] == ((PciAdr & 0x0000ff00) >> 8)) &&
            (PciAdr < 0x10000)
           ) ||
           //-----------------------
           ((dsdt[j +  9] == 0x0A) &&
            (dsdt[j + 10] == (PciAdr & 0x000000ff)) &&
            (PciAdr < 0x100)
           ) ||
           //-----------------
           ((dsdt[j +  9] == 0x00) && (PciAdr == 0)) ||    
           //------------------
           ((dsdt[j +  9] == 0x01) && (PciAdr == 1)) 
          )
        );
}

BOOLEAN CmpPNP (UINT8 *dsdt, UINT32 j, UINT16 PNP)
{
  // Name (_HID, EisaId ("PNP0C0F"))
  return (BOOLEAN)((dsdt[j + 0] == 0x08) &&
          (dsdt[j + 1] == 0x5F) &&
          (dsdt[j + 2] == 0x48) &&
          (dsdt[j + 3] == 0x49) &&
          (dsdt[j + 4] == 0x44) &&
          (dsdt[j + 5] == 0x0C) &&
          (dsdt[j + 6] == 0x41) &&
          (dsdt[j + 7] == 0xD0) &&
          (dsdt[j + 8] == ((PNP & 0xff00) >> 8)) &&
          (dsdt[j + 9] == ((PNP & 0x00ff) >> 0)));
}

UINT32 devFind(UINT8 *dsdt, UINT32 j)
{
  UINT32 k;
  for (k=j; k>20; k--)
  {
    if (dsdt[k] == 0x82 && dsdt[k-1] == 0x5B)
    {
      return (k+1); //pointer to size
    }
  }
  DBG("Device definition not found\n");
  return 0; //impossible value for fool proof  
}

VOID FixAddr(UINT32 MinAdr, UINT32 offset)
{
  if (DisplayADR[0] > MinAdr) DisplayADR[0] += offset;
	if (DisplayADR[1] > MinAdr) DisplayADR[1] += offset;
	if (NetworkADR > MinAdr) NetworkADR += offset;
	if (FirewireADR > MinAdr) FirewireADR += offset;
	if (SBUSADR > MinAdr) SBUSADR += offset;
	if (IDEADR > MinAdr) IDEADR += offset;
	if (SATAADR > MinAdr) SATAADR += offset;
	if (SATAAHCIADR > MinAdr) SATAAHCIADR += offset;
  if (ArptADR > MinAdr) ArptADR += offset;
}

//len = DeleteDevice("AZAL", dsdt, len);
UINT32 DeleteDevice(CONST CHAR8 *Name, UINT8 *dsdt, UINT32 len)
{
  UINT32 i, j;
  INT32 size = 0;
  for (i=20; i<len; i++) {
    if ((dsdt[i+0] == Name[0]) && (dsdt[i+1] == Name[1]) &&
        (dsdt[i+2] == Name[2]) && (dsdt[i+3] == Name[3]) &&
        ((dsdt[i-3] == 0x82) || (dsdt[i-2] == 0x82)) && 
        ((dsdt[i-4] == 0x5B) || (dsdt[i-3] == 0x5B))) {
      if (dsdt[i-3] == 0x82) {
        j = i - 2;
      } else {
        j = i - 1;
      }
      size = get_size(dsdt, j);
      len = move_data(j-2, dsdt, len, -2-size);
      //to correct outers we have to calculate offset
      sizeoffset = -2 - size;
      len = CorrectOuters(dsdt, len, j-3);
      FixAddr(j, sizeoffset);
    }
  }  
  return len;
}

// Find PCIRootUID and all need Fix Device 
UINTN  findPciRoot (UINT8 *dsdt, UINT32 len)
{
	INTN    i, j, k, n, m=0;
	INTN    root = 0;
	INTN    step = 0;
  UINT32  BridgeSize;
  //initialising
  NetworkName = FALSE;
  DisplayName1 = FALSE;
  DisplayName2 = FALSE;
  FirewireName = FALSE;
  ArptName = FALSE;
  
	for (i=20; i<len-20; i++) 
	{
    // find Scope(\_SB) ...
		if (dsdt[i] == '_' && dsdt[i+1] == 'S' && dsdt[i+2] == 'B' && dsdt[i+3] == '_' &&
		    (dsdt[i-4] == 0x10 || dsdt[i-5] == 0x10) && step == 0) 
		{		   
      m=0;     
      for (j=0; j<10; j++)
      {
        if (dsdt[i-1] == 0x5C) m=1;
        if (dsdt[i-j] == 0x10)
        {
          SBADR = i-j+1;
          SBSIZE = get_size(dsdt, i-j+1);
          //DBG("found Scope(\\_SB) address = 0x%08x size = 0x%08x\n", SBADR, SBSIZE);
          if (SBSIZE) {
            break;
          }          
        }
      }
      step++;
    }
    
    // Device(PCIX) ...
    //sample
    /*
     5B 82 8A F1 05 50 43 49 30         Device (PCI0)	{
     08 5F 48 49 44 0C 41 D0 0A 08        Name (_HID, EisaId ("PNP0A08"))
     08 5F 43 49 44 0C 41 D0 0A 03        Name (_CID, EisaId ("PNP0A03"))
     08 5F 41 44 52 00                    Name (_ADR, Zero)
     14 09 5E 42 4E 30 30 00 A4 00        Method (^BN00, 0, NotSerialized) {Return (Zero)}
     14 0B 5F 42 42 4E 00 A4 42 4E 30 30  Method (_BBN, 0, NotSerialized) {Return (BN00 ())}
     08 5F 55 49 44 00                    Name (_UID, Zero)
     14 16 5F 50 52 54 00                 Method (_PRT, 0, NotSerialized)
     */
    
		if (dsdt[i] == 'P' && dsdt[i+1] == 'C' && dsdt[i+2] == 'I' && (dsdt[i+3] == 0x08 || dsdt [i+4] == 0x08) &&
		    (dsdt[i-3] == 0x82 || dsdt[i-4] == 0x82 || dsdt[i-5] == 0x82) && step == 1) 
		{
      // find PCIRootUID
      for (j=i; j<i+64; j++) 
      {
        if (dsdt[j] == '_' && dsdt[j+1] == 'U' && dsdt[j+2] == 'I' && dsdt[j+3] == 'D')  
        {
          /* Slice - I want to set root to zero instead of keeping original value
           if (dsdt[j+4] == 0x0A) 
           root = dsdt[j+5];
           
           if (dsdt[j+5] == 0x08) //what is it???
           root = dsdt[j+4];
           */
          if (dsdt[j+4] == 0x0A) dsdt[j+5] = 0;  //AML_BYTE_PREFIX followed by a number
          else dsdt[j+4] = 0;  //any other will be considered as ONE or WRONG, replace to ZERO
          DBG("found PCIROOTUID = %d\n", root);
          break;
        }	
      }
      
      m=0;
      for (j=0; j<10; j++)
      {
        if (dsdt[i-1] == 0x5C) m=1;  //Root '\'
        if (dsdt[i-j] == 0x82 && dsdt[i-j-1] == 0x5B)  //Device
        {
          PCIADR = i-j+1;
          PCISIZE = get_size(dsdt, i-j+1);
//          DBG("found Device(PCIX) address = 0x%08x size = 0x%08x\n", PCIADR, PCISIZE);
          if(PCISIZE) break;
        }
      }
      
      // From PCI find all Devices
      for (n=0; n<PCISIZE; n++)
      {
        j=i+n;
        // Display Address
        //if (DisplayADR1 == 0x00000000) return;
        if (DisplayADR1[0] != 0x00000000 && 
            CmpAdr(dsdt, j, DisplayADR1[0]))
        {
          DBG("Found DisplayADR1[0]=%x at %x\n", DisplayADR1[0], j);
//There is a one place where I will keep this check. But this is a nonsense
// excluded everywhere          
          
/*        if (dsdt[j-8] == 0x0C && dsdt[j-9] == 0x52 && dsdt[j-10] == 0x44 && dsdt[j-11] == 0x41 &&
              dsdt[j-12] == 0x5F && dsdt[j-13] == 0x08)
          {
            DBG("device NAME(_ADR,0x%08x) before is Name(_ADR,0x%02x%02x%02x%02x) this is not display device\n",
                   DisplayADR1, dsdt[j-4], dsdt[j-5], dsdt[j-6], dsdt[j-7]);
          } 
          else 
          { */
            //device_name[0] = AllocateZeroPool(5);
            //CopyMem(device_name[0], dsdt+j, 4);
            //DBG("found display device NAME(_ADR,0x%08x) And Name is %a\n", 
            //       DisplayADR1, device_name[0]);
            DisplayName1 = TRUE;
            DisplayADR[0] = devFind(dsdt, j);
//         }
        } // End Display1
        
        if (DisplayADR1[1] != 0x00000000 && 
            CmpAdr(dsdt, j, DisplayADR1[1]))
        {
  //        DBG("Found DisplayADR1[1]=%x at %x\n", DisplayADR1[1], j);
            DisplayName2 = TRUE;
            DisplayADR[1] = devFind(dsdt, j);
        } // End Display
        
        if (IDEADR1 != 0x00000000 && 
            CmpAdr(dsdt, j, IDEADR1))
        {
  //        DBG("Found IDEADR1=%x at %x\n", IDEADR1, j);
            IDEADR = devFind(dsdt, j);
        } // End IDE
        
        if (SATAADR1 != 0x00000000 && 
            CmpAdr(dsdt, j, SATAADR1))
        {
  //        DBG("Found SATAADR1=%x at %x\n", SATAADR1, j);
            SATAADR = devFind(dsdt, j);
        } // End SATA
        
        if (SATAAHCIADR1 != 0x00000000 && 
            CmpAdr(dsdt, j, SATAAHCIADR1))
        {
          DBG("Found SATAAHCIADR1=%x at %x\n", SATAAHCIADR1, j);
            SATAAHCIADR = devFind(dsdt, j);
          DBG("... SATAAHCIADR=%x\n", SATAAHCIADR);
        } // End SATA AHCI
                
        // Network Address
        if (NetworkADR1 != 0x00000000 &&
            CmpAdr(dsdt, j, NetworkADR1))
        {
            //this is not a LAN, this is bridge of the lan
            // we have to find next Device
            /*
             Device (NIC)
             {
                Name (_ADR, Zero) - this is NetworkADR2
              */
 //         DBG("found NetworkADR1=%x at %x\n", NetworkADR1, j);
          NetworkADR = devFind(dsdt, j);
 //         DBG("now NetworkADR=%x\n", NetworkADR); 
          BridgeSize = get_size(dsdt, NetworkADR);
    //      DBG("its size=%x\n", BridgeSize);
          if (NetworkADR2 != 0xFFFE){
            for (k=NetworkADR+9; k<NetworkADR+BridgeSize; k++) {
              if (CmpAdr(dsdt, k, NetworkADR2))
              {
                DBG("found NetworkADR2 at %x\n", k);
                NetworkADR = devFind(dsdt, k);
                device_name[1] = AllocateZeroPool(5);
                CopyMem(device_name[1], dsdt+k, 4);
                DBG("found NetWork device NAME(_ADR,0x%08x) at %x And Name is %a\n", 
                    NetworkADR2, k, device_name[1]);
                if (dsdt[k] != 'G' || dsdt[k+1] != 'I' || dsdt[k+2] != 'G' || dsdt[k+3] != 'E') {
                  DBG("Replace the name with GIGE\n");
                  ReplaceName(dsdt, len, device_name[1], "GIGE");
                }              
                NetworkName = TRUE;   
                break;
              }
            }
          }
          
        } // End Network
                
        // Firewire Address
        if (FirewireADR1 != 0x00000000 && 
            CmpAdr(dsdt, j, FirewireADR1))
        {
          FirewireADR = devFind(dsdt, j);
          BridgeSize = get_size(dsdt, FirewireADR);
          if (FirewireADR2 != 0xFFFE ){
            for (k=FirewireADR+9; k<FirewireADR+BridgeSize; k++) {
              if (CmpAdr(dsdt, k, FirewireADR2))
              {
                FirewireADR = devFind(dsdt, k);
                device_name[2] = AllocateZeroPool(5);
                CopyMem(device_name[2], dsdt+k, 4);
                DBG("found Firewire device NAME(_ADR,0x%08x) at %x And Name is %a\n", 
                    FirewireADR2, k, device_name[2]);
                if (dsdt[k] != 'F' || dsdt[k+1] != 'R' || dsdt[k+2] != 'W' || dsdt[k+3] != 'R') {
                  DBG("Replace the name with FRWR\n");
                  ReplaceName(dsdt, len, device_name[2], "FRWR");
                }              
                FirewireName = TRUE;   
                break;
              }
            }
          }
          
        } // End Firewire

        // AirPort Address
        if (ArptADR1 != 0x00000000 && 
            CmpAdr(dsdt, j, ArptADR1))
        {
          ArptADR = devFind(dsdt, j);
          BridgeSize = get_size(dsdt, ArptADR);
          if (ArptADR2 != 0xFFFE){
            for (k=ArptADR+9; k<ArptADR+BridgeSize; k++) {
              if (CmpAdr(dsdt, k, ArptADR2))
              {
                ArptADR = devFind(dsdt, k);
                device_name[9] = AllocateZeroPool(5);
                CopyMem(device_name[9], dsdt+k, 4);
                DBG("found Airport device NAME(_ADR,0x%08x) at %x And Name is %a\n", 
                    ArptADR2, k, device_name[9]);
                if (dsdt[k] != 'A' || dsdt[k+1] != 'R' || dsdt[k+2] != 'P' || dsdt[k+3] != 'T') {
                  DBG("Replace the name with ARPT\n");
                  ReplaceName(dsdt, len, device_name[9], "ARPT");
                }   
                ArptName = TRUE;
                break;
              }
            }
          }
          
        } // End Firewire
        
        
        // Find HDA Deviсe Address
        if (HDAADR != 0x00000000 && HDAFIX &&
            CmpAdr(dsdt, j, HDAADR))
        {
            device_name[4] = AllocateZeroPool(5);
            CopyMem(device_name[4], dsdt+j, 4);
            if (dsdt[j] != 'H' || dsdt[j+1] != 'D' || dsdt[j+2] != 'E' || dsdt[j+3] != 'F') 
            {
              DBG("found HDA device NAME(_ADR,0x%08x) And Name is %a, it is not HDEF will patch to HDEF\n", 
                     HDAADR, device_name[4]);
              ReplaceName(dsdt, len, device_name[4], "HDEF");
             }
            HDAFIX = FALSE;
        } // End HDA
        
        // Find LPCB Device Address
        if (CmpAdr(dsdt, j, 0x001F0000))
        {
          LPCBADR1 = j+14;
          LPCBADR = devFind(dsdt, j);
          LPCBSIZE = get_size(dsdt, LPCBADR);
            device_name[3] = AllocateZeroPool(5);
            CopyMem(device_name[3], dsdt+j, 4);
            DBG("found LPCB device NAME(_ADR,0x001F0000) at %x And Name is %a\n", j,
                device_name[3]);
          if (dsdt[j] != 'L' || dsdt[j+1] != 'P' || dsdt[j+2] != 'C' || dsdt[j+3] != 'B') {
            DBG("Replace the name with LPCB\n");
            ReplaceName(dsdt, len, device_name[3], "LPCB");
          }
        }  // End LPCB
        
        // Find Device SBUS
        if (CmpAdr(dsdt, j, 0x001F0003))
        {
          SBUSADR = devFind(dsdt, j);
        } // end SBUS
        
        // Find Device HPET   // PNP0103
        if (CmpPNP(dsdt, j, 0x0103)) {
          HPETADR = devFind(dsdt, j);
        }
                
        // Find Device TMR   PNP0100
        if (CmpPNP(dsdt, j, 0x0100)) {
          TMRADR = devFind(dsdt, j);
        }
            
        // Find Device PIC or IPIC  PNP0000
        if (CmpPNP(dsdt, j, 0x0000)) {
          PICADR = devFind(dsdt, j);
        }
        
        // Find Device RTC // Name (_HID, EisaId ("PNP0B00")) for RTC
        if (CmpPNP(dsdt, j, 0x0B00)) {
          RTCADR = devFind(dsdt, j);
        }
        
      } // n loop => j=n+i
      step++;
      break;
		} // all PCI Device End		
	} // i loop
  
  //DBG("RTCADR = 0x%08x, TMRADR = 0x%08x, PICADR = 0x%08x\n", RTCADR, TMRADR, PICADR);
  
	return root;
}

/*
UINT32 FixADP1 (UINT8* dsdt, INTN len)
{
  UINT32 i, j;
  UINT32 adr=0, size;
  
  for (i=20; i<len-10; i++) {
    //find Name (_HID, "ACPI0003")
  }    
 // read evice name, replace to ADP1
 //check for
 Name (_PRW, Package (0x02)
 {
   0x1C, 
   0x03
 })
//if absent - add it 
}
 5B 82 4B 04 41 44 50 31 //device (ADP1)
 08 5F 48 49 44 0D       //	name (_HID.
 41 43 50 49 30 30 30 33 00 // ACPI0003
 08 5F 50 52 57 12 06 02 0A 1C 0A 03	.._PRW..
  
*/
UINT32 AddPNLF (UINT8 *dsdt, UINT32 len)
{
  DBG("Start PNLF Fix\n");
  UINT32 i; //, j;
  UINT32  adr  = 0;
//  UINT32  size = 0;
  for (i=20; i<len-10; i++) {  //search APP0002
    if ((dsdt[i + 0] == 0x08) &&
        (dsdt[i + 1] == 0x5F) &&
        (dsdt[i + 2] == 0x48) &&
        (dsdt[i + 3] == 0x49) &&
        (dsdt[i + 4] == 0x44) &&
        (dsdt[i + 5] == 0x0C) &&
        (dsdt[i + 6] == 0x06) &&
        (dsdt[i + 7] == 0x10) &&
        (dsdt[i + 8] == 0x00) &&
        (dsdt[i + 9] == 0x02)){
      return len; //the device already exists
    }
  }
  //search a good place, for example before PWRB PNP0C0C
  for (i=20; i<len; i++) {
    if (CmpPNP(dsdt, i, 0x0C0C)) {
      adr = devFind(dsdt, i);
      break;
    }
  }
  i = adr - 2;
  sizeoffset = sizeof(pnlf);
  len = move_data(i, dsdt, len, sizeoffset);
  CopyMem(dsdt+i, pnlf, sizeof(pnlf));
  CorrectOuters(dsdt, len, adr-2);
  FixAddr(adr, sizeoffset);
	return len;  
}

UINT32 FixRTC (UINT8 *dsdt, UINT32 len)
{
	UINT32 i, j, k, l;
  //	UINT32 m, n;
	UINT32 IOADR   = 0;
	UINT32 RESADR  = 0;
  UINT32 adr     = 0;
  UINT32 rtcsize = 0;
  INT32  offset  = 0;
  
  DBG("Start RTC Fix\n");
  
  for (j=20; j<len; j++) {
    // Find Device RTC // Name (_HID, EisaId ("PNP0B00")) for RTC
    if (CmpPNP(dsdt, j, 0x0B00))
    {
      RTCADR = devFind(dsdt, j);
      adr = RTCADR;
      rtcsize = get_size(dsdt, adr);
  //    DBG("RTC addr=%x RTC size=%x\n", adr, rtcsize);
      if (rtcsize) {
        break;
      }
    } // End RTC    
  }
    
  if (!rtcsize) {
    DBG("BUG! rtcsize not found\n");
    return len;
  }
  
 	sizeoffset = 0;  // for check how many byte add or remove
  
  // Fix RTC
	// Find Name(_CRS, ResourceTemplate ()) find ResourceTemplate 0x11
	
	//DBG("len = 0x%08x, adr = 0x%08x.\n", len, adr);
	for (i=adr+4; i<adr+rtcsize; i++) {
	  // IO (Decode16, ((0x0070, 0x0070)) =>> find this
    if (dsdt[i] == 0x70 && dsdt[i+1] == 0x00 && dsdt[i+2] == 0x70 && dsdt[i+3] == 0x00) 
    {   
      // First Fix RTC CMOS Reset Problem
      if (dsdt[i+4] != 0x00 || dsdt[i+5] != 0x02)  //dsdt[j+4] => Alignment  dsdt[j+5] => Length
      {
        dsdt[i+4] = 0x00;  //Alignment
        dsdt[i+5] = 0x02;  //Length
        DBG("found RTC Length not match, Maybe will case CMOS reset will patch it.\n");
      }
      for (l=adr+4; l<i; l++)
      {
        if (dsdt[l] == 0x11 && dsdt[l+2] == 0x0A)
        {
          RESADR = l+1;  //Format 11, size, 0A, size-3,... 79, 00
          IOADR = l+3;  //IO (Decode16 ==> 47, 01
        }  
      }        
      break;
    }
    if ((dsdt[i+1] == 0x5B) && (dsdt[i+2] == 0x82)) {
      break; //end of RTC device and begin of new Device()
    }    
  }
  
  for (l=adr+4; l<adr+rtcsize; l++)
  {
    if ((dsdt[l] == 0x22) && (l>IOADR) && (l<IOADR+dsdt[IOADR]))  // Had IRQNoFlag
    {
      for (k=l; k<l+20; k++)   
      {
        if ((dsdt[k] == 0x79) || ((dsdt[k] == 0x47) && (dsdt[k+1] == 0x01)) || ((dsdt[k] == 0x86) && (dsdt[k+1] == 0x09)))
        {
          sizeoffset = l - k;  //usually = -3
          DBG("found RTC had IRQNoFlag will move %d bytes\n", sizeoffset);
          // First move offset byte remove IRQNoFlag
          len = move_data(l, dsdt, len, sizeoffset);
          // Fix IO (Decode16, size and _CRS size 
          dsdt[RESADR] += sizeoffset;
          dsdt[IOADR] += sizeoffset;
          break;
        }
      }
    }
    
    // if offset > 0 Fix Device RTC size
    if (sizeoffset != 0) 
    {        
      // RTC size
      len = write_size(adr, dsdt, len, rtcsize); //sizeoffset autochanged
      CorrectOuters(dsdt, len, adr-3);
      offset += sizeoffset;      
      sizeoffset = 0;
    } // sizeoffset if
  } // l loop
//      DBG("Finish RTC patch");		
	
	// need fix other device address
	if (TMRADR > RTCADR) TMRADR += offset;
	if (PICADR > RTCADR) PICADR += offset;
	if (HPETADR > RTCADR) HPETADR += offset;
  FixAddr(RTCADR, offset);
	return len;
}	


UINT32 FixTMR (UINT8 *dsdt, UINT32 len)
{
	UINT32 i, j, k;
//	UINT32 m, n;
	UINT32 IOADR   = 0;
	UINT32 RESADR  = 0;
  UINT32 adr     = 0;
  UINT32 tmrsize = 0;
  INT32  offset  = 0;
  
  for (j=20; j<len; j++) {
    // Find Device TMR   PNP0100
    if (CmpPNP(dsdt, j, 0x0100))
    {
      TMRADR = devFind(dsdt, j);
      adr = TMRADR;
      tmrsize = get_size(dsdt, adr);
 //     DBG("TMR size=%x at %x\n", tmrsize, adr);
      if (tmrsize) {
        break;
      }
    } // End TMR      
  }
  
  	sizeoffset=0;  // for check how many byte add or remove

  // Fix TMR
	// Find Name(_CRS, ResourceTemplate ()) find ResourceTemplate 0x11
//	DBG("Start TMR Fix\n");
	//DBG("len = 0x%08x, adr = 0x%08x.\n", len, adr);
  j = 0;
	for (i=adr; i<adr+tmrsize; i++) //until next Device()
	{  
    if (dsdt[i] == 0x11 && dsdt[i+2] == 0x0A)
		{
      RESADR = i+1;  //Format 11, size, 0A, size-3,... 79, 00
      IOADR = i+3;  //IO (Decode16 ==> 47, 01
      j = get_size(dsdt, IOADR);
		}  
    
    if (dsdt[i] == 0x22)  // Had IRQNoFlag
    {
      for (k=i; k<i+j; k++)   
      {
        if ((dsdt[k] == 0x79) || ((dsdt[k] == 0x47) && (dsdt[k+1] == 0x01)) ||
            ((dsdt[k] == 0x86) && (dsdt[k+1] == 0x09))) {
          sizeoffset = i - k;
          //DBG("found TMR had IRQNoFlag will move %d bytes\n", sizeoffset);
          // First move offset byte remove IRQNoFlag
          len = move_data(i, dsdt, len, sizeoffset);
          // Fix IO (Decode16, size and _CRS size 
          dsdt[RESADR] += sizeoffset;
          dsdt[IOADR] += sizeoffset;
          break;
        }
      }
    }    
    
    // if offset > 0 Fix Device TMR size
		if (sizeoffset != 0) 
    {        
      // TMR size
      len = write_size(adr, dsdt, len, tmrsize);
      CorrectOuters(dsdt, len, adr-3);
      offset += sizeoffset;
      sizeoffset = 0;
    } // offset if
    
    if ((dsdt[i+1] == 0x5B) && (dsdt[i+2] == 0x82)) {
      break; //end of TMR device and begin of new Device()
    }
	} // i loop
	
	// need fix other device address
	if (PICADR > TMRADR) PICADR += offset;
	if (HPETADR > TMRADR) HPETADR += offset;
  FixAddr(TMRADR, offset);	
	return len;
}	

UINT32 FixPIC (UINT8 *dsdt, UINT32 len)
{
	UINT32 i, j, k;
//	UINT32 m, n;
	UINT32 IOADR  = 0;
	UINT32 RESADR = 0;
  UINT32 adr = 0;
  INT32  offset = 0;
  UINT32 picsize = 0;
  
 // DBG("Start PIC Fix\n");
  for (j=20; j<len; j++) {
    // Find Device PIC or IPIC  PNP0000
    if (CmpPNP(dsdt, j, 0x0000))
    {
      PICADR = devFind(dsdt, j);
      adr = PICADR;
      picsize = get_size(dsdt, adr);
 //     DBG("PIC size=%x at %x\n", picsize, adr);
      if (picsize) {
        break;
      }
    } // End PIC    
  }
  
  if (!picsize) {
    return len;
  }
  
  sizeoffset = 0;  // for check how many byte add or remove
  
  // Fix PIC
	// Find Name(_CRS, ResourceTemplate ()) find ResourceTemplate 0x11
	
	//DBG("len = 0x%08x, adr = 0x%08x.\n", len, adr);
	for (i=adr; i<adr+picsize; i++) 
	{  
    if (dsdt[i] == 0x11 && dsdt[i+2] == 0x0A)
		{
      RESADR = i+1;  //Format 11, size, 0A, size-3,... 79, 00
      IOADR = i+3;  //IO (Decode16 ==> 47, 01
		}  
    
    if (dsdt[i] == 0x22)  // Had IRQNoFlag
    {
      for (k=i; k<i+dsdt[IOADR]; k++)   
      {
        if ((dsdt[k] == 0x79) || ((dsdt[k] == 0x47) && (dsdt[k+1] == 0x01)) ||
            ((dsdt[k] == 0x86) && (dsdt[k+1] == 0x09))) {
          sizeoffset = i - k;
          //DBG("found PIC had IRQNoFlag will move %d bytes\n", sizeoffset);
          // First move offset byte remove IRQNoFlag
          len = move_data(i, dsdt, len, sizeoffset);
          // Fix IO (Decode16, size and _CRS size 
          dsdt[RESADR] += sizeoffset;
          dsdt[IOADR] += sizeoffset;
          break;
        }
      }
    }    
    
    // if offset > 0 Fix Device PIC size
		if (sizeoffset != 0 ) 
    {        
      len = write_size(adr, dsdt, len, picsize);
      CorrectOuters(dsdt, len, adr-3);
      offset += sizeoffset;
      sizeoffset = 0;
      //DBG("Finish PIC patch");		
    //  break;        
    } // offset if
    if ((dsdt[i+1] == 0x5B) && (dsdt[i+2] == 0x82)) {
      break; //end of PIC device and begin of new Device()
    }
    
	} // i loop
	
	// need fix other device address
	if (HPETADR > PICADR) HPETADR += offset;
  FixAddr(PICADR, offset);	
	
	return len;
}	

UINT32 FixHPET (UINT8* dsdt, UINT32 len)
{
  UINT32  i, j;
  UINT32  adr    = 0;
  UINT32  hpetsize = 0;
  
//	DBG("Start HPET Fix\n");  
  
  for (j=20; j<len; j++) {
    // Find Device HPET   // PNP0103
    if (CmpPNP(dsdt, j, 0x0103))
    {
      HPETADR = devFind(dsdt, j);
      adr = HPETADR;
      hpetsize = get_size(dsdt, adr);
 //     DBG("HPET size=%x at %x\n", hpetsize, adr);
      if (hpetsize) {
        break;
      }
    } // End HPET   
  }
 
  sizeoffset = sizeof(hpet0);
  if (hpetsize) {
    i = HPETADR - 2;  //pointer to device
    j = hpetsize + 2;
    sizeoffset -= j;
    len = move_data(i, dsdt, len, sizeoffset);   //destroy old HPET
    // add new HPET code 
    CopyMem(dsdt+i, hpet0, sizeof(hpet0));

  } else {
    len = move_data(LPCBADR+LPCBSIZE, dsdt, len, sizeoffset);
    // add HPET code 
    CopyMem(dsdt+LPCBADR+LPCBSIZE, hpet0, sizeoffset);
    HPETADR = LPCBADR + LPCBSIZE + 2;
  }
	  
  CorrectOuters(dsdt, len, HPETADR-3);
	// need fix other device address
	if (TMRADR > HPETADR) TMRADR += sizeoffset;
	if (PICADR > HPETADR) PICADR += sizeoffset;
	if (RTCADR > HPETADR) RTCADR += sizeoffset;
  FixAddr(HPETADR, sizeoffset);
  
	return len;
}	

UINT32 FIXLPCB (UINT8 *dsdt, UINT32 len)
{   
//  DBG("Start LPCB Fix\n");
	//DBG("len = 0x%08x\n", len);
  
	AML_CHUNK* root = aml_create_node(NULL);
	
	// add Method(_DSM,4,NotSerialized) for LPC
  AML_CHUNK* met = aml_add_method(root, "_DSM", 4);
  met = aml_add_store(met);
  AML_CHUNK* pack = aml_add_package(met);
  aml_add_string(pack, "device-id");
  CHAR8 data[] = {0x18, 0x3A, 0x00, 0x00};
  aml_add_byte_buffer(pack, data, 4);
  aml_add_local0(met);
  aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
  // finish Method(_DSM,4,NotSerialized)
  
  aml_calculate_size(root);
  
  CHAR8 *lpcb = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  aml_write_node(root, lpcb, 0);      
  
  aml_destroy_node(root);
  
  // add LPCB code 
  len = move_data(LPCBADR1, dsdt, len, sizeoffset);
  CopyMem(dsdt+LPCBADR1, lpcb, sizeoffset);
	len = write_size(LPCBADR, dsdt, len, LPCBSIZE);
	LPCBSIZE += sizeoffset;
  CorrectOuters(dsdt, len, LPCBADR-3);
  
  FixAddr(LPCBADR, sizeoffset);  
  //DBG("len = 0x%08x\n", len);
  FreePool(lpcb);
  return len;
  
}

UINT32 FIXDisplay1 (UINT8 *dsdt, UINT32 len)
{
  UINT32 i, j;
  
  DBG("Start Display1 Fix\n");
	//DBG("len = 0x%08x\n", len);
  // Display device_id 
  AML_CHUNK* root = aml_create_node(NULL);
  AML_CHUNK* gfx0 = aml_create_node(NULL);
  AML_CHUNK* met = aml_create_node(NULL);
  
  CHAR8 Yes[] = {0x01,0x00,0x00,0x00};
  CHAR8 *portname;
  CHAR8 *CFGname = NULL;
  CHAR8 *name = NULL;
  UINT32 devadr=0, devadr1=0;
  BOOLEAN DISPLAYFIX = FALSE;
  
  // get device size
  devadr1 = devFind(dsdt, DisplayADR[0]);
  devadr = get_size(dsdt, devadr1);
  for (j=devadr1; j<devadr1+devadr; j++) {
    if (CmpAdr(dsdt, j, 0)) {
      devadr1 = devFind(dsdt, j);
      devadr = get_size(dsdt, devadr1);
      DISPLAYFIX = TRUE;
      break;      
    }
  }

    
  if (DisplayADR1[0]) {
    if (!DisplayName1) 
    {
//      AML_CHUNK* pegp = aml_add_device(root, "PEGP");
//      aml_add_name(pegp, "_ADR");
//      aml_add_dword(pegp, DisplayADR1[0]);
      gfx0 = aml_add_device(root, "GFX0");
      aml_add_name(gfx0, "_ADR");
      if (DisplayADR2[0] > 0x3F)
        aml_add_dword(gfx0, DisplayADR2[0]);
      else
        aml_add_byte(gfx0, DisplayADR2[0]);
    }
    //Slice - don't add device as we have one
/*    else 
    {   
      if(!DISPLAYFIX && DisplayVendor[0] != 0x8086)
      { 
        gfx0 = aml_add_device(root, "GFX0");
        aml_add_name(gfx0, "_ADR");
        if (DisplayADR2[0] > 0x3F)
          aml_add_dword(gfx0, DisplayADR2[0]);
        else
          aml_add_byte(gfx0, DisplayADR2[0]);  
      }
    }
 */   
    // Intel GMA and HD
    if (DisplayVendor[0] == 0x8086)
    {
      CHAR8 *modelname = get_gma_model(DisplayID[0]);
      if (AsciiStrnCmp(modelname, "Unknown", 7) == 0)
      {
        DBG("Found Unsupported Intel Display Card Vendor id 0x%04x, device id 0x%04x, don't patch DSDT.\n",
            DisplayVendor[0], DisplayID[0]);
        return len;
      }   
      
      //CHAR8 ClassFix[] =	{ 0x00, 0x00, 0x03, 0x00 };   
      // add Method(_DSM,4,NotSerialized) for GFX0
      //if (!DISPLAYFIX)
      //{
      //    met = aml_add_method(gfx0, "_DSM", 4);
      //}
      //else
      //{      
      
      met = aml_add_method(gfx0, "_DSM", 4);
      //}
      met = aml_add_store(met);
      AML_CHUNK* pack = aml_add_package(met);
      aml_add_string(pack, "AAPL,aux-power-connected");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      aml_add_string(pack, "model");
      aml_add_string_buffer(pack, modelname);
      aml_add_string(pack, "device_type");
      aml_add_string_buffer(pack, "display");
      //
      if ((AsciiStrnCmp(modelname, "Mobile GMA950", 13) == 0) ||
          (AsciiStrnCmp(modelname, "Mobile GMA3150", 14) == 0))
      {
        aml_add_string(pack, "AAPL,HasPanel");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes)); 
        aml_add_string(pack, "built-in");
        aml_add_byte_buffer(pack, (CHAR8*)0x01, 1); 
        //    aml_add_string(pack, "class-code");
        //    aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix)); 
      } 
      else if (AsciiStrnCmp(modelname, "Desktop GMA950", 14) == 0 ||
               AsciiStrnCmp(modelname, "Desktop GMA3150", 15) == 0)
      {
        aml_add_string(pack, "built-in");
        aml_add_byte_buffer(pack, (CHAR8*)0x01, 1); 
        //     aml_add_string(pack, "class-code");
        //     aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
      } 
      else if (AsciiStrnCmp(modelname, "GMAX3100", 8) == 0)
      {
        aml_add_string(pack, "AAPL,HasPanel");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[0], 4); 
        aml_add_string(pack, "AAPL,SelfRefreshSupported");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[1], 4); 
        aml_add_string(pack, "AAPL,backlight-control");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[2], 4);
        aml_add_string(pack, "AAPL00,blackscreen-preferences");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[3], 4);
        aml_add_string(pack, "AAPL01,BacklightIntensity");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[4], 4);
        aml_add_string(pack, "AAPL01,blackscreen-preferences");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[5], 4);
        aml_add_string(pack, "AAPL01,DataJustify");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[6], 4);
        aml_add_string(pack, "AAPL01,Depth");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[7], 4);
        aml_add_string(pack, "AAPL01,Dither");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[8], 4);
        aml_add_string(pack, "AAPL01,DualLink");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[9], 4);
        aml_add_string(pack, "AAPL01,Height");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[10], 4);
        aml_add_string(pack, "AAPL01,Interlace");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[11], 4);
        aml_add_string(pack, "AAPL01,Inverter");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[12], 4);
 //       aml_add_string(pack, "AAPL01,InverterCurrent");
 //       aml_add_byte_buffer(pack, GMAX3100_vals_bad[13], 4);
 //       aml_add_string(pack, "AAPL01,InverterCurrency");
 //       aml_add_byte_buffer(pack, GMAX3100_vals_bad[14], 4);
        aml_add_string(pack, "AAPL01,LinkFormat");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[15], 4);
        aml_add_string(pack, "AAPL01,LinkType");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[16], 4);
        aml_add_string(pack, "AAPL01,Pipe");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[17], 4);
        aml_add_string(pack, "AAPL01,PixelFormat");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[18], 4);
        aml_add_string(pack, "AAPL01,Refresh");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[19], 4);
        aml_add_string(pack, "AAPL01,Stretch");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[20], 4);
        //     aml_add_string(pack, "class-code");
        //     aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
        aml_add_string(pack, "AAPL01,boot-display");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      }
      else if (AsciiStrnCmp(modelname, "Intel HD Graphics 2000 Mobile", 29) == 0)
      {
        //    aml_add_string(pack, "class-code");
        //    aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
        aml_add_string(pack, "hda-gfx");
        aml_add_string_buffer(pack, "onboard-1");
        aml_add_string(pack, "AAPL00,PixelFormat");
        aml_add_byte_buffer(pack, HD2000_vals[0], 4); 
        aml_add_string(pack, "AAPL00,T1");
        aml_add_byte_buffer(pack, HD2000_vals[1], 4);
        aml_add_string(pack, "AAPL00,T2");
        aml_add_byte_buffer(pack, HD2000_vals[2], 4);
        aml_add_string(pack, "AAPL00,T3");
        aml_add_byte_buffer(pack, HD2000_vals[3], 4);
        aml_add_string(pack, "AAPL00,T4");
        aml_add_byte_buffer(pack, HD2000_vals[4], 4);
        aml_add_string(pack, "AAPL00,T5");
        aml_add_byte_buffer(pack, HD2000_vals[5], 4);
        aml_add_string(pack, "AAPL00,T6");
        aml_add_byte_buffer(pack, HD2000_vals[6], 4);
        aml_add_string(pack, "AAPL00,T7");
        aml_add_byte_buffer(pack, HD2000_vals[7], 4);
        aml_add_string(pack, "AAPL00,LinkType");
        aml_add_byte_buffer(pack, HD2000_vals[8], 4);
        aml_add_string(pack, "AAPL00,LinkFormat");
        aml_add_byte_buffer(pack, HD2000_vals[9], 4);
        aml_add_string(pack, "AAPL00,DualLink");
        aml_add_byte_buffer(pack, HD2000_vals[10], 4);
        aml_add_string(pack, "AAPL00,Dither");
        aml_add_byte_buffer(pack, HD2000_vals[11], 4);
        aml_add_string(pack, "AAPL00,DataJustify");
        aml_add_byte_buffer(pack, HD2000_vals[12], 4);
        aml_add_string(pack, "graphic-options");
        aml_add_byte_buffer(pack, HD2000_vals[13], 4);
        aml_add_string(pack, "AAPL,tbl-info");
        aml_add_byte_buffer(pack, HD2000_tbl_info, 18);
        aml_add_string(pack, "AAPL,os-info");
        aml_add_byte_buffer(pack, HD2000_os_info, 20);
        aml_add_string(pack, "AAPL00,boot-display");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      }
      else if (AsciiStrnCmp(modelname, "Intel HD Graphics 3000 Mobile", 29) == 0)
      {
        //      aml_add_string(pack, "class-code");
        //      aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
        aml_add_string(pack, "hda-gfx");
        aml_add_string_buffer(pack, "onboard-1");
        aml_add_string(pack, "AAPL00,PixelFormat");
        aml_add_byte_buffer(pack, HD3000_vals[0], 4); 
        aml_add_string(pack, "AAPL00,T1");
        aml_add_byte_buffer(pack, HD3000_vals[1], 4);
        aml_add_string(pack, "AAPL00,T2");
        aml_add_byte_buffer(pack, HD3000_vals[2], 4);
        aml_add_string(pack, "AAPL00,T3");
        aml_add_byte_buffer(pack, HD3000_vals[3], 4);
        aml_add_string(pack, "AAPL00,T4");
        aml_add_byte_buffer(pack, HD3000_vals[4], 4);
        aml_add_string(pack, "AAPL00,T5");
        aml_add_byte_buffer(pack, HD3000_vals[5], 4);
        aml_add_string(pack, "AAPL00,T6");
        aml_add_byte_buffer(pack, HD3000_vals[6], 4);
        aml_add_string(pack, "AAPL00,T7");
        aml_add_byte_buffer(pack, HD3000_vals[7], 4);
        aml_add_string(pack, "AAPL00,LinkType");
        aml_add_byte_buffer(pack, HD3000_vals[8], 4);
        aml_add_string(pack, "AAPL00,LinkFormat");
        aml_add_byte_buffer(pack, HD3000_vals[9], 4);
        aml_add_string(pack, "AAPL00,DualLink");
        aml_add_byte_buffer(pack, HD3000_vals[10], 4);
        aml_add_string(pack, "AAPL00,Dither");
        aml_add_byte_buffer(pack, HD3000_vals[11], 4);
        aml_add_string(pack, "AAPL00,DataJustify");
        aml_add_byte_buffer(pack, HD3000_vals[12], 4);
        aml_add_string(pack, "graphic-options");
        aml_add_byte_buffer(pack, HD3000_vals[13], 4);
        aml_add_string(pack, "AAPL,tbl-info");
        aml_add_byte_buffer(pack, HD3000_tbl_info, 18);
        aml_add_string(pack, "AAPL,os-info");
        aml_add_byte_buffer(pack, HD3000_os_info, 20);
        aml_add_string(pack, "AAPL,snb-platform-id");
        aml_add_byte_buffer(pack, HD3000_vals[16], 4);
        aml_add_string(pack, "AAPL00,boot-display");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      }
      else if (AsciiStrnCmp(modelname, "Intel HD Graphics 2000", 22) == 0)
      {
        aml_add_string(pack, "built-in");
        aml_add_byte_buffer(pack, (CHAR8*)0x01, 1);
        //      aml_add_string(pack, "class-code");
        //      aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
        aml_add_string(pack, "device-id");
        aml_add_byte_buffer(pack, (CHAR8*)&DisplayID[0], 4);
        aml_add_string(pack, "hda-gfx");
        aml_add_string_buffer(pack, "onboard-1");
        aml_add_string(pack, "AAPL,tbl-info");
        aml_add_byte_buffer(pack, HD2000_tbl_info, 18);
        aml_add_string(pack, "AAPL,os-info");
        aml_add_byte_buffer(pack, HD2000_os_info, 20);
        aml_add_string(pack, "AAPL00,boot-display");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      }
      else if (AsciiStrnCmp(modelname, "Intel HD Graphics 3000", 22) == 0)
      {
        aml_add_string(pack, "built-in");
        aml_add_byte_buffer(pack, (CHAR8*)0x01, 1);
        //      aml_add_string(pack, "class-code");
        //      aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
        aml_add_string(pack, "device-id");
        aml_add_byte_buffer(pack, (CHAR8*)&DisplayID[0], 4);
        aml_add_string(pack, "hda-gfx");
        aml_add_string_buffer(pack, "onboard-1");
        aml_add_string(pack, "AAPL,tbl-info");
        aml_add_byte_buffer(pack, HD3000_tbl_info, 18);
        aml_add_string(pack, "AAPL,os-info");
        aml_add_byte_buffer(pack, HD3000_os_info, 20);
        aml_add_string(pack, "AAPL00,boot-display");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      }
      aml_add_local0(met);
      aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
      // finish Method(_DSM,4,NotSerialized)
    }
    
    // NVIDIA
    if (DisplayVendor[0] == 0x10DE)
    {
      CHAR8 *modelname = nv_name(DisplayVendor[0], DisplayID[0]);
      // add Method(_DSM,4,NotSerialized) for GFX0
      if (!DISPLAYFIX)
      {
        met = aml_add_method(gfx0, "_DSM", 4);
      }
      else
      {
        met = aml_add_method(root, "_DSM", 4);
      }
      met = aml_add_store(met);
      AML_CHUNK* pack = aml_add_package(met);
      aml_add_string(pack, "AAPL,aux-power-connected");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      aml_add_string(pack, "AAPL00,DualLink");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      aml_add_string(pack, "@0,AAPL,boot-display");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));  
      aml_add_string(pack, "@0,name");
      aml_add_string_buffer(pack, "NVDA,Display-A");  
      aml_add_string(pack, "@0,compatible");
      aml_add_string_buffer(pack, "NVDA,NVMac");  
      aml_add_string(pack, "@0,device_type");
      aml_add_string_buffer(pack, "display");  
      aml_add_string(pack, "@1,name");
      aml_add_string_buffer(pack, "NVDA,Display-B");  
      aml_add_string(pack, "@1,compatible");
      aml_add_string_buffer(pack, "NVDA,NVMac"); 
      aml_add_string(pack, "@1,device_type");
      aml_add_string_buffer(pack, "display"); 
      aml_add_string(pack, "device_type");
      aml_add_string_buffer(pack, "NVDA,Parent"); 
      aml_add_string(pack, "NVCAP");
      aml_add_byte_buffer(pack, NVCAP, sizeof(NVCAP)); 
      aml_add_string(pack, "NVPM");
      aml_add_byte_buffer(pack, NVPM, sizeof(NVPM)); 
      aml_add_string(pack, "model");
      aml_add_string_buffer(pack, modelname); 
      aml_add_string(pack, "rom-revision");
      aml_add_string_buffer(pack, "pcj auto patch DSDT ver1.0");
      aml_add_string(pack, "hda-gfx");
      aml_add_string_buffer(pack, "onboard-1"); 
      UINT32 vedioram = nv_mem_detect(&Displaydevice[0]); 
      aml_add_string(pack, "VRAM,totalsize");
      aml_add_dword(pack, vedioram); 
      aml_add_string(pack, "device-id");
      aml_add_byte_buffer(pack, (CHAR8*)&DisplayID[0], 4);
      if (!Display1PCIE)
      {
        aml_add_string(pack, "IOPCIExpressLinkCapabilities");
        aml_add_dword(pack, 0x130d1) ;//0x1e80); 
        aml_add_string(pack, "IOPCIExpressLinkStatus");
        aml_add_dword(pack, 0x10880); //0x880); 
      }
      aml_add_local0(met);
      aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
      // finish Method(_DSM,4,NotSerialized)
    }
    
    // ATI
    if (DisplayVendor[0] == 0x1002)
    {
      CHAR8 *modelname = ati_name(DisplayID[0], DisplaySubID[0]);
      // add Method(_DSM,4,NotSerialized) for GFX0
      if (!DISPLAYFIX)
      {
        met = aml_add_method(gfx0, "_DSM", 4);
      }
      else
      {
        met = aml_add_method(root, "_DSM", 4);
      }
      met = aml_add_store(met);
      AML_CHUNK* pack = aml_add_package(met);
      aml_add_string(pack, "AAPL,aux-power-connected");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      aml_add_string(pack, "AAPL00,DualLink");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      aml_add_string(pack, "@0,AAPL,boot-display");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      UINT8 ports = ati_port(DisplayID[0], DisplaySubID[0]);
      CHAR8 *cfgname = ati_cfg_name(DisplayID[0], DisplaySubID[0]);
      CFGname = AllocateZeroPool(sizeof(cfgname)+5);
      AsciiSPrint(CFGname, sizeof(cfgname)+5, "ATY,%a", cfgname);
      for (i=0; i<ports; i++)
      {
        portname = AllocateZeroPool(8);
        AsciiSPrint(portname, 8, "@%d,name", i);
        aml_add_string(pack, portname);
        aml_add_string_buffer(pack, CFGname);  
      } 
      
      CHAR8 *cardver = ATI_romrevision(&Displaydevice[0]);
      aml_add_string(pack, "ATY,Card#");
      aml_add_string_buffer(pack, cardver);       
      aml_add_string(pack, "ATY,Copyright");
      aml_add_string_buffer(pack, "Copyright AMD Inc. All Rights Reserved. 2005-2011");    
      aml_add_string(pack, "ATY,EFIVersion");
      aml_add_string_buffer(pack, "01.00.3180");  
      aml_add_string(pack, "model");
      aml_add_string_buffer(pack, modelname);
      aml_add_string(pack, "name");
      name = AllocateZeroPool(sizeof(cfgname)+11);
      AsciiSPrint(name, sizeof(cfgname)+11, "ATY,%aParent", cfgname);
      aml_add_string_buffer(pack, name);  
      aml_add_string(pack, "ATY,VendorID");
      aml_add_byte_buffer(pack, (CHAR8*)&DisplayVendor[0], 4); 
      aml_add_string(pack, "ATY,DeviceID");
      aml_add_byte_buffer(pack, (CHAR8*)&DisplayID[0], 4); 
      aml_add_string(pack, "device-id");
      CHAR8 data[] = {0xE1,0x68,0x00,0x00};
      aml_add_byte_buffer(pack, data, sizeof(data));
      aml_add_string(pack, "org-device-id");
      aml_add_byte_buffer(pack, (CHAR8*)&DisplayID[0], 4); 
      aml_add_string(pack, "hda-gfx");
      aml_add_string_buffer(pack, "onboard-1");
      UINT32 vedioram = ATI_vram_size(&Displaydevice[0]); 
      aml_add_string(pack, "VRAM,totalsize");
      aml_add_dword(pack, vedioram); 
      if (!Display1PCIE)
      {
        aml_add_string(pack, "IOPCIExpressLinkCapabilities");
        aml_add_dword(pack, 0x130d1) ;//0x1e80); 
        aml_add_string(pack, "IOPCIExpressLinkStatus");
        aml_add_dword(pack, 0x10880); //0x880);  
      }
      aml_add_local0(met);
      aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
      // finish Method(_DSM,4,NotSerialized)
    }
    
    // HDAU
    if (GFXHDAFIX)
    {
      AML_CHUNK* device = aml_add_device(root, "HDAU");
      aml_add_name(device, "_ADR");
      aml_add_byte(device, 0x01);
      // add Method(_DSM,4,NotSerialized) for GFX0
      AML_CHUNK* met = aml_add_method(device, "_DSM", 4);
      met = aml_add_store(met);
      AML_CHUNK* pack = aml_add_package(met);
      //aml_add_string(pack, "device-id");
      //CHAR8 data[] = {0x38,0xAA,0x00,0x00};
      //aml_add_byte_buffer(pack, data, sizeof(data));
      //aml_add_string(pack, "codec-id");
      //aml_add_byte_buffer(pack, (CHAR8*)&GfxcodecId, 4);
      aml_add_string(pack, "layout-id");
      aml_add_byte_buffer(pack, (CHAR8*)&GfxlayoutId, 4);
      aml_add_string(pack, "hda-gfx");
      aml_add_string_buffer(pack, "onboard-2");
      //aml_add_string(pack, "name");
      //aml_add_string(pack, "pci1002,aa38");
      //aml_add_string(pack, "IOName");
      //aml_add_string(pack, "pci1002,aa38");
      //aml_add_string(pack, "layout-id");
      //CHAR8 data1[] = {0x12,0x00,0x00,0x00};
      //aml_add_byte_buffer(pack, data1, sizeof(data1));
      aml_add_string(pack, "PinConfigurations");
      CHAR8 data2[] = {0xe0,0x00,0x56,0x28};
      aml_add_byte_buffer(pack, data2, sizeof(data2));
      aml_add_local0(met);
      aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
      // finish Method(_DSM,4,NotSerialized)
    }
  }
  
  aml_calculate_size(root);
  
  CHAR8 *display = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  //DBG("Display code size = 0x%08x\n", sizeoffset);
  
  aml_write_node(root, display, 0);
  
  aml_destroy_node(root);
  
  if (DisplayADR[0])
  {
    UINT32 adr=0, adr1=0;
    // get Display device size
    adr1 = devFind(dsdt, DisplayADR[0]);
    adr = get_size(dsdt, adr1);
    // move data to back for add Display
    if (!DISPLAYFIX)
    { 
      len = move_data(adr1+adr, dsdt, len, sizeoffset);
      CopyMem(dsdt+adr1+adr, display, sizeoffset);
    }
    else
    {
      len = move_data(devadr1+devadr, dsdt, len, sizeoffset);
      CopyMem(dsdt+devadr1+devadr, display, sizeoffset);
      len = write_size(devadr1, dsdt, len, devadr);
    }
    CorrectOuters(dsdt, len, adr1-3);
  }
  else
  {
    len = move_data(PCIADR+PCISIZE, dsdt, len, sizeoffset);
    CopyMem(dsdt+PCIADR+PCISIZE, display, sizeoffset);
    CorrectOuters(dsdt, len, PCIADR-3);
  }
  
  if (DisplayADR[0])
  {
    FixAddr(DisplayADR[0], sizeoffset); 
  }
  if (CFGname) {
    FreePool(CFGname);
  }
  if (name) {
    FreePool(name);
  } 
  
  FreePool(display);
  return len;  
}

CHAR8 data2[] = {0xe0,0x00,0x56,0x28};
CHAR8 Yes[] = {0x01,0x00,0x00,0x00};

UINT32 FIXDisplay2 (UINT8 *dsdt, UINT32 len)
{
  UINT32 i, j, k;
  
  DBG("Start Display2 Fix\n");
	//DBG("len = 0x%08x\n", len);
  // Display device_id 
  AML_CHUNK* root = aml_create_node(NULL);
  AML_CHUNK* gfx0 = aml_create_node(NULL);
  AML_CHUNK* met = aml_create_node(NULL);
    
  CHAR8 *portname;
  CHAR8 *CFGname = NULL;
  CHAR8 *name = NULL;
  
  UINT32 devadr=0, devadr1=0;
  BOOLEAN DISPLAYFIX = FALSE;
  // get ide device size
  for (i=0; i<100; i++)
  {
    if (dsdt[DisplayADR[1]-i] == 0x82 && dsdt[DisplayADR[1]-i-1] == 0x5B)
    {
      //DBG("Found Display2 Device\n");
      devadr1 = DisplayADR[1]-i+1;
      devadr = get_size(dsdt, devadr1);
      // find Name(_ADR, Zero) if yes, don't need to inject GFX0 name
      for (j=0 ; j<devadr; j++)
      {
        if (dsdt[devadr1+j] == '_' && dsdt[devadr1+j+1] == 'A' && dsdt[devadr1+j+2] == 'D' &&
            dsdt[devadr1+j+3] == 'R' && (dsdt[devadr1+j+4] == 0x00 || (dsdt[devadr1+j+4] == 0x0A && dsdt[devadr1+j+5] == 0x00)))
        {
          //DBG("Found Display Fix TRUE\n");
          for (k=0; k<50; k++)
          {
            if (dsdt[devadr1+j-k] == 0x82 && dsdt[devadr1+j-k-1] == 0x5B)
            {
              //DBG("Found Display Fix TRUE\n");
              devadr1 = devadr1+j-k+1;
              devadr = get_size(dsdt, devadr1);
              DISPLAYFIX = TRUE;
              break;
            }
          }
          break;
        }
      }  
      len = DeleteDevice("CRT_", dsdt, len);
      break;
    }
  }
  
  
  if (DisplayADR1[1]) {
    if (!DisplayName2) 
    {
      AML_CHUNK* pegp = aml_add_device(root, "PEGP");
      aml_add_name(pegp, "_ADR");
      aml_add_dword(pegp, DisplayADR1[1]);
      gfx0 = aml_add_device(pegp, "GFX0");
      aml_add_name(gfx0, "_ADR");
      if (DisplayADR2[1] > 0x3F)
        aml_add_dword(gfx0, DisplayADR2[1]);
      else
        aml_add_byte(gfx0, DisplayADR2[1]);
    }
    else 
    {    
      if(!DISPLAYFIX && DisplayVendor[1] != 0x8086)
      {
        gfx0 = aml_add_device(root, "GFX0");
        aml_add_name(gfx0, "_ADR");
        if (DisplayADR2[1] > 0x3F)
          aml_add_dword(gfx0, DisplayADR2[1]);
        else
          aml_add_byte(gfx0, DisplayADR2[1]);  
      }
    }
    
    // Intel GMA and HD
    if (DisplayVendor[1] == 0x8086)
    {
      CHAR8 *modelname = get_gma_model(DisplayID[1]);
      if (AsciiStrnCmp(modelname, "Unknown", 7) == 0)
      {
        DBG("Found Unsupported Intel Display Card Vendor id 0x%04x, device id 0x%04x, don't patch DSDT.\n",
            DisplayVendor[1], DisplayID[1]);
        return len;
      }   
      
      //CHAR8 ClassFix[] =	{ 0x00, 0x00, 0x03, 0x00 };   
      // add Method(_DSM,4,NotSerialized) for GFX0
      //if (!DISPLAYFIX)
      //{
      //    met = aml_add_method(gfx0, "_DSM", 4);
      //}
      //else
      //{
      met = aml_add_method(root, "_DSM", 4);
      //}
      met = aml_add_store(met);
      AML_CHUNK* pack = aml_add_package(met);
      aml_add_string(pack, "AAPL,aux-power-connected");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      aml_add_string(pack, "model");
      aml_add_string_buffer(pack, modelname);
      aml_add_string(pack, "device_type");
      aml_add_string_buffer(pack, "display");
      //
      if (AsciiStrnCmp(modelname, "Mobile GMA950", 13) == 0 ||
          AsciiStrnCmp(modelname, "Mobile GMA3150", 14) == 0)
      {
        aml_add_string(pack, "AAPL,HasPanel");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes)); 
        aml_add_string(pack, "built-in");
        aml_add_byte_buffer(pack, (CHAR8*)0x01, 1); 
        //   aml_add_string(pack, "class-code");
        //   aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix)); 
      } 
      else if (AsciiStrnCmp(modelname, "Desktop GMA950", 14) == 0 ||
               AsciiStrnCmp(modelname, "Desktop GMA3150", 15) == 0)
      {
        aml_add_string(pack, "built-in");
        aml_add_byte_buffer(pack, (CHAR8*)0x01, 1); 
        //    aml_add_string(pack, "class-code");
        //    aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
      } 
      else if (AsciiStrnCmp(modelname, "GMAX3100", 8) == 0)
      {
        aml_add_string(pack, "AAPL,HasPanel");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[0], 4); 
        aml_add_string(pack, "AAPL,SelfRefreshSupported");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[1], 4); 
        aml_add_string(pack, "AAPL,backlight-control");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[2], 4);
        aml_add_string(pack, "AAPL00,blackscreen-preferences");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[3], 4);
        aml_add_string(pack, "AAPL01,BacklightIntensity");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[4], 4);
        aml_add_string(pack, "AAPL01,blackscreen-preferences");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[5], 4);
        aml_add_string(pack, "AAPL01,DataJustify");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[6], 4);
        aml_add_string(pack, "AAPL01,Depth");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[7], 4);
        aml_add_string(pack, "AAPL01,Dither");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[8], 4);
        aml_add_string(pack, "AAPL01,DualLink");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[9], 4);
        aml_add_string(pack, "AAPL01,Height");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[10], 4);
        aml_add_string(pack, "AAPL01,Interlace");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[11], 4);
        aml_add_string(pack, "AAPL01,Inverter");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[12], 4);
        aml_add_string(pack, "AAPL01,InverterCurrent");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[13], 4);
        aml_add_string(pack, "AAPL01,InverterCurrency");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[14], 4);
        aml_add_string(pack, "AAPL01,LinkFormat");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[15], 4);
        aml_add_string(pack, "AAPL01,LinkType");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[16], 4);
        aml_add_string(pack, "AAPL01,Pipe");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[17], 4);
        aml_add_string(pack, "AAPL01,PixelFormat");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[18], 4);
        aml_add_string(pack, "AAPL01,Refresh");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[19], 4);
        aml_add_string(pack, "AAPL01,Stretch");
        aml_add_byte_buffer(pack, GMAX3100_vals_bad[20], 4);
        //     aml_add_string(pack, "class-code");
        //     aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
        aml_add_string(pack, "AAPL01,boot-display");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      }
      else if (AsciiStrnCmp(modelname, "Intel HD Graphics 2000 Mobile", 29) == 0)
      {
        //      aml_add_string(pack, "class-code");
        //      aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
        aml_add_string(pack, "hda-gfx");
        aml_add_string_buffer(pack, "onboard-1");
        aml_add_string(pack, "AAPL00,PixelFormat");
        aml_add_byte_buffer(pack, HD2000_vals[0], 4); 
        aml_add_string(pack, "AAPL00,T1");
        aml_add_byte_buffer(pack, HD2000_vals[1], 4);
        aml_add_string(pack, "AAPL00,T2");
        aml_add_byte_buffer(pack, HD2000_vals[2], 4);
        aml_add_string(pack, "AAPL00,T3");
        aml_add_byte_buffer(pack, HD2000_vals[3], 4);
        aml_add_string(pack, "AAPL00,T4");
        aml_add_byte_buffer(pack, HD2000_vals[4], 4);
        aml_add_string(pack, "AAPL00,T5");
        aml_add_byte_buffer(pack, HD2000_vals[5], 4);
        aml_add_string(pack, "AAPL00,T6");
        aml_add_byte_buffer(pack, HD2000_vals[6], 4);
        aml_add_string(pack, "AAPL00,T7");
        aml_add_byte_buffer(pack, HD2000_vals[7], 4);
        aml_add_string(pack, "AAPL00,LinkType");
        aml_add_byte_buffer(pack, HD2000_vals[8], 4);
        aml_add_string(pack, "AAPL00,LinkFormat");
        aml_add_byte_buffer(pack, HD2000_vals[9], 4);
        aml_add_string(pack, "AAPL00,DualLink");
        aml_add_byte_buffer(pack, HD2000_vals[10], 4);
        aml_add_string(pack, "AAPL00,Dither");
        aml_add_byte_buffer(pack, HD2000_vals[11], 4);
        aml_add_string(pack, "AAPL00,DataJustify");
        aml_add_byte_buffer(pack, HD2000_vals[12], 4);
        aml_add_string(pack, "graphic-options");
        aml_add_byte_buffer(pack, HD2000_vals[13], 4);
        aml_add_string(pack, "AAPL,tbl-info");
        aml_add_byte_buffer(pack, HD2000_tbl_info, 18);
        aml_add_string(pack, "AAPL,os-info");
        aml_add_byte_buffer(pack, HD2000_os_info, 20);
        aml_add_string(pack, "AAPL00,boot-display");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      }
      else if (AsciiStrnCmp(modelname, "Intel HD Graphics 3000 Mobile", 29) == 0)
      {
        //     aml_add_string(pack, "class-code");
        //     aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
        aml_add_string(pack, "hda-gfx");
        aml_add_string_buffer(pack, "onboard-1");
        aml_add_string(pack, "AAPL00,PixelFormat");
        aml_add_byte_buffer(pack, HD3000_vals[0], 4); 
        aml_add_string(pack, "AAPL00,T1");
        aml_add_byte_buffer(pack, HD3000_vals[1], 4);
        aml_add_string(pack, "AAPL00,T2");
        aml_add_byte_buffer(pack, HD3000_vals[2], 4);
        aml_add_string(pack, "AAPL00,T3");
        aml_add_byte_buffer(pack, HD3000_vals[3], 4);
        aml_add_string(pack, "AAPL00,T4");
        aml_add_byte_buffer(pack, HD3000_vals[4], 4);
        aml_add_string(pack, "AAPL00,T5");
        aml_add_byte_buffer(pack, HD3000_vals[5], 4);
        aml_add_string(pack, "AAPL00,T6");
        aml_add_byte_buffer(pack, HD3000_vals[6], 4);
        aml_add_string(pack, "AAPL00,T7");
        aml_add_byte_buffer(pack, HD3000_vals[7], 4);
        aml_add_string(pack, "AAPL00,LinkType");
        aml_add_byte_buffer(pack, HD3000_vals[8], 4);
        aml_add_string(pack, "AAPL00,LinkFormat");
        aml_add_byte_buffer(pack, HD3000_vals[9], 4);
        aml_add_string(pack, "AAPL00,DualLink");
        aml_add_byte_buffer(pack, HD3000_vals[10], 4);
        aml_add_string(pack, "AAPL00,Dither");
        aml_add_byte_buffer(pack, HD3000_vals[11], 4);
        aml_add_string(pack, "AAPL00,DataJustify");
        aml_add_byte_buffer(pack, HD3000_vals[12], 4);
        aml_add_string(pack, "graphic-options");
        aml_add_byte_buffer(pack, HD3000_vals[13], 4);
        aml_add_string(pack, "AAPL,tbl-info");
        aml_add_byte_buffer(pack, HD3000_tbl_info, 18);
        aml_add_string(pack, "AAPL,os-info");
        aml_add_byte_buffer(pack, HD3000_os_info, 20);
        aml_add_string(pack, "AAPL,snb-platform-id");
        aml_add_byte_buffer(pack, HD3000_vals[16], 4);
        aml_add_string(pack, "AAPL00,boot-display");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      }
      else if (AsciiStrnCmp(modelname, "Intel HD Graphics 2000", 22) == 0)
      {
        aml_add_string(pack, "built-in");
        aml_add_byte_buffer(pack, (CHAR8*)0x01, 1);
        //      aml_add_string(pack, "class-code");
        //      aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
        aml_add_string(pack, "device-id");
        aml_add_byte_buffer(pack, (CHAR8*)&DisplayID[1], 4);
        aml_add_string(pack, "hda-gfx");
        aml_add_string_buffer(pack, "onboard-1");
        aml_add_string(pack, "AAPL,tbl-info");
        aml_add_byte_buffer(pack, HD2000_tbl_info, 18);
        aml_add_string(pack, "AAPL,os-info");
        aml_add_byte_buffer(pack, HD2000_os_info, 20);
        aml_add_string(pack, "AAPL00,boot-display");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      }
      else if (AsciiStrnCmp(modelname, "Intel HD Graphics 3000", 22) == 0)
      {
        aml_add_string(pack, "built-in");
        aml_add_byte_buffer(pack, (CHAR8*)0x01, 1);
        //      aml_add_string(pack, "class-code");
        //       aml_add_byte_buffer(pack, ClassFix, sizeof(ClassFix));
        aml_add_string(pack, "device-id");
        aml_add_byte_buffer(pack, (CHAR8*)&DisplayID[1], 4);
        aml_add_string(pack, "hda-gfx");
        aml_add_string_buffer(pack, "onboard-1");
        aml_add_string(pack, "AAPL,tbl-info");
        aml_add_byte_buffer(pack, HD3000_tbl_info, 18);
        aml_add_string(pack, "AAPL,os-info");
        aml_add_byte_buffer(pack, HD3000_os_info, 20);
        aml_add_string(pack, "AAPL00,boot-display");
        aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      }
      aml_add_local0(met);
      aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
      // finish Method(_DSM,4,NotSerialized)
    }
    
    // NVIDIA
    if (DisplayVendor[1] == 0x10DE)
    {
      CHAR8 *modelname = nv_name(DisplayVendor[1], DisplayID[1]);
      // add Method(_DSM,4,NotSerialized) for GFX0
      if (!DISPLAYFIX)
      {
        met = aml_add_method(gfx0, "_DSM", 4);
      }
      else
      {
        met = aml_add_method(root, "_DSM", 4);
      }
      met = aml_add_store(met);
      AML_CHUNK* pack = aml_add_package(met);
      aml_add_string(pack, "AAPL,aux-power-connected");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      aml_add_string(pack, "AAPL00,DualLink");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      aml_add_string(pack, "@0,AAPL,boot-display");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));  
      aml_add_string(pack, "@0,name");
      aml_add_string_buffer(pack, "NVDA,Display-A");  
      aml_add_string(pack, "@0,compatible");
      aml_add_string_buffer(pack, "NVDA,NVMac");  
      aml_add_string(pack, "@0,device_type");
      aml_add_string_buffer(pack, "display");  
      aml_add_string(pack, "@1,name");
      aml_add_string_buffer(pack, "NVDA,Display-B");  
      aml_add_string(pack, "@1,compatible");
      aml_add_string_buffer(pack, "NVDA,NVMac"); 
      aml_add_string(pack, "@1,device_type");
      aml_add_string_buffer(pack, "display"); 
      aml_add_string(pack, "device_type");
      aml_add_string_buffer(pack, "NVDA,Parent"); 
      aml_add_string(pack, "NVCAP");
      aml_add_byte_buffer(pack, NVCAP, sizeof(NVCAP)); 
      aml_add_string(pack, "NVPM");
      aml_add_byte_buffer(pack, NVPM, sizeof(NVPM)); 
      aml_add_string(pack, "model");
      aml_add_string_buffer(pack, modelname); 
      aml_add_string(pack, "rom-revision");
      aml_add_string_buffer(pack, "pcj auto patch DSDT ver1.0");
      aml_add_string(pack, "hda-gfx");
      aml_add_string_buffer(pack, "onboard-1"); 
      UINT32 vedioram = nv_mem_detect(&Displaydevice[1]); 
      aml_add_string(pack, "VRAM,totalsize");
      aml_add_dword(pack, vedioram); 
      aml_add_string(pack, "device-id");
      aml_add_byte_buffer(pack, (CHAR8*)&DisplayID[1], 4);
      if (!Display2PCIE)
      {
        aml_add_string(pack, "IOPCIExpressLinkCapabilities");
        aml_add_dword(pack, 0x130d1) ;//0x1e80); 
        aml_add_string(pack, "IOPCIExpressLinkStatus");
        aml_add_dword(pack, 0x10880); //0x880);  
      }
      aml_add_local0(met);
      aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
      // finish Method(_DSM,4,NotSerialized)
    }
    
    // ATI
    if (DisplayVendor[1] == 0x1002)
    {
      CHAR8 *modelname = ati_name(DisplayID[1], DisplaySubID[1]);
      // add Method(_DSM,4,NotSerialized) for GFX0
      if (!DISPLAYFIX)
      {
        met = aml_add_method(gfx0, "_DSM", 4);
      }
      else
      {
        met = aml_add_method(root, "_DSM", 4);
      }
      met = aml_add_store(met);
      AML_CHUNK* pack = aml_add_package(met);
      aml_add_string(pack, "AAPL,aux-power-connected");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      aml_add_string(pack, "AAPL00,DualLink");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));
      aml_add_string(pack, "@0,AAPL,boot-display");
      aml_add_byte_buffer(pack, Yes, sizeof(Yes));  
      UINT8 ports = ati_port(DisplayID[1], DisplaySubID[1]);
      CHAR8 *cfgname = ati_cfg_name(DisplayID[1], DisplaySubID[1]);
      CFGname = AllocateZeroPool(sizeof(cfgname)+5);
      AsciiSPrint(CFGname, sizeof(cfgname)+5, "ATY,%a", cfgname);
      for (i=0; i<ports; i++)
      {
        portname = AllocateZeroPool(8);
        AsciiSPrint(portname, 8, "@%d,name", i);
        aml_add_string(pack, portname);
        aml_add_string_buffer(pack, CFGname);  
      }          
      CHAR8 *cardver = ATI_romrevision(&Displaydevice[1]);
      aml_add_string(pack, "ATY,Card#");
      aml_add_string_buffer(pack, cardver);       
      aml_add_string(pack, "ATY,Copyright");
      aml_add_string_buffer(pack, "Copyright AMD Inc. All Rights Reserved. 2005-2011");    
      aml_add_string(pack, "ATY,EFIVersion");
      aml_add_string_buffer(pack, "01.00.3180");  
      aml_add_string(pack, "model");
      aml_add_string_buffer(pack, modelname);
      aml_add_string(pack, "name");
      name = AllocateZeroPool(sizeof(cfgname)+11);
      AsciiSPrint(name, sizeof(cfgname)+11, "ATY,%aParent", cfgname);
      aml_add_string_buffer(pack, name);  
      aml_add_string(pack, "ATY,VendorID");
      aml_add_byte_buffer(pack, (CHAR8*)&DisplayVendor[1], 4); 
      aml_add_string(pack, "ATY,DeviceID");
      aml_add_byte_buffer(pack, (CHAR8*)&DisplayID[1], 4); 
      //     aml_add_string(pack, "device-id");
      //     CHAR8 data[] = {0xE1,0x68,0x00,0x00};
      //     aml_add_byte_buffer(pack, data, sizeof(data));
      aml_add_string(pack, "org-device-id");
      aml_add_byte_buffer(pack, (CHAR8*)&DisplayID[1], 4); 
      aml_add_string(pack, "hda-gfx");
      aml_add_string_buffer(pack, "onboard-1");
      UINT32 vedioram = ATI_vram_size(&Displaydevice[1]); 
      aml_add_string(pack, "VRAM,totalsize");
      aml_add_dword(pack, vedioram); 
      if (!Display1PCIE)
      {
        aml_add_string(pack, "IOPCIExpressLinkCapabilities");
        aml_add_dword(pack, 0x130d1) ;//0x1e80); 
        aml_add_string(pack, "IOPCIExpressLinkStatus");
        aml_add_dword(pack, 0x10880); //0x880); 
      }
      aml_add_local0(met);
      aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
      // finish Method(_DSM,4,NotSerialized)
    }
    
    // HDAU
    if (GFXHDAFIX)
    {
      AML_CHUNK* device = aml_add_device(root, "HDAU");
      aml_add_name(device, "_ADR");
      aml_add_byte(device, 0x01);
      // add Method(_DSM,4,NotSerialized) for GFX0
      AML_CHUNK* met = aml_add_method(device, "_DSM", 4);
      met = aml_add_store(met);
      AML_CHUNK* pack = aml_add_package(met);
      //aml_add_string(pack, "device-id");
      //CHAR8 data[] = {0x38,0xAA,0x00,0x00};
      //aml_add_byte_buffer(pack, data, sizeof(data));
      //aml_add_string(pack, "codec-id");
      //aml_add_byte_buffer(pack, (CHAR8*)&GfxcodecId, 4);
      aml_add_string(pack, "layout-id");
      aml_add_byte_buffer(pack, (CHAR8*)&GfxlayoutId, 4);
      aml_add_string(pack, "hda-gfx");
      aml_add_string_buffer(pack, "onboard-2");
      //aml_add_string(pack, "name");
      //aml_add_string(pack, "pci1002,aa38");
      //aml_add_string(pack, "IOName");
      //aml_add_string(pack, "pci1002,aa38");
      //aml_add_string(pack, "layout-id");
      //CHAR8 data1[] = {0x12,0x00,0x00,0x00};
      //aml_add_byte_buffer(pack, data1, sizeof(data1));
      aml_add_string(pack, "PinConfigurations");
      aml_add_byte_buffer(pack, data2, sizeof(data2));
      aml_add_local0(met);
      aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
      // finish Method(_DSM,4,NotSerialized)
    }
  }
  
  aml_calculate_size(root);
  
  CHAR8 *display = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  //DBG("Display code size = 0x%08x\n", sizeoffset);
  
  aml_write_node(root, display, 0);
  
  aml_destroy_node(root);
  
  if (DisplayADR[1])
  {
    UINT32 adr=0, adr1=0;
    // get Display device size
    for (i=0; i<10; i++)
    {
      if (dsdt[DisplayADR[1]-i] == 0x82 && dsdt[DisplayADR[1]-i-1] == 0x5B)
      {
        adr1 = DisplayADR[1]-i+1;
        adr = get_size(dsdt, adr1);
        break;
      }
    }
    // move data to back for add Display 
    if (!DISPLAYFIX)
    { 
      len = move_data(adr1+adr, dsdt, len, sizeoffset);
      CopyMem(dsdt+adr1+adr, display, sizeoffset);
    }
    else
    {
      len = move_data(devadr1+devadr, dsdt, len, sizeoffset);
      CopyMem(dsdt+devadr1+devadr, display, sizeoffset);
      len = write_size(devadr1, dsdt, len, devadr);
    }
    // Fix Device Display size
    len = write_size(adr1, dsdt, len, adr);
    CorrectOuters(dsdt, len, adr1-3);
/*    
    // Fix PCIX size
    len = write_size(PCIADR, dsdt, len, sizeoffset, PCISIZE);
    PCISIZE += sizeoffset;
    // Fix _SB_ size
    len = write_size(SBADR, dsdt, len, sizeoffset, SBSIZE);
    SBSIZE += sizeoffset;
 */
  }
  else
  {
    len = move_data(PCIADR+PCISIZE, dsdt, len, sizeoffset);
    CopyMem(dsdt+PCIADR+PCISIZE, display, sizeoffset);
    CorrectOuters(dsdt, len, PCIADR-3);
/*    
    // Fix PCIX size
    len = write_size(PCIADR, dsdt, len, sizeoffset, PCISIZE);
    PCISIZE += sizeoffset;
    // Fix _SB_ size
    len = write_size(SBADR, dsdt, len, sizeoffset, SBSIZE);
    SBSIZE += sizeoffset;
 */
  }
  
  if (DisplayADR[1])
  {
    FixAddr(DisplayADR[1], sizeoffset); 
  }
  if (CFGname) {
    FreePool(CFGname);
  }
  if (name) {
    FreePool(name);
  } 
  FreePool(display);
  
  return len;  
}


UINT32 FIXNetwork (UINT8 *dsdt, UINT32 len)
{
  //  UINT32 i;
  AML_CHUNK* met;
  AML_CHUNK* root = aml_create_node(NULL);
  
  DBG("Start NetWork Fix\n");
  
  DBG("NetworkADR1=%x NetworkADR2=%x NetworkName=%x\n", NetworkADR1, NetworkADR2, NetworkName);
	//DBG("len = 0x%08x\n", len);
  if (!NetworkName) //there is no network device at dsdt, creating new one
  {
    //      AML_CHUNK* net = aml_add_device(root, "ETH1");
    //      aml_add_name(net, "_ADR");
    //      aml_add_dword(net, NetworkADR1);
    AML_CHUNK* dev = aml_add_device(root, "GIGE");
    aml_add_name(dev, "_ADR");
    if (NetworkADR2) 
    {
      if (NetworkADR2> 0x3F)
        aml_add_dword(dev, NetworkADR2);
      else
        aml_add_byte(dev, NetworkADR2);
    }
    else
    {
      aml_add_byte(dev, 0x00);
    }
    met = aml_add_method(dev, "_DSM", 4);
  } else {
    met = aml_add_method(root, "_DSM", 4);
  }  
  // add Method(_DSM,4,NotSerialized) for network
  met = aml_add_store(met);
  AML_CHUNK* pack = aml_add_package(met);
  aml_add_string(pack, "built-in");
  aml_add_byte_buffer(pack, data, sizeof(data));
  aml_add_string(pack, "model");
  aml_add_string_buffer(pack, Netmodel);
  aml_add_string(pack, "device_type");
  aml_add_string_buffer(pack, "Ethernet");
  aml_add_local0(met);
  aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
  // finish Method(_DSM,4,NotSerialized)
  
  aml_calculate_size(root);
  
  CHAR8 *network = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  //DBG("Network code size = 0x%08x\n", sizeoffset);
  
  aml_write_node(root, network, 0);
  
  aml_destroy_node(root);
  DBG("NetworkADR=%x add size=%x\n", NetworkADR, sizeoffset);
  if (NetworkADR)
  {
    UINT32 adr=0, adr1=0;
    // get Network device size
    adr1 = devFind(dsdt, NetworkADR);
    adr = get_size(dsdt, adr1);
    DBG("found Network device @%x size=5x\n", adr1, adr);
    // move data to back for add network 
    len = move_data(adr1+adr, dsdt, len, sizeoffset);
    CopyMem(dsdt+adr1+adr, network, sizeoffset);
    // Fix Device network size
    len = write_size(adr1, dsdt, len, adr);
    CorrectOuters(dsdt, len, adr1-3);
  }
  else
  {
    len = move_data(PCIADR+PCISIZE, dsdt, len, sizeoffset);
    CopyMem(dsdt+PCIADR+PCISIZE, network, sizeoffset);
    CorrectOuters(dsdt, len, PCIADR-3);
    NetworkADR = PCIADR + PCISIZE + 2;
  }
  
  if (NetworkADR) 
  {
    FixAddr(NetworkADR, sizeoffset); 
  }
  //DBG("len = 0x%08x\n", len);
  FreePool(network);
  return len;
}

UINT32 FIXAirport (UINT8 *dsdt, UINT32 len)
{
  AML_CHUNK* met;
  AML_CHUNK* root = aml_create_node(NULL);
  
  DBG("Start Airport Fix\n");
  if (!ArptADR1) return len;
    if (!ArptName) {//there is no Airport device at dsdt, creating new one
    // but we propose that bridge exists
   //   AML_CHUNK* net = aml_add_device(root, "ETH1");
   //   aml_add_name(net, "_ADR");
   //   aml_add_dword(net, NetworkADR1);
      AML_CHUNK* dev = aml_add_device(root, "ARPT");
      aml_add_name(dev, "_ADR");
      if (ArptADR2) 
      {
        if (ArptADR2> 0x3F)
          aml_add_dword(dev, ArptADR2);
        else
          aml_add_byte(dev, ArptADR2);
      }
      else
      {
        aml_add_byte(dev, 0x00);
      }
      met = aml_add_method(dev, "_DSM", 4);
    } else {
      met = aml_add_method(root, "_DSM", 4);
    }  
  // add Method(_DSM,4,NotSerialized) for network
      met = aml_add_store(met);
      AML_CHUNK* pack = aml_add_package(met);
      aml_add_string(pack, "built-in");

      aml_add_byte_buffer(pack, data, sizeof(data));
  if (ArptBCM) {
    aml_add_string(pack, "model");
    aml_add_string_buffer(pack, "Dell Wireless 1395");
    aml_add_string(pack, "name");
    aml_add_string_buffer(pack, "pci14e4,4312");
    aml_add_string(pack, "device-id");
    CHAR8 data[] = {0x12, 0x43, 0x00, 0x00};
    aml_add_byte_buffer(pack, data, 4);    
  }
  aml_add_string(pack, "device_type");
  aml_add_string_buffer(pack, "Airport");

  aml_add_local0(met);
      aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
      // finish Method(_DSM,4,NotSerialized)
   
  aml_calculate_size(root);
  
  CHAR8 *network = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  aml_write_node(root, network, 0);
  
  aml_destroy_node(root);
  DBG("NetworkADR=%x add size=%x\n", ArptADR, sizeoffset);
  if (ArptADR)
  {
    UINT32 adr=0, adr1=0;
    // get Airport device size
    adr1 = devFind(dsdt, ArptADR);
    adr = get_size(dsdt, adr1);
    DBG("found Airport device @%x size=5x\n", adr1, adr);
    // move data to back for add network 
    len = move_data(adr1+adr, dsdt, len, sizeoffset);
    CopyMem(dsdt+adr1+adr, network, sizeoffset);
    // Fix Device network size
    len = write_size(adr1, dsdt, len, adr);
    CorrectOuters(dsdt, len, adr1-3);
  }
  else
  {
    len = move_data(PCIADR+PCISIZE, dsdt, len, sizeoffset);
    CopyMem(dsdt+PCIADR+PCISIZE, network, sizeoffset);
    CorrectOuters(dsdt, len, PCIADR-3);
    ArptADR = PCIADR + PCISIZE + 2;
  }
  
  if (ArptADR) 
  {
    FixAddr(ArptADR, sizeoffset); 
  }
  //DBG("len = 0x%08x\n", len);
  FreePool(network);
  return len;
}

UINT32 FIXSBUS (UINT8 *dsdt, UINT32 len)
{
//  UINT32 i;
  
  DBG("Start SBUS Fix\n");
	//DBG("len = 0x%08x\n", len);
  
  PCISIZE = get_size(dsdt, PCIADR);
  if (!PCISIZE) {
    DBG("wrong PCI0 address, patch SBUS will not be applyed\n");
    return len;
  }
  
  if (SBUSADR)
    sizeoffset = sizeof(bus0);
  else
    sizeoffset = sizeof(sbus); 
  
  DBG("SBUS code size = 0x%08x\n", sizeoffset);
    
  if (SBUSADR)
  {
    UINT32 adr=0, adr1=0;
    // get SBUS device size
    adr1 = devFind(dsdt, SBUSADR);
    adr = get_size(dsdt, adr1);
    // move data to back for add sbus 
    len = move_data(adr1+adr, dsdt, len, sizeoffset);
    CopyMem(dsdt+adr1+adr, bus0, sizeoffset);
    // Fix Device sbus size
    len = write_size(adr1, dsdt, len, adr);
    CorrectOuters(dsdt, len, adr1-3);
    SBUSADR = adr1;
    DBG("SBUS code size fix = 0x%08x\n", sizeoffset);
  }
  else
  {
    DBG("SBUS absent, adding to the end of PCI0 at %x\n", PCIADR+PCISIZE);
    len = move_data(PCIADR+PCISIZE, dsdt, len, sizeoffset);
    CopyMem(dsdt+PCIADR+PCISIZE, sbus, sizeoffset);
    
    // Fix PCIX size
    len = write_size(PCIADR, dsdt, len, PCISIZE);
    SBUSADR = PCIADR+PCISIZE+2;
    PCISIZE += sizeoffset;
    CorrectOuters(dsdt, len, PCIADR-3);    
  }
  
  if (SBUSADR) 
  {
    FixAddr(SBUSADR, sizeoffset); 
  }
  
  return len;
  
}

UINT32 AddMCHC (UINT8 *dsdt, UINT32 len)
{    
  AML_CHUNK* root = aml_create_node(NULL);
  
  DBG("Start Add MCHC\n");
	//DBG("len = 0x%08x\n", len);
  PCISIZE = get_size(dsdt, PCIADR);
  if (!PCISIZE) {
    DBG("wrong PCI0 address, patch MCHC will not be applyed\n");
    return len;
  }
  	
  AML_CHUNK* device = aml_add_device(root, "MCHC");
  aml_add_name(device, "_ADR");
  aml_add_byte(device, 0x00); 
  
	// add Method(_DSM,4,NotSerialized) for MCHC
  AML_CHUNK* met = aml_add_method(device, "_DSM", 4);
  met = aml_add_store(met);
  AML_CHUNK* pack = aml_add_package(met);
  aml_add_string(pack, "device-id");
  CHAR8 data[] = {0x44,0x00,0x00,0x00};
  aml_add_byte_buffer(pack, data, sizeof(data));
  aml_add_string(pack, "name");
  aml_add_string(pack, "pci8086,44");
  aml_add_string(pack, "IOName");
  aml_add_string(pack, "pci8086,44");
  aml_add_local0(met);
  aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
  // finish Method(_DSM,4,NotSerialized) 
  
  aml_calculate_size(root);
  
  CHAR8 *mchc = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  //DBG("MCHC code size = 0x%08x\n", sizeoffset);
  
  aml_write_node(root, mchc, 0);
  
  aml_destroy_node(root);
  
  // always add on PCIX back
  len = move_data(PCIADR+PCISIZE, dsdt, len, sizeoffset);
  CopyMem(dsdt+PCIADR+PCISIZE, mchc, sizeoffset);
  
  // Fix PCIX size
  len = write_size(PCIADR, dsdt, len, PCISIZE);
	PCISIZE += sizeoffset;
  CorrectOuters(dsdt, len, PCIADR-3);
  FreePool(mchc);
  return len;
  
}

UINT32 FIXFirewire (UINT8 *dsdt, UINT32 len)
{
//  UINT32 i;
  AML_CHUNK* met;
  AML_CHUNK* root = aml_create_node(NULL);
  
  DBG("Start Firewire Fix\n");
	//DBG("len = 0x%08x\n", len);
  PCISIZE = get_size(dsdt, PCIADR);
  if (!PCISIZE) {
    DBG("wrong PCI0 address, patch FRWR will not be applyed\n");
    return len;
  }
  
  if (!FirewireName) {
    AML_CHUNK* device = aml_add_device(root, "FRWR");
    aml_add_name(device, "_ADR");
    if (FirewireADR2) {
      if (FirewireADR2 <= 0x3F) {
        aml_add_byte(device, FirewireADR2);
      } else {
        aml_add_dword(device, FirewireADR2);
      }
    } else aml_add_byte(device, 0);
    aml_add_name(device, "_GPE");
    aml_add_byte(device, 0x1A);
    met = aml_add_method(device, "_DSM", 4);
  } else
    met = aml_add_method(root, "_DSM", 4);
  AML_CHUNK* stro = aml_add_store(met);
  AML_CHUNK* pack = aml_add_package(stro);
  aml_add_string(pack, "fwhub");
  CHAR8 data[] = {0x00,0x00,0x00,0x00};
  aml_add_byte_buffer(pack, data, sizeof(data));
  aml_add_local0(stro);
  aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
  // finish Method(_DSM,4,NotSerialized)  
  
  aml_calculate_size(root);
  
  CHAR8 *firewire = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  aml_write_node(root, firewire, 0);
  
  aml_destroy_node(root);
  
  if (FirewireADR)
  {
    UINT32 adr=0, adr1=0;
    // get Network device size
    adr1 = devFind(dsdt, FirewireADR);
    adr = get_size(dsdt, adr1);
    // move data to back for add network 
    len = move_data(adr1+adr, dsdt, len, sizeoffset);
    CopyMem(dsdt+adr1+adr, firewire, sizeoffset);
    // Fix Device network size
    len = write_size(adr1, dsdt, len, adr);
    CorrectOuters(dsdt, len, adr1-3);
    FirewireADR = adr1;
  }
  else
  {
    len = move_data(PCIADR+PCISIZE, dsdt, len, sizeoffset);
    CopyMem(dsdt+PCIADR+PCISIZE, firewire, sizeoffset);
    // Fix PCIX size
    len = write_size(PCIADR, dsdt, len, PCISIZE);
    PCISIZE += sizeoffset;
    CorrectOuters(dsdt, len, PCIADR-3);
    FirewireADR = PCIADR+PCISIZE+2; 
  }
  //DBG("len = 0x%08x\n", len);
  if (FirewireADR) {
    FixAddr(FirewireADR, sizeoffset);
  }
  FreePool(firewire);
  return len;
}

UINT32 AddHDEF (UINT8 *dsdt, UINT32 len)
{   
  len = DeleteDevice("AZAL", dsdt, len);
  ReplaceName(dsdt, len, "AZAL", "HDEF");
  
  AML_CHUNK* root = aml_create_node(NULL);
  
  DBG("Start Add Device HDEF\n");
	//DBG("len = 0x%08x\n", len);
	
  AML_CHUNK* device = aml_add_device(root, "HDEF");
  aml_add_name(device, "_ADR");
  aml_add_dword(device, 0x001B0000);  
  
	// add Method(_DSM,4,NotSerialized) 
  AML_CHUNK* met = aml_add_method(device, "_DSM", 4);
  met = aml_add_store(met);
  AML_CHUNK* pack = aml_add_package(met);
  //aml_add_string(pack, "codec-id");
  //aml_add_byte_buffer(pack, (CHAR8*)&HDAcodecId, 4);
  aml_add_string(pack, "layout-id");
  aml_add_byte_buffer(pack, (CHAR8*)&HDAlayoutId, 4);
  aml_add_string(pack, "PinConfigurations");
  CHAR8 data[] = {};
  aml_add_byte_buffer(pack, data, sizeof(data));
  aml_add_local0(met);
  aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
  // finish Method(_DSM,4,NotSerialized)
  
  aml_calculate_size(root);
  
  CHAR8 *hdef = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  //DBG("HDEF code size = 0x%08x\n", sizeoffset);
  
  aml_write_node(root, hdef, 0);
  
  aml_destroy_node(root);
  
  // always add on PCIX back
  len = move_data(PCIADR+PCISIZE, dsdt, len, sizeoffset);
  CopyMem(dsdt+PCIADR+PCISIZE, hdef, sizeoffset);
  // Fix PCIX size
  len = write_size(PCIADR, dsdt, len, PCISIZE);
	PCISIZE += sizeoffset;
  CorrectOuters(dsdt, len, PCIADR-3);
  FreePool(hdef);
  return len;
}

UINT32 FIXUSB (UINT8 *dsdt, UINT32 len)
{
  UINT32 i, j;
  UINT32 size1, size2;    
  UINT32 adr=0, adr1=0;
  
  DBG("Start USB Fix\n");
	//DBG("len = 0x%08x\n", len);
  
	AML_CHUNK* root = aml_create_node(NULL);
	AML_CHUNK* root1 = aml_create_node(NULL);
  
  // add Method(_DSM,4,NotSerialized) for USB
  AML_CHUNK* met = aml_add_method(root, "_DSM", 4);
  met = aml_add_store(met);
  AML_CHUNK* pack = aml_add_package(met);
  aml_add_string(pack, "device-id");
  aml_add_byte_buffer(pack, (CONST CHAR8*)&USBID[0], 4);
  aml_add_string(pack, "built-in");
  aml_add_byte_buffer(pack, data, sizeof(data));
  aml_add_string(pack, "device_type");
  aml_add_string_buffer(pack, "UHCI");
  aml_add_string(pack, "AAPL,clock-id");
  aml_add_byte_buffer(pack, data, sizeof(data));
  
  aml_add_local0(met);
  aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
  // finish Method(_DSM,4,NotSerialized)      
  aml_calculate_size(root);
  
  CHAR8 *USBDATA1 = AllocateZeroPool(root->Size);
  size1 = root->Size;
  //DBG("USB code size = 0x%08x\n", sizeoffset);
  aml_write_node(root, USBDATA1, 0);
  aml_destroy_node(root);
  
  // add Method(_DSM,4,NotSerialized) for USB
  AML_CHUNK* met1 = aml_add_method(root1, "_DSM", 4);
  met1 = aml_add_store(met1);
  AML_CHUNK* pack1 = aml_add_package(met1);
  aml_add_string(pack1, "device-id");
  aml_add_byte_buffer(pack1, (CONST CHAR8*)&USBID[0], 4);
  aml_add_string(pack1, "built-in");
  aml_add_byte_buffer(pack1, data, sizeof(data));
  aml_add_string(pack1, "device_type");
  aml_add_string_buffer(pack1, "EHCI");
  aml_add_string(pack1, "AAPL,clock-id");
  aml_add_byte_buffer(pack1, data, sizeof(data));
  aml_add_string(pack1, "AAPL,current-available");
  aml_add_word(pack1, 0x05DC);
  aml_add_string(pack1, "AAPL,current-extra");
  aml_add_word(pack1, 0x03E8);
  aml_add_string(pack1, "AAPL,current-in-sleep");
  aml_add_word(pack1, 0x0BB8);
//  aml_add_string(pack1, "AAPL,device-internal");
//  aml_add_byte(pack1, 0x02);

  aml_add_byte_buffer(pack1, data, sizeof(data));
  aml_add_local0(met1);
  aml_add_buffer(met1, dtgp_1, sizeof(dtgp_1));
  // finish Method(_DSM,4,NotSerialized)
  
  aml_calculate_size(root1);
  CHAR8 *USBDATA2 = AllocateZeroPool(root1->Size);
  size2 = root1->Size;
  //DBG("USB code size = 0x%08x\n", sizeoffset);
  aml_write_node(root1, USBDATA2, 0);
  aml_destroy_node(root1);
  
  if (usb > 0) 
  {
    for (i=0; i<usb; i++) {
      // find USB adr
      for (j=0; j<len-4; j++)
      {
        if (CmpAdr(dsdt, j, USBADR[i]))
        {
          UsbName[i] = AllocateZeroPool(5);
          CopyMem(UsbName[i], dsdt+j, 4);		          
          
          adr1 = devFind(dsdt, j);
          adr = get_size(dsdt, adr1);
          if (USB20[i])
          {
            CopyMem(USBDATA2+28, (VOID*)&USBID[i], 4);
            sizeoffset = size2;
          }
          else
          {
            CopyMem(USBDATA1+26, (VOID*)&USBID[i], 4);
            sizeoffset = size1;
          }
          
          len = move_data(adr1+adr, dsdt, len, sizeoffset);
          if (USB20[i])
          {
            CopyMem(dsdt+adr1+adr, USBDATA2, sizeoffset);
          }
          else
          {
            CopyMem(dsdt+adr1+adr, USBDATA1, sizeoffset);
          }
          // Fix Device USB size
          len = write_size(adr1, dsdt, len, adr);
          CorrectOuters(dsdt, len, adr1-3);
          break;
        }  
      }
    }
  }
  FreePool(USBDATA1);
  FreePool(USBDATA2);
  return len;
  
}

UINT32 FIXIDE (UINT8 *dsdt, UINT32 len)
{
  UINT32 j;
  BOOLEAN PATAFIX=TRUE;        
  
  DBG("Start IDE Fix\n");
	//DBG("len = 0x%08x\n", len);
	
  UINT32 adr=0, adr1=0;
  // get ide device size
  adr1 = devFind(dsdt, IDEADR);
  adr = get_size(dsdt, adr1);
  // find Name(_ADR, Zero) if yes, don't need to inject PATA name
  for (j=0 ; j<adr; j++)
  {
    if (CmpAdr(dsdt, adr1+j-5, 0))
    {
      PATAFIX = FALSE;
      break;
    }
  }  
  
	AML_CHUNK* root = aml_create_node(NULL);
	
	if (PATAFIX)
	{
    AML_CHUNK* device = aml_add_device(root, "ICHX");
    aml_add_name(device, "_ADR");
    if (IDEADR2 < 0x3F) {
      aml_add_byte(device, IDEADR2);
    } else {
      aml_add_dword(device, IDEADR2);
    }
    AML_CHUNK* met = aml_add_method(device, "_DSM", 4);
    met = aml_add_store(met);
    AML_CHUNK* pack = aml_add_package(met);
    aml_add_string(pack, "device-id");
    CHAR8 data[] = {0x9E,0x26,0x00,0x00};
    aml_add_byte_buffer(pack, data, sizeof(data));
    aml_add_string(pack, "vendor-id");
    CHAR8 data1[] = {0x86,0x80,0x00,0x00};
    aml_add_byte_buffer(pack, data1, sizeof(data1));
    aml_add_string(pack, "name");
    aml_add_string(pack, "pci8086,269e");
    aml_add_string(pack, "IOName");
    aml_add_string(pack, "pci8086,269e");
    aml_add_local0(met);
    aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
    AML_CHUNK* device1 = aml_add_device(device, "PRIM");
    aml_add_name(device1, "_ADR");
    aml_add_byte(device1, 0x00);
    AML_CHUNK* device2 = aml_add_device(device1, "MAST");
    aml_add_name(device2, "_ADR");
    aml_add_byte(device2, 0x00);
    device2 = aml_add_device(device1, "SLAV");
    aml_add_name(device2, "_ADR");
    aml_add_byte(device2, 0x01);
    // Marvell only one connected cable
    //AML_CHUNK* device3 = aml_add_device(device, "SLAB");
    //aml_add_name(device3, "_ADR");
    //aml_add_byte(device3, 0x00);
    //AML_CHUNK* device4 = aml_add_device(device3, "MAST");
    //aml_add_name(device4, "_ADR");
    //aml_add_byte(device4, 0x00);
    //device4 = aml_add_device(device3, "SLAV");
    //aml_add_name(device4, "_ADR");
    //aml_add_byte(device4, 0x01);
    
  }
  else
  {
    AML_CHUNK* met = aml_add_method(root, "_DSM", 4);
    met = aml_add_store(met);
    AML_CHUNK* pack = aml_add_package(met);
    aml_add_string(pack, "device-id");
    CHAR8 data[] = {0x9E,0x26,0x00,0x00};
    aml_add_byte_buffer(pack, data, sizeof(data));
    aml_add_string(pack, "vendor-id");
    CHAR8 data1[] = {0x86,0x80,0x00,0x00};
    aml_add_byte_buffer(pack, data1, sizeof(data1));
    aml_add_string(pack, "name");
    aml_add_string(pack, "pci8086,269e");
    aml_add_string(pack, "IOName");
    aml_add_string(pack, "pci8086,269e");
    aml_add_local0(met);
    aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
  }
  // finish Method(_DSM,4,NotSerialized) 
  
  aml_calculate_size(root);
  
  CHAR8 *ide = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  //DBG("IDE code size = 0x%08x\n", sizeoffset);
  
  aml_write_node(root, ide, 0);
  
  aml_destroy_node(root);
  
  if (IDEADR)
  {
    // move data to back for add DSM 
    len = move_data(adr1+adr, dsdt, len, sizeoffset);
    CopyMem(dsdt+adr1+adr, ide, sizeoffset);
    // Fix Device ide size
    len = write_size(adr1, dsdt, len, adr);
    CorrectOuters(dsdt, len, adr1-3);
  }
  
  if (IDEADR) 
  {
    FixAddr(IDEADR, sizeoffset);
  }
  FreePool(ide);
  return len;
}

UINT32 FIXSATAAHCI (UINT8 *dsdt, UINT32 len)
{
//  UINT32 i;
    
  DBG("Start SATA AHCI Fix\n");
  
	AML_CHUNK* root = aml_create_node(NULL);
	
	// add Method(_DSM,4,NotSerialized) 
  AML_CHUNK* met = aml_add_method(root, "_DSM", 4);
  met = aml_add_store(met);
  AML_CHUNK* pack = aml_add_package(met);
  aml_add_string(pack, "device-id");
  CHAR8 data[] = {0x81, 0x26, 0x00, 0x00};
  aml_add_byte_buffer(pack, data, 4);
  aml_add_string(pack, "vendor-id");
  CHAR8 data1[] = {0x86, 0x80, 0x00, 0x00};
  aml_add_byte_buffer(pack, data1, 4);
  aml_add_local0(met);
  aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
  // finish Method(_DSM,4,NotSerialized)
  
  aml_calculate_size(root);
  
  CHAR8 *sata = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  //DBG("IDE code size = 0x%08x\n", sizeoffset);
  
  aml_write_node(root, sata, 0);
  
  aml_destroy_node(root);
  
  if (SATAAHCIADR)
  {
    UINT32 adr=0, adr1=0;
    // get ide device size
    adr1 = devFind(dsdt, SATAAHCIADR+2);
    adr = get_size(dsdt, adr1);
    // move data to back for add DSM
    len = move_data(adr1+adr, dsdt, len, sizeoffset);
    CopyMem(dsdt+adr1+adr, sata, sizeoffset);
    // Fix Device SATA size
    len = write_size(adr1, dsdt, len, adr);
    CorrectOuters(dsdt, len, adr1-3);
  }
  
  if (SATAAHCIADR) 
  {
    FixAddr(SATAAHCIADR, sizeoffset);
  }
  FreePool(sata);
  return len;
}

UINT32 FIXSATA (UINT8 *dsdt, UINT32 len)
{
//  UINT32 i;
  
  
  DBG("Start SATA Fix\n");
	//DBG("len = 0x%08x\n", len);
  
	AML_CHUNK* root = aml_create_node(NULL);
	
	// add Method(_DSM,4,NotSerialized) 
  AML_CHUNK* met = aml_add_method(root, "_DSM", 4);
  met = aml_add_store(met);
  AML_CHUNK* pack = aml_add_package(met);
  aml_add_string(pack, "device-id");
  CHAR8 data[] = {0x80, 0x26, 0x00, 0x00};
  aml_add_byte_buffer(pack, data, 4);
  aml_add_string(pack, "vendor-id");
  CHAR8 data1[] = {0x86, 0x80, 0x00, 0x00};
  aml_add_byte_buffer(pack, data1, 4);
  aml_add_local0(met);
  aml_add_buffer(met, dtgp_1, sizeof(dtgp_1));
  // finish Method(_DSM,4,NotSerialized)
  
  aml_calculate_size(root);
  
  CHAR8 *sata = AllocateZeroPool(root->Size);
  
  sizeoffset = root->Size;
  
  //DBG("IDE code size = 0x%08x\n", sizeoffset);
  
  aml_write_node(root, sata, 0);
  
  aml_destroy_node(root);
  
  if (SATAADR)
  {
    UINT32 adr=0, adr1=0;
    // get ide device size
    adr1 = devFind(dsdt, SATAADR);
    adr = get_size(dsdt, adr1);
    // move data to back for add DSM
    len = move_data(adr1+adr, dsdt, len, sizeoffset);
    CopyMem(dsdt+adr1+adr, sata, sizeoffset);
    // Fix Device SATA size
    len = write_size(adr1, dsdt, len, adr);
    CorrectOuters(dsdt, len, adr1-3);
  }
  
  if (SATAADR) 
  {
     FixAddr(SATAADR, sizeoffset);
  }
  FreePool(sata);
  return len;
}


UINT32 FIXCPU1 (UINT8 *dsdt, UINT32 len)
{    
  UINT32 i, j;
  UINT32 count=0;
  UINT32 pradr=0;
  UINT32 prsize=0, size=0;
  UINT32 prsize1=0;
  UINT32 offset;
  
  DBG("Start CPUS=1 Fix\n");
	//DBG("len = 0x%08x\n", len);
  
  // find _PR_ and get PR size
  for (i=0; i<len-4; i++)
  {
    if (dsdt[i] == '_' && dsdt[i+1] == 'P' && dsdt[i+2] == 'R' && dsdt[i+3] == '_')
    {
      //DBG("Found _PR_\n");
      for (j=0; j<10; j++)
      {
        if (dsdt[i-j] == 0x10)
        {
          prsize = get_size(dsdt, i-j+1);
          if(!prsize) continue;
          prsize1 = prsize;
          pradr = i-j+1;
          // size > 0x3F there should be had P_states code so don't fix
          if (prsize > 0x3F) return len;
          //DBG("_PR_ adr = 0x%08x size = 0x%08x\n", pradr, prsize);
          break;
        }
      }
      break;
    }
  }
  
  sizeoffset = 9;
  
  // find alias
  for (i=pradr; i<prsize1; i++)
  {
    if (dsdt[i] == 0x5B && dsdt[i+1] == 0x83)
    {
      size = get_size(dsdt, i+2);
      //DBG("OP size = 0x%08x\n", size);
      // if OP name not CPUX.... need add alias in OP back 
      offset = i + 3 + (dsdt[i+2] >> 6); 
      if (dsdt[offset] == '\\') offset = i + 8 + (dsdt[i+7] >> 6); ; 
      if (dsdt[i+2+size] != 0x06 && dsdt[offset] != 'C' && dsdt[offset+1] != 'P' && dsdt[offset+2] != 'U')
      {
        //DBG("Found alias CPU.\n");
        len = move_data(i+2+size, dsdt, len, sizeoffset);
        dsdt[i+2+size] = 0x06;
        dsdt[i+3+size] = dsdt[i+2];
        dsdt[i+4+size] = dsdt[i+3];
        dsdt[i+5+size] = dsdt[i+4];
        dsdt[i+6+size] = dsdt[i+5];
        dsdt[i+7+size] = 'C';
        dsdt[i+8+size] = 'P';
        dsdt[i+9+size] = 'U';
        dsdt[i+9+size] = dsdt[i+5];
        len = write_size(pradr, dsdt, len, prsize);
        count++;
        continue;
      }
    }
  }
  
  if (count)
  {
    if (PCIADR > pradr) PCIADR += (sizeoffset*count);
    //if (LPCBADR > pradr) LPCBADR += (sizeoffset*count);
    if (SBADR > pradr) SBADR += (sizeoffset*count);
  }
  
  return len;             
}

UINT32 FIXWAK (UINT8 *dsdt, UINT32 len)
{    
  UINT32 i, j, k;
  UINT32 wakadr=0;
  UINT32 waksize=0;
  
  DBG("Start _WAK Return Fix\n");
	//DBG("len = 0x%08x\n", len);
	
	sizeoffset = 0;
  
	for (i=20; i<len-5; i++) 
	{ 	
		if(dsdt[i] == '_' && dsdt[i+1] == 'W' && dsdt[i+2] == 'A' && dsdt[i+3] == 'K')
		{
      for (j=0; j<10; j++) 
      {
        if(dsdt[i-j] == 0x14) 
        {
          waksize = get_size(dsdt, i-j+1);
          wakadr = i-j+1;
          //DBG( "_WAK adr = 0x%08x, size = 0x%08x\n", wakadr, waksize);
          for (k=0; k<waksize; k++) 
          {
            if (dsdt[i+k] == 0xA4) 
            {
              //DBG( "_WAK Method find return data, don't need to patch.\n");
              return len;
            }
          }
          //DBG( "_WAK Method need return data, will patch it.\n");
          sizeoffset = sizeof(wakret);
          len = move_data(wakadr+waksize, dsdt, len, sizeoffset);
          CopyMem(dsdt+wakadr+waksize, wakret, sizeoffset);
          len = write_size(wakadr, dsdt, len, waksize);
          CorrectOuters(dsdt, len, wakadr-2);
          break;
        }
      }
      break;    
		}
	}	    
  
  if (sizeoffset)
  {
    if (PCIADR > wakadr) PCIADR += sizeoffset;
    if (SBADR > wakadr) SBADR += sizeoffset;
  }
  //DBG("len = 0x%08x\n", len);
  
  return len;             
}

UINT32 FIXGPE (UINT8 *dsdt, UINT32 len)
{    
  UINT32 i, j, k, l, m, n;
  UINT32 gpeadr=0;
  UINT32 gpesize=0;
  UINT32 pwrbadr=0;
  UINT32 pwrbsize=0;
  UINT32 usbcount=0;
  UINT32 adr=0;
  INT32  offset=0;
  sizeoffset = sizeof(pwrb);
  BOOLEAN pwrbfix = FALSE;
  BOOLEAN usbpwrb = FALSE;
  BOOLEAN foundpwrb = FALSE;
  
  // if NB maybe not had device PWRB
  for (i=0; i<len-10; i++)
  {
    if (dsdt[i] == 'P' && dsdt[i+1] == 'W' && dsdt[i+2] == 'R' && dsdt[i+3] == 'B' &&
        (dsdt[i-2] == 0x82 || dsdt[i-3] == 0x82 || dsdt[i-4] == 0x82))
    {
      foundpwrb = TRUE;
      break;    
    }
  }
  
  if (!foundpwrb)
  {
    DBG("No found PWRB device, don't fix _GPE\n");
    return len;
  }
  
  DBG("Start _GPE device remove error Fix\n");
	//DBG("len = 0x%08x\n", len);
  
	for (i=0; i<len-10; i++) 
	{ 	
		if(dsdt[i] == '_' && dsdt[i+1] == 'L' && (dsdt[i-2] == 0x14 || dsdt[i-3] == 0x14 || dsdt[i-4] == 0x14 || dsdt[i-5] == 0x14))
		{
      for (j=0; j<10; j++) 
      {
        if(dsdt[i-j] == 0x14) 
        {
          pwrbsize = get_size(dsdt, i-j+1);
          pwrbadr = i-j+1;
          //DBG( "_LXX adr = 0x%08x, size = 0x%08x\n", pwrbadr, pwrbsize);
          for (k=pwrbadr; k<pwrbadr+pwrbsize; k++) 
          {
            for (l=0; l<usb; l++)
            {   // find USB _LXX
              if (dsdt[k] == UsbName[l][0] && dsdt[k+1] == UsbName[l][1] && 
                  dsdt[k+2] == UsbName[l][2] && dsdt[k+3] == UsbName[l][3]) 
              {
                //DBG( "found USB _GPE Method.\n");
                usbpwrb = TRUE;
                
                if (!usbcount)
                {
                  //DBG( "will to find Scope(\\_GPE).\n");
                  for (m=0; m<300; m++)
                  {
                    if (dsdt[i-m] == 'E' && dsdt[i-m-1] == 'P' && dsdt[i-m-2] == 'G' && dsdt[i-m-3] == '_')
                    {
                      for (n=0; n<15; n++)
                      {
                        if (dsdt[i-m-n] == 0x10)
                        {
                          gpeadr = i-m-n+1;
                          gpesize = get_size(dsdt, i-m-n+1);
                          //DBG( "_GPE adr = 0x%08x, size = 0x%08x\n", gpeadr, gpesize);
                          break;
                        }
                      }
                      break; 
                    }
                  }
                }
                break;
              }
            }
            if (usbpwrb)
            {   // check if there had PWRB             
              for (m=pwrbadr; m<pwrbadr+pwrbsize; m++)
              {
                if (dsdt[m] == 'P' && dsdt[m+1] == 'W' && dsdt[m+2] == 'R' && dsdt[m+3] == 'B')
                {
                  //DBG( "found PWRB don't patch.\n");
                  pwrbfix = TRUE;
                  break;
                }
              }
            }
            if (pwrbfix)
              break;
            else if (usbpwrb)
            {
              if (pwrbadr == adr) break;  // same Method
              len = move_data(pwrbadr+pwrbsize, dsdt, len, sizeoffset);
              CopyMem(dsdt+pwrbadr+pwrbsize, pwrb, sizeoffset);
              len = write_size(pwrbadr, dsdt, len, pwrbsize);
              offset += sizeoffset;
              usbpwrb = FALSE;
              usbcount++;
              adr = pwrbadr;
              break;
            }
          }
        }
      }
		}
	}	
	
	if (usbcount)
	{
    sizeoffset = offset;
    len = write_size(gpeadr, dsdt, len, gpesize);
    
    if (PCIADR < gpeadr && PCIADR+PCISIZE > gpeadr)
    {
      len = write_size(PCIADR, dsdt, len, PCISIZE);
      PCISIZE += sizeoffset;
    }
    else if (PCIADR > gpeadr)
    {
      PCIADR += sizeoffset;
    }
    
    if (SBADR < gpeadr && SBADR+SBSIZE > gpeadr)
    {
      len = write_size(SBADR, dsdt, len, SBSIZE);
      SBSIZE += offset;    
    }
    else if (SBADR > gpeadr)
    {
      SBADR += sizeoffset;
    }
    
  }
  
  return len;             
}

//not corrected
UINT32 FIXPWRB (UINT8* dsdt, INTN len)
{
  UINT32 i, j;
  UINT32 adr=0, size;
  for (i=0; i<len-10; i++)
  {
    if (dsdt[i] == 'P' && dsdt[i+1] == 'W' && dsdt[i+2] == 'R' && dsdt[i+3] == 'B' &&
        (dsdt[i-2] == 0x82 || dsdt[i-3] == 0x82 || dsdt[i-4] == 0x82))
    {
      for (j=0; j<10; j++)
      {
        if (dsdt[i-j] == 0x82 && dsdt[i-j-1] == 0x5B)
        {
          size = get_size(dsdt, i-j+1);
          adr = i-j+1;
          sizeoffset = sizeof(pwrbcid);
          len = move_data(adr+size, dsdt, len, sizeoffset);
          CopyMem(dsdt+adr+size, pwrbcid, sizeoffset);
          len = write_size(adr, dsdt, len, size);
          if (PCIADR < adr && PCIADR+PCISIZE > adr)
          {
            len = write_size(PCIADR, dsdt, len, PCISIZE);
            PCISIZE += sizeoffset;
          }
          if (SBADR < adr && SBADR+SBSIZE > adr)
          {
            len = write_size(SBADR, dsdt, len, SBSIZE);
            SBSIZE += sizeoffset;
          }
          break;
        }
      }
      break;    
    }
  }
  
  return len;  
}

UINT32 FIXSHUTDOWN_ASUS (UINT8 *dsdt, INTN len)
{
	INTN i, j;
	UINT32 adr, adr1, adr2, size;
	
	DBG("Start SHUTDOWN Fix\n");
	// Fix USB _PRW value for 0x0X, 0x04 ==> 0x0X, 0x01
	for (i=0; i<len-5; i++) 
	{ 
    if (dsdt[i] == '_' && dsdt[i+1] == 'P' && dsdt[i+2] == 'T' &&
        dsdt[i+3] == 'S' )
    {
      //DBG("found _PTS\n");
      for (j=0; j<10; j++) {
        if (dsdt[i-j] == 0x14)  //Method
        {
          sizeoffset = sizeof(shutdown);
          size = get_size(dsdt, i-j+1); //size of Method
          adr = i-j+1;
          adr1 = i+5;
          adr2 = i+5+sizeoffset-1;  //jump adr
          len = move_data(adr1, dsdt, len, sizeoffset);
          CopyMem(dsdt+adr1, shutdown, sizeoffset);  //insert shutdown
          len = write_offset(adr2, dsdt, len, size-4-j);
          len = write_size(adr, dsdt, len, size);
          break;
        }
      }
      break;
    }
	}
  
  return len;  
}

//Slice - this procedure was not corrected and mostly wrong
UINT32 FIXOTHER (UINT8 *dsdt, INTN len)
{
	INTN i, j, k, m, offset, l;
	UINT32 size;
	
	// Fix USB _PRW value for 0x0X, 0x04 ==> 0x0X, 0x01
	for(j=0; j<usb; j++) {
    for (i=0; i<len-5; i++) { 
      if (CmpAdr(dsdt, i, USBADR[j]))
      {
          // get USB name
          UsbName[j] = AllocateZeroPool(5);
          CopyMem(UsbName[j], dsdt+i, 4);
          DBG("found USB device NAME(_ADR,0x%08x) And Name is %a\n", 
              USBADR[j], UsbName[j]);
          
          for (k=i+1; k<i+200; k++) 
          {
            if (dsdt[k] == 0x14 && dsdt[k+2] == '_' && dsdt[k+3] == 'P' && dsdt[k+4] == 'R' && dsdt[k+5] == 'W') 
            {
              offset = k;
              m = dsdt[k+1];
              if (dsdt[offset+m] != 0x03) 
              {
                if (dsdt[offset+m] == 0x01)
                  dsdt[offset+m] = 0x03;
		            
                if (dsdt[offset+m] == 0x04)
                  dsdt[offset+m] = 0x01;
                
                //DBG("found USB Method(_PRW) and will patch fix\n");
              }
              break;
            }
          for (k=i+1; k<i+200; k++) 
          {
            if (dsdt[k] == 0x14 && dsdt[k+2] == '_' && dsdt[k+3] == 'S' && dsdt[k+4] == '3' && dsdt[k+5] == 'D') 
            {
              size = dsdt[k+1];
              for (l=0; l<size; l++)
              {
                if (dsdt[k+1+l] == 0xA4 && dsdt[k+2+l] == 0x0A && dsdt[k+3+l] != 0x03) 
                {
                  dsdt[k+3+l] = 0x03;
                }
              }
              break;
            }
          } 
          //for (k=i+1; k<i+200; k++) {
          //    if (dsdt[k] == 0x14 && dsdt[k+2] == '_' && dsdt[k+3] == 'P' && dsdt[k+4] == 'S' && dsdt[k+5] == 'W') 
          //    {
          //        size = dsdt[k+1];
          //        for (l=0; l<size; l++)
          //        {
          //            if (dsdt[k+1+l] == 0x70 && dsdt[k+2+l] == 0x00) 
          //            {
          //                dsdt[k+2+l] = 0x03;
          //            }
          //        }
          //        break;
          //    }
          //}
          break;
        }
      }
    }
	}    
	/*
   // chage USB name to UHC1..... for Mac name
   for (j=0; j<usb; j++)
   {
   for (i=0; i<len-5; i++)
   {
   if (dsdt[i] == UsbName[j][0] && dsdt[i+1] == UsbName[j][1] && dsdt[i+2] == UsbName[j][2] && dsdt[i+3] == UsbName[j][3])
   {
   if (USBADR[j] == 0x001D0000)
   {
   dsdt[i] = 'U';
   dsdt[i+1] = 'H';
   dsdt[i+2] = 'C';
   dsdt[i+3] = '1';
   }
   if (USBADR[j] == 0x001D0001)
   {
   dsdt[i] = 'U';
   dsdt[i+1] = 'H';
   dsdt[i+2] = 'C';
   dsdt[i+3] = '2';
   }
   if (USBADR[j] == 0x001D0002)
   {
   dsdt[i] = 'U';
   dsdt[i+1] = 'H';
   dsdt[i+2] = 'C';
   dsdt[i+3] = '3';
   }	
   if (USBADR[j] == 0x001D0003)
   {
   dsdt[i] = 'U';
   dsdt[i+1] = 'H';
   dsdt[i+2] = 'C';
   dsdt[i+3] = '6';
   }            
   if (USBADR[j] == 0x001A0000)
   {
   dsdt[i] = 'U';
   dsdt[i+1] = 'H';
   dsdt[i+2] = 'C';
   dsdt[i+3] = '4';
   }	            
   if (USBADR[j] == 0x001A0001)
   {
   dsdt[i] = 'U';
   dsdt[i+1] = 'H';
   dsdt[i+2] = 'C';
   dsdt[i+3] = '5';
   }	            
   if (USBADR[j] == 0x001D0007)
   {
   dsdt[i] = 'E';
   dsdt[i+1] = 'H';
   dsdt[i+2] = 'C';
   dsdt[i+3] = '1';
   }	            
   if (USBADR[j] == 0x001A0007)
   {
   dsdt[i] = 'E';
   dsdt[i+1] = 'H';
   dsdt[i+2] = 'C';
   dsdt[i+3] = '2';
   }	            
   }
   }
   }            
   */            
  for (j=0; j<usb; j++){
    FreePool(UsbName[j]);
  }
    
	// fix _T_0 _T_1 _T_2 _T_3
	for (i=0; i<len-10; i++) { 
		if (dsdt[i] == '_' && dsdt[i+1] == 'T' && dsdt[i+2] == '_' &&
        (dsdt[i+3] == '0' || dsdt[i+3] == '1' || dsdt[i+3] == '2' || dsdt[i+3] == '3'))
		{
      dsdt[i] = dsdt[i+1];
      dsdt[i+1] = dsdt[i+2];
      dsdt[i+2] = dsdt[i+3];
      dsdt[i+3] = '_';
		}
	}
	
	// fix MUTE Possible operator timeout is ignored
	for (i=0; i<len-10; i++) { 
		if (dsdt[i] == 0x5B && dsdt[i+1] == 0x23 && dsdt[i+2] == 0x4D &&
		    dsdt[i+3] == 0x55 && dsdt[i+4] == 0x54 && dsdt[i+5] == 0x45)
		{
      dsdt[i+6] = 0xFF;
      dsdt[i+7] = 0xFF;
		}
	}
  
  return len;
  
}

VOID FixBiosDsdt (UINT8* temp)
{    
  UINT32 DsdtLen;
  if (!temp)
    return;
  
  DsdtLen = ((EFI_ACPI_DESCRIPTION_HEADER*)temp)->Length;
  if ((DsdtLen < 20) || (DsdtLen > 100000)) { //fool proof
    DBG("DSDT length out of range\n");
    return;
  }
  //   UINT8* temp = (UINT8*)(UINTN)BiosDsdt;
  
  DBG("\nAuto patch BiosDSDT Starting.................\n\n");
  
  //DBG("orgBiosDsdtLen = 0x%08x\n", orgBiosDsdtLen);
  
  // First check hardware address
  CheckHardware();
  
  // find ACPI CPU name and hardware address
  findCPU(temp, DsdtLen);
  
  // add Method (DTGP, 5, NotSerialized)
  if ((gSettings.FixDsdt & FIX_DTGP)) {
    if (!FindMethod(temp, DsdtLen, "DTGP")) {
      CopyMem((VOID*)temp+DsdtLen, dtgp, sizeof(dtgp));
      DsdtLen += sizeof(dtgp);
      ((EFI_ACPI_DESCRIPTION_HEADER*)temp)->Length = DsdtLen;      
    }
  }
  
  // get PCIRootUID and all DSDT Fix address
  gSettings.PCIRootUID = findPciRoot(temp, DsdtLen);
  
  // Fix RTC
  if (RTCADR  && (gSettings.FixDsdt & FIX_HPET))
  {
    DBG("patch RTC in DSDT \n");
    DsdtLen = FixRTC(temp, DsdtLen);
  }
  
  // Fix TMR
  if (TMRADR  && (gSettings.FixDsdt & FIX_HPET))
  {
    DBG("patch TMR in DSDT \n");
    DsdtLen = FixTMR(temp, DsdtLen);
  }
  
  // Fix PIC or IPIC
  if (PICADR && (gSettings.FixDsdt & FIX_IPIC))
  {
    DBG("patch IPIC in DSDT \n");
    DsdtLen = FixPIC(temp, DsdtLen);
  }
  
  // Fix HPET
  if ((gSettings.FixDsdt & FIX_HPET) != 0)
  {
    DBG("patch HPET in DSDT \n");
    DsdtLen = FixHPET(temp, DsdtLen);
  }
  
  // Fix LPC if don't had HPET don't need to inject LPC??
  if (LPCBFIX && (gCPUStructure.Family == 0x06)  && (gSettings.FixDsdt & FIX_LPC))
  {
    DBG("patch LPC in DSDT \n");
    DsdtLen = FIXLPCB(temp, DsdtLen);
  }
  
  // Fix Display
  if (gSettings.FixDsdt & FIX_DISPLAY) {
    if (DisplayADR1[0])
    {
      DBG("patch Display0 in DSDT \n");
      DsdtLen = FIXDisplay1(temp, DsdtLen);
    }
    
    if (DisplayADR1[1])
    {
      DBG("patch Display1 in DSDT \n");
      DsdtLen = FIXDisplay2(temp, DsdtLen);
    }    
  }
  
  // Fix Network
  if (NetworkADR1 && (gSettings.FixDsdt & FIX_LAN))
  {
    DBG("patch LAN in DSDT \n");
    DsdtLen = FIXNetwork(temp, DsdtLen);
  }

  // Fix Airport
  if (ArptADR1 && (gSettings.FixDsdt & FIX_WIFI))
  {
    DBG("patch Airport in DSDT \n");
    DsdtLen = FIXAirport(temp, DsdtLen);
  }
  
  // Fix SBUS
  if (SBUSADR1  && (gSettings.FixDsdt & FIX_SBUS))
  {
    DBG("patch SBUS in DSDT \n");
    DsdtLen = FIXSBUS(temp, DsdtLen);
  }
  
  // Fix IDE inject
  if (IDEFIX && (IDEVENDOR == 0x8086 || IDEVENDOR == 0x11ab)  && (gSettings.FixDsdt & FIX_IDE))
  {
    DBG("patch IDE in DSDT \n");
    DsdtLen = FIXIDE(temp, DsdtLen);
  }
  
  // Fix SATA AHCI orange icon
  if (SATAAHCIADR && (SATAAHCIVENDOR == 0x8086)  && (gSettings.FixDsdt & FIX_SATA))
  {
    DBG("patch AHCI in DSDT \n");
    DsdtLen = FIXSATAAHCI(temp, DsdtLen);
  }
  
  // Fix SATA inject
  if (SATAFIX && (SATAVENDOR == 0x8086)  && (gSettings.FixDsdt & FIX_SATA))
  {
    DBG("patch SATA in DSDT \n");
    DsdtLen = FIXSATA(temp, DsdtLen);
  }
  
  // Fix Firewire
  if (FirewireADR1  && (gSettings.FixDsdt & FIX_FIREWIRE))
  {
    DBG("patch FRWR in DSDT \n");
    DsdtLen = FIXFirewire(temp, DsdtLen);
  }
  
  // HDA HDEF
  if (HDAFIX  && (gSettings.FixDsdt & FIX_HDA))
  {
    DBG("patch HDEF in DSDT \n");
    DsdtLen = AddHDEF(temp, DsdtLen);
  }
  
  //Always add MCHC for PM
  if ((gCPUStructure.Family == 0x06)  && (gSettings.FixDsdt & FIX_MCHC))
  {
    DBG("patch MCHC in DSDT \n");
    DsdtLen = AddMCHC(temp, DsdtLen);
  }
  
  // Always Fix USB
  if ((gSettings.FixDsdt & FIX_USB)) {
    DBG("patch USB in DSDT \n");
    DsdtLen = FIXUSB(temp, DsdtLen);
  }
  
  if ((gSettings.FixDsdt & FIX_WARNING)) {
    DBG("patch warnings \n");
    // Always Fix alias cpu FIX cpus=1
    DsdtLen = FIXCPU1(temp, DsdtLen);
    
    // Always Fix _WAK Return value
    DsdtLen = FIXWAK(temp, DsdtLen);
    
    // USB Device remove error Fix
    DsdtLen = FIXGPE(temp, DsdtLen);
    
    //I want these fixes even if no Display fix. We have GraphicsInjector
    DsdtLen = DeleteDevice("CRT_", temp, DsdtLen);  
    DsdtLen = DeleteDevice("DVI_", temp, DsdtLen);
    //good company
    DsdtLen = DeleteDevice("SPKR", temp, DsdtLen);
    DsdtLen = DeleteDevice("ECP_", temp, DsdtLen);
    DsdtLen = DeleteDevice("LPT_", temp, DsdtLen);
    
    if (gMobile) {
      DsdtLen = AddPNLF(temp, DsdtLen);
    }

    /*    
     // pwrb add _CID sleep button fix
     //DsdtLen = FIXPWRB(temp, DsdtLen);
     */    
    // other compiler warning fix _T_X,  MUTE .... USB _PRW value form 0x04 => 0x01
//    DsdtLen = FIXOTHER(temp, DsdtLen);
  } 
  // Fix SHUTDOWN For ASUS
  if ((gSettings.FixDsdt & FIX_SHUTDOWN))
  {
    DsdtLen = FIXSHUTDOWN_ASUS(temp, DsdtLen);
  }
  
  // Finish DSDT patch and resize DSDT Length
  temp[4] = (DsdtLen & 0x000000FF) >>  0;
  temp[5] = (DsdtLen & 0x0000FF00) >>  8;
  temp[6] = (DsdtLen & 0x00FF0000) >> 16;
  temp[7] = (DsdtLen & 0xFF000000) >> 24;
  
  //DBG("orgBiosDsdtLen = 0x%08x\n", orgBiosDsdtLen);
  ((EFI_ACPI_DESCRIPTION_HEADER*)temp)->Checksum = 0;
  ((EFI_ACPI_DESCRIPTION_HEADER*)temp)->Checksum = (UINT8)(256-Checksum8(temp, DsdtLen));
  
  
  DBG("\nAuto patch BiosDSDT Finish.................\n\n");
  
  //PauseForKey(L"waiting for key press...\n");
}
