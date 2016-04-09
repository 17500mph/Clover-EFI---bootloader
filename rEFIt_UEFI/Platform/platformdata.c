/**
 platformdata.c
 **/

#include "Platform.h"
#include "nvidia.h"

/* Machine Default Data */

CHAR8   *DefaultMemEntry        = "N/A";

CHAR8   *DefaultSerial          = "CT288GT9VT6";

CHAR8   *BiosVendor             = "Apple Inc.";

CHAR8   *AppleManufacturer      = "Apple Computer, Inc."; //Old name, before 2007

UINT32  gFwFeatures             = 0xE001f537;             //default values for iMac13,1

CHAR8   *AppleFirmwareVersion[] =
{
  "MB11.88Z.0061.B03.0809221748",
  "MB21.88Z.00A5.B07.0706270922",
  "MB41.88Z.0073.B00.0809221748",
  "MB52.88Z.0088.B05.0809221748",
  "MBP51.88Z.007E.B06.0906151647",
  "MBP61.88Z.0057.B11.1509232013", //MBP62
  "MBP81.88Z.0047.B2A.1506082203",
  "MBP83.88Z.0047.B24.1110261426",
  "MBP91.88Z.00D3.B0C.1509111653",
  "MBP101.88Z.00EE.B0A.1509111559",
  "MBP111.88Z.0138.B16.1509081438", // MacBookPro11,1 with Haswell
  "MBA31.88Z.0061.B07.0712201139",
  "MBA51.88Z.00EF.B04.1509111654",
  "MBA61.88Z.0099.B20.1509081314",  // MBA62
  "MM21.88Z.009A.B00.0706281359",
  "MM51.88Z.0077.B12.1506081728",
  "MM61.88Z.0106.B0A.1509111654",   // MM61.88Z.0106.B00.1208091121
  "IM81.88Z.00C1.B00.0803051705",
  "IM101.88Z.00CC.B00.0909031926",
  "IM111.88Z.0034.B04.1509231906",
  "IM112.88Z.0057.B03.1509231647",
  "IM112.88Z.0057.B01.1112090906",  //iMac11,3
  "IM121.88Z.0047.B21.1506101610",
  "IM122.88Z.0047.B1F.1223021110",
  "IM131.88Z.010A.B09.1509111558",  //IM131.88Z.010A.B05.1209042338
  "IM131.88Z.010A.B05.1211151146",
  "IM141.88Z.0118.B12.1509081313",
  "IM142.88Z.0118.B13.1602221716",
  "IM151.88Z.0207.B00.1409291931", //correct data
  "IM171.88Z.0105.B05.1511031741", //im171
  "MP31.88Z.006C.B05.0802291410",
  "MP41.88Z.0081.B07.0903051113",
  "MP51.88Z.007F.B03.1010071432",   //007F.B00.1008031144"
  "MP61.88Z.0116.B16.1509081436",
};

CHAR8* AppleBoardID[] =    //Lion DR1 compatible
{
  "Mac-F4208CC8",          // MB11   - yonah
  "Mac-F4208CA9",          // MB21   - merom 05/07
  "Mac-F22788A9",          // MB41   - penryn
  "Mac-F22788AA",          // MB52
  "Mac-F42D86C8",          // MBP51
  "Mac-F22586C8",          // MBP62  - arrandale + nVidia
  "Mac-94245B3640C91C81",  // MBP81  - i5 SB IntelHD3000
  "Mac-942459F5819B171B",  // MBP83  - i7 SB  ATI
  "Mac-6F01561E16C75D06",  // MBP92  - i5-3210M IvyBridge HD4000
  "Mac-C3EC7CD22292981F",  // MBP101
  "Mac-189A3D4F975D5FFC",  // MBP111 - i5-4258U CPU @ 2.40GHz
  "Mac-942452F5819B1C1B",  // MBA31
  "Mac-2E6FAB96566FE58C",  // MBA52  - i5-3427U IVY BRIDGE IntelHD4000 did=166
  "Mac-7DF21CB3ED6977E5",  // MBA62  - i5-4250U/i7-4650U HD5000
  "Mac-F4208EAA",          // MM21   - merom GMA950 07/07
  "Mac-8ED6AF5B48C039E1",  // MM51   - Sandy + Intel 30000
  "Mac-F65AE981FFA204ED",  // MM62   - Ivy
  "Mac-F227BEC8",          // IM81   - merom 01/09, penryn E8435
  "Mac-F2268CC8",          // IM101  - wolfdale? E7600 01/
  "Mac-F2268DAE",          // IM111  - Nehalem
  "Mac-F2238AC8",          // IM112  - Clarkdale
  "Mac-F2238BAE",          // IM113  - lynnfield
  "Mac-942B5BF58194151B",  // IM121  - i5-2500 - sandy
  "Mac-942B59F58194171B",  // IM122  - i7-2600
  "Mac-00BE6ED71E35EB86",  // IM131  - -i5-3470S -IVY
  "Mac-FC02E91DDD3FA6A4",  // IM132  - i5-3470 CPU @ 3.20GHz
  "Mac-031B6874CF7F642A",  // IM141  - i5-4570S/i7-4770S
  "Mac-27ADBB7B4CEE8E61",  // IM142  - i5-4670/i7-4771
  "Mac-42FD25EABCABB274",  // IM151  - i5-4690 CPU @ 3.50GHz
  "Mac-DB15BD556843C820",  // IM171  - i5-4690 CPU @ 3.50GHz
  "Mac-F2268DC8",          // MP31   - xeon quad 02/09 conroe
  "Mac-F4238CC8",          // MP41   - xeon wolfdale
  "Mac-F221BEC8",          // MP51   - Xeon Nehalem 4 cores
  "Mac-F60DEB81FF30ACF6",  // MP61   - Intel(R) Xeon(R) CPU E5-1620 v2 @ 3.70GHz Model 0x3E
  
};

CHAR8* AppleReleaseDate[] =
{
  "09/22/08",    // Mb11
  "06/27/07",
  "09/22/08",
  "01/21/09",
  "06/15/09",    // Mbp51
  "09/23/15",    // MBP61
  "06/08/15",    // MBP81
  "10/26/11",
  "09/11/2015",  // MBP92
  "09/11/2015",  // MBP101
  "09/08/2015",  // MBP111
  "12/20/07",
  "09/11/2015",  // MBA51
  "09/08/2015",
  "08/07/07",    // Mm21
  "06/08/15",    // MM51
  "09/11/2015",  // MM62
  "03/05/08",
  "09/03/09",    // im101
  "09/23/15",
  "09/23/15",    // 11,2
  "12/09/11",    // 113 111209
  "06/10/15",    // 121 120124
  "02/23/12",    // 122
  "09/11/2015",  // 131
  "11/15/2012",  // IM132
  "09/08/2015",
  "02/22/2016",  // IM142
  "09/29/2014",  // IM151  - correct date
  "11/03/2015",  // IM171
  "02/29/08",
  "03/05/09",
  "10/07/10",
  "09/08/2015",  // MacPro6,1
};

CHAR8* AppleProductName[] =
{
  "MacBook1,1",
  "MacBook2,1",
  "MacBook4,1",
  "MacBook5,2",
  "MacBookPro5,1",
  "MacBookPro6,2",
  "MacBookPro8,1",
  "MacBookPro8,3",
  "MacBookPro9,2",
  "MacBookPro10,1",
  "MacBookPro11,1",
  "MacBookAir3,1",
  "MacBookAir5,2",
  "MacBookAir6,2",
  "Macmini2,1",
  "Macmini5,1",
  "Macmini6,2",
  "iMac8,1",
  "iMac10,1",
  "iMac11,1",
  "iMac11,2",
  "iMac11,3",
  "iMac12,1",
  "iMac12,2",
  "iMac13,1",
  "iMac13,2",
  "iMac14,1",
  "iMac14,2",
  "iMac15,1",
  "iMac17,1",
  "MacPro3,1",
  "MacPro4,1",
  "MacPro5,1",
  "MacPro6,1",
};

CHAR8* AppleFamilies[] =
{
  "MacBook",
  "MacBook",
  "MacBook",
  "MacBook",
  "MacBookPro",
  "MacBookPro",
  "MacBookPro",
  "MacBookPro",
  "MacBook Pro",
  "MacBook Pro",
  "Mac",
  "MacBookAir",
  "MacBook Air",
  "MacBook Air",
  "Macmini",
  "Mac mini",
  "Macmini",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac17,1",
  "MacPro",
  "MacPro",
  "MacPro",
  "MacPro"
};


CHAR8* AppleSystemVersion[] =
{
  "1.1",
  "1.2",
  "1.3",
  "1.3",
  "1.0",
  "1.0", //mbp62
  "1.0",
  "1.0",
  "1.0",
  "1.0",
  "1.0",
  "1.0",
  "1.0",
  "1.0",
  "1.1",
  "1.0", // MM51
  "1.0",
  "1.3",
  "1.0",
  "1.0",
  "1.2",
  "1.0",
  "1.9",
  "1.9",
  "1.0",
  "1.0",
  "1.0",
  "1.0",
  "1.0",
  "1.0", //IM171
  "1.3",
  "1.4",
  "1.2",
  "1.0",
};

CHAR8* AppleSerialNumber[] = //random generated
{
  "W80A041AU9B",  // MB11
  "W88A041AWGP",  // MB21  - merom 05/07
  "W88A041A0P0",  // MB41
  "W88AAAAA9GU",  // MB52
  "W88439FE1G0",  // MBP51
  "CK1093T7AGW",  // MBP62 - i7 arrandale + nvidia 330
  "W89F9196DH2G", // MBP81 - i5 SB IntelHD3000
  "W88F9CDEDF93", // MBP83 - i7 SB  ATI
  "C02HA041DTY3", // MBP92 - i5 IvyBridge HD4000
  "C02K2IQNDKQ1", // MBP101
  "C02LSLIPFH00", // MBP111
  "W8649476DQX",  // MBA31
  "C02HA041DRVC", // MBA52 - IvyBridge
  "C02HACKUF5V7", // MBA62  //F5V8
  "W88A56BYYL2",  // MM21  - merom GMA950 07/07
  "C07GA041DJD0", // MM51  - sandy
  "C07JD041DWYN", // MM62  - IVY
  "W89A00AAX88",  // IM81  - merom 01/09
  "W80AA98A5PE",  // IM101 - wolfdale? E7600 01/09
  "G8942B1V5PJ",  // IM111 - Nehalem
  "W8034342DB7",  // IM112 - Clarkdale
  "QP0312PBDNR",  // IM113 - lynnfield
  "W80CF65ADHJF", // IM121 - i5-2500 - sandy
  "W88GG136DHJQ", // IM122 - i7-2600
  "C02JA041DNCT", // IM131 - i5-3470S -IVY
  "C02JB041DNCW", // IM132
  "D25LHACKF8J3", // IM141 - i5-4570S/i7-4770S
  "D25LHACKF8JC", // IM142 - i5-4670/i7-4771
  "C02ND2VTFY11", // IM151 - New 5k iMac
  "C02QFHACGG7L", // IM171 - Skylake iMac
  "W88A77AA5J4",  // MP31  - xeon quad 02/09
  "CT93051DK9Y",  // MP41
  "C07J77F7F4MC", // MP51 C07J50F7F4MC  CK04000AHFC  "CG154TB9WU3"
  "F5KLA770F9VM", // MP61
};

//no! ChassisVersion == BoardID
CHAR8* AppleChassisAsset[] =
{
  "MacBook-White",
  "MacBook-White",
  "MacBook-Black",
  "MacBook-Black",
  "MacBook-Aluminum",
  "MacBook-Aluminum",
  "MacBook-Aluminum",
  "MacBook-Aluminum",
  "MacBook-Aluminum",
  "MacBook-Aluminum",
  "MacBook-Aluminum",
  "MacBook-Aluminum",
  "Air-Enclosure",
  "Air-Enclosure",
  "Mini-Aluminum",
  "Mini-Aluminum",
  "Mini-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum", //iMac15,1
  "iMac-Aluminum", //iMac17,1
  "Pro-Enclosure",
  "Pro-Enclosure",
  "Pro-Enclosure",
  "Pro-Enclosure"
};

//TODO - find more information and correct all SMC arrays
CHAR8* SmcPlatform[] =
{
  "m70",  // MacBook1,1,
  "m75",  // MacBook2,1,
  "m82",  // MacBook4,1,
  "m97",  // MacBook5,2,
  "NA",   // MacBookPro5,1,
  "k74",  // MacBookPro6,2
  "k90i", // MacBookPro8,1,
  "k92i", // MacBookPro8,3,
  "j30",  // MacBookPro9,2,  "d2" for 10,1
  "d2",   // MacBookPro10,1,
  "j44",  // MacBookPro11,1
  "k99",  // MacBookAir3,1,
  "j13",  // MacBookAir5,2,
  "NA",   // MacBookAir6,2,
  "NA",   // Macmini2,1,    //31 -> m88
  "NA",   // Macmini5,1,
  "j50s", // Macmini6,2,
  "k3",   // iMac8,1,
  "k22",  // iMac10,1,
  "NA",   // iMac11,1,
  "k74",  // iMac11,2,
  "k74",  // iMac11,3,
  "k60",  // iMac12,1,
  "k62",  // iMac12,2,
  "d8",   // iMac13,1,
  "d8",   // iMac13,2
  "NA",   // iMac14,1
  "NA",   // iMac14,2
  "j78",  // iMac15,1
  "j95",  // iMac17,1
  "m86",  // MacPro3,1,
  "NA",   // MacPro4,1,
  "k5",   // MacPro5,1
  "j90",  // MacPro6,1
};


UINT8 SmcRevision[][6] =
{
  { 0x01, 0x04, 0x0F, 0, 0, 0x12 },   // MacBook1,1,
  { 0x01, 0x13, 0x0F, 0, 0, 0x03 },   // MacBook2,1,
  { 0x01, 0x31, 0x0F, 0, 0, 0x01 },   // MacBook4,1,
  { 0x01, 0x32, 0x0F, 0, 0, 0x08 },   // MacBook5,1,
  { 0x01, 0x33, 0x0F, 0, 0, 0x08 },   // MacBookPro5,1,
  { 0x01, 0x58, 0x0F, 0, 0, 0x17 },   // MacBookPro6,2,
  { 0x01, 0x68, 0x0F, 0, 0, 0x99 },   // MacBookPro8,1,
  { 0x01, 0x70, 0x0F, 0, 0, 0x05 },   // MacBookPro8,3,
  { 0x02, 0x02, 0x0F, 0, 0, 0x44 },   // MacBookPro9,2,  2.03f36 for 10,1
  { 0x02, 0x03, 0x0F, 0, 0, 0x36 },   // MacBookPro10,1,
  { 0x02, 0x16, 0x0F, 0, 0, 0x58 },   // MacBookPro11,1
  { 0x01, 0x67, 0x0F, 0, 0, 0x10 },   // MacBookAir3,1,
  { 0x02, 0x05, 0x0F, 0, 0, 0x09 },   // MacBookAir5,2,
  { 0x02, 0x13, 0x0F, 0, 0, 0x09 },   // MacBookAir6,2,
  { 0x01, 0x19, 0x0F, 0, 0, 0x02 },   // Macmini2,1,
  { 0x01, 0x30, 0x0F, 0, 0, 0x03 },   // Macmini5,1,
  { 0x02, 0x08, 0x0F, 0, 0, 0x00 },   // Macmini6,2,
  { 0x01, 0x30, 0x0F, 0, 0, 0x01 },   // iMac8,1,
  { 0x01, 0x53, 0x0F, 0, 0, 0x13 },   // iMac10,1,
  { 0x01, 0x54, 0x0F, 0, 0, 0x36 },   // iMac11,1,
  { 0x01, 0x64, 0x0F, 0, 0, 0x05 },   // iMac11,2,
  { 0x01, 0x59, 0x0F, 0, 0, 0x02 },   // iMac11,3,
  { 0x01, 0x71, 0x0F, 0, 0, 0x22 },   // iMac12,1,
  { 0x01, 0x72, 0x0F, 0, 0, 0x02 },   // iMac12,2,
  { 0x02, 0x11, 0x0F, 0, 0, 0x14 },   // iMac13,1,
  { 0x02, 0x11, 0x0F, 0, 0, 0x14 },   // iMac13,2
  { 0x02, 0x14, 0x0F, 0, 0, 0x19 },   // iMac14,1
  { 0x02, 0x15, 0x0F, 0, 0, 0x02 },   // iMac14,2
  { 0x02, 0x22, 0x0F, 0, 0, 0x16 },   // iMac15,1
  { 0x02, 0x33, 0x0F, 0, 0, 0x10 },   // iMac17,1
  { 0x01, 0x25, 0x0F, 0, 0, 0x04 },   // MacPro3,1,
  { 0x01, 0x39, 0x0F, 0, 0, 0x05 },   // MacPro4,1,
  { 0x01, 0x39, 0x0F, 0, 0, 0x11 },   // MacPro5,1
  { 0x02, 0x02, 0x0F, 0, 0, 0x18 },   // MacPro6,1
};


UINT32 SmcConfig[] =
{
  0x71001,  //"MacBook1,1",
  0x72001,  //"MacBook2,1",
  0x74001,  //"MacBook4,1",
  0x7a002,  //"MacBook5,2",
  0x7b002,  //"MacBookPro5,1",
  0x7a004,  //"MacBookPro6,2",
  0x7b005,  //"MacBookPro8,1",
  0x7c005,  //"MacBookPro8,3",
  0x76006,  //"MacBookPro9,2",  //074006 for 10,1  073007 for 10,2
  0x74006,  // MacBookPro10,1
  0xf0b007, // MacBookPro11,1
  0x76005,  //"MacBookAir3,1",
  0x7b006,  //"MacBookAir5,2",
  0x7b007,  //"MacBookAir6,2",
  0x78002,  //"Macmini2,1",     //31 -> m88
  0x7d005,  //"Macmini5,1",
  0x7d006,  //"Macmini6,2",
  0x73002,  //"iMac8,1",
  0x7b002,  //"iMac10,1",
  0x7b004,  //"iMac11,1",
  0x7c004,  //"iMac11,2",
  0x7d004,  //"iMac11,3",
  0x73005,  //"iMac12,1",
  0x75005,  //"iMac12,2",
  0x78006,  //"iMac13,1",
  0x79006,  //iMac13,2
  0x79007,  //iMac14,1
  0x7a007,  //iMac14,2
  0xf00008,  //iMac15,1
  0xf0c008,  //iMac17,1
  0x79001,  //"MacPro3,1",
  0x7c002,  //"MacPro4,1",
  0x7c002,  //"MacPro5,1"
  0xf0f006, // MacPro6,1
};


CHAR8 *AppleBoardSN       = "C02140302D5DMT31M";
CHAR8 *AppleBoardLocation = "Part Component";

VOID
SetDMISettingsForModel (
                        MACHINE_TYPES Model
                        )
{
  AsciiStrCpy (gSettings.VendorName,           BiosVendor);
  AsciiStrCpy (gSettings.RomVersion,           AppleFirmwareVersion[Model]);
  AsciiStrCpy (gSettings.ReleaseDate,          AppleReleaseDate[Model]);
  AsciiStrCpy (gSettings.ManufactureName,      BiosVendor);
  AsciiStrCpy (gSettings.ProductName,          AppleProductName[Model]);
  AsciiStrCpy (gSettings.VersionNr,            AppleSystemVersion[Model]);
  AsciiStrCpy (gSettings.SerialNr,             AppleSerialNumber[Model]);
  AsciiStrCpy (gSettings.FamilyName,           AppleFamilies[Model]);
  AsciiStrCpy (gSettings.BoardManufactureName, BiosVendor);
  AsciiStrCpy (gSettings.BoardSerialNumber,    AppleBoardSN);
  AsciiStrCpy (gSettings.BoardNumber,          AppleBoardID[Model]);
  AsciiStrCpy (gSettings.BoardVersion,         AppleProductName[Model]);
  AsciiStrCpy (gSettings.LocationInChassis,    AppleBoardLocation);
  AsciiStrCpy (gSettings.ChassisManufacturer,  BiosVendor);
  AsciiStrCpy (gSettings.ChassisAssetTag,      AppleChassisAsset[Model]);
  
  if (Model >= MacPro31) {
    gSettings.BoardType = BaseBoardTypeProcessorMemoryModule; //11;
  } else {
    gSettings.BoardType = BaseBoardTypeMotherBoard; //10;
  }
  
  switch (Model) {
    case MacBook11:
    case MacBook21:
    case MacBook41:
    case MacBook52:
    case MacBookAir31:
    case MacBookAir52:
    case MacBookAir62:
    case MacBookPro111:
      gSettings.ChassisType = MiscChassisTypeNotebook; //10;
      gSettings.Mobile      = TRUE;
      break;
      
    case MacBookPro51:
    case MacBookPro62:
    case MacBookPro81:
    case MacBookPro83:
    case MacBookPro92:
    case MacBookPro101:
      gSettings.ChassisType = MiscChassisTypePortable; //08;
      gSettings.Mobile      = TRUE;
      break;
      
    case iMac81:
    case iMac101:
    case iMac111:
    case iMac112:
    case iMac113:
    case iMac121:
    case iMac122:
    case iMac131:
    case iMac132:
    case iMac141:
    case iMac142:
    case iMac151:
    case iMac171:
      gSettings.ChassisType = MiscChassisTypeAllInOne; //13;
      gSettings.Mobile      = FALSE;
      break;
      
    case MacMini21:
    case MacMini51:
    case MacMini62:
      gSettings.ChassisType = MiscChassisTypeLunchBox; //16;
      gSettings.Mobile      = FALSE;
      break;
      
    case MacPro31:
    case MacPro41:
    case MacPro51:
      gSettings.ChassisType = MiscChassisTypeMiniTower; //06;
      gSettings.Mobile      = FALSE;
      break;
      
    case MacPro61:
      gSettings.ChassisType = MiscChassisTypeUnknown;  //02; this is a joke but think different!
      gSettings.Mobile      = FALSE;
      break;
      
    default: //unknown - use oem SMBIOS value to be default
      gSettings.Mobile      = gMobile;
      gSettings.ChassisType = 0; //let SMBIOS value to be
      /*      if (gMobile) {
       gSettings.ChassisType = 10; //notebook
       } else {
       gSettings.ChassisType = MiscChassisTypeDeskTop; //03;
       } */
      break;
  }
  //smc helper
  if (SmcPlatform[Model][0] != 'N') {
    AsciiStrCpy (gSettings.RPlt, SmcPlatform[Model]);
  } else {
    switch (gCPUStructure.Model) {
      case CPU_MODEL_PENTIUM_M:
      case CPU_MODEL_CELERON:
        AsciiStrCpy (gSettings.RPlt, "M70");
        break;
        
      case CPU_MODEL_YONAH:
        AsciiStrCpy (gSettings.RPlt, "k22");
        break;
        
      case CPU_MODEL_MEROM: //TODO check for mobile
        AsciiStrCpy (gSettings.RPlt, "M75");
        break;
        
      case CPU_MODEL_PENRYN:
        if (gSettings.Mobile) {
          AsciiStrCpy (gSettings.RPlt, "M82");
        } else {
          AsciiStrCpy (gSettings.RPlt, "k36");
        }
        break;
        
      case CPU_MODEL_SANDY_BRIDGE:
        if (gSettings.Mobile) {
          AsciiStrCpy (gSettings.RPlt, "k90i");
        } else {
          AsciiStrCpy (gSettings.RPlt, "k60");
        }
        break;
        
      case CPU_MODEL_IVY_BRIDGE:
        AsciiStrCpy (gSettings.RPlt, "j30");
        break;
        
      case CPU_MODEL_IVY_BRIDGE_E5:
        AsciiStrCpy (gSettings.RPlt, "j90");
        break;
        
      case CPU_MODEL_HASWELL_ULT:
        AsciiStrCpy (gSettings.RPlt, "j44");
        break;
        
      case CPU_MODEL_SKYLAKE_S:
        AsciiStrCpy (gSettings.RPlt, "j95");
        break;
        
      default:
        AsciiStrCpy (gSettings.RPlt, "T9");
        break;
    }
  }
  
  CopyMem (gSettings.REV,  SmcRevision[Model], 6);
  AsciiStrCpy (gSettings.RBr,  gSettings.RPlt); //SmcBranch[Model]); // as no other ideas
  CopyMem (gSettings.EPCI, &SmcConfig[Model],  4);
}

//Other info
/*
 MacBookPro7,1 - penryn P8800 RPlt=k6 REV=1.62f5
 MacBookPro6,2 - i5 M520 arrandale
 */

MACHINE_TYPES
GetModelFromString (
                    CHAR8 *ProductName
                    )
{
  UINTN i;
  
  for (i = 0; i < MaxMachineType; ++i) {
    if (AsciiStrCmp (AppleProductName[i], ProductName) == 0) {
      return i;
    }
  }
  // return ending enum as "not found"
  return MaxMachineType;
}

VOID
GetDefaultSettings ()
{
  MACHINE_TYPES  Model;
  //UINT64         msr = 0;
  
  //gLanguage         = english;
  Model             = GetDefaultModel ();
  gSettings.CpuType	= GetAdvancedCpuType ();
  
  SetDMISettingsForModel (Model);
  
  //default values will be overritten by config.plist
  //use explicitly settings TRUE or FALSE (Yes or No)
  
  gSettings.InjectIntel          = (gGraphics[0].Vendor == Intel) || (gGraphics[1].Vendor == Intel);
  
  gSettings.InjectATI            = (((gGraphics[0].Vendor == Ati) && ((gGraphics[0].DeviceID & 0xF000) != 0x6000)) ||
                                    ((gGraphics[1].Vendor == Ati) && ((gGraphics[1].DeviceID & 0xF000) != 0x6000)));
  
  gSettings.InjectNVidia         = (((gGraphics[0].Vendor == Nvidia) && (gGraphics[0].Family < 0xE0)) ||
                                    ((gGraphics[1].Vendor == Nvidia) && (gGraphics[1].Family < 0xE0)));
  
  gSettings.GraphicsInjector     = gSettings.InjectATI || gSettings.InjectNVidia;
  //gSettings.CustomEDID           = NULL; //no sense to assign 0 as the structure is zeroed
  gSettings.DualLink             = 1;
  gSettings.HDAInjection         = TRUE;
  //gSettings.HDALayoutId          = 0;
  gSettings.USBInjection         = TRUE; // enabled by default to have the same behavior as before
  StrCpy (gSettings.DsdtName, L"DSDT.aml");
  gSettings.BacklightLevel       = 0xFFFF; //0x0503; -- the value from MBA52
  gSettings.BacklightLevelConfig = FALSE;
  gSettings.TrustSMBIOS          = TRUE;
  
  gSettings.SmUUIDConfig         = FALSE;
  
  gSettings.RtROM                = NULL;
  gSettings.RtROMLen             = 0;
  
  if (gCPUStructure.Model >= CPU_MODEL_IVY_BRIDGE) {
    gSettings.GeneratePStates    = TRUE;
    gSettings.GenerateCStates    = TRUE;
    //  gSettings.EnableISS          = FALSE;
    //  gSettings.EnableC2           = TRUE;
    gSettings.EnableC6           = TRUE;
    gSettings.PluginType         = 1;
    
    if (gCPUStructure.Model == CPU_MODEL_IVY_BRIDGE) {
      gSettings.MinMultiplier    = 7;
    }
    //  gSettings.DoubleFirstState   = FALSE;
    gSettings.DropSSDT           = TRUE;
    gSettings.C3Latency          = 0x00FA;
  }
  
  //gSettings.EnableISS            = FALSE; //((gCPUStructure.CPUID[CPUID_1][ECX] & (1<<7)) != 0);
  gSettings.Turbo                = gCPUStructure.Turbo;
  //MsgLog ("Turbo default value: %a\n", gCPUStructure.Turbo ? "Yes" : "No");
  //msr                            = AsmReadMsr64(MSR_IA32_MISC_ENABLE);
  //force enable EIST
  //msr                            |= (1<<16);
  //AsmWriteMsr64 (MSR_IA32_MISC_ENABLE, msr);
  //gSettings.Turbo                = ((msr & (1ULL<<38)) == 0);
  //gSettings.EnableISS            = ((msr & (1ULL<<16)) != 0);
  
  //Fill ACPI table list
  //  GetAcpiTablesList ();
}
