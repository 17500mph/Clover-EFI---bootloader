/*
 * refit/main.c
 * Main code for the boot menu
 *
 * Copyright (c) 2006-2010 Christoph Pfisterer
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

#include "Platform.h"
//#include "../include/Handle.h"

#include "Version.h"

#ifndef DEBUG_ALL
#define DEBUG_MAIN 1
#else
#define DEBUG_MAIN DEBUG_ALL
#endif

#if DEBUG_MAIN == 0
#define DBG(...)
#else
#define DBG(...) DebugLog(DEBUG_MAIN, __VA_ARGS__)
#endif


// variables

#define MACOSX_LOADER_PATH      L"\\System\\Library\\CoreServices\\boot.efi"

//static CHAR8 FirmwareRevisionStr[] = FIRMWARE_REVISION_STR;

BOOLEAN                 gThemeNeedInit = TRUE;
EFI_HANDLE              gImageHandle;
EFI_SYSTEM_TABLE*       gST;
EFI_BOOT_SERVICES*		  gBS; 
EFI_RUNTIME_SERVICES*	  gRS;
EFI_DXE_SERVICES*       gDS;

static REFIT_MENU_ENTRY MenuEntryOptions  = { L"Options", TAG_OPTIONS, 1, 0, 'O', NULL, NULL, NULL, {0, 0, 0, 0}, ActionEnter, ActionEnter, ActionNone, NULL };
static REFIT_MENU_ENTRY MenuEntryAbout    = { L"About Clover", TAG_ABOUT, 1, 0, 'A', NULL, NULL, NULL, {0, 0, 0, 0}, ActionEnter, ActionEnter, ActionNone,  NULL };
static REFIT_MENU_ENTRY MenuEntryReset    = { L"Restart Computer", TAG_RESET, 1, 0, 'R', NULL, NULL, NULL, {0, 0, 0, 0}, ActionSelect, ActionEnter, ActionNone,  NULL };
static REFIT_MENU_ENTRY MenuEntryShutdown = { L"Exit Clover", TAG_SHUTDOWN, 1, 0, 'U', NULL, NULL, NULL, {0, 0, 0, 0}, ActionSelect, ActionEnter, ActionNone,  NULL };
REFIT_MENU_ENTRY MenuEntryReturn   = { L"Return", TAG_RETURN, 0, 0, 0, NULL, NULL, NULL, {0, 0, 0, 0}, ActionEnter, ActionEnter, ActionNone,  NULL };

static REFIT_MENU_SCREEN MainMenu    = {1, L"Main Menu", NULL, 0, NULL, 0, NULL, 0, L"Automatic boot", FALSE, FALSE, 0, 0, 0,
  0, {0, 0, 0, 0}, NULL};
static REFIT_MENU_SCREEN AboutMenu   = {2, L"About", NULL, 0, NULL, 0, NULL, 0, NULL, FALSE, FALSE, 0, 0, 0, 0, {0, 0, 0, 0}, NULL };
static REFIT_MENU_SCREEN HelpMenu    = {3, L"Help",  NULL, 0, NULL, 0, NULL, 0, NULL, FALSE, FALSE, 0, 0, 0, 0, {0, 0, 0, 0}, NULL };

DRIVERS_FLAGS gDriversFlags = {FALSE, FALSE, FALSE};

EMU_VARIABLE_CONTROL_PROTOCOL *gEmuVariableControl = NULL;

static VOID AboutRefit(VOID)
{
//  CHAR8* Revision = NULL;
    if (AboutMenu.EntryCount == 0) {
        AboutMenu.TitleImage = BuiltinIcon(BUILTIN_ICON_FUNC_ABOUT);
        AddMenuInfoLine(&AboutMenu, L"Clover Version 2.10 UEFI by Slice, dmazar, apianti, JrCs, and others");
#ifdef FIRMWARE_BUILDDATE
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Build: %a", FIRMWARE_BUILDDATE));
#else
        AddMenuInfoLine(&AboutMenu, L" Build: unknown");
#endif
        AddMenuInfoLine(&AboutMenu, L"");
        AddMenuInfoLine(&AboutMenu, L"Based on rEFIt Copyright (c) 2006-2010 Christoph Pfisterer");
        AddMenuInfoLine(&AboutMenu, L"Portions Copyright (c) Intel Corporation");
        AddMenuInfoLine(&AboutMenu, L"");
        AddMenuInfoLine(&AboutMenu, L"Running on:");
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" EFI Revision %d.%02d",
            gST->Hdr.Revision >> 16, gST->Hdr.Revision & ((1 << 16) - 1)));
#if defined(MDE_CPU_IA32)
        AddMenuInfoLine(&AboutMenu, L" Platform: i386 (32 bit)");
#elif defined(MDE_CPU_X64)
        AddMenuInfoLine(&AboutMenu, L" Platform: x86_64 (64 bit)");
#else
        AddMenuInfoLine(&AboutMenu, L" Platform: unknown");
#endif
#ifdef FIRMWARE_REVISION
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Firmware: %s rev %s", gST->FirmwareVendor, FIRMWARE_REVISION));
#else
      AddMenuInfoLine(&AboutMenu, PoolPrint(L" Firmware: %s rev %d", gST->FirmwareVendor, gST->FirmwareRevision));
#endif
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Screen Output: %s", egScreenDescription()));
      AboutMenu.AnimeRun = GetAnime(&AboutMenu);
        AddMenuEntry(&AboutMenu, &MenuEntryReturn);
    }  else {
      FreePool(AboutMenu.InfoLines[AboutMenu.InfoLineCount-1]);
      AboutMenu.InfoLines[AboutMenu.InfoLineCount-1]=PoolPrint(L" Screen Output: %s", egScreenDescription());
    }
    
    RunMenu(&AboutMenu, NULL);
}

static VOID HelpRefit(VOID)
{
  if (HelpMenu.EntryCount == 0) {
    HelpMenu.TitleImage = BuiltinIcon(BUILTIN_ICON_FUNC_HELP);
    switch (gLanguage)
    {
      case russian:
        AddMenuInfoLine(&HelpMenu, L"ESC - Выход из подменю, обновление главного меню");
        AddMenuInfoLine(&HelpMenu, L"F1  - Помощь по горячим клавишам");
        AddMenuInfoLine(&HelpMenu, L"F2  - Сохранить отчет в preboot.log (только если FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Показать скрытые значки в меню");
        AddMenuInfoLine(&HelpMenu, L"F4  - Родной DSDT сохранить в EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Патченный DSDT сохранить в EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Сохранить ВидеоБиос в EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Снимок экрана в папку EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Извлечь указанный DVD");
        AddMenuInfoLine(&HelpMenu, L"Пробел - Подробнее о выбранном пункте");
        AddMenuInfoLine(&HelpMenu, L"Цифры 1-9 - Быстрый запуск тома по порядку в меню");
        AddMenuInfoLine(&HelpMenu, L"A - О загрузчике");
        AddMenuInfoLine(&HelpMenu, L"O - Дополнительные настройки");
        AddMenuInfoLine(&HelpMenu, L"R - Теплый перезапуск");
        AddMenuInfoLine(&HelpMenu, L"U - Завершить работу в Кловере");
        break;
      case ukrainian:
        AddMenuInfoLine(&HelpMenu, L"ESC - Вийти з меню, оновити головне меню");
        AddMenuInfoLine(&HelpMenu, L"F1  - Ця довідка");
        AddMenuInfoLine(&HelpMenu, L"F2  - Зберегти preboot.log (тiльки FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Відображати приховані розділи");
        AddMenuInfoLine(&HelpMenu, L"F4  - Зберегти OEM DSDT в EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Зберегти патчений DSDT в EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Зберегти VideoBios в EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Зберегти знімок екрану в EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Відкрити обраний диск (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Пробіл - докладніше про обраний пункт меню");
        AddMenuInfoLine(&HelpMenu, L"Клавіші 1-9 -  клавіші пунктів меню");
        AddMenuInfoLine(&HelpMenu, L"A - Про систему");
        AddMenuInfoLine(&HelpMenu, L"O - Опції меню");
        AddMenuInfoLine(&HelpMenu, L"R - Перезавантаження");
        AddMenuInfoLine(&HelpMenu, L"U - Відключити ПК");
        break;
      case spanish:
        AddMenuInfoLine(&HelpMenu, L"ESC - Salir de submenu o actualizar el menu principal");
        AddMenuInfoLine(&HelpMenu, L"F1  - Esta Ayuda");
        AddMenuInfoLine(&HelpMenu, L"F2  - Guardar preboot.log (Solo FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Guardar DSDT oem en EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Guardar DSDT parcheado en EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Guardar VideoBios en EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Guardar Captura de pantalla en EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Expulsar volumen seleccionado (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Espacio - Detalles acerca selected menu entry");
        AddMenuInfoLine(&HelpMenu, L"Digitos 1-9 - Atajo a la entrada del menu");
        AddMenuInfoLine(&HelpMenu, L"A - Menu Acerca de");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Optiones");
        AddMenuInfoLine(&HelpMenu, L"R - Reiniciar Equipo");
        AddMenuInfoLine(&HelpMenu, L"U - Apagar");
        break;
      case portuguese:
      case brasil:
        AddMenuInfoLine(&HelpMenu, L"ESC - Sai do submenu, atualiza o menu principal");
        AddMenuInfoLine(&HelpMenu, L"F1  - Esta ajuda");
        AddMenuInfoLine(&HelpMenu, L"F2  - Salva preboot.log (somente FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Salva oem DSDT em EFI/CLOVER/ACPI/origin/ (somente FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Salva DSDT corrigido em EFI/CLOVER/ACPI/origin/ (somente FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Salva VideoBios em EFI/CLOVER/misc/ (somente FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Salva screenshot em EFI/CLOVER/misc/ (somente FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Ejeta o volume selecionado (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Espaco - Detalhes sobre a opcao do menu selecionada");
        AddMenuInfoLine(&HelpMenu, L"Tecle 1-9 - Atalho para as entradas do menu");
        AddMenuInfoLine(&HelpMenu, L"A - Sobre o Menu");
        AddMenuInfoLine(&HelpMenu, L"O - Opcoes do Menu");
        AddMenuInfoLine(&HelpMenu, L"R - Reiniciar");
        AddMenuInfoLine(&HelpMenu, L"U - Desligar");
        break;
      case italian:
        AddMenuInfoLine(&HelpMenu, L"ESC - Esci dal submenu, Aggiorna menu principale");
        AddMenuInfoLine(&HelpMenu, L"F1  - Aiuto");
        AddMenuInfoLine(&HelpMenu, L"F2  - Salva il preboot.log (solo su FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Salva il DSDT oem in EFI/CLOVER/ACPI/origin/ (solo suFAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Salva il patched DSDT in EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Salva il VideoBios in EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Salva screenshot in EFI/CLOVER/misc/ (solo su FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Espelli il volume selezionato (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Spazio - Dettagli sul menu selezionato");
        AddMenuInfoLine(&HelpMenu, L"Digita 1-9 - Abbreviazioni per il menu");
        AddMenuInfoLine(&HelpMenu, L"A - Informazioni");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Opzioni");
        AddMenuInfoLine(&HelpMenu, L"R - Riavvio");
        AddMenuInfoLine(&HelpMenu, L"U - Spegnimento");
        break;
      case german:
        AddMenuInfoLine(&HelpMenu, L"ESC - Zurueck aus Untermenue, Hauptmenue erneuern");
        AddMenuInfoLine(&HelpMenu, L"F1  - Diese Hilfe");
        AddMenuInfoLine(&HelpMenu, L"F2  - Sichere preboot.log (nur mit FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Sichere OEM DSDT in EFI/CLOVER/ACPI/origin/ (nur mit FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Sichere gepatchtes DSDT in EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Sichere VideoBios in EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Sichere Bildschirmfoto in EFI/CLOVER/misc/ (nur mit FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Volume auswerfen (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Leertaste - Details über den gewählten Menue Eintrag");
        AddMenuInfoLine(&HelpMenu, L"Zahlen 1-9 - Kurzwahl zum Menue Eintrag");
        AddMenuInfoLine(&HelpMenu, L"A - Menue Informationen");
        AddMenuInfoLine(&HelpMenu, L"O - Menue Optionen");
        AddMenuInfoLine(&HelpMenu, L"R - Neustart");
        AddMenuInfoLine(&HelpMenu, L"U - Ausschalten");
        break;		
      case dutch:
        AddMenuInfoLine(&HelpMenu, L"ESC - Verlaat submenu, Vernieuwen hoofdmenu");
        AddMenuInfoLine(&HelpMenu, L"F1  - Onderdeel hulp");
        AddMenuInfoLine(&HelpMenu, L"F2  - preboot.log opslaan (FAT32 only)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Opslaan oem DSDT in EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Opslaan gepatchte DSDT in EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Opslaan VideoBios in EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Opslaan schermafdruk in EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Uitwerpen geselecteerd volume (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Spatie - Details over geselecteerd menuoptie");
        AddMenuInfoLine(&HelpMenu, L"Cijfers 1-9 - Snelkoppeling naar menuoptie");
        AddMenuInfoLine(&HelpMenu, L"A - Menu Over");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Opties");
        AddMenuInfoLine(&HelpMenu, L"R - Soft Reset");
        AddMenuInfoLine(&HelpMenu, L"U - Verlaten");
        break;
     case french:
        AddMenuInfoLine(&HelpMenu, L"ESC - Quitter sous-menu, Retour menu principal");
        AddMenuInfoLine(&HelpMenu, L"F1  - Aide");
        AddMenuInfoLine(&HelpMenu, L"F2  - Enregistrer preboot.log (FAT32 only)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Enregistrer oem DSDT dans EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Enregistrer DSDT modifié dans EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Enregistrer VideoBios dans EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Enregistrer la capture d'écran dans EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Ejecter le volume (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Space - Détails a propos du menu selectionné");
        AddMenuInfoLine(&HelpMenu, L"Digits 1-9 - Raccourci vers entrée menu");
        AddMenuInfoLine(&HelpMenu, L"A - A propos");
        AddMenuInfoLine(&HelpMenu, L"O - Options Menu");
        AddMenuInfoLine(&HelpMenu, L"R - Redémarrer");
        AddMenuInfoLine(&HelpMenu, L"U - Eteindre");
        break;
      case indonesian:
        AddMenuInfoLine(&HelpMenu, L"ESC - Keluar submenu, Refresh main menu");
        AddMenuInfoLine(&HelpMenu, L"F1  - Help");
        AddMenuInfoLine(&HelpMenu, L"F2  - Simpan preboot.log (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Simpan oem DSDT ke EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Simpan patched DSDT ke EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Simpan VideoBios ke EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Simpan screenshot ke EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Eject volume (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Spasi - Detail dari menu yang dipilih");
        AddMenuInfoLine(&HelpMenu, L"Tombol 1-9 - Shortcut pilihan menu");
        AddMenuInfoLine(&HelpMenu, L"A - About");
        AddMenuInfoLine(&HelpMenu, L"O - Opsi");
        AddMenuInfoLine(&HelpMenu, L"R - Soft Reset");
        AddMenuInfoLine(&HelpMenu, L"U - Shutdown");
        break;
      case polish:
/*        AddMenuInfoLine(&HelpMenu, L"ESC - Wyjście z podmenu, Odświeżenie głównego menu");
        AddMenuInfoLine(&HelpMenu, L"F1  - Pomoc");
        AddMenuInfoLine(&HelpMenu, L"F2  - Zapis preboot.log (tylko dla FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F4  - Zapis czystych tabel DSDT do EFI/CLOVER/ACPI/origin/ (tylko dla FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Zapis zmodyfikowanego DSDT do EFI/CLOVER/ACPI/origin/ (tylko dla FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Zapis BIOSu karty graficznej do EFI/CLOVER/misc/ (tylko dla FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Zapis zrzutu ekranu do EFI/CLOVER/misc/ (tylko dla FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Wysunięcie zaznaczonego dysku (tylko dla DVD)");
        AddMenuInfoLine(&HelpMenu, L"Spacja - Informacje na temat dostępnych opcji dla zaznaczonego dysku");
        AddMenuInfoLine(&HelpMenu, L"Znaki 1-9 - Skróty do opcji dla zaznaczonego dysku");
        AddMenuInfoLine(&HelpMenu, L"A - Menu Informacyjne");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Opcje");
        AddMenuInfoLine(&HelpMenu, L"R - Restart komputera");
        AddMenuInfoLine(&HelpMenu, L"U - Wyłączenie komputera");*/
        
        AddMenuInfoLine(&HelpMenu, L"ESC - Wyjscie z podmenu, Odswiezenie glownego menu");
        AddMenuInfoLine(&HelpMenu, L"F1  - Pomoc");
        AddMenuInfoLine(&HelpMenu, L"F2  - Zapis preboot.log (tylko FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Zapis DSDT do EFI/CLOVER/ACPI/origin/ (tylko FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Zapis poprawionego DSDT do EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Zapis BIOSu k. graficznej do EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Zapis zrzutu ekranu do EFI/CLOVER/misc/ (tylko FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Wysuniecie zaznaczonego dysku (tylko dla DVD)");
        AddMenuInfoLine(&HelpMenu, L"Spacja - Informacje nt. dostepnych opcji dla zaznaczonego dysku");
        AddMenuInfoLine(&HelpMenu, L"Znaki 1-9 - Skroty opcji dla wybranego dysku");
        AddMenuInfoLine(&HelpMenu, L"A - Menu Informacyjne");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Opcje");
        AddMenuInfoLine(&HelpMenu, L"R - Restart komputera");
        AddMenuInfoLine(&HelpMenu, L"U - Wylaczenie komputera");
        break;
      case croatian:
        AddMenuInfoLine(&HelpMenu, L"ESC - izlaz iz podizbornika, Osvježi glavni izbornik");
        AddMenuInfoLine(&HelpMenu, L"F1  - Ovaj izbornik");
        AddMenuInfoLine(&HelpMenu, L"F2  - Spremi preboot.log (samo na FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Spremi oem DSDT u EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Spremi patched DSDT into EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Spremi VideoBios into EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Spremi screenshot into EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Izbaci izabrai (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Space - Detalji o odabranom sistemu");
        AddMenuInfoLine(&HelpMenu, L"Brojevi 1 do 9 su prečac do izbora");
        AddMenuInfoLine(&HelpMenu, L"A - Izbornik o meni");
        AddMenuInfoLine(&HelpMenu, L"O - Izbornik opcije");
        AddMenuInfoLine(&HelpMenu, L"R - Restart računala");
        AddMenuInfoLine(&HelpMenu, L"U - Isključivanje računala");
        break;
      case czech:
        AddMenuInfoLine(&HelpMenu, L"ESC - Vrátit se do hlavní nabídky");
        AddMenuInfoLine(&HelpMenu, L"F1  - Tato Nápověda");
        AddMenuInfoLine(&HelpMenu, L"F2  - Uložit preboot.log (FAT32 only)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Uložit oem DSDT do EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Uložit patchnuté DSDT do EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Uložit VideoBios do EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Uložit snímek obrazovky do EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Vysunout vybranou mechaniku (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Mezerník - Podrobnosti o vybraném disku");
        AddMenuInfoLine(&HelpMenu, L"čísla 1-9 - Klávesové zkratky pro disky");
        AddMenuInfoLine(&HelpMenu, L"A - Menu O Programu");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Možnosti");
        AddMenuInfoLine(&HelpMenu, L"R - Částečný restart");
        AddMenuInfoLine(&HelpMenu, L"U - Odejít");
        break;
      case korean:
        AddMenuInfoLine(&HelpMenu, L"ESC - 하위메뉴에서 나감, 메인메뉴 새로 고침");
        AddMenuInfoLine(&HelpMenu, L"F1  - 이 도움말");
        AddMenuInfoLine(&HelpMenu, L"F2  - preboot.log를 저장합니다. (FAT32방식에만 해당됨)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - oem DSDT를 EFI/CLOVER/ACPI/origin/에 저장합니다. (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - 패치된 DSDT를 EFI/CLOVER/ACPI/origin/에 저장합니다. (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - VideoBios를 EFI/CLOVER/misc/에 저장합니다. (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - 스크린샷을 EFI/CLOVER/misc/에 저장합니다. (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - 선택한 볼륨을 제거합니다. (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Space - 선택한 메뉴의 상세 설명");
        AddMenuInfoLine(&HelpMenu, L"Digits 1-9 - 메뉴 단축 번호");
        AddMenuInfoLine(&HelpMenu, L"A - 단축키 - 이 부트로더에 관하여");
        AddMenuInfoLine(&HelpMenu, L"O - 단축키 - 부트 옵션");
        AddMenuInfoLine(&HelpMenu, L"R - 단축키 - 리셋");
        AddMenuInfoLine(&HelpMenu, L"U - 단축키 - 시스템 종료");
        break;
      case english:
      default:
        AddMenuInfoLine(&HelpMenu, L"ESC - Escape from submenu, Refresh main menu");
        AddMenuInfoLine(&HelpMenu, L"F1  - This help");
        AddMenuInfoLine(&HelpMenu, L"F2  - Save preboot.log (FAT32 only)");
        AddMenuInfoLine(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfoLine(&HelpMenu, L"F4  - Save oem DSDT into EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F5  - Save patched DSDT into EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F6  - Save VideoBios into EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F10 - Save screenshot into EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfoLine(&HelpMenu, L"F12 - Eject selected volume (DVD)");
        AddMenuInfoLine(&HelpMenu, L"Space - Details about selected menu entry");
        AddMenuInfoLine(&HelpMenu, L"Digits 1-9 - Shortcut to menu entry");
        AddMenuInfoLine(&HelpMenu, L"A - Menu About");
        AddMenuInfoLine(&HelpMenu, L"O - Menu Options");
        AddMenuInfoLine(&HelpMenu, L"R - Soft Reset");
        AddMenuInfoLine(&HelpMenu, L"U - Exit");
        break;
    }
    HelpMenu.AnimeRun = GetAnime(&HelpMenu);
    AddMenuEntry(&HelpMenu, &MenuEntryReturn);
  }
  
  RunMenu(&HelpMenu, NULL);
}


static EFI_STATUS StartEFIImageList(IN EFI_DEVICE_PATH **DevicePaths,
                                    IN CHAR16 *LoadOptions, IN CHAR16 *LoadOptionsPrefix,
                                    IN CHAR16 *ImageTitle,
                                    OUT UINTN *ErrorInStep,
                                    OUT EFI_HANDLE *NewImageHandle)
{
  EFI_STATUS              Status, ReturnStatus;
  EFI_HANDLE              ChildImageHandle = 0;
  EFI_LOADED_IMAGE        *ChildLoadedImage;
  UINTN                   DevicePathIndex;
  CHAR16                  ErrorInfo[256];
  CHAR16                  *FullLoadOptions = NULL;
  
  DBG("Starting %s\n", ImageTitle);
  if (ErrorInStep != NULL) {
    *ErrorInStep = 0;
  }
  if (NewImageHandle != NULL) {
    *NewImageHandle = NULL;
  }
  
  // load the image into memory
  ReturnStatus = Status = EFI_NOT_FOUND;  // in case the list is empty
  for (DevicePathIndex = 0; DevicePaths[DevicePathIndex] != NULL; DevicePathIndex++) {
    ReturnStatus = Status = gBS->LoadImage(FALSE, SelfImageHandle, DevicePaths[DevicePathIndex], NULL, 0, &ChildImageHandle);
    if (ReturnStatus != EFI_NOT_FOUND)
      break;
  }
  UnicodeSPrint(ErrorInfo, 512, L"while loading %s", ImageTitle);
  if (CheckError(Status, ErrorInfo)) {
    if (ErrorInStep != NULL)
      *ErrorInStep = 1;
    goto bailout;
  }
  
  // set load options
  if (LoadOptions != NULL) {
    ReturnStatus = Status = gBS->HandleProtocol(ChildImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ChildLoadedImage);
    if (CheckError(Status, L"while getting a LoadedImageProtocol handle")) {
      if (ErrorInStep != NULL)
        *ErrorInStep = 2;
      goto bailout_unload;
    }
    
    if (LoadOptionsPrefix != NULL) {
      FullLoadOptions = PoolPrint(L"%s %s ", LoadOptionsPrefix, LoadOptions);
      // NOTE: That last space is also added by the EFI shell and seems to be significant
      //  when passing options to Apple's boot.efi...
      LoadOptions = FullLoadOptions;
    }
    // NOTE: We also include the terminating null in the length for safety.
    ChildLoadedImage->LoadOptions = (VOID *)LoadOptions;
    ChildLoadedImage->LoadOptionsSize = (UINT32)StrSize(LoadOptions);
    //((UINT32)StrLen(LoadOptions) + 1) * sizeof(CHAR16);
    DBG("Using load options '%s'\n", LoadOptions);
  }
  //DBG("Image loaded at: %p\n", ChildLoadedImage->ImageBase);
  //PauseForKey(L"continue");
  
  // close open file handles
  UninitRefitLib();
  
  // turn control over to the image
  //
  // Before calling the image, enable the Watchdog Timer for
  // the 5 Minute period - Slice - NO! 60seconds is enough
  //  
  gBS->SetWatchdogTimer (180, 0x0000, 0x00, NULL);
  
  ReturnStatus = Status = gBS->StartImage(ChildImageHandle, NULL, NULL);
  //
  // Clear the Watchdog Timer after the image returns
  //
  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);
  
  //PauseForKey(L"Returned from StartImage\n");
  
  // control returns here when the child image calls Exit()
  if (ImageTitle) {
    UnicodeSPrint(ErrorInfo, 512, L"returned from %s", ImageTitle);
  }
  
  if (CheckError(Status, ErrorInfo)) {
    if (ErrorInStep != NULL)
      *ErrorInStep = 3;
  }
//  PauseForKey(L"Error checked\n");
  // re-open file handles
//  Status = ReinitRefitLib();  
//  PauseForKey(L"ReinitRefitLib OK\n");
  //Slice
/*  if (EFI_ERROR(Status)) {
    goto bailout_unload;
  }
 */
  if (!EFI_ERROR(ReturnStatus)) { //why unload driver?!
    if (NewImageHandle != NULL) {
      *NewImageHandle = ChildImageHandle;
    }
    goto bailout;
  }
  
bailout_unload:
  // unload the image, we don't care if it works or not...
  Status = gBS->UnloadImage(ChildImageHandle);
  if (FullLoadOptions != NULL)
    FreePool(FullLoadOptions);
bailout:
  return ReturnStatus;
}

static EFI_STATUS StartEFIImage(IN EFI_DEVICE_PATH *DevicePath,
                                IN CHAR16 *LoadOptions, IN CHAR16 *LoadOptionsPrefix,
                                IN CHAR16 *ImageTitle,
                                OUT UINTN *ErrorInStep,
                                OUT EFI_HANDLE *NewImageHandle)
{
  EFI_DEVICE_PATH *DevicePaths[2];
  
  DevicePaths[0] = DevicePath;
  DevicePaths[1] = NULL;
  return StartEFIImageList(DevicePaths, LoadOptions, LoadOptionsPrefix, ImageTitle, ErrorInStep, NewImageHandle);
}

//
// Null ConOut OutputString() implementation - for blocking
// text output from boot.efi when booting in graphics mode
//
EFI_STATUS EFIAPI
NullConOutOutputString(IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, IN CHAR16 *String) {
	return EFI_SUCCESS;
}

//
// EFI OS loader functions
//
EG_PIXEL DarkBackgroundPixel  = { 0x0, 0x0, 0x0, 0xFF };

static CHAR16 *AddLoadOption(IN CHAR16 *LoadOptions, IN CHAR16 *LoadOption)
{
  // If either option strings are null nothing to do
  if (LoadOptions == NULL)
  {
    if (LoadOption == NULL) return NULL;
    // Duplicate original options as nothing to add
    return EfiStrDuplicate(LoadOption);
  }
  // If there is no option or it is already present duplicate original
  else if ((LoadOption == NULL) || StrStr(LoadOptions, LoadOption)) return EfiStrDuplicate(LoadOptions);
  // Otherwise add option
  return PoolPrint(L"%s %s", LoadOptions, LoadOption);
}

static VOID StartLoader(IN LOADER_ENTRY *Entry)
{
  EFI_STATUS              Status;
  BOOLEAN                 BlockConOut = FALSE;
  EFI_TEXT_STRING         ConOutOutputString = 0;
  
  DBG("StartLoader() start\n");
  egClearScreen(&DarkBackgroundPixel);
  MsgLog("Finally: Bus=%ldkHz CPU=%ldMHz\n",
         DivU64x32(gCPUStructure.FSBFrequency, kilo),
         gCPUStructure.MaxSpeed);

//  MsgLog("Turbo=%c\n", gSettings.Turbo?'Y':'N');
//  MsgLog("PatchAPIC=%c\n", gSettings.PatchNMI?'Y':'N');
//  MsgLog("PatchVBios=%c\n", gSettings.PatchVBios?'Y':'N');
//  DBG("KillMouse\n");
  KillMouse();
//    DBG("BeginExternalScreen\n");
  BeginExternalScreen(OSFLAG_ISSET(Entry->Flags, OSFLAG_USEGRAPHICS), L"Booting OS");
//  if (Entry->LoaderType == OSTYPE_OSX) {
  if (OSTYPE_IS_OSX(Entry->LoaderType) ||
      OSTYPE_IS_OSX_RECOVERY(Entry->LoaderType) ||
      OSTYPE_IS_OSX_INSTALLER(Entry->LoaderType)) {
    // first patchACPI and find PCIROOT and RTC
    // but before ACPI patch we need smbios patch
    PatchSmbios();
//    DBG("PatchACPI\n");
    PatchACPI(Entry->Volume);
//DBG("GetOSVersion\n");
    Status = GetOSVersion(Entry->Volume);

    // Prepare boot arguments
    // Entry->LoadOptions = PoolPrint(L"%a", gSettings.BootArgs);
    if ((StrCmp(gST->FirmwareVendor, L"CLOVER") != 0) &&
        (StrCmp(gST->FirmwareVendor, L"EDKII") != 0)) {
      // Add slide=0 argument for ML and Mavericks if not present
      CHAR16 *TempOptions = AddLoadOption(Entry->LoadOptions, L"slide=0");
      FreePool(Entry->LoadOptions);
      Entry->LoadOptions = TempOptions;
    }

    // If KPDebug is true boot in verbose mode to see the debug messages
    if (gSettings.KPDebug) {
      CHAR16 *TempOptions = AddLoadOption(Entry->LoadOptions, L"-v");
      FreePool(Entry->LoadOptions);
      Entry->LoadOptions = TempOptions;
    }

//    DBG("SetDevices\n");
    SetDevices();
//    DBG("SetFSInjection\n");
    SetFSInjection(Entry);
    //PauseForKey(L"SetFSInjection");
//    DBG("SetVariablesForOSX\n");
    SetVariablesForOSX();
//    DBG("SetVariablesForOSX\n");
    EventsInitialize(Entry);
//    DBG("FinalizeSmbios\n");
    FinalizeSmbios();
//    DBG("SetupDataForOSX\n");
    SetupDataForOSX();
//    DBG("LoadKexts\n");
    LoadKexts(Entry);
//    DBG("SetupBooterLog\n");
    DBG("Closing log\n");
    Status = SetupBooterLog();
    
    // blocking boot.efi output if -v is not specified
    // note: this blocks output even if -v is specified in
    // /Library/Preferences/SystemConfiguration/com.apple.Boot.plist
    // which is wrong
    if (Entry->LoadOptions == NULL || (StrStr(Entry->LoadOptions, L"-v") == NULL && StrStr(Entry->LoadOptions, L"-V") == NULL)) {
      BlockConOut = TRUE;
    }
  }
  else if (OSTYPE_IS_WINDOWS(Entry->LoaderType)) {
    
    if (gEmuVariableControl != NULL) {
      gEmuVariableControl->UninstallEmulation(gEmuVariableControl);
    }
    
    PatchACPI_OtherOS(L"Windows", FALSE);
    //PauseForKey(L"continue");
      
  }
  else if (OSTYPE_IS_LINUX(Entry->LoaderType)) {
    if (gEmuVariableControl != NULL) {
      gEmuVariableControl->UninstallEmulation(gEmuVariableControl);
    }
    //FinalizeSmbios();
    PatchACPI_OtherOS(L"Linux", FALSE);
    //PauseForKey(L"continue");
    Entry->LoadOptions     = NULL;
  }
  
  SetStartupDiskVolume(Entry->Volume, Entry->LoaderType == OSTYPE_OSX ? NULL : Entry->LoaderPath);
  
  if (BlockConOut) {
    // save orig OutputString and replace it with
    // null implementation
    ConOutOutputString = gST->ConOut->OutputString;
    gST->ConOut->OutputString = NullConOutOutputString;
  }
//  DBG("StartEFIImage\n");
  StartEFIImage(Entry->DevicePath, Entry->LoadOptions,
                Basename(Entry->LoaderPath), Basename(Entry->LoaderPath), NULL, NULL);
  
  if (BlockConOut) {
    // return back orig OutputString
    gST->ConOut->OutputString = ConOutOutputString;
  }
  
//  PauseForKey(L"FinishExternalScreen");
  FinishExternalScreen();
//  PauseForKey(L"System started?!");
}


//Set Entry->VolName to .disk_label.contentDetails if it exists
static EFI_STATUS GetOSXVolumeName(LOADER_ENTRY *Entry)
{
  EFI_STATUS	Status = EFI_NOT_FOUND;
  CHAR16* targetNameFile = L"System\\Library\\CoreServices\\.disk_label.contentDetails";
  CHAR8* 	fileBuffer;
  CHAR8*  targetString;
  UINTN   fileLen = 0;
  if(FileExists(Entry->Volume->RootDir, targetNameFile)) {
    Status = egLoadFile(Entry->Volume->RootDir, targetNameFile, (UINT8 **)&fileBuffer, &fileLen);
    if(!EFI_ERROR(Status)) {
      CHAR16  *tmpName;
      INTN i;
      //Create null terminated string
      targetString = (CHAR8*) AllocateZeroPool(fileLen+1);
      CopyMem( (VOID*)targetString, (VOID*)fileBuffer, fileLen);
      
      if (Entry->Volume->OSType == OSTYPE_BOOT_OSX) {
        //remove occurence number. eg: "vol_name 2" --> "vol_name"
        i=fileLen-1;
        while ((i>0) && (targetString[i]>='0') && (targetString[i]<='9')) {
          i--;
        }
        if (targetString[i] == ' ') {
          targetString[i] = 0;
        }
      }
      
      //Convert to Unicode
      tmpName = (CHAR16*)AllocateZeroPool((fileLen+1)*2);
      tmpName = AsciiStrToUnicodeStr(targetString, tmpName);
      
      Entry->VolName = EfiStrDuplicate(tmpName);
      
      FreePool(tmpName);
      FreePool(fileBuffer);
      FreePool(targetString);
    }
  }
  return Status;
}

/*
static CHAR16 *RemoveLoadOption(IN CHAR16 *LoadOptions, IN CHAR16 *LoadOption)
{
  CHAR16 *Placement;
  CHAR16 *NewLoadOptions;
  UINTN   Length, Offset, OptionLength;
  
  //DBG("LoadOptions: '%s', remove LoadOption: '%s'\n", LoadOptions, LoadOption);
  // If there are no options then nothing to do
  if (LoadOptions == NULL) return NULL;
  // If there is no option to remove then duplicate original
  if (LoadOption == NULL) return EfiStrDuplicate(LoadOptions);
  // If not present duplicate original
  Placement = StrStr(LoadOptions, LoadOption);
  if (Placement == NULL) return EfiStrDuplicate(LoadOptions);
  
  // Get placement of option in original options
  Offset = (Placement - LoadOptions);
  Length = StrLen(LoadOptions);
  OptionLength = StrLen(LoadOption);
  
  // If this is just part of some larger option (contains non-space at the beginning or end)
  if ((Offset > 0 && LoadOptions[Offset - 1] != L' ')
      || ((Offset + OptionLength) < Length && LoadOptions[Offset + OptionLength] != L' ')
      )
  {
    return EfiStrDuplicate(LoadOptions);
  }
  
  // Consume preceeding spaces
  while (Offset > 0 && LoadOptions[Offset - 1] == L' ') {
    OptionLength++;
    Offset--;
  }
  
  // Consume following spaces
  while (LoadOptions[Offset + OptionLength] == L' ') {
    OptionLength++;
  }
  
  // If it's the whole string return NULL
  if (OptionLength == Length) return NULL;
  
  if (Offset == 0) {
    // Simple case - we just need substring after OptionLength position
    NewLoadOptions = EfiStrDuplicate(LoadOptions + OptionLength);
  } else {
    // The rest of LoadOptions is Length - OptionLength, but we may need additional space and ending 0
    NewLoadOptions = AllocateZeroPool((Length - OptionLength + 2) * sizeof(CHAR16));
    // Copy preceeding substring
    CopyMem(NewLoadOptions, LoadOptions, Offset * sizeof(CHAR16));
    if ((Offset + OptionLength) < Length) {
      // Copy following substring, but include one space also
      OptionLength--;
      CopyMem(NewLoadOptions + Offset, LoadOptions + Offset + OptionLength, (Length - OptionLength - Offset) * sizeof(CHAR16));
    }
  }
  return NewLoadOptions;
  
  // PoolPrint(L"%*s",... does not work as expected in previous code
  //if ((Offset + OptionLength) >= Length) return PoolPrint(L"%*s", Offset - 1, LoadOptions);
  // Otherwise remove the option
  //return PoolPrint(L"%*s%s", Offset, LoadOptions, Placement + OptionLength + 1);
  
}

// */

static LOADER_ENTRY *CreateLoaderEntry(IN CHAR16 *LoaderPath, IN CHAR16 *LoaderOptions, IN CHAR16 *FullTitle, IN CHAR16 *LoaderTitle, IN REFIT_VOLUME *Volume,
                                       IN EG_IMAGE *Image, IN EG_IMAGE *DriveImage, IN UINT8 OSType, IN UINT8 Flags, IN CHAR16 Hotkey, IN BOOLEAN CustomEntry)
{
  EFI_DEVICE_PATH *LoaderDevicePath;
  CHAR16          *LoaderDevicePathString;
  CHAR16          *FilePathAsString;
  CHAR16          *OSIconName;
  CHAR16           IconFileName[256];
  CHAR16           ShortcutLetter;
  LOADER_ENTRY    *Entry;
  INTN             i;

  // Check parameters are valid
  if ((LoaderPath == NULL) || (Volume == NULL)) {
    return NULL;
  }

  // Get the loader device path
  LoaderDevicePath = FileDevicePath(Volume->DeviceHandle, LoaderPath);
  if (LoaderDevicePath == NULL) {
    return NULL;
  }
  LoaderDevicePathString = FileDevicePathToStr(LoaderDevicePath);
  if (LoaderDevicePathString == NULL) {
    return NULL;
  }

  // Ignore this loader if it's self path
  FilePathAsString = FileDevicePathToStr(SelfLoadedImage->FilePath);
  if (FilePathAsString) {
    INTN Comparison = StriCmp(FilePathAsString, LoaderDevicePathString);
    FreePool(FilePathAsString);
    if (Comparison == 0) {
      DBG("skipped because path `%s` is self path!\n", LoaderDevicePathString);
      FreePool(LoaderDevicePathString);
      return NULL;
    }
  }

  if (!CustomEntry) {
    CUSTOM_LOADER_ENTRY *Custom;
    UINTN                CustomIndex = 0;

    // Ignore this loader if it's device path is already present in another loader
    if (MainMenu.Entries) {
      for (i = 0; i < MainMenu.EntryCount; ++i) {
        REFIT_MENU_ENTRY *MainEntry = MainMenu.Entries[i];
        // Only want loaders
        if (MainEntry && (MainEntry->Tag == TAG_LOADER)) {
          LOADER_ENTRY *Loader = (LOADER_ENTRY *)MainEntry;
          if (StriCmp(Loader->DevicePathString, LoaderDevicePathString) == 0) {
            DBG("skipped because path `%s` already exists for another entry!\n", LoaderDevicePathString);
            FreePool(LoaderDevicePathString);
            return NULL;
          }
        }
      }
    }
    // If this isn't a custom entry make sure it's not hidden by a custom entry
    Custom = gSettings.CustomEntries;
    while (Custom) {
      // Check if the custom entry is hidden or disabled
      if (OSFLAG_ISSET(Custom->Flags, OSFLAG_DISABLED) ||
          (OSFLAG_ISSET(Custom->Flags, OSFLAG_HIDDEN) && !gSettings.ShowHiddenEntries)) {
        // Check if there needs to be a volume match
        if (Custom->Volume != NULL) {
          // Check if the string matches the volume
          if ((StrStr(Volume->DevicePathString, Custom->Volume) != NULL) ||
              ((Volume->VolName != NULL) && (StrStr(Volume->VolName, Custom->Volume) != NULL))) {
            if (Custom->VolumeType != 0) {
              if (((Volume->DiskKind == DISK_KIND_INTERNAL) && ((Custom->VolumeType & DISABLE_FLAG_INTERNAL) == 0)) ||
                  ((Volume->DiskKind == DISK_KIND_EXTERNAL) && ((Custom->VolumeType & DISABLE_FLAG_EXTERNAL) == 0)) ||
                  ((Volume->DiskKind == DISK_KIND_OPTICAL) && ((Custom->VolumeType & DISABLE_FLAG_OPTICAL) == 0)) ||
                  ((Volume->DiskKind == DISK_KIND_FIREWIRE) && ((Custom->VolumeType & DISABLE_FLAG_FIREWIRE) == 0))) {
                if (Custom->Path != NULL) {
                  // Try to match the loader paths and types
                  if (StriCmp(Custom->Path, LoaderPath) == 0) {
                    if (Custom->Type != 0) {
                      if (OSTYPE_COMPARE(Custom->Type, OSType)) {
                        DBG("skipped path `%s` because it is a volume, volumetype, path and type match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                        FreePool(LoaderDevicePathString);
                        return NULL;
                      } else {
                         DBG("partial volume, volumetype, and path match for path `%s` and custom entry %d, type did not match\n", LoaderDevicePathString, CustomIndex);
                      }
                    } else {
                      DBG("skipped path `%s` because it is a volume, volumetype and path match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                      FreePool(LoaderDevicePathString);
                      return NULL;
                    }
                  } else {
                    DBG("partial volume, and volumetype match for path `%s` and custom entry %d, path did not match\n", LoaderDevicePathString, CustomIndex);
                  }
                } else if (Custom->Type != 0) {
                  if (OSTYPE_COMPARE(Custom->Type, OSType)) {
                    // Only match the loader type
                    DBG("skipped path `%s` because it is a volume, volumetype and type match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                    FreePool(LoaderDevicePathString);
                    return NULL;
                  } else {
                     DBG("partial volume, and volumetype match for path `%s` and custom entry %d, type did not match\n", LoaderDevicePathString, CustomIndex);
                  }
                } else {
                  DBG("skipped path `%s` because it is a volume and volumetype match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                  FreePool(LoaderDevicePathString);
                  return NULL;
                }
              } else {
                DBG("partial volume match for path `%s` and custom entry %d, volumetype did not match\n", LoaderDevicePathString, CustomIndex);
              }
            } if (Custom->Path != NULL) {
              // Check if there needs to be a path match also
              if (StriCmp(Custom->Path, LoaderPath) == 0) {
                if (Custom->Type != 0) {
                  if (OSTYPE_COMPARE(Custom->Type, OSType)) {
                    DBG("skipped path `%s` because it is a volume, path and type match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                    FreePool(LoaderDevicePathString);
                    return NULL;
                  } else {
                    DBG("partial volume, and path match for path `%s` and custom entry %d, type did not match\n", LoaderDevicePathString, CustomIndex);
                  }
                } else {
                  DBG("skipped path `%s` because it is a volume and path match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                  FreePool(LoaderDevicePathString);
                  return NULL;
                }
              } else {
                DBG("partial volume match for path `%s` and custom entry %d, path did not match\n", LoaderDevicePathString, CustomIndex);
              }
            } else if (Custom->Type != 0) {
              if (OSTYPE_COMPARE(Custom->Type, OSType)) {
                DBG("skipped path `%s` because it is a volume and type match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                FreePool(LoaderDevicePathString);
                return NULL;
              } else {
                DBG("partial volume match for path `%s` and custom entry %d, type did not match\n", LoaderDevicePathString, CustomIndex);
              }
            } else {
              DBG("skipped path `%s` because it is a volume match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
              FreePool(LoaderDevicePathString);
              return NULL;
            }
          } else {
            DBG("volume did not match for path `%s` and custom entry %d\n", LoaderDevicePathString, CustomIndex);
          }
        } else if (Custom->VolumeType != 0) {
          if (((Volume->DiskKind == DISK_KIND_INTERNAL) && ((Custom->VolumeType & DISABLE_FLAG_INTERNAL) == 0)) ||
              ((Volume->DiskKind == DISK_KIND_EXTERNAL) && ((Custom->VolumeType & DISABLE_FLAG_EXTERNAL) == 0)) ||
              ((Volume->DiskKind == DISK_KIND_OPTICAL) && ((Custom->VolumeType & DISABLE_FLAG_OPTICAL) == 0)) ||
              ((Volume->DiskKind == DISK_KIND_FIREWIRE) && ((Custom->VolumeType & DISABLE_FLAG_FIREWIRE) == 0))) {
            if (Custom->Path != NULL) {
              // Try to match the loader paths and types
              if (StriCmp(Custom->Path, LoaderPath) == 0) {
                if (Custom->Type != 0) {
                  if (OSTYPE_COMPARE(Custom->Type, OSType)) {
                    DBG("skipped path `%s` because it is a volumetype, path and type match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                    FreePool(LoaderDevicePathString);
                    return NULL;
                  } else {
                    DBG("partial volumetype, and path match for path `%s` and custom entry %d, type did not match\n", LoaderDevicePathString, CustomIndex);
                  }
                } else {
                  DBG("skipped path `%s` because it is a volumetype and path match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                  FreePool(LoaderDevicePathString);
                  return NULL;
                }
              } else {
                DBG("partial volumetype match for path `%s` and custom entry %d, path did not match\n", LoaderDevicePathString, CustomIndex);
              }
            } else if (Custom->Type != 0) {
              if (OSTYPE_COMPARE(Custom->Type, OSType)) {
                // Only match the loader type
                DBG("skipped path `%s` because it is a volumetype and type match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                FreePool(LoaderDevicePathString);
                return NULL;
              } else {
                DBG("partial volumetype match for path `%s` and custom entry %d, type did not match\n", LoaderDevicePathString, CustomIndex);
              }
            } else {
              DBG("skipped path `%s` because it is a volumetype match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
              FreePool(LoaderDevicePathString);
              return NULL;
            }
          } else {
            DBG("did not match volumetype for path `%s` and custom entry %d\n", LoaderDevicePathString, CustomIndex);
          }
        } else if (Custom->Path != NULL) {
          // Try to match the loader paths and types
          if (StriCmp(Custom->Path, LoaderPath) == 0) {
            if (Custom->Type != 0) {
              if (OSTYPE_COMPARE(Custom->Type, OSType)) {
                DBG("skipped path `%s` because it is a path and type match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
                FreePool(LoaderDevicePathString);
                return NULL;
              } else {
                DBG("partial path match for path `%s` and custom entry %d, type did not match\n", LoaderDevicePathString, CustomIndex);
              }
            } else {
              DBG("skipped path `%s` because it is a path match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
              FreePool(LoaderDevicePathString);
              return NULL;
            }
          } else {
            DBG("did not match path for path `%s` and custom entry %d\n", LoaderDevicePathString, CustomIndex);
          }
        } else if (Custom->Type != 0) {
          if (OSTYPE_COMPARE(Custom->Type, OSType)) {
            // Only match the loader type
            DBG("skipped path `%s` because it is a type match for custom entry %d!\n", LoaderDevicePathString, CustomIndex);
            FreePool(LoaderDevicePathString);
            return NULL;
          } else {
            DBG("did not match type for path `%s` and custom entry %d!\n", LoaderDevicePathString, CustomIndex);
          }
        }
      }
      Custom = Custom->Next;
      ++CustomIndex;
    }
  }

  // prepare the menu entry
  Entry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  if (Volume->BootType == BOOTING_BY_EFI) {
    Entry->me.Tag          = TAG_LOADER;
  } else {
    Entry->me.Tag          = TAG_LEGACY;
  }

  Entry->me.Row = 0;
  Entry->Volume = Volume;

  Entry->LoaderPath       = EfiStrDuplicate(LoaderPath);
  Entry->VolName          = Volume->VolName;
  Entry->DevicePath       = LoaderDevicePath;
  Entry->DevicePathString = LoaderDevicePathString;
  Entry->Flags            = Flags;
  if (LoaderOptions) {
    Entry->LoadOptions    = EfiStrDuplicate(LoaderOptions);
  } else if (AsciiStrLen(gSettings.BootArgs) > 0) {
    Entry->LoadOptions    = PoolPrint(L"%a", gSettings.BootArgs);
  }

  // locate a custom icon for the loader
  StrCpy(IconFileName, Volume->OSIconName);
  //actions
  Entry->me.AtClick = ActionSelect;
  Entry->me.AtDoubleClick = ActionEnter;
  Entry->me.AtRightClick = ActionDetails;
  
  // detect specific loaders
  OSIconName = NULL;
  ShortcutLetter = 0;

  switch (OSType) {
    case OSTYPE_OSX:
    case OSTYPE_TIGER:
    case OSTYPE_LEO:
    case OSTYPE_SNOW:
    case OSTYPE_LION:
    case OSTYPE_ML:
    case OSTYPE_MAV:
    case OSTYPE_RECOVERY:
    case OSTYPE_BOOT_OSX:
    case OSTYPE_OSX_INSTALLER:
      OSIconName = Volume->OSIconName;
      if (Entry->LoadOptions == NULL || (StrStr(Entry->LoadOptions, L"-v") == NULL && StrStr(Entry->LoadOptions, L"-V") == NULL)) {
        // OSX is not booting verbose, so we can set console to graphics mode
        Entry->Flags = OSFLAG_SET(Entry->Flags, OSFLAG_USEGRAPHICS);
      }
      if (gSettings.WithKexts) {
        Entry->Flags = OSFLAG_SET(Entry->Flags, OSFLAG_WITHKEXTS);
      }
      ShortcutLetter = 'M';
      Entry->LoaderType = OSTYPE_OSX;
      GetOSXVolumeName(Entry);
      break;
    case OSTYPE_WIN:
      OSIconName = L"win";
      ShortcutLetter = 'W';
      Entry->LoaderType = OSTYPE_WIN;
      break;
    case OSTYPE_WINEFI:
      OSIconName = L"vista";
      ShortcutLetter = 'V';
      Entry->LoaderType = OSTYPE_WINEFI;
      break;
    case OSTYPE_LIN:
      OSIconName = Volume->OSIconName != NULL ? Volume->OSIconName : L"linux";
      ShortcutLetter = 'L';
      Entry->LoaderType = OSTYPE_LIN;
      break;
    case OSTYPE_VAR:
    case OSTYPE_EFI:
      OSIconName = L"clover";
      ShortcutLetter = 'U';
      Entry->LoaderType = OSTYPE_VAR;
      break;
    default:
      OSIconName = L"unknown";
      Entry->LoaderType = OSTYPE_VAR;
      break;
  }

  if (FullTitle) {
    Entry->me.Title = EfiStrDuplicate(FullTitle);
  } else {
    Entry->me.Title = PoolPrint(L"Boot %s from %s", (LoaderTitle != NULL) ? LoaderTitle : LoaderPath + 1, Entry->VolName);
  }
  Entry->me.ShortcutLetter = (Hotkey == 0) ? ShortcutLetter : Hotkey;

  // get custom volume icon if present
  if (Image) {
    Entry->me.Image = Image;
  } else {
    if (GlobalConfig.CustomIcons && FileExists(Volume->RootDir, L"VolumeIcon.icns")){
      Entry->me.Image = LoadIcns(Volume->RootDir, L"VolumeIcon.icns", 128);
      DBG("using VolumeIcon.icns image from Volume\n");
    } else {
      Entry->me.Image = LoadOSIcon(OSIconName, L"unknown", 128, FALSE, TRUE);
    }
  }
  // Load DriveImage
  Entry->me.DriveImage = (DriveImage != NULL) ? DriveImage : ScanVolumeDefaultIcon(Volume);

  // DBG("HideBadges=%d Volume=%s ", GlobalConfig.HideBadges, Volume->VolName);
  if (GlobalConfig.HideBadges & HDBADGES_SHOW) {
    if (GlobalConfig.HideBadges & HDBADGES_SWAP) {
      Entry->me.BadgeImage = egCopyScaledImage(Entry->me.DriveImage, 4);
      // DBG(" Show badge as Drive.");
    } else {
      Entry->me.BadgeImage = egCopyScaledImage(Entry->me.Image, 8);
      // DBG(" Show badge as OSImage.");
    }
  }
  return Entry;
}

static LOADER_ENTRY * AddLoaderEntry(IN CHAR16 *LoaderPath, IN CHAR16 *LoaderOptions, IN CHAR16 *FullTitle, IN CHAR16 *LoaderTitle, IN REFIT_VOLUME *Volume,
                                     IN EG_IMAGE *Image, IN EG_IMAGE *DriveImage, IN UINT8 OSType, IN UINT8 Flags, IN CHAR16 Hotkey, IN BOOLEAN CustomEntry)
{
  CHAR16            *FileName, *TempOptions;
  CHAR16            DiagsFileName[256];
  LOADER_ENTRY      *Entry, *SubEntry;
  REFIT_MENU_SCREEN *SubScreen;
  UINT64            VolumeSize;
  EFI_GUID          *Guid = NULL;

  Entry = CreateLoaderEntry(LoaderPath, LoaderOptions, FullTitle, LoaderTitle, Volume, Image, DriveImage, OSType, Flags, Hotkey, CustomEntry);
  if (Entry == NULL) {
    return NULL;
  }

  FileName = Basename(LoaderPath);

  // create the submenu
  SubScreen = AllocateZeroPool(sizeof(REFIT_MENU_SCREEN));
  SubScreen->Title = PoolPrint(L"Boot Options for %s on %s", (LoaderTitle != NULL) ? LoaderTitle : FileName, Entry->VolName);
  SubScreen->TitleImage = Entry->me.Image;
  SubScreen->ID = OSType + 20;
//  DBG("get anime for os=%d\n", SubScreen->ID);
  SubScreen->AnimeRun = GetAnime(SubScreen);
  VolumeSize = RShiftU64(MultU64x32(Volume->BlockIO->Media->LastBlock, Volume->BlockIO->Media->BlockSize), 20);
  AddMenuInfoLine(SubScreen, PoolPrint(L"Volume size: %dMb", VolumeSize));
  AddMenuInfoLine(SubScreen, DevicePathToStr(Entry->DevicePath));
  Guid = FindGPTPartitionGuidInDevicePath(Volume->DevicePath);
  if (Guid) {
    CHAR8 *GuidStr = AllocateZeroPool(50);
    AsciiSPrint(GuidStr, 50, "%g", Guid);
    AddMenuInfoLine(SubScreen, PoolPrint(L"UUID: %a", GuidStr));
    FreePool(GuidStr);
  }

  
  // default entry
  SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  SubEntry->me.Title        = (Entry->LoaderType == OSTYPE_OSX) ? L"Boot Mac OS X" : PoolPrint(L"Run %s", FileName);
  SubEntry->me.Tag          = TAG_LOADER;
  SubEntry->LoaderPath      = Entry->LoaderPath;
  SubEntry->Volume          = Entry->Volume;
  SubEntry->VolName         = Entry->VolName;
  SubEntry->DevicePath      = Entry->DevicePath;
  SubEntry->DevicePathString = Entry->DevicePathString;
  SubEntry->Flags           = Entry->Flags;
  SubEntry->LoadOptions     = LoaderOptions;
  SubEntry->LoaderType      = Entry->LoaderType;
  SubEntry->me.AtClick      = ActionEnter;
  AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  
  // loader-specific submenu entries
  if (Entry->LoaderType == OSTYPE_OSX) {          // entries for Mac OS X
#if defined(MDE_CPU_X64)
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    if ((OSType != OSTYPE_ML) &&
        (OSType != OSTYPE_MAV)) {      
      SubEntry->me.Title        = L"Boot Mac OS X (64-bit)";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->DevicePathString = Entry->DevicePathString;
      SubEntry->Flags           = Entry->Flags;
      SubEntry->LoadOptions     = AddLoadOption(Entry->LoadOptions, L"arch=x86_64");
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);   
    }
#endif
    
    if ((OSType != OSTYPE_ML) &&
        (OSType != OSTYPE_MAV)) {
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = L"Boot Mac OS X (32-bit)";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->DevicePathString = Entry->DevicePathString;
      SubEntry->Flags           = Entry->Flags;
      SubEntry->LoadOptions     = AddLoadOption(Entry->LoadOptions, L"arch=i386");
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);      
    }
    
    if (!(GlobalConfig.DisableFlags & DISABLE_FLAG_SINGLEUSER)) {
      
#if defined(MDE_CPU_X64)
      if ((OSType == OSTYPE_ML) ||
          (OSType == OSTYPE_MAV)) {        
        SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
        SubEntry->me.Title        = L"Boot Mac OS X in verbose mode";
        SubEntry->me.Tag          = TAG_LOADER;
        SubEntry->LoaderPath      = Entry->LoaderPath;
        SubEntry->Volume          = Entry->Volume;
        SubEntry->VolName         = Entry->VolName;
        SubEntry->DevicePath      = Entry->DevicePath;
        SubEntry->DevicePathString = Entry->DevicePathString;
        SubEntry->Flags           = OSFLAG_UNSET(Entry->Flags, OSFLAG_USEGRAPHICS);
        SubEntry->LoadOptions     = AddLoadOption(Entry->LoadOptions, L"-v");
        SubEntry->LoaderType      = OSTYPE_OSX;
        SubEntry->me.AtClick      = ActionEnter;
        AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
      } else {        
        SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
        SubEntry->me.Title        = L"Boot Mac OS X in verbose mode (64bit)";
        SubEntry->me.Tag          = TAG_LOADER;
        SubEntry->LoaderPath      = Entry->LoaderPath;
        SubEntry->Volume          = Entry->Volume;
        SubEntry->VolName         = Entry->VolName;
        SubEntry->DevicePath      = Entry->DevicePath;
        SubEntry->DevicePathString = Entry->DevicePathString;
        SubEntry->Flags           = OSFLAG_UNSET(Entry->Flags, OSFLAG_USEGRAPHICS);
        TempOptions = AddLoadOption(Entry->LoadOptions, L"-v");
        SubEntry->LoadOptions     = AddLoadOption(TempOptions, L"arch=x86_64");
        FreePool(TempOptions);
        SubEntry->LoaderType      = OSTYPE_OSX;
        SubEntry->me.AtClick      = ActionEnter;
        AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);        
      }

#endif

      if ((OSType != OSTYPE_ML) &&
          (OSType != OSTYPE_MAV)) {
        SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
        SubEntry->me.Title        = L"Boot Mac OS X in verbose mode (32-bit)";
        SubEntry->me.Tag          = TAG_LOADER;
        SubEntry->LoaderPath      = Entry->LoaderPath;
        SubEntry->Volume          = Entry->Volume;
        SubEntry->VolName         = Entry->VolName;
        SubEntry->DevicePath      = Entry->DevicePath;
        SubEntry->DevicePathString = Entry->DevicePathString;
        SubEntry->Flags           = OSFLAG_SET(Entry->Flags, OSFLAG_USEGRAPHICS);
        TempOptions = AddLoadOption(Entry->LoadOptions, L"-v");
        SubEntry->LoadOptions     = AddLoadOption(TempOptions, L"arch=i386");
        FreePool(TempOptions);
        SubEntry->LoaderType      = OSTYPE_OSX;
        SubEntry->me.AtClick      = ActionEnter;
        AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);        
      }
      
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = L"Boot Mac OS X in safe mode";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->DevicePathString = Entry->DevicePathString;
      SubEntry->Flags           = OSFLAG_UNSET(Entry->Flags, OSFLAG_USEGRAPHICS);
      TempOptions = AddLoadOption(Entry->LoadOptions, L"-v");
      SubEntry->LoadOptions     = AddLoadOption(TempOptions, L"-x");
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
      
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = L"Boot Mac OS X in single user verbose mode";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->DevicePathString = Entry->DevicePathString;
      SubEntry->Flags           = OSFLAG_UNSET(Entry->Flags, OSFLAG_USEGRAPHICS);
      TempOptions = AddLoadOption(Entry->LoadOptions, L"-v");
      SubEntry->LoadOptions     = AddLoadOption(TempOptions, L"-s");
      FreePool(TempOptions);
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
      
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = OSFLAG_ISSET(Entry->Flags, OSFLAG_NOCACHES) ?
                                    L"Boot Mac OS X with caches" :
                                    L"Boot Mac OS X without caches";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->DevicePathString = Entry->DevicePathString;
      SubEntry->Flags           = OSFLAG_TOGGLE(Entry->Flags, OSFLAG_NOCACHES);
      SubEntry->LoadOptions     = AddLoadOption(Entry->LoadOptions, L"-v");
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);

      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = OSFLAG_ISSET(Entry->Flags, OSFLAG_WITHKEXTS) ?
                                    L"Boot Mac OS X without injected kexts" :
                                    L"Boot Mac OS X with injected kexts";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = Entry->LoaderPath;
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = Entry->DevicePath;
      SubEntry->DevicePathString = Entry->DevicePathString;
      SubEntry->Flags           = OSFLAG_TOGGLE(Entry->Flags, OSFLAG_WITHKEXTS);
      SubEntry->LoadOptions     = AddLoadOption(Entry->LoadOptions, L"-v");
      SubEntry->LoaderType      = OSTYPE_OSX;
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);

      if (OSFLAG_ISSET(Entry->Flags, OSFLAG_WITHKEXTS))
      {
        if (OSFLAG_ISSET(Entry->Flags, OSFLAG_NOCACHES))
        {
          SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
          SubEntry->me.Title        = L"Boot Mac OS X with caches and without injected kexts";
          SubEntry->me.Tag          = TAG_LOADER;
          SubEntry->LoaderPath      = Entry->LoaderPath;
          SubEntry->Volume          = Entry->Volume;
          SubEntry->VolName         = Entry->VolName;
          SubEntry->DevicePath      = Entry->DevicePath;
          SubEntry->DevicePathString = Entry->DevicePathString;
          SubEntry->Flags           = OSFLAG_UNSET(OSFLAG_UNSET(Entry->Flags, OSFLAG_NOCACHES), OSFLAG_WITHKEXTS);
          SubEntry->LoadOptions     = AddLoadOption(Entry->LoadOptions, L"-v");
          SubEntry->LoaderType      = OSTYPE_OSX;
          SubEntry->me.AtClick      = ActionEnter;
          AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        }
        else
        {
          SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
          SubEntry->me.Title        = L"Boot Mac OS X without caches and without injected kexts";
          SubEntry->me.Tag          = TAG_LOADER;
          SubEntry->LoaderPath      = Entry->LoaderPath;
          SubEntry->Volume          = Entry->Volume;
          SubEntry->VolName         = Entry->VolName;
          SubEntry->DevicePath      = Entry->DevicePath;
          SubEntry->DevicePathString = Entry->DevicePathString;
          SubEntry->Flags           = OSFLAG_UNSET(OSFLAG_SET(Entry->Flags, OSFLAG_NOCACHES), OSFLAG_WITHKEXTS);
          SubEntry->LoadOptions     = AddLoadOption(Entry->LoadOptions, L"-v");
          SubEntry->LoaderType      = OSTYPE_OSX;
          SubEntry->me.AtClick      = ActionEnter;
          AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        }
      }
      else if (OSFLAG_ISSET(Entry->Flags, OSFLAG_NOCACHES))
      {
        SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
        SubEntry->me.Title        = L"Boot Mac OS X with caches and with injected kexts";
        SubEntry->me.Tag          = TAG_LOADER;
        SubEntry->LoaderPath      = Entry->LoaderPath;
        SubEntry->Volume          = Entry->Volume;
        SubEntry->VolName         = Entry->VolName;
        SubEntry->DevicePath      = Entry->DevicePath;
        SubEntry->DevicePathString = Entry->DevicePathString;
        SubEntry->Flags           = OSFLAG_SET(OSFLAG_UNSET(Entry->Flags, OSFLAG_NOCACHES), OSFLAG_WITHKEXTS);
        SubEntry->LoadOptions     = AddLoadOption(Entry->LoadOptions, L"-v");
//        SubEntry->LoadOptions     = Entry->LoadOptions;
        SubEntry->LoaderType      = OSTYPE_OSX;
        SubEntry->me.AtClick      = ActionEnter;
        AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
      }
      else
      {
        SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
        SubEntry->me.Title        = L"Boot Mac OS X without caches and with injected kexts";
        SubEntry->me.Tag          = TAG_LOADER;
        SubEntry->LoaderPath      = Entry->LoaderPath;
        SubEntry->Volume          = Entry->Volume;
        SubEntry->VolName         = Entry->VolName;
        SubEntry->DevicePath      = Entry->DevicePath;
        SubEntry->DevicePathString = Entry->DevicePathString;
        SubEntry->Flags           = OSFLAG_SET(OSFLAG_SET(Entry->Flags, OSFLAG_NOCACHES), OSFLAG_WITHKEXTS);
        SubEntry->LoadOptions     = AddLoadOption(Entry->LoadOptions, L"-v");
 //       SubEntry->LoadOptions     = Entry->LoadOptions;
        SubEntry->LoaderType      = OSTYPE_OSX;
        SubEntry->me.AtClick      = ActionEnter;
        AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
      }
    }
    
    // check for Apple hardware diagnostics
    StrCpy(DiagsFileName, L"\\System\\Library\\CoreServices\\.diagnostics\\diags.efi");
    if (FileExists(Volume->RootDir, DiagsFileName) && !(GlobalConfig.DisableFlags & DISABLE_FLAG_HWTEST)) {
      DBG("  - Apple Hardware Test found\n");
      
      SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
      SubEntry->me.Title        = L"Run Apple Hardware Test";
      SubEntry->me.Tag          = TAG_LOADER;
      SubEntry->LoaderPath      = EfiStrDuplicate(DiagsFileName);
      SubEntry->Volume          = Entry->Volume;
      SubEntry->VolName         = Entry->VolName;
      SubEntry->DevicePath      = FileDevicePath(Volume->DeviceHandle, SubEntry->LoaderPath);
      SubEntry->DevicePathString = Entry->DevicePathString;
      SubEntry->Flags           = OSFLAG_SET(Entry->Flags, OSFLAG_USEGRAPHICS);
      SubEntry->me.AtClick      = ActionEnter;
      AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    }
    
  } else if (Entry->LoaderType == OSTYPE_LIN) {   // entries for elilo
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = PoolPrint(L"Run %s in interactive mode", FileName);
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->DevicePathString = Entry->DevicePathString;
    SubEntry->Flags           = Entry->Flags;
    SubEntry->LoadOptions     = L"-p";
    SubEntry->LoaderType      = OSTYPE_LIN;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Linux for a 17\" iMac or a 15\" MacBook Pro (*)";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->DevicePathString = Entry->DevicePathString;
    SubEntry->Flags           = OSFLAG_SET(Entry->Flags, OSFLAG_USEGRAPHICS);
    SubEntry->LoadOptions     = L"-d 0 i17";
    SubEntry->LoaderType      = OSTYPE_LIN;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Linux for a 20\" iMac (*)";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->DevicePathString = Entry->DevicePathString;
    SubEntry->Flags           = OSFLAG_SET(Entry->Flags, OSFLAG_USEGRAPHICS);
    SubEntry->LoadOptions     = L"-d 0 i20";
    SubEntry->LoaderType      = OSTYPE_LIN;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Linux for a Mac Mini (*)";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->DevicePathString = Entry->DevicePathString;
    SubEntry->Flags           = OSFLAG_SET(Entry->Flags, OSFLAG_USEGRAPHICS);
    SubEntry->LoadOptions     = L"-d 0 mini";
    SubEntry->LoaderType      = OSTYPE_LIN;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    AddMenuInfoLine(SubScreen, L"NOTE: This is an example. Entries");
    AddMenuInfoLine(SubScreen, L"marked with (*) may not work.");
    
  } else if ((Entry->LoaderType == OSTYPE_WIN) || (Entry->LoaderType == OSTYPE_WINEFI)) {   // entries for xom.efi
                                  // by default, skip the built-in selection and boot from hard disk only
    Entry->LoadOptions = L"-s -h";
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Windows from Hard Disk";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->DevicePathString = Entry->DevicePathString;
    SubEntry->Flags           = Entry->Flags;
    SubEntry->LoadOptions     = L"-s -h";
    SubEntry->LoaderType      = Entry->LoaderType;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Boot Windows from CD-ROM";
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->DevicePathString = Entry->DevicePathString;
    SubEntry->Flags           = Entry->Flags;
    SubEntry->LoadOptions     = L"-s -c";
    SubEntry->LoaderType      = Entry->LoaderType;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = PoolPrint(L"Run %s in text mode", FileName);
    SubEntry->me.Tag          = TAG_LOADER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->DevicePathString = Entry->DevicePathString;
    SubEntry->Flags           = OSFLAG_UNSET(Entry->Flags, OSFLAG_USEGRAPHICS);
    SubEntry->LoadOptions     = L"-v";
    SubEntry->LoaderType      = OSTYPE_VAR;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
    
  }
  
  AddMenuEntry(SubScreen, &MenuEntryReturn);
  Entry->me.SubScreen = SubScreen;
  AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
  DBG("    Added '%s'\n", Entry->me.Title);
  return Entry;
}

static LOADER_ENTRY * AddCloverEntry(IN CHAR16 *LoaderPath, IN CHAR16 *LoaderTitle, IN REFIT_VOLUME *Volume)
{
  LOADER_ENTRY      *Entry, *SubEntry;
  REFIT_MENU_SCREEN *SubScreen;
  EFI_STATUS        Status;
  
  // prepare the menu entry
  Entry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  Entry->me.Title          = LoaderTitle;
  Entry->me.Tag            = TAG_CLOVER;
  Entry->me.Row            = 1;
  Entry->me.ShortcutLetter = 'C';
  Entry->me.Image          = BuiltinIcon(BUILTIN_ICON_FUNC_CLOVER);
  Entry->Volume = Volume;
  Entry->LoaderPath      = EfiStrDuplicate(LoaderPath);
  Entry->VolName         = Volume->VolName;
  Entry->DevicePath      = FileDevicePath(Volume->DeviceHandle, Entry->LoaderPath);
  Entry->DevicePathString = FileDevicePathToStr(Entry->DevicePath);
  Entry->Flags           = 0;
  Entry->LoadOptions     = NULL;
  
  //actions
  Entry->me.AtClick = ActionSelect;
  Entry->me.AtDoubleClick = ActionDetails;
  Entry->me.AtRightClick = ActionDetails;
  
  // create the submenu
  SubScreen = AllocateZeroPool(sizeof(REFIT_MENU_SCREEN));
  SubScreen->Title = EfiStrDuplicate(LoaderTitle);
  SubScreen->TitleImage = Entry->me.Image;
  SubScreen->ID = SCREEN_BOOT;
  SubScreen->AnimeRun = GetAnime(SubScreen);
  AddMenuInfoLine(SubScreen, DevicePathToStr(Volume->DevicePath));
  
  if (gEmuVariableControl != NULL) {
    gEmuVariableControl->UninstallEmulation(gEmuVariableControl);
  }
  Status = FindBootOptionForFile (Entry->Volume->DeviceHandle,
                                  Entry->LoaderPath,
                                  NULL,
                                  NULL
                                  );
  if (gEmuVariableControl != NULL) {
    gEmuVariableControl->InstallEmulation(gEmuVariableControl);
  }
  
  if (Status == EFI_SUCCESS) {
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Remove Clover as UEFI boot option";
    SubEntry->me.Tag          = TAG_CLOVER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->DevicePathString = Entry->DevicePathString;
    SubEntry->Flags           = Entry->Flags;
    SubEntry->LoadOptions     = L"BO-REMOVE";
    SubEntry->LoaderType      = OSTYPE_VAR;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  } else {
    SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    SubEntry->me.Title        = L"Add Clover as UEFI boot option";
    SubEntry->me.Tag          = TAG_CLOVER;
    SubEntry->LoaderPath      = Entry->LoaderPath;
    SubEntry->Volume          = Entry->Volume;
    SubEntry->VolName         = Entry->VolName;
    SubEntry->DevicePath      = Entry->DevicePath;
    SubEntry->DevicePathString = Entry->DevicePathString;
    SubEntry->Flags           = Entry->Flags;
    SubEntry->LoadOptions     = L"BO-ADD";
    SubEntry->LoaderType      = OSTYPE_VAR;
    SubEntry->me.AtClick      = ActionEnter;
    AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  }
  
  SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  SubEntry->me.Title        = L"Remove all Clover boot options";
  SubEntry->me.Tag          = TAG_CLOVER;
  SubEntry->LoaderPath      = Entry->LoaderPath;
  SubEntry->Volume          = Entry->Volume;
  SubEntry->VolName         = Entry->VolName;
  SubEntry->DevicePath      = Entry->DevicePath;
  SubEntry->DevicePathString = Entry->DevicePathString;
  SubEntry->Flags           = Entry->Flags;
  SubEntry->LoadOptions     = L"BO-REMOVE-ALL";
  SubEntry->LoaderType      = OSTYPE_VAR;
  SubEntry->me.AtClick      = ActionEnter;
  AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  
  SubEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  SubEntry->me.Title        = L"Print all UEFI boot options to log";
  SubEntry->me.Tag          = TAG_CLOVER;
  SubEntry->LoaderPath      = Entry->LoaderPath;
  SubEntry->Volume          = Entry->Volume;
  SubEntry->VolName         = Entry->VolName;
  SubEntry->DevicePath      = Entry->DevicePath;
  SubEntry->DevicePathString = Entry->DevicePathString;
  SubEntry->Flags           = Entry->Flags;
  SubEntry->LoadOptions     = L"BO-PRINT";
  SubEntry->LoaderType      = OSTYPE_VAR;
  SubEntry->me.AtClick      = ActionEnter;
  AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  AddMenuEntry(SubScreen, &MenuEntryReturn);
  Entry->me.SubScreen = SubScreen;
  AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
  return Entry;
}

BOOLEAN isFirstRootUUID(REFIT_VOLUME *Volume)
{
    UINTN                   VolumeIndex;
    REFIT_VOLUME            *scanedVolume;

    for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
        scanedVolume = Volumes[VolumeIndex];
        
        if ( scanedVolume == Volume)
            return TRUE;
        
        if (CompareGuid(&scanedVolume->RootUUID, &Volume->RootUUID))
            return FALSE;

    }
    return TRUE;
}

VOID ScanLoader(VOID)
{
  UINTN                   VolumeIndex;
  REFIT_VOLUME            *Volume;
  CHAR16                  FileName[256];
  LOADER_ENTRY            *Entry;
  EFI_STATUS              Status;
  
  DBG("Scanning loaders...\n");
  
  for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
    Volume = Volumes[VolumeIndex];
    DBG("%2d: '%s'", VolumeIndex, Volume->VolName);
    if (Volume->RootDir == NULL) { // || Volume->VolName == NULL)
      DBG(" no file system\n", VolumeIndex);
      continue;
    }
    if (Volume->VolName == NULL) {
      Volume->VolName = L"Unknown";
    }
    
    // skip volume if its kind is configured as disabled
    if ((Volume->DiskKind == DISK_KIND_OPTICAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_OPTICAL)) ||
        (Volume->DiskKind == DISK_KIND_EXTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_EXTERNAL)) ||
        (Volume->DiskKind == DISK_KIND_INTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_INTERNAL)) ||
        (Volume->DiskKind == DISK_KIND_FIREWIRE && (GlobalConfig.DisableFlags & DISABLE_FLAG_FIREWIRE)))
    {
      DBG(" hidden\n");
      continue;
    }
    
    if (Volume->OSType == OSTYPE_HIDE) {
      DBG(" hidden\n");
      continue;
    }
    DBG("\n");
    
    // check for Mac OS X boot loader
    StrCpy(FileName, MACOSX_LOADER_PATH);
    if (FileExists(Volume->RootDir, FileName)) {
      //     Print(L"  - Mac OS X boot file found\n");
      Volume->BootType = BOOTING_BY_EFI;
        // check for Mac OS X Boot target
        Status = GetRootUUID(Volume);
        if(!EFI_ERROR(Status)) {
            Volume->OSType = OSTYPE_BOOT_OSX;
            if (isFirstRootUUID(Volume))
            Entry = AddLoaderEntry(FileName, NULL, NULL, L"Mac OS X", Volume, NULL, NULL, Volume->OSType, 0, 0, FALSE);
        }
        else {
            Entry = AddLoaderEntry(FileName, NULL, NULL, L"Mac OS X", Volume, NULL, NULL, Volume->OSType, 0, 0, FALSE);
        }
      //     continue; //boot MacOSX only
    }
//crazybirdy
    //============ add in begin ============
    // check for Mac OS X Install Data
    StrCpy(FileName, L"\\OS X Install Data\\boot.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->BootType = BOOTING_BY_EFI;
      Volume->OSType = OSTYPE_ML;
      Volume->OSIconName = L"cougar";
      Entry = AddLoaderEntry(FileName, NULL, NULL, L"OS X Install", Volume, NULL, NULL, Volume->OSType, 0, 0, FALSE);
      continue; //boot MacOSX only
    }
    // check for Mac OS X Install Data
    StrCpy(FileName, L"\\Mac OS X Install Data\\boot.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->BootType = BOOTING_BY_EFI;
      Volume->OSType = OSTYPE_LION;
      Volume->OSIconName = L"lion";
      Entry = AddLoaderEntry(FileName, NULL, NULL, L"Mac OS X Install", Volume, NULL, NULL, Volume->OSType, 0, 0, FALSE);
      continue; //boot MacOSX only
    }
    // dmazar: ML install from Lion to empty partition
    // starting (Lion) partition: /.IABootFiles with boot.efi and kernelcache,
    // and with DMGs used from Install app.
    // destination partition: just logs and config
    StrCpy(FileName, L"\\.IABootFiles\\boot.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->BootType = BOOTING_BY_EFI;
      Volume->OSType = OSTYPE_ML;
      Volume->OSIconName = L"cougar";
      Entry = AddLoaderEntry(FileName, NULL, NULL, L"OS X Install", Volume, NULL, NULL, Volume->OSType, 0, 0, FALSE);
      //continue; //boot MacOSX only
    }
    //============ add in end ============
    
    // check for Mac OS X Recovery Boot
    StrCpy(FileName,  L"\\com.apple.recovery.boot\\boot.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->BootType = BOOTING_BY_EFI;
      Volume->OSType = OSTYPE_RECOVERY;
      Volume->OSIconName = L"mac";
      Entry = AddLoaderEntry(FileName, NULL, NULL, L"Recovery", Volume, NULL, NULL, Volume->OSType, 0, 0, FALSE);
      continue; //boot recovery only
    }
          
    // Sometimes, on some systems (HP UEFI, if Win is installed first)
    // it is needed to get rid of bootmgfw.efi to allow starting of
    // Clover as /efi/boot/bootx64.efi from HD. We can do that by renaming
    // bootmgfw.efi to bootmgfw-orig.efi
    StrCpy(FileName, L"\\EFI\\Microsoft\\Boot\\bootmgfw-orig.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      //     Print(L"  - Microsoft boot menu found\n");
      Volume->OSType = OSTYPE_WINEFI;
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"Microsoft EFI boot menu", Volume, NULL, NULL, OSTYPE_WINEFI, 0, 0, FALSE);
      //     continue;
      
    } else {
      // check for Microsoft boot loader/menu
      // If there is bootmgfw-orig.efi, then do not check for bootmgfw.efi
      // since on some systems this will actually be CloverX64.efi
      // renamed to bootmgfw.efi
      StrCpy(FileName, L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi");
      if (FileExists(Volume->RootDir, FileName)) {
        //     Print(L"  - Microsoft boot menu found\n");
        Volume->OSType = OSTYPE_WINEFI;
        Volume->BootType = BOOTING_BY_EFI;
        Entry = AddLoaderEntry(FileName, L"", NULL, L"Microsoft EFI boot menu", Volume, NULL, NULL, OSTYPE_WINEFI, 0, 0, FALSE);
        //     continue;
      }
      
    }
    
    // check for Microsoft boot loader/menu
    StrCpy(FileName, L"\\bootmgr.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      //     Print(L"  - Microsoft boot menu found\n");
      Volume->OSType = OSTYPE_WINEFI;
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"Microsoft EFI boot menu", Volume, NULL, NULL, OSTYPE_WINEFI, 0, 0, FALSE);
      continue;
    }
 
    // check for Microsoft boot loader/menu on CDROM
    StrCpy(FileName, L"\\EFI\\MICROSOFT\\BOOT\\cdboot.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      //     Print(L"  - Microsoft boot menu found\n");
      Volume->OSType = OSTYPE_WINEFI;
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"Microsoft EFI boot menu", Volume, NULL, NULL, OSTYPE_WINEFI, 0, 0, FALSE);
      continue;
    }
    

    // check for grub boot loader/menu
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\grub\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\grub\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSType = OSTYPE_LIN;
      Volume->OSIconName = L"grub,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"Grub EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }

    // check for Gentoo boot loader/menu
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\Gentoo\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\Gentoo\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"gentoo,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"Gentoo EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }
    
    // check for Gentoo kernel
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\Gentoo\\kernelx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\Gentoo\\kernel.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"gentoo,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"Gentoo EFI kernel", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }
    
    // check for Redhat boot loader/menu
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\RedHat\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\RedHat\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"redhat,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"RedHat EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }
    
    // check for Ubuntu boot loader/menu
#if defined(MDE_CPU_X64)    
    StrCpy(FileName, L"\\EFI\\Ubuntu\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\Ubuntu\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"ubuntu,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"Ubuntu EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }
    
    // check for kubuntu boot loader/menu
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\kubuntu\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\kubuntu\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"kubuntu,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"kubuntu EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }
    
    // check for Linux Mint boot loader/menu
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\Linuxmint\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\Linuxmint\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"mint,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"Linux Mint EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }
    
    // check for Fedora boot loader/menu
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\Fedora\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\Fedora\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"fedora,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"Fedora EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }
    
    // check for OpenSuse boot loader/menu
    StrCpy(FileName, L"\\EFI\\SuSe\\elilo.efi");
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"suse,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"OpenSuse EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }

#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\opensuse\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\opensuse\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"suse,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"OpenSuse EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }
    
    // check for archlinux boot loader/menu
#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\arch\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\arch\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"arch,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"ArchLinux EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }

#if defined(MDE_CPU_X64)
    StrCpy(FileName, L"\\EFI\\arch_grub\\grubx64.efi");
#else
    StrCpy(FileName, L"\\EFI\\arch_grub\\grub.efi");
#endif
    if (FileExists(Volume->RootDir, FileName)) {
      Volume->OSIconName = L"arch,linux";
      Volume->BootType = BOOTING_BY_EFI;
      Entry = AddLoaderEntry(FileName, L"", NULL, L"ArchLinux EFI boot menu", Volume, NULL, NULL, OSTYPE_LIN, 0, 0, FALSE);
    }

#if defined(MDE_CPU_X64)
      StrCpy(FileName, L"\\EFI\\BOOT\\BOOTX64.efi");
#else      
      StrCpy(FileName, L"\\EFI\\BOOT\\BOOTIA32.efi");
#endif
 //     DBG("search for  optical UEFI\n");
      if (FileExists(Volume->RootDir, FileName) && Volume->DiskKind == DISK_KIND_OPTICAL) {
          //      Volume->OSType = OSTYPE_VAR;
          Volume->BootType = BOOTING_BY_EFI;
          AddLoaderEntry(FileName, L"", NULL, L"UEFI optical", Volume, NULL, NULL, OSTYPE_VAR, 0, 0, FALSE);
          //      continue;
      }
      
 //     DBG("search for internal UEFI\n");
      if (FileExists(Volume->RootDir, FileName) && Volume->DiskKind == DISK_KIND_INTERNAL) {
          //      Volume->OSType = OSTYPE_VAR;
          Volume->BootType = BOOTING_BY_EFI;
          AddLoaderEntry(FileName, L"", NULL, L"UEFI internal", Volume, NULL, NULL, OSTYPE_VAR, 0, 0, FALSE);
          //      continue;
      }

  //    DBG("search for external UEFI\n");
      if (FileExists(Volume->RootDir, FileName) && Volume->DiskKind == DISK_KIND_EXTERNAL) {
          //      Volume->OSType = OSTYPE_VAR;
          Volume->BootType = BOOTING_BY_EFI;
          AddLoaderEntry(FileName, L"", NULL, L"UEFI external", Volume, NULL, NULL, OSTYPE_VAR, 0, 0, FALSE);
          //      continue;
    }
  }
}

static CHAR16 *LinuxEntryPaths[] = {
   L"\\EFI\\SuSe\\elilo.efi",
#if defined(MDE_CPU_X64)
   L"\\EFI\\grub\\grubx64.efi",
   L"\\EFI\\Gentoo\\grubx64.efi",
   L"\\EFI\\Gentoo\\kernelx64.efi",
   L"\\EFI\\RedHat\\grubx64.efi",
   L"\\EFI\\ubuntu\\grubx64.efi",
   L"\\EFI\\kubuntu\\grubx64.efi",
   L"\\EFI\\LinuxMint\\grubx64.efi",
   L"\\EFI\\Fedora\\grubx64.efi",
   L"\\EFI\\opensuse\\grubx64.efi",
   L"\\EFI\\arch\\grubx64.efi",
   L"\\EFI\\arch_grub\\grubx64.efi",
#else
   L"\\EFI\\grub\\grub.efi",
   L"\\EFI\\Gentoo\\grub.efi",
   L"\\EFI\\Gentoo\\kernel.efi",
   L"\\EFI\\RedHat\\grub.efi",
   L"\\EFI\\ubuntu\\grub.efi",
   L"\\EFI\\kubuntu\\grub.efi",
   L"\\EFI\\LinuxMint\\grub.efi",
   L"\\EFI\\Fedora\\grub.efi",
   L"\\EFI\\opensuse\\grub.efi",
   L"\\EFI\\arch\\grub.efi",
   L"\\EFI\\arch_grub\\grub.efi",
#endif
};
static const UINTN LinuxEntryPathsCount = (sizeof(LinuxEntryPaths) / sizeof(CHAR16 *));

static UINT8 GetOSTypeFromPath(IN CHAR16 *Path, IN UINT8 OSType)
{
   if (Path == NULL) {
     return (OSType == 0) ? OSTYPE_OTHER : OSType;
   }
   if ((StriCmp(Path, MACOSX_LOADER_PATH) == 0) ||
       (StriCmp(Path, L"\\OS X Install Data\\boot.efi") == 0) ||
       (StriCmp(Path, L"\\Mac OS X Install Data\\boot.efi") == 0) ||
       (StriCmp(Path, L"\\.IABootFiles\\boot.efi") == 0)) {
     return OSType;
   } else if (StriCmp(Path, L"\\com.apple.recovery.boot\\boot.efi") == 0) {
     return OSTYPE_RECOVERY;
   } else if ((StriCmp(Path, L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi") == 0) ||
              (StriCmp(Path, L"\\bootmgr.efi") == 0) ||
              (StriCmp(Path, L"\\EFI\\MICROSOFT\\BOOT\\cdboot.efi") == 0)) {
     return OSTYPE_WINEFI;
   } else {
     UINTN Index = 0;
     while (Index < LinuxEntryPathsCount) {
       if (StriCmp(Path, LinuxEntryPaths[Index]) == 0) {
         return OSTYPE_LIN;
       }
       ++Index;
     }
   }
   return (OSType == 0) ? OSTYPE_OTHER : OSType;
}

static EG_IMAGE *LoadBuiltinIcon(IN CHAR16 *IconName)
{
   return NULL;
}

static LOADER_ENTRY *AddCustomEntry(IN UINTN                CustomIndex,
                                    IN CHAR16              *CustomPath,
                                    IN CUSTOM_LOADER_ENTRY *Custom)
{
  UINTN                VolumeIndex;
  REFIT_VOLUME        *Volume;
  CUSTOM_LOADER_ENTRY *CustomSubEntry;
  LOADER_ENTRY        *Entry = NULL, *SubEntry;
  EG_IMAGE            *Image, *DriveImage;
  EFI_GUID            *Guid = NULL;
  CHAR16              *Path;
  UINT64               VolumeSize;
  UINT8                OSType;

  if ((CustomPath == NULL) && !OSTYPE_IS_OSX_INSTALLER(Custom->Type)) {
    DBG("Custom entry %d skipped because it didn't have a path or valid type.\n", CustomIndex);
    return NULL;
  }
  if (OSFLAG_ISSET(Custom->Flags, OSFLAG_DISABLED)) {
    DBG("Custom entry %d skipped because it is disabled.\n", CustomIndex);
    return NULL;
  }
  if (!gSettings.ShowHiddenEntries && OSFLAG_ISSET(Custom->Flags, OSFLAG_HIDDEN)) {
    DBG("Custom entry %d skipped because it is hidden.\n", CustomIndex);
    return NULL;
  }
  if (Custom->Volume) {
    if (Custom->Title) {
      if (CustomPath) {
        DBG("Custom entry %d \"%s\" \"%s\" \"%s\" (%d) 0x%X matching \"%s\" ...\n", CustomIndex, Custom->Title, CustomPath, ((Custom->Options != NULL) ? Custom->Options : L""), Custom->Type, Custom->Flags, Custom->Volume);
      } else {
        DBG("Custom entry %d \"%s\" \"%s\" (%d) 0x%X matching \"%s\" ...\n", CustomIndex, Custom->Title, ((Custom->Options != NULL) ? Custom->Options : L""), Custom->Type, Custom->Flags, Custom->Volume);
      }
    } else if (CustomPath) {
      DBG("Custom entry %d \"%s\" \"%s\" (%d) 0x%X matching \"%s\" ...\n", CustomIndex, CustomPath, ((Custom->Options != NULL) ? Custom->Options : L""), Custom->Type, Custom->Flags, Custom->Volume);
    } else {
      DBG("Custom entry %d \"%s\" (%d) 0x%X matching \"%s\" ...\n", CustomIndex, ((Custom->Options != NULL) ? Custom->Options : L""), Custom->Type, Custom->Flags, Custom->Volume);
    }
  } else if (CustomPath) {
    DBG("Custom entry %d \"%s\" \"%s\" (%d) 0x%X matching all volumes ...\n", CustomIndex, CustomPath, ((Custom->Options != NULL) ? Custom->Options : L""), Custom->Type, Custom->Flags);
  } else {
    DBG("Custom entry %d \"%s\" (%d) 0x%X matching all volumes ...\n", CustomIndex, ((Custom->Options != NULL) ? Custom->Options : L""), Custom->Type, Custom->Flags);
  }
  for (VolumeIndex = 0; VolumeIndex < VolumesCount; ++VolumeIndex) {
    Volume = Volumes[VolumeIndex];

    if (Volume->RootDir == NULL) {
      continue;
    }
    if (Volume->VolName == NULL) {
      Volume->VolName = L"Unknown";
    }

    DBG("   Checking volume \"%s\" (%s) ... ", Volume->VolName, Volume->DevicePathString);

    // skip volume if its kind is configured as disabled
    if ((Volume->DiskKind == DISK_KIND_OPTICAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_OPTICAL)) ||
        (Volume->DiskKind == DISK_KIND_EXTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_EXTERNAL)) ||
        (Volume->DiskKind == DISK_KIND_INTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_INTERNAL)) ||
        (Volume->DiskKind == DISK_KIND_FIREWIRE && (GlobalConfig.DisableFlags & DISABLE_FLAG_FIREWIRE)))
    {
      DBG("skipped because media is disabled\n");
      continue;
    }

    if (Custom->VolumeType != 0) {
      if ((Volume->DiskKind == DISK_KIND_OPTICAL && ((Custom->VolumeType & DISABLE_FLAG_OPTICAL) == 0)) ||
          (Volume->DiskKind == DISK_KIND_EXTERNAL && ((Custom->VolumeType & DISABLE_FLAG_EXTERNAL) == 0)) ||
          (Volume->DiskKind == DISK_KIND_INTERNAL && ((Custom->VolumeType & DISABLE_FLAG_INTERNAL) == 0)) ||
          (Volume->DiskKind == DISK_KIND_FIREWIRE && ((Custom->VolumeType & DISABLE_FLAG_FIREWIRE) == 0))) {
        DBG("skipped because media is ignored\n");
        continue;
      }
    }

    if (Volume->OSType == OSTYPE_HIDE) {
      DBG("skipped because volume is hidden\n");
      continue;
    }

    // Check volume for installer
    Path = CustomPath;
    if ((Path == NULL) && (Volume->RootDir != NULL) && OSTYPE_IS_OSX_INSTALLER(Custom->Type)) {
      static CHAR16 *InstallerPath[] = {
        L"\\Mac OS X Install Data\\boot.efi",
        L"\\OS X Install Data\\boot.efi",
        L"\\.IABootFiles\\boot.efi"
      };
      static UINTN InstallerPathCount = (sizeof(InstallerPath) / sizeof(CHAR16 *));
      UINTN InstallerIndex = 0;
      while (InstallerIndex < InstallerPathCount) {
        if (FileExists(Volume->RootDir, InstallerPath[InstallerIndex])) {
          Path = InstallerPath[InstallerIndex];
          break;
        }
        ++InstallerIndex;
      }
    }
    if (Path == NULL) {
      DBG("skipped because volume is not installation media\n", Custom->Type);
      continue;
    }
    // Check for exact volume matches
    OSType = (Custom->Type == 0) ? GetOSTypeFromPath(Path, Volume->OSType) : Custom->Type;
    if (Custom->Volume) {
      
      if ((StrStr(Volume->DevicePathString, Custom->Volume) == NULL) &&
          ((Volume->VolName == NULL) || (StrStr(Volume->VolName, Custom->Volume) == NULL))) {
        DBG("skipped\n");
        continue;
      }
      // Check if the volume should be of certain os type
      if ((Custom->Type != 0) && (Volume->OSType != 0) && !OSTYPE_COMPARE(OSType, Volume->OSType)) {
        DBG("skipped because wrong type (%d != %d)\n", OSType, Volume->OSType);
        continue;
      }
    } else if ((Custom->Type != 0) && (Volume->OSType != 0) && !OSTYPE_COMPARE(OSType, Volume->OSType)) {
      DBG("skipped because wrong type (%d != %d)\n", OSType, Volume->OSType);
      continue;
    }
    // Check the volume is readable and the entry exists on the volume
    if (Volume->RootDir == NULL) {
      DBG("skipped because filesystem is not readable\n");
      continue;
    }
    if (!FileExists(Volume->RootDir, Path)) {
      DBG("skipped because path does not exist\n");
      continue;
    }
    // Change to custom image if needed
    Image = Custom->Image;
    if ((Image == NULL) && Custom->ImagePath) {
      Image = egLoadImage(Volume->RootDir, Custom->ImagePath, TRUE);
      if (Image == NULL) {
        Image = egLoadImage(ThemeDir, Custom->ImagePath, TRUE);
        if (Image == NULL) {
          Image = egLoadImage(SelfDir, Custom->ImagePath, TRUE);
          if (Image == NULL) {
            Image = egLoadImage(SelfRootDir, Custom->ImagePath, TRUE);
          }
        }
      }
    }
    if (Image == NULL) {
      Image = LoadOSIcon(Custom->ImagePath, L"unknown", 128, FALSE, FALSE);
    }
    // Change to custom drive image if needed
    DriveImage = Custom->DriveImage;
    if ((DriveImage == NULL) && Custom->DriveImagePath) {
      DriveImage = egLoadImage(Volume->RootDir, Custom->DriveImagePath, TRUE);
      if (DriveImage == NULL) {
        DriveImage = egLoadImage(ThemeDir, Custom->DriveImagePath, TRUE);
        if (DriveImage == NULL) {
          DriveImage = egLoadImage(SelfDir, Custom->DriveImagePath, TRUE);
          if (DriveImage == NULL) {
            DriveImage = egLoadImage(SelfRootDir, Custom->DriveImagePath, TRUE);
            if (DriveImage == NULL) {
              DriveImage = LoadBuiltinIcon(Custom->DriveImagePath);
            }
          }
        }
      }
    }
    if (Image == NULL) {
      Image = LoadOSIcon(Custom->ImagePath, L"unknown", 128, FALSE, FALSE);
    }
    // Change custom drive image if needed
    // Update volume boot type
    Volume->BootType = BOOTING_BY_EFI;
    DBG("match!\n");
    // Create a legacy entry for this volume
    if (OSFLAG_ISUNSET(Custom->Flags, OSFLAG_NODEFAULTMENU)) {
      Entry = AddLoaderEntry(Path, Custom->Options, Custom->FullTitle, Custom->Title, Volume, Image, DriveImage, OSType, Custom->Flags, Custom->Hotkey, TRUE);
    } else {
      Entry = CreateLoaderEntry(Path, Custom->Options, Custom->FullTitle, Custom->Title, Volume, Image, DriveImage, OSType, Custom->Flags, Custom->Hotkey, TRUE);
      if (Entry) {
        if (Custom->SubEntries) {
          // Add subscreen
          REFIT_MENU_SCREEN *SubScreen = AllocateZeroPool(sizeof(REFIT_MENU_SCREEN));
          if (SubScreen) {
            SubScreen->Title = PoolPrint(L"Boot Options for %s on %s", (Custom->Title != NULL) ? Custom->Title : Path, Entry->VolName);
            SubScreen->TitleImage = Entry->me.Image;
            SubScreen->ID = OSType + 20;
            SubScreen->AnimeRun = GetAnime(SubScreen);
            VolumeSize = RShiftU64(MultU64x32(Volume->BlockIO->Media->LastBlock, Volume->BlockIO->Media->BlockSize), 20);
            AddMenuInfoLine(SubScreen, PoolPrint(L"Volume size: %dMb", VolumeSize));
            AddMenuInfoLine(SubScreen, DevicePathToStr(Entry->DevicePath));
            Guid = FindGPTPartitionGuidInDevicePath(Volume->DevicePath);
            if (Guid) {
              CHAR8 *GuidStr = AllocateZeroPool(50);
              AsciiSPrint(GuidStr, 50, "%g", Guid);
              AddMenuInfoLine(SubScreen, PoolPrint(L"UUID: %a", GuidStr));
              FreePool(GuidStr);
            }
            // Create sub entries
            for (CustomSubEntry = Custom->SubEntries; CustomSubEntry; CustomSubEntry = CustomSubEntry->Next) {
              SubEntry = AddCustomEntry(CustomIndex, (CustomSubEntry->Path != NULL) ? CustomSubEntry->Path : Path, CustomSubEntry);
              if (SubEntry) {
                AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
              }
            }
            AddMenuEntry(SubScreen, &MenuEntryReturn);
            Entry->me.SubScreen = SubScreen;
          }
        }
        AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
      }
    }
  }
  return Entry;
}

// Add custom entries
static VOID AddCustomEntries(VOID)
{
  CUSTOM_LOADER_ENTRY *Custom;
  UINTN                i = 0;

  DBG("Custom entries start\n");
  // Traverse the custom entries
  for (Custom = gSettings.CustomEntries; Custom; ++i, Custom = Custom->Next) {
    if ((Custom->Path == NULL) && (Custom->Type != 0)) {
      if (OSTYPE_IS_OSX(Custom->Type)) {
        AddCustomEntry(i, MACOSX_LOADER_PATH, Custom);
      } else if (OSTYPE_IS_OSX_RECOVERY(Custom->Type)) {
        AddCustomEntry(i, L"\\com.apple.recovery.boot\\boot.efi", Custom);
      } else if (OSTYPE_IS_WINDOWS(Custom->Type)) {
        AddCustomEntry(i, L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi", Custom);
      } else if (OSTYPE_IS_LINUX(Custom->Type)) {
         UINTN Index = 0;
        while (Index < LinuxEntryPathsCount) {
          AddCustomEntry(i, LinuxEntryPaths[Index++], Custom);
        }
      } else {
        AddCustomEntry(i, Custom->Path, Custom);
      }
    } else {
      AddCustomEntry(i, Custom->Path, Custom);
    }
  }
  DBG("Custom entries finish\n");
}

#define MAX_DISCOVERED_PATHS (16)
//#define PREBOOT_LOG L"EFI\\CLOVER\\misc\\preboot.log"

static VOID StartLegacy(IN LEGACY_ENTRY *Entry)
{
    EFI_STATUS          Status = EFI_UNSUPPORTED;
    EG_IMAGE            *BootLogoImage;
//    UINTN               ErrorInStep = 0;
//    EFI_DEVICE_PATH     *DiscoveredPathList[MAX_DISCOVERED_PATHS];

    // Unload EmuVariable before booting legacy.
    // This is not needed in most cases, but it seems to interfere with legacy OS
    // booted on some UEFI bioses, such as Phoenix UEFI 2.0
    if (gEmuVariableControl != NULL) {
      gEmuVariableControl->UninstallEmulation(gEmuVariableControl);
    }  

    SetStartupDiskVolume(Entry->Volume, NULL);
  
    egClearScreen(&DarkBackgroundPixel);
    BeginExternalScreen(TRUE, L"Booting Legacy OS");
    
    BootLogoImage = LoadOSIcon(Entry->Volume->OSIconName, L"legacy", 128, TRUE, TRUE);
    if (BootLogoImage != NULL)
        BltImageAlpha(BootLogoImage,
                      (UGAWidth  - BootLogoImage->Width) >> 1,
                      (UGAHeight - BootLogoImage->Height) >> 1,
                      &StdBackgroundPixel, 16);
  
/*    Status = ExtractLegacyLoaderPaths(DiscoveredPathList, MAX_DISCOVERED_PATHS, LegacyLoaderList);
    if (!EFI_ERROR(Status)) {
      Status = StartEFIImageList(DiscoveredPathList, Entry->LoadOptions, NULL, L"legacy loader", &ErrorInStep, NULL);
    } */
 //   if (EFI_ERROR(Status)) {
      //try my LegacyBoot
      switch (Entry->Volume->BootType) {
        case BOOTING_BY_CD:
          Status = bootElTorito(Entry->Volume);
          break;
        case BOOTING_BY_MBR:
          Status = bootMBR(Entry->Volume);
          break;
        case BOOTING_BY_PBR:
          if (StrCmp(gSettings.LegacyBoot, L"LegacyBiosDefault") == 0) {
            Status = bootLegacyBiosDefault(Entry->Volume);
          } else if (StrCmp(gSettings.LegacyBoot, L"PBRtest") == 0) {
            Status = bootPBRtest(Entry->Volume);
          } else {
            // default
            Status = bootPBR(Entry->Volume);
          }
          break;
        default:
          break;
      }
      CheckError(Status, L"while LegacyBoot");
//      if (0 && Entry->Volume->IsMbrPartition && !Entry->Volume->HasBootCode)
//         ActivateMbrPartition(Entry->Volume->WholeDiskBlockIO, Entry->Volume->MbrPartitionIndex);

/*        if (ErrorInStep == 1)
            Print(L"\nPlease make sure that you have the latest firmware update installed.\n");
        else if (ErrorInStep == 3)
            Print(L"\nThe firmware refused to boot from the selected volume. Note that external\n"
                  L"hard drives are not well-supported by Apple's firmware for legacy OS booting.\n");
 */
//    }
    FinishExternalScreen();
}

static LEGACY_ENTRY * AddLegacyEntry(IN CHAR16 *FullTitle, IN CHAR16 *LoaderTitle, IN REFIT_VOLUME *Volume, IN EG_IMAGE *Image, IN EG_IMAGE *DriveImage, IN CHAR16 Hotkey, IN BOOLEAN CustomEntry)
{
  LEGACY_ENTRY      *Entry, *SubEntry;
  REFIT_MENU_SCREEN *SubScreen;
  CHAR16            *VolDesc;
  CHAR16             ShortcutLetter = 0;
  INTN               i;

  if (Volume == NULL) {
     return NULL;
  }

  // Ignore this loader if it's device path is already present in another loader
  if (MainMenu.Entries) {
    for (i = 0; i < MainMenu.EntryCount; ++i) {
      REFIT_MENU_ENTRY *MainEntry = MainMenu.Entries[i];
      // Only want legacy
      if (MainEntry && (MainEntry->Tag == TAG_LEGACY)) {
        LOADER_ENTRY *Loader = (LOADER_ENTRY *)MainEntry;
        if (StriCmp(Loader->DevicePathString, Volume->DevicePathString) == 0) {
          return NULL;
        }
      }
    }
  }

  // If this isn't a custom entry make sure it's not hidden by a custom entry
  if (!CustomEntry) {
    CUSTOM_LEGACY_ENTRY *Custom = gSettings.CustomLegacy;
    while (Custom) {
      if (OSFLAG_ISSET(Custom->Flags, OSFLAG_DISABLED) ||
          (OSFLAG_ISSET(Custom->Flags, OSFLAG_HIDDEN) && !gSettings.ShowHiddenEntries)) {
        if (Custom->Volume) {
          if ((StrStr(Volume->DevicePathString, Custom->Volume) == NULL) &&
              ((Volume->VolName == NULL) || (StrStr(Volume->VolName, Custom->Volume) == NULL))) {
            if (Custom->Type != 0) {
              if (Custom->Type == Volume->OSType) {
                return NULL;
              }
            } else {
              return NULL;
            }
          }
        } else if (Custom->Type != 0) {
          if (Custom->Type == Volume->OSType) {
            return NULL;
          }
        }
      }
      Custom = Custom->Next;
    }
  }

  if (LoaderTitle == NULL) {
    if (Volume->OSName != NULL) {
      LoaderTitle = Volume->OSName;
      if (LoaderTitle[0] == 'W' || LoaderTitle[0] == 'L')
        ShortcutLetter = LoaderTitle[0];
    } else
      LoaderTitle = L"Legacy OS";
  }
  if (Volume->VolName != NULL)
    VolDesc = Volume->VolName;
  else
    VolDesc = (Volume->DiskKind == DISK_KIND_OPTICAL) ? L"CD" : L"HD";
  
  // prepare the menu entry
  Entry = AllocateZeroPool(sizeof(LEGACY_ENTRY));
  if (FullTitle) {
    Entry->me.Title = EfiStrDuplicate(FullTitle);
  } else {
    Entry->me.Title = PoolPrint(L"Boot %s from %s", LoaderTitle, VolDesc);
  }
  Entry->me.Tag          = TAG_LEGACY;
  Entry->me.Row          = 0;
  Entry->me.ShortcutLetter = (Hotkey == 0) ? ShortcutLetter : Hotkey;
  if (Image) {
    Entry->me.Image = Image;
  } else {
    Entry->me.Image = LoadOSIcon(Volume->OSIconName, L"legacy", 128, FALSE, TRUE);
  }
  Entry->me.DriveImage = (DriveImage != NULL) ? DriveImage : ScanVolumeDefaultIcon(Volume);
  //  DBG("HideBadges=%d Volume=%s\n", GlobalConfig.HideBadges, Volume->VolName);
  //  DBG("Title=%s OSName=%s OSIconName=%s\n", LoaderTitle, Volume->OSName, Volume->OSIconName);
  
  //actions
  Entry->me.AtClick = ActionSelect;
  Entry->me.AtDoubleClick = ActionEnter;
  Entry->me.AtRightClick = ActionDetails;

  if (GlobalConfig.HideBadges & HDBADGES_SHOW) {
    if (GlobalConfig.HideBadges & HDBADGES_SWAP) {
      Entry->me.BadgeImage = egCopyScaledImage(Entry->me.DriveImage, 4);
    } else {
      Entry->me.BadgeImage = egCopyScaledImage(Entry->me.Image, 8);
    }
  }

  Entry->Volume           = Volume;
  Entry->DevicePathString = Volume->DevicePathString;
  Entry->LoadOptions      = (Volume->DiskKind == DISK_KIND_OPTICAL) ? L"CD" :
                            ((Volume->DiskKind == DISK_KIND_EXTERNAL) ? L"USB" : L"HD");
  
  // create the submenu
  SubScreen = AllocateZeroPool(sizeof(REFIT_MENU_SCREEN));
  SubScreen->Title = PoolPrint(L"Boot Options for %s on %s", LoaderTitle, VolDesc);
  SubScreen->TitleImage = Entry->me.Image;
  SubScreen->AnimeRun = GetAnime(SubScreen);;
  
  // default entry
  SubEntry = AllocateZeroPool(sizeof(LEGACY_ENTRY));
  SubEntry->me.Title         = PoolPrint(L"Boot %s", LoaderTitle);
  SubEntry->me.Tag           = TAG_LEGACY;
  SubEntry->Volume           = Entry->Volume;
  SubEntry->DevicePathString = Entry->DevicePathString;
  SubEntry->LoadOptions      = Entry->LoadOptions;
  SubEntry->me.AtClick       = ActionEnter;
  AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
  
  AddMenuEntry(SubScreen, &MenuEntryReturn);
  Entry->me.SubScreen = SubScreen;
  AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
  DBG(" added '%s' OSType=%d Icon=%s\n", Entry->me.Title, Volume->OSType, Volume->OSIconName);
  return Entry;
}

static VOID ScanLegacy(VOID)
{
    UINTN                   VolumeIndex, VolumeIndex2;
    BOOLEAN                 ShowVolume, HideIfOthersFound;
    REFIT_VOLUME            *Volume;
    
    DBG("Scanning legacy ...\n");
    
    for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
        Volume = Volumes[VolumeIndex];
        DBG("%2d: '%s' (%s)", VolumeIndex, Volume->VolName, Volume->OSIconName);

#if 0 // REFIT_DEBUG > 0
        DBG(" %d %s\n  %d %d %s %d %s\n",
              VolumeIndex, DevicePathToStr(Volume->DevicePath),
              Volume->DiskKind, Volume->MbrPartitionIndex,
              Volume->IsAppleLegacy ? L"AL" : L"--", Volume->HasBootCode,
              Volume->VolName ? Volume->VolName : L"(no name)");
#endif
        
        // skip volume if its kind is configured as disabled
        if ((Volume->DiskKind == DISK_KIND_OPTICAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_OPTICAL)) ||
            (Volume->DiskKind == DISK_KIND_EXTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_EXTERNAL)) ||
            (Volume->DiskKind == DISK_KIND_INTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_INTERNAL)) ||
            (Volume->DiskKind == DISK_KIND_FIREWIRE && (GlobalConfig.DisableFlags & DISABLE_FLAG_FIREWIRE)))
        {
          DBG(" hidden\n");
          continue;
        }
      
        if ((Volume->BootType == BOOTING_BY_EFI) ||
            (Volume->BootType == BOOTING_BY_BOOTEFI)) {
          DBG(" not legacy\n");
          continue;
        }
      
        ShowVolume = FALSE;
        HideIfOthersFound = FALSE;
        if (Volume->IsAppleLegacy) {
            ShowVolume = TRUE;
            HideIfOthersFound = TRUE;
        } else if (Volume->HasBootCode) {
            ShowVolume = TRUE;
            //DBG("Volume %d will be shown BlockIo=%x WholeIo=%x\n",
            //  VolumeIndex, Volume->BlockIO, Volume->WholeDiskBlockIO);
            if ((Volume->WholeDiskBlockIO == 0) &&
                Volume->BlockIOOffset == 0 /* &&
                Volume->OSName == NULL */)
                // this is a whole disk (MBR) entry; hide if we have entries for partitions
                HideIfOthersFound = TRUE;
        }
        if (HideIfOthersFound) {          
          // check for other bootable entries on the same disk
          //if PBR exists then Hide MBR
            for (VolumeIndex2 = 0; VolumeIndex2 < VolumesCount; VolumeIndex2++) {
                if (VolumeIndex2 != VolumeIndex && 
                    Volumes[VolumeIndex2]->HasBootCode && 
                    Volumes[VolumeIndex2]->WholeDiskBlockIO == Volume->BlockIO){
                    ShowVolume = FALSE;
     //             DBG("PBR volume at index %d\n", VolumeIndex2);
                  break;
                }
            }
        }
        
        if (ShowVolume && (Volume->OSType != OSTYPE_HIDE)){
            DBG(" add legacy\n");
            AddLegacyEntry(NULL, NULL, Volume, NULL, NULL, 0, FALSE);
        } else {
          DBG(" hidden\n");
        }
    }
}

// Add custom legacy
static VOID AddCustomLegacy(VOID)
{
  UINTN                VolumeIndex, VolumeIndex2;
  BOOLEAN              ShowVolume, HideIfOthersFound;
  REFIT_VOLUME        *Volume;
  CUSTOM_LEGACY_ENTRY *Custom;
  EG_IMAGE            *Image, *DriveImage;
  UINTN                i = 0;

  DBG("Custom legacy start\n");
  // Traverse the custom entries
  for (Custom = gSettings.CustomLegacy; Custom; ++i, Custom = Custom->Next) {
    if (OSFLAG_ISSET(Custom->Flags, OSFLAG_DISABLED)) {
      DBG("Custom legacy %d skipped because it is disabled.\n", i);
      continue;
    }
    if (!gSettings.ShowHiddenEntries && OSFLAG_ISSET(Custom->Flags, OSFLAG_HIDDEN)) {
      DBG("Custom legacy %d skipped because it is hidden.\n", i);
      continue;
    }
    if (Custom->Volume) {
      DBG("Custom legacy %d matching \"%s\" ...\n", i, Custom->Volume);
    }
    for (VolumeIndex = 0; VolumeIndex < VolumesCount; ++VolumeIndex) {
      Volume = Volumes[VolumeIndex];

      DBG("   Checking volume \"%s\" (%s) ... ", Volume->VolName, Volume->DevicePathString);

      // skip volume if its kind is configured as disabled
      if ((Volume->DiskKind == DISK_KIND_OPTICAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_OPTICAL)) ||
          (Volume->DiskKind == DISK_KIND_EXTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_EXTERNAL)) ||
          (Volume->DiskKind == DISK_KIND_INTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_INTERNAL)) ||
          (Volume->DiskKind == DISK_KIND_FIREWIRE && (GlobalConfig.DisableFlags & DISABLE_FLAG_FIREWIRE)))
      {
         DBG("skipped because media is disabled\n");
        continue;
      }

      if (Custom->VolumeType != 0) {
        if ((Volume->DiskKind == DISK_KIND_OPTICAL && ((Custom->VolumeType & DISABLE_FLAG_OPTICAL) == 0)) ||
            (Volume->DiskKind == DISK_KIND_EXTERNAL && ((Custom->VolumeType & DISABLE_FLAG_EXTERNAL) == 0)) ||
            (Volume->DiskKind == DISK_KIND_INTERNAL && ((Custom->VolumeType & DISABLE_FLAG_INTERNAL) == 0)) ||
            (Volume->DiskKind == DISK_KIND_FIREWIRE && ((Custom->VolumeType & DISABLE_FLAG_FIREWIRE) == 0))) {
          DBG("skipped because media is ignored\n");
          continue;
        }
      }

      if ((Volume->BootType == BOOTING_BY_EFI) ||
          (Volume->BootType == BOOTING_BY_BOOTEFI)) {
        DBG("skipped because volume is not legacy bootable\n");
        continue;
      }

      ShowVolume = FALSE;
      HideIfOthersFound = FALSE;
      if (Volume->IsAppleLegacy) {
        ShowVolume = TRUE;
        HideIfOthersFound = TRUE;
      } else if (Volume->HasBootCode) {
        ShowVolume = TRUE;
        if ((Volume->WholeDiskBlockIO == 0) &&
             Volume->BlockIOOffset == 0) {
          // this is a whole disk (MBR) entry; hide if we have entries for partitions
          HideIfOthersFound = TRUE;
        }
      }
      if (HideIfOthersFound) {
        // check for other bootable entries on the same disk
        //if PBR exists then Hide MBR
        for (VolumeIndex2 = 0; VolumeIndex2 < VolumesCount; VolumeIndex2++) {
          if (VolumeIndex2 != VolumeIndex && 
              Volumes[VolumeIndex2]->HasBootCode && 
              Volumes[VolumeIndex2]->WholeDiskBlockIO == Volume->BlockIO) {
            ShowVolume = FALSE;
            break;
          }
        }
      }

      if (!ShowVolume || (Volume->OSType == OSTYPE_HIDE)) {
        DBG("skipped because volume is hidden\n");
        continue;
      }

      // Check for exact volume matches
      if (Custom->Volume) {
        if ((StrStr(Volume->DevicePathString, Custom->Volume) == NULL) &&
            ((Volume->VolName == NULL) || (StrStr(Volume->VolName, Custom->Volume) == NULL))) {
          DBG("skipped\n");
          continue;
        }
        // Check if the volume should be of certain os type
        if ((Custom->Type != 0) && (Custom->Type != Volume->OSType)) {
          DBG("skipped because wrong type\n");
          continue;
        }
      } else if ((Custom->Type != 0) && (Custom->Type != Volume->OSType)) {
        DBG("skipped because wrong type\n");
        continue;
      }
      // Change to custom image if needed
      Image = Custom->Image;
      if ((Image == NULL) && Custom->ImagePath) {
        Image = egLoadImage(Volume->RootDir, Custom->ImagePath, TRUE);
        if (Image == NULL) {
          Image = egLoadImage(ThemeDir, Custom->ImagePath, TRUE);
          if (Image == NULL) {
            Image = egLoadImage(SelfDir, Custom->ImagePath, TRUE);
            if (Image == NULL) {
              Image = egLoadImage(SelfRootDir, Custom->ImagePath, TRUE);
            }
          }
        }
      }
      if (Image == NULL) {
        Image = LoadOSIcon(Custom->ImagePath, L"unknown", 128, FALSE, FALSE);
      }
      // Change to custom drive image if needed
      DriveImage = Custom->DriveImage;
      if ((DriveImage == NULL) && Custom->DriveImagePath) {
        DriveImage = egLoadImage(Volume->RootDir, Custom->DriveImagePath, TRUE);
        if (DriveImage == NULL) {
          DriveImage = egLoadImage(ThemeDir, Custom->DriveImagePath, TRUE);
          if (DriveImage == NULL) {
            DriveImage = egLoadImage(SelfDir, Custom->DriveImagePath, TRUE);
            if (DriveImage == NULL) {
              DriveImage = egLoadImage(SelfRootDir, Custom->DriveImagePath, TRUE);
              if (DriveImage == NULL) {
                DriveImage = LoadBuiltinIcon(Custom->DriveImagePath);
              }
            }
          }
        }
      }
      if (Image == NULL) {
        Image = LoadOSIcon(Custom->ImagePath, L"unknown", 128, FALSE, FALSE);
      }
      // Create a legacy entry for this volume
      AddLegacyEntry(Custom->FullTitle, Custom->Title, Volume, Image, DriveImage, Custom->Hotkey, TRUE);
      DBG("match!\n");
    }
  }
  DBG("Custom legacy end\n");
}

//
// pre-boot tool functions
//

static VOID StartTool(IN LOADER_ENTRY *Entry)
{
  egClearScreen(&DarkBackgroundPixel);
    BeginExternalScreen(OSFLAG_ISSET(Entry->Flags, OSFLAG_USEGRAPHICS), Entry->me.Title + 6);  // assumes "Start <title>" as assigned below
    StartEFIImage(Entry->DevicePath, Entry->LoadOptions, Basename(Entry->LoaderPath),
                  Basename(Entry->LoaderPath), NULL, NULL);
    FinishExternalScreen();
//  ReinitSelfLib();
}

static LOADER_ENTRY * AddToolEntry(IN CHAR16 *LoaderPath, IN CHAR16 *FullTitle, IN CHAR16 *LoaderTitle,
                                   IN REFIT_VOLUME *Volume, IN EG_IMAGE *Image, IN CHAR16 ShortcutLetter)
{
  LOADER_ENTRY *Entry;
  
  Entry = AllocateZeroPool(sizeof(LOADER_ENTRY));
  
  if (FullTitle) {
    Entry->me.Title = EfiStrDuplicate(FullTitle);
  } else {
    Entry->me.Title = PoolPrint(L"Start %s", LoaderTitle);
  }
  Entry->me.Tag = TAG_TOOL;
  Entry->me.Row = 1;
  Entry->me.ShortcutLetter = ShortcutLetter;
  Entry->me.Image = Image;
  Entry->LoaderPath = EfiStrDuplicate(LoaderPath);
  Entry->DevicePath = FileDevicePath(Volume->DeviceHandle, Entry->LoaderPath);
  Entry->DevicePathString = FileDevicePathToStr(Entry->DevicePath);
  //actions
  Entry->me.AtClick = ActionSelect;
  Entry->me.AtDoubleClick = ActionEnter;
  Entry->me.AtRightClick = ActionHelp;
  
  AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
  return Entry;
}

static VOID ScanTool(VOID)
{
  EFI_STATUS              Status;
  CHAR16                  FileName[256];
  LOADER_ENTRY            *Entry;
  UINTN                   VolumeIndex;
  REFIT_VOLUME            *Volume;
  VOID                    *Interface;
  
  if (GlobalConfig.DisableFlags & DISABLE_FLAG_TOOLS)
    return;
  
  //    Print(L"Scanning for tools...\n");
  
  // look for the EFI shell
  if (!(GlobalConfig.DisableFlags & DISABLE_FLAG_SHELL)) {
#if defined(MDE_CPU_IA32)
    StrCpy(FileName, L"\\EFI\\CLOVER\\tools\\Shell32.efi");
    if (FileExists(SelfRootDir, FileName)) {
      Entry = AddToolEntry(FileName, NULL, L"EFI Shell 32", SelfVolume, BuiltinIcon(BUILTIN_ICON_TOOL_SHELL), 'S');
      DBG("found tools\\Shell32.efi\n");
    }
#elif defined(MDE_CPU_X64)
   if (gFirmwareClover) {
      StrCpy(FileName, L"\\EFI\\CLOVER\\tools\\Shell64.efi");
      if (FileExists(SelfRootDir, FileName)) {
         Entry = AddToolEntry(FileName, NULL, L"EFI Shell 64", SelfVolume, BuiltinIcon(BUILTIN_ICON_TOOL_SHELL), 'S');
         DBG("found tools\\Shell64.efi\n");
      }
   } else {
     StrCpy(FileName, L"\\EFI\\CLOVER\\tools\\Shell64U.efi");
     if (FileExists(SelfRootDir, FileName)) {
       Entry = AddToolEntry(FileName, NULL, L"UEFI Shell 64", SelfVolume, BuiltinIcon(BUILTIN_ICON_TOOL_SHELL), 'S');
       DBG("found tools\\Shell64U.efi\n");
     } else {
       StrCpy(FileName, L"\\EFI\\CLOVER\\tools\\Shell64.efi");
       if (FileExists(SelfRootDir, FileName)) {
          Entry = AddToolEntry(FileName, NULL, L"EFI Shell 64", SelfVolume, BuiltinIcon(BUILTIN_ICON_TOOL_SHELL), 'S');
          DBG("found tools\\Shell64.efi\n");
       }
     }
   }
#else //what else? ARM?
    UnicodeSPrint(FileName, 512, L"\\EFI\\CLOVER\\tools\\shell.efi");
    if (FileExists(SelfRootDir, FileName)) {
      Entry = AddToolEntry(FileName, NULL, L"EFI Shell", SelfVolume, BuiltinIcon(BUILTIN_ICON_TOOL_SHELL), 'S');
      DBG("found apps\\shell.efi\n");
    }
#endif
  }
  
  // look for the GPT/MBR sync tool
  /*    StrCpy(FileName, L"\\efi\\CLOVER\\tools\\gptsync.efi");
   if (FileExists(SelfRootDir, FileName)) {
   Entry = AddToolEntry(FileName, L"Partitioning Tool", BuiltinIcon(BUILTIN_ICON_TOOL_PART), 'P');
   }*/
  /*    
   // look for rescue Linux
   StrCpy(FileName, L"\\efi\\rescue\\elilo.efi");
   if (SelfVolume != NULL && FileExists(SelfRootDir, FileName)) {
   Entry = AddToolEntry(FileName, L"Rescue Linux", BuiltinIcon(BUILTIN_ICON_TOOL_RESCUE), 0);
   
   if (UGAWidth == 1440 && UGAHeight == 900)
   Entry->LoadOptions = L"-d 0 i17";
   else if (UGAWidth == 1680 && UGAHeight == 1050)
   Entry->LoadOptions = L"-d 0 i20";
   else
   Entry->LoadOptions = L"-d 0 mini";
   }
   */

  if (!gFirmwareClover) {
    for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
      Volume = Volumes[VolumeIndex];
      if (!Volume->RootDir || !Volume->DeviceHandle) {
        continue;
      }
      
      Status = gBS->HandleProtocol (Volume->DeviceHandle, &gEfiPartTypeSystemPartGuid, &Interface);
      if (Status == EFI_SUCCESS) {
        DBG("Checking EFI partition Volume %d for Clover\n", VolumeIndex);
        
#if defined(MDE_CPU_X64)
        StrCpy(FileName, L"\\EFI\\CLOVER\\CLOVERX64.EFI");
#else
        StrCpy(FileName, L"\\EFI\\CLOVER\\CLOVERIA32.EFI");
#endif
        
        // OSX adds label "EFI" to EFI volumes and some UEFIs see that
        // as a file. This file then blocks access to the /EFI directory.
        // We will delete /EFI file here and leave only /EFI directory.
        if (DeleteFile(Volume->RootDir, L"EFI")) {
          DBG(" Deleted /EFI label\n");
        }
        
        if (FileExists(Volume->RootDir, FileName)) {
          DBG(" Found Clover\n");
          Volume->BootType = BOOTING_BY_EFI;
          AddCloverEntry(FileName, L"Clover Boot Options", Volume);
        }
      }
    }
  }
}

// Add custom tool entries
static VOID AddCustomTool()
{
  UINTN             VolumeIndex;
  REFIT_VOLUME      *Volume;
  CUSTOM_TOOL_ENTRY *Custom;
  EG_IMAGE          *Image;
  UINTN              i = 0;

  DBG("Custom tool start\n");
  // Traverse the custom entries
  for (Custom = gSettings.CustomTool; Custom; ++i, Custom = Custom->Next) {
    if (OSFLAG_ISSET(Custom->Flags, OSFLAG_DISABLED)) {
      DBG("Custom tool %d skipped because it is disabled.\n", i);
      continue;
    }
    if (!gSettings.ShowHiddenEntries && OSFLAG_ISSET(Custom->Flags, OSFLAG_HIDDEN)) {
       DBG("Custom tool %d skipped because it is hidden.\n", i);
      continue;
    }

    if (Custom->Volume) {
      DBG("Custom tool %d matching \"%s\" ...\n", i, Custom->Volume);
    }
    for (VolumeIndex = 0; VolumeIndex < VolumesCount; ++VolumeIndex) {
      Volume = Volumes[VolumeIndex];

      DBG("   Checking volume \"%s\" (%s) ... ", Volume->VolName, Volume->DevicePathString);

      // skip volume if its kind is configured as disabled
      if ((Volume->DiskKind == DISK_KIND_OPTICAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_OPTICAL)) ||
          (Volume->DiskKind == DISK_KIND_EXTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_EXTERNAL)) ||
          (Volume->DiskKind == DISK_KIND_INTERNAL && (GlobalConfig.DisableFlags & DISABLE_FLAG_INTERNAL)) ||
          (Volume->DiskKind == DISK_KIND_FIREWIRE && (GlobalConfig.DisableFlags & DISABLE_FLAG_FIREWIRE)))
      {
        DBG("skipped because media is disabled\n");
        continue;
      }

      if (Custom->VolumeType != 0) {
        if ((Volume->DiskKind == DISK_KIND_OPTICAL && ((Custom->VolumeType & DISABLE_FLAG_OPTICAL) == 0)) ||
            (Volume->DiskKind == DISK_KIND_EXTERNAL && ((Custom->VolumeType & DISABLE_FLAG_EXTERNAL) == 0)) ||
            (Volume->DiskKind == DISK_KIND_INTERNAL && ((Custom->VolumeType & DISABLE_FLAG_INTERNAL) == 0)) ||
            (Volume->DiskKind == DISK_KIND_FIREWIRE && ((Custom->VolumeType & DISABLE_FLAG_FIREWIRE) == 0))) {
          DBG("skipped because media is ignored\n");
          continue;
        }
      }

      if (Volume->OSType == OSTYPE_HIDE) {
        DBG("skipped because volume is hidden\n");
        continue;
      }

      // Check for exact volume matches
      if (Custom->Volume) {
        if ((StrStr(Volume->DevicePathString, Custom->Volume) == NULL) &&
            ((Volume->VolName == NULL) || (StrStr(Volume->VolName, Custom->Volume) == NULL))) {
           DBG("skipped\n");
           continue;
        }
      }
      // Check the tool exists on the volume
      if (Volume->RootDir == NULL) {
        DBG("skipped because volume is not readable\n");
        continue;
      }
      if (!FileExists(Volume->RootDir, Custom->Path)) {
        DBG("skipped because path does not exist\n");
        continue;
      }
      // Change to custom image if needed
      Image = Custom->Image;
      if ((Image == NULL) && Custom->ImagePath) {
        Image = egLoadImage(Volume->RootDir, Custom->ImagePath, TRUE);
        if (Image == NULL) {
          Image = egLoadImage(ThemeDir, Custom->ImagePath, TRUE);
          if (Image == NULL) {
            Image = egLoadImage(SelfDir, Custom->ImagePath, TRUE);
            if (Image == NULL) {
              Image = egLoadImage(SelfRootDir, Custom->ImagePath, TRUE);
            }
          }
        }
      }
      if (Image == NULL) {
        Image = LoadOSIcon(Custom->ImagePath, L"unknown", 48, FALSE, FALSE);
      }
      // Create a legacy entry for this volume
      AddToolEntry(Custom->Path, Custom->FullTitle, Custom->Title, Volume, Image, Custom->Hotkey);
      DBG("match!\n");
    }
  }
  DBG("Custom tool end\n");
}

/*

VOID AddUEFIBootOption(BO_BOOT_OPTION *bootOption)
{
  UINTN                       volumeIndex;
  REFIT_VOLUME                *volume;
  EFI_DEVICE_PATH_PROTOCOL    *devPathNode;
  LOADER_ENTRY                *loaderEntry;
  FILEPATH_DEVICE_PATH        *loaderDevicePath;
  CHAR16                      *loaderPath;
  CHAR16                      *bootParams;
  CHAR16                      *title;
  EFI_GUID                    *volumeGUID;
  EFI_GUID                    *bootGUID;
  
  for (volumeIndex = 0; volumeIndex < VolumesCount; volumeIndex++) {
    volume = Volumes[volumeIndex];
    volumeGUID = FindGPTPartitionGuidInDevicePath(volume->DevicePath);
    bootGUID = FindGPTPartitionGuidInDevicePath(bootOption->FilePathList);
    if (!volumeGUID || !bootGUID || !CompareGuid(volumeGUID, bootGUID)) continue;
    
    devPathNode = bootOption->FilePathList;
    loaderDevicePath = (FILEPATH_DEVICE_PATH *) FindDevicePathNodeWithType(devPathNode, MEDIA_DEVICE_PATH,
                                                                           MEDIA_FILEPATH_DP);
    if (!loaderDevicePath) continue;
    
    loaderPath = loaderDevicePath->PathName;
    bootParams = (CHAR16 *) bootOption->OptionalData;
    title = bootOption->Description;
    
    volume->OSIconName = L"unknown";
    volume->BootType = BOOTING_BY_EFI;
    
    loaderEntry = AddLoaderEntry(loaderPath, title, volume, OSTYPE_EFI);
    loaderEntry->LoadOptions = EfiStrDuplicate(bootParams);
  }
}

VOID ScanUEFIBootOptions(BOOLEAN allBootOptions)
{
  EFI_STATUS          status;
  UINT16              *bootOrder;
  UINTN               bootOrderCnt;
  UINT16              i;
  BO_BOOT_OPTION      bootOption;
  
  DBG("Scanning boot options from UEFI ...\n");
  
  if (allBootOptions) {
    for (i = 0; i <= 0xFFFF; i++) {
      status = GetBootOption(i, &bootOption);
      if (EFI_ERROR(status)) continue;
      
      AddUEFIBootOption(&bootOption);
      
      FreePool(bootOption.Variable);
    }
  } else {
    status = GetBootOrder(&bootOrder, &bootOrderCnt);
    if (EFI_ERROR(status)) return;
    
    for (i = 0; i < bootOrderCnt; i++) {
      status = GetBootOption(bootOrder[i], &bootOption);
      if (EFI_ERROR(status)) continue;
      
      AddUEFIBootOption(&bootOption);
      
      FreePool(bootOption.Variable);
    }
    
    FreePool(bootOrder);
  }
}

// */

//
// pre-boot driver functions
//

static VOID ScanDriverDir(IN CHAR16 *Path, OUT EFI_HANDLE **DriversToConnect, OUT UINTN *DriversToConnectNum)
{
  EFI_STATUS              Status;
  REFIT_DIR_ITER          DirIter;
  EFI_FILE_INFO           *DirEntry;
  CHAR16                  FileName[256];
  EFI_HANDLE              DriverHandle;
  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding;
  UINTN                   DriversArrSize;
  UINTN                   DriversArrNum;
  EFI_HANDLE              *DriversArr;
  INTN                    i;
  BOOLEAN                 Skip;
  
  DriversArrSize = 0;
  DriversArrNum = 0;
  DriversArr = NULL;
  
  // look through contents of the directory
  DirIterOpen(SelfRootDir, Path, &DirIter);
  while (DirIterNext(&DirIter, 2, L"*.EFI", &DirEntry)) {
    Skip = (DirEntry->FileName[0] == '.');
//    if (DirEntry->FileName[0] == '.')
//      continue;   // skip this    
    for (i=0; i<gSettings.BlackListCount; i++) {
      if (StrStr(DirEntry->FileName, gSettings.BlackList[i]) != NULL) {
        Skip = TRUE;   // skip this
        break;
      }
    }
    if (Skip) {
      continue;
    }
    UnicodeSPrint(FileName, 512, L"%s\\%s", Path, DirEntry->FileName);
    Status = StartEFIImage(FileDevicePath(SelfLoadedImage->DeviceHandle, FileName),
                           L"", DirEntry->FileName, DirEntry->FileName, NULL, &DriverHandle);
    if (EFI_ERROR(Status)) {
      continue;
    }
    if (StrStr(FileName, L"EmuVariable") != NULL) {
      gDriversFlags.EmuVariableLoaded = TRUE;
    } else if (StrStr(FileName, L"Video") != NULL) {
      gDriversFlags.VideoLoaded = TRUE;
    } else if (StrStr(FileName, L"Partition") != NULL) {
      gDriversFlags.PartitionLoaded = TRUE;
    }
    if (DriverHandle != NULL && DriversToConnectNum != NULL && DriversToConnect != NULL) {
      // driver loaded - check for EFI_DRIVER_BINDING_PROTOCOL
      Status = gBS->HandleProtocol(DriverHandle, &gEfiDriverBindingProtocolGuid, (VOID **) &DriverBinding);
      if (!EFI_ERROR(Status) && DriverBinding != NULL) {
        DBG(" - driver needs connecting\n");
        // standard UEFI driver - we would reconnect after loading - add to array
        if (DriversArrSize == 0) {
          // new array
          DriversArrSize = 16;
          DriversArr = AllocateZeroPool(sizeof(EFI_HANDLE) * DriversArrSize);
        } else if (DriversArrNum + 1 == DriversArrSize) {
          // extend array
          DriversArr = ReallocatePool(DriversArrSize, DriversArrSize + 16, DriversArr);
          DriversArrSize += 16;
        }
        DriversArr[DriversArrNum] = DriverHandle;
        DriversArrNum++;
        // we'll make array terminated
        DriversArr[DriversArrNum] = NULL;
      }
    }
  }
  Status = DirIterClose(&DirIter);
  if (Status != EFI_NOT_FOUND) {
    UnicodeSPrint(FileName, 512, L"while scanning the %s directory", Path);
    CheckError(Status, FileName);
  }
  
  if (DriversToConnectNum != NULL && DriversToConnect != NULL) {
    *DriversToConnectNum = DriversArrNum;
    *DriversToConnect = DriversArr;
  }
//release memory for BlackList
  for (i=0; i<gSettings.BlackListCount; i++) {
    if (gSettings.BlackList[i]) {
      FreePool(gSettings.BlackList[i]);
      gSettings.BlackList[i] = NULL;
    }
  }
}


/**
 * Some UEFI's (like HPQ EFI from HP notebooks) have DiskIo protocols
 * opened BY_DRIVER (by Partition driver in HP case) even when no file system
 * is produced from this DiskIo. This then blocks our FS drivers from connecting
 * and producing file systems.
 * To fix it: we will disconnect drivers that connected to DiskIo BY_DRIVER
 * if this is partition volume and if those drivers did not produce file system.
 */
VOID DisconnectInvalidDiskIoChildDrivers(VOID)
{
  EFI_STATUS                            Status;
  UINTN                                 HandleCount = 0;
  UINTN                                 Index;
  UINTN                                 OpenInfoIndex;
  EFI_HANDLE                            *Handles = NULL;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL       *Fs;
  EFI_BLOCK_IO_PROTOCOL                 *BlockIo;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY   *OpenInfo;
  UINTN                                 OpenInfoCount;
  BOOLEAN                               Found;
  
  DBG("Searching for invalid DiskIo BY_DRIVER connects:");
  
  //
  // Get all DiskIo handles
  //
  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiDiskIoProtocolGuid, NULL, &HandleCount, &Handles);
  if (EFI_ERROR(Status) || HandleCount == 0) {
    DBG(" no DiskIo handles\n");
    return;
  }
  
  //
  // Check every DiskIo handle
  //
  Found = FALSE;
  for (Index = 0; Index < HandleCount; Index++) {
    //DBG("\n");
    //DBG(" - Handle %p:", Handles[Index]);
    //
    // If this is not partition - skip it.
    // This is then whole disk and DiskIo
    // should be opened here BY_DRIVER by Partition driver
    // to produce partition volumes.
    //
    Status = gBS->HandleProtocol (
                                  Handles[Index],
                                  &gEfiBlockIoProtocolGuid,
                                  (VOID **) &BlockIo
                                  );
    if (EFI_ERROR (Status)) {
      //DBG(" BlockIo: %r - skipping\n", Status);
      continue;
    }
    if (BlockIo->Media == NULL) {
      //DBG(" BlockIo: no media - skipping\n");
      continue;
      
    }
    if (!BlockIo->Media->LogicalPartition) {
      //DBG(" BlockIo: whole disk - skipping\n");
      continue;
      
    }
    //DBG(" BlockIo: partition");
    
    //
    // If SimpleFileSystem is already produced - skip it, this is ok
    //
    Status = gBS->HandleProtocol (
                                  Handles[Index],
                                  &gEfiSimpleFileSystemProtocolGuid,
                                  (VOID **) &Fs
                                  );
    if (Status == EFI_SUCCESS) {
      //DBG(" FS: ok - skipping\n");
      continue;
    }
    //DBG(" FS: no");
    
    //
    // If no SimpleFileSystem on this handle but DiskIo is opened BY_DRIVER
    // then disconnect this connection
    //
    Status = gBS->OpenProtocolInformation (
                                           Handles[Index],
                                           &gEfiDiskIoProtocolGuid,
                                           &OpenInfo,
                                           &OpenInfoCount
                                           );
    if (EFI_ERROR (Status)) {
      //DBG(" OpenInfo: no - skipping\n");
      continue;
    }
    //DBG(" OpenInfo: %d", OpenInfoCount);
    for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
      if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) == EFI_OPEN_PROTOCOL_BY_DRIVER) {
        if (!Found) {
          DBG("\n");
        }
        Found = TRUE;
        Status = gBS->DisconnectController (Handles[Index], OpenInfo[OpenInfoIndex].AgentHandle, NULL);
        //DBG(" BY_DRIVER Agent: %p, Disconnect: %r", OpenInfo[OpenInfoIndex].AgentHandle, Status);
        DBG(" - Handle %p with DiskIo, is Partition, no Fs, BY_DRIVER Agent: %p, Disconnect: %r\n", Handles[Index], OpenInfo[OpenInfoIndex].AgentHandle, Status);
      }
    }
    FreePool (OpenInfo);
  }
  FreePool(Handles);
  
  if (!Found) {
    DBG(" not found, all ok\n");
  }
}

VOID DisconnectSomeDevices(VOID)
{
  EFI_STATUS              Status;
  UINTN                   HandleCount = 0;
  UINTN                   Index;
  EFI_HANDLE              *Handles = NULL;
	EFI_BLOCK_IO_PROTOCOL   *BlockIo	= NULL;
	EFI_PCI_IO_PROTOCOL     *PciIo	= NULL;
	PCI_TYPE00              Pci;
  
  if (gDriversFlags.PartitionLoaded) {
    DBG("Partition driver loaded: ");
    // get all BlockIo handles
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &HandleCount, &Handles);
    if (Status == EFI_SUCCESS) {
      for (Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol(Handles[Index], &gEfiBlockIoProtocolGuid, (VOID **) &BlockIo);
        if (EFI_ERROR(Status)) {
          continue;
        }
        if (BlockIo->Media->BlockSize == 2048) {
          // disconnect CD driver
          Status = gBS->DisconnectController(Handles[Index], NULL, NULL);
          DBG("CD disconnect %r", Status);
        }
      }
      FreePool(Handles);
    }
    DBG("\n");
  }
  
  if (gDriversFlags.VideoLoaded) {
    DBG("Video driver loaded: ");
    // get all PciIo handles
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiPciIoProtocolGuid, NULL, &HandleCount, &Handles);
    if (Status == EFI_SUCCESS) {
      for (Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol(Handles[Index], &gEfiPciIoProtocolGuid, (VOID **) &PciIo);
        if (EFI_ERROR(Status)) {
          continue;
        }
        Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0, sizeof (Pci) / sizeof (UINT32), &Pci);
        if (!EFI_ERROR (Status))
        {
          if(IS_PCI_VGA(&Pci) == TRUE)
          {
            // disconnect VGA
            Status = gBS->DisconnectController(Handles[Index], NULL, NULL);
            DBG("disconnect %r", Status);
          }
        }
      }
      FreePool(Handles);
    }
    DBG("\n");
  }
  
  if (!gFirmwareClover) {
    DisconnectInvalidDiskIoChildDrivers();
  }
}

//UINT8 *mCurrentEdid;
/*
UINT8* getCurrentEdid (VOID)
{
  EFI_STATUS                      Status;
  EFI_EDID_ACTIVE_PROTOCOL        *EdidProtocol;
  UINT8                           *Edid;
  
  DBG ("Edid:");
  Edid = NULL;
  Status = gBS->LocateProtocol (&gEfiEdidActiveProtocolGuid, NULL, (VOID**)&EdidProtocol);
  if (!EFI_ERROR (Status)) {
    DBG(" size=%d", EdidProtocol->SizeOfEdid);
    if (EdidProtocol->SizeOfEdid > 0) {
      Edid = AllocateCopyPool (EdidProtocol->SizeOfEdid, EdidProtocol->Edid);
    }
  }
  DBG(" %a\n", Edid != NULL ? "found" : "not found");
  
  return Edid;
}
*/

VOID PatchVideoBios(UINT8 *Edid)
{

  if (gSettings.PatchVBiosBytesCount > 0 && gSettings.PatchVBiosBytes != NULL) {
    VideoBiosPatchBytes(gSettings.PatchVBiosBytes, gSettings.PatchVBiosBytesCount);
  }

  if (gSettings.PatchVBios) {
    VideoBiosPatchNativeFromEdid(Edid);
  }
}


static VOID LoadDrivers(VOID)
{
  EFI_STATUS  Status;
  EFI_HANDLE  *DriversToConnect = NULL;
  UINTN       DriversToConnectNum = 0;
  UINT8       *Edid;
  UINTN       VarSize = 0;
  BOOLEAN     VBiosPatchNeeded;
  
    // load drivers from /efi/drivers
#if defined(MDE_CPU_X64)
  if (gFirmwareClover) {
    ScanDriverDir(L"\\EFI\\CLOVER\\drivers64", &DriversToConnect, &DriversToConnectNum);
  } else
    ScanDriverDir(L"\\EFI\\CLOVER\\drivers64UEFI", &DriversToConnect, &DriversToConnectNum);
#else
  ScanDriverDir(L"\\EFI\\CLOVER\\drivers32", &DriversToConnect, &DriversToConnectNum);
#endif
  
  VBiosPatchNeeded = gSettings.PatchVBios || (gSettings.PatchVBiosBytesCount > 0 && gSettings.PatchVBiosBytes != NULL);
  if (VBiosPatchNeeded) {
    // check if it is already done in CloverEFI BiosVideo
    Status = gRT->GetVariable (
                               L"CloverVBiosPatchDone",
                               &gEfiGlobalVariableGuid,
                               NULL,
                               &VarSize,
                               NULL
                               );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      // var exists - it's done - let's not do it again
      VBiosPatchNeeded = FALSE;
    }
  }
  
  if (VBiosPatchNeeded && !gDriversFlags.VideoLoaded) {
    // we have video bios patch - force video driver reconnect
    DBG("Video bios patch requested - forcing video reconnect\n");
    gDriversFlags.VideoLoaded = TRUE;
    DriversToConnectNum++;
  }
  
  if (DriversToConnectNum > 0) {
    DBG("%d drivers needs connecting ...\n", DriversToConnectNum);
    // note: our platform driver protocol
    // will use DriversToConnect - do not release it
    RegisterDriversToHighestPriority(DriversToConnect);
    if (VBiosPatchNeeded) {
      if (gSettings.CustomEDID != NULL) {
        Edid = gSettings.CustomEDID;
      } else {
        Edid = getCurrentEdid();
      }
      DisconnectSomeDevices();
      PatchVideoBios(Edid);
      if (gSettings.CustomEDID == NULL) {
        FreePool(Edid);
      }
    } else {
      DisconnectSomeDevices();
    }
    BdsLibConnectAllDriversToAllControllers();
    
    // Boot speedup: remove temporary "BiosVideoBlockSwitchMode" RT var
    // to unlock mode switching in CsmVideo
    gRT->SetVariable(L"BiosVideoBlockSwitchMode", &gEfiGlobalVariableGuid, EFI_VARIABLE_BOOTSERVICE_ACCESS, 0, NULL);
  }
}


INTN FindDefaultEntry(VOID)
{
  INTN                Index;
  REFIT_VOLUME        *Volume;
  LOADER_ENTRY        *Entry;

  
  DBG("FindDefaultEntry ...\n");
  
  //
  // try to detect volume set by Startup Disk or previous Clover selection
  //
  Index = FindStartupDiskVolume(&MainMenu);
  if (Index >= 0) {
    DBG("Boot redirected to Entry %d. '%s'\n", Index, MainMenu.Entries[Index]->Title);
    return Index;
  }
  
  //
  // if not found, then try DefaultBoot from config.plist
  // search volume with name == gSettings.DefaultBoot
  //
  if (gSettings.DefaultBoot != NULL && gSettings.DefaultBoot[0] != L'\0') {
    
    DBG("Searching config.plist DefaultBoot ...");
    for (Index = 0; ((Index < (INTN)MainMenu.EntryCount) && (MainMenu.Entries[Index]->Row == 0)); Index++) {
      
      Entry = (LOADER_ENTRY*)MainMenu.Entries[Index];
      if (!Entry->Volume) {
        continue;
      }
      
      Volume = Entry->Volume;
      if (StrCmp(Volume->VolName, gSettings.DefaultBoot) != 0) {
        continue;
      }
      
      DBG(" found\nBoot redirected to Entry %d. '%s', Volume '%s'\n", Index, Entry->me.Title, Volume->VolName);
      return Index;
    }
    
  }
  
  DBG("Default boot entry not found\n");
  return -1;
}

VOID SetVariablesFromNvram()
{
  CHAR16  UStr[80];
  CHAR8  *tmpString;
  UINTN   Size = 0;
  UINTN   index = 0, index2, len, i;
  CHAR8  *arg = NULL;
  
  tmpString = GetNvramVariable(L"Clover.LogLineCount", &gEfiAppleBootGuid, NULL, &Size);
  ZeroMem(UStr, 10);
  if (tmpString) {
    AsciiStrToUnicodeStr(tmpString, (CHAR16*)&UStr[0]);
    gSettings.LogLineCount = (UINT32)StrDecimalToUintn((CHAR16*)&UStr[0]);
  }
  
  tmpString = GetNvramVariable(L"Clover.MountEFI", &gEfiAppleBootGuid, NULL, &Size);
  if (tmpString) {
    gSettings.MountEFI = AllocateCopyPool(AsciiStrSize(tmpString), tmpString);
  }
  
  tmpString = GetNvramVariable(L"Clover.LogEveryBoot", &gEfiAppleBootGuid, NULL, &Size);
  if (tmpString) {
    gSettings.LogEveryBoot = AllocateCopyPool(AsciiStrSize(tmpString), tmpString);
  }

  tmpString = GetNvramVariable(L"boot-args", &gEfiAppleBootGuid, NULL, &Size);
  if (tmpString && (Size <= 0x1000) && (Size > 0)) {
    DBG("found boot-args in NVRAM:%a, size=%d\n", tmpString, Size);
    Size = AsciiStrLen(tmpString); // some EFI implementations include '\0' in Size, and others don't, so update Size to string length
    arg = AllocatePool(Size+1);
    //first we will find new args that is not present in main args
    index = 0;
    while ((index < Size) && (tmpString[index] != 0x0)) {
      ZeroMem(arg, Size+1);
      index2 = 0;
      if (tmpString[index] != '\"') {
 //       DBG("search space index=%d\n", index);
        while ((index < Size) && (tmpString[index] != 0x20) && (tmpString[index] != 0x0)) {
          arg[index2++] = tmpString[index++];
        }
        DBG("...found arg:%a\n", arg);
      } else {
        index++;
//        DBG("search quote index=%d\n", index);
        while ((index < Size) && (tmpString[index] != '\"') && (tmpString[index] != 0x0)) {
          arg[index2++] = tmpString[index++];
        }     
        if (tmpString[index] == '\"') {
          index++;
        }
        DBG("...found quoted arg:\n", arg);
      }
      while (tmpString[index] == 0x20) {
        index++;
      }
      if ((AsciiStrCmp(arg, "-v") == 0) ||
          (AsciiStrCmp(arg, "-s") == 0) ||
          (AsciiStrCmp(arg, "-x") == 0)) {
        continue;
      }
      if (!AsciiStrStr(gSettings.BootArgs, arg)) {
        //this arg is not present will add
        DBG("...adding arg:%a\n", arg); 
        len = iStrLen(gSettings.BootArgs, 256);
        if (len + index2 > 256) {
          DBG("boot-args overflow... bytes=%d+%d\n", len, index2);
          break;
        }
        gSettings.BootArgs[len++] = 0x20;
        for (i = 0; i < index2; i++) {
          gSettings.BootArgs[len++] = arg[i];
        }
        gSettings.BootArgs[len++] = 0x20;
      }
    }
    FreePool(arg);
  }
}

//
// main entry point
//
EFI_STATUS
EFIAPI
RefitMain (IN EFI_HANDLE           ImageHandle,
           IN EFI_SYSTEM_TABLE     *SystemTable)
{
  EFI_STATUS Status;
  BOOLEAN           MainLoopRunning = TRUE;
  BOOLEAN           ReinitDesktop = TRUE;
  BOOLEAN           AfterTool = FALSE;
  REFIT_MENU_ENTRY  *ChosenEntry = NULL;
  REFIT_MENU_ENTRY  *DefaultEntry = NULL;
  REFIT_MENU_ENTRY  *OptionEntry = NULL;
  INTN              DefaultIndex;
  UINTN             MenuExit;
  UINTN             Size, i;
//  UINT64            TscDiv;
//  UINT64            TscRemainder = 0;
  LOADER_ENTRY      *LoaderEntry;
  
  // CHAR16            *InputBuffer; //, *Y;
  //  EFI_INPUT_KEY Key;
  
  // get TSC freq and init MemLog if needed
  gCPUStructure.TSCCalibr = GetMemLogTscTicksPerSecond(); //ticks for 1second
  //GlobalConfig.TextOnly = TRUE;
  
  // bootstrap
	gST				= SystemTable;
	gImageHandle	= ImageHandle;
	gBS				= SystemTable->BootServices;
	gRS				= SystemTable->RuntimeServices;
	Status = EfiGetSystemConfigurationTable (&gEfiDxeServicesTableGuid, (VOID **) &gDS);

  // firmware detection
  gFirmwareClover = StrCmp(gST->FirmwareVendor, L"CLOVER") == 0;
  InitializeConsoleSim();
  InitBooterLog();
  DBG("\n");
  DBG("Starting Clover rev %s on %s EFI\n", FIRMWARE_REVISION, gST->FirmwareVendor);

  Status = InitRefitLib(gImageHandle);
  if (EFI_ERROR(Status))
    return Status;
  
  // disable EFI watchdog timer
  gBS->SetWatchdogTimer(0x0000, 0x0000, 0x0000, NULL);
  ZeroMem((VOID*)&gSettings, sizeof(SETTINGS_DATA));
  
  InitializeUnicodeCollationProtocol();
  PrepatchSmbios();
  
#ifdef REVISION_STR
  MsgLog(REVISION_STR); 
#endif
  //replace / with _
  Size = iStrLen(gSettings.OEMProduct, 64);
  for (i=0; i<Size; i++) {
    if (gSettings.OEMProduct[i] == 0x2F) {
      gSettings.OEMProduct[i] = 0x5F;
    }
  }
  Size = iStrLen(gSettings.OEMBoard, 64);
  for (i=0; i<Size; i++) {
    if (gSettings.OEMBoard[i] == 0x2F) {
      gSettings.OEMBoard[i] = 0x5F;
    }
  }
  DBG("  running on %a\n",   gSettings.OEMProduct);
  DBG("... with board %a\n", gSettings.OEMBoard);
  
  UnicodeSPrint(gSettings.ConfigName, 64, L"config");
  
  if (!gFirmwareClover && FileExists(SelfRootDir, PoolPrint(L"EFI\\CLOVER\\OEM\\%a\\UEFI\\%s.plist", gSettings.OEMBoard, gSettings.ConfigName))) {
    OEMPath = PoolPrint(L"EFI\\CLOVER\\OEM\\%a\\UEFI", gSettings.OEMBoard);
  } else if (FileExists(SelfRootDir, PoolPrint(L"EFI\\CLOVER\\OEM\\%a\\%s.plist", gSettings.OEMProduct, gSettings.ConfigName))) {
    OEMPath = PoolPrint(L"EFI\\CLOVER\\OEM\\%a", gSettings.OEMProduct);
  } else if (FileExists(SelfRootDir, PoolPrint(L"EFI\\CLOVER\\OEM\\%a\\%s.plist", gSettings.OEMBoard, gSettings.ConfigName))) {
    OEMPath = PoolPrint(L"EFI\\CLOVER\\OEM\\%a", gSettings.OEMBoard);
  } else {
    OEMPath = L"EFI\\CLOVER";
  }

  gSettings.PointerEnabled = TRUE;
  gSettings.PointerSpeed = 2;
  gSettings.DoubleClickTime = 500;

  if (!GlobalConfig.FastBoot) {
    GetListOfThemes();
  }
  Status = GetEarlyUserSettings(SelfRootDir);
  if (EFI_ERROR(Status)) {
    DBG("Early settings: %r\n", Status);
  }

  MainMenu.TimeoutSeconds = GlobalConfig.Timeout >= 0 ? GlobalConfig.Timeout : 0;
  
  DBG("LoadDrivers() start\n");
  LoadDrivers();
  DBG("LoadDrivers() end\n");
  
  // for DUET
  if (StrCmp(gST->FirmwareVendor, L"EDK II") == 0) {
    gDriversFlags.EmuVariableLoaded = TRUE;
  }
  
  Status = gBS->LocateProtocol (&gEmuVariableControlProtocolGuid, NULL, (VOID**)&gEmuVariableControl);
  if (EFI_ERROR(Status)) {
    gEmuVariableControl = NULL;
  }
  if (gEmuVariableControl != NULL) {
    gEmuVariableControl->InstallEmulation(gEmuVariableControl);
  }

  if (!GlobalConfig.FastBoot) {
    // init screen and dump video modes to log
    if (gDriversFlags.VideoLoaded) {
      InitScreen(FALSE);
    } else {
      InitScreen(!gFirmwareClover); // ? FALSE : TRUE);
    }
    SetupScreen();
    //  DBG("InitScreen\n");
  } else {
    InitScreen(FALSE);
  }
  
  //Now we have to reinit handles
  Status = ReinitSelfLib();
  if (EFI_ERROR(Status)){
    DebugLog(2, " %r", Status);
    PauseForKey(L"Error reinit refit\n");
    return Status;
  }
  
//        DBG("reinit OK\n");
//  ZeroMem((VOID*)&gSettings, sizeof(SETTINGS_DATA));
  ZeroMem((VOID*)&gGraphics[0], sizeof(GFX_PROPERTIES) * 4);
  
//  DumpBiosMemoryMap();

  GuiEventsInitialize();
  
  GetCPUProperties();
  if (!gSettings.EnabledCores) {
    gSettings.EnabledCores = gCPUStructure.Cores;
  }
//  DBG("GetCPUProperties OK\n");
  GetDevices();
 //     DBG("GetDevices OK\n");
 // if (!GlobalConfig.FastBoot) {
  DBG("ScanSPD() start\n");
  ScanSPD();
  DBG("ScanSPD() end\n");
 // }
 //       DBG("ScanSPD OK\n");
  SetPrivateVarProto();
//        DBG("SetPrivateVarProto OK\n");
  GetDefaultSettings();
  DBG("Calibrated TSC frequency =%ld =%ldMHz\n", gCPUStructure.TSCCalibr, DivU64x32(gCPUStructure.TSCCalibr, Mega));
  if (gCPUStructure.TSCCalibr > 200000000ULL) {  //200MHz
    gCPUStructure.TSCFrequency = gCPUStructure.TSCCalibr;
  }
  
  gCPUStructure.CPUFrequency = gCPUStructure.TSCFrequency;
  gCPUStructure.FSBFrequency = DivU64x32(MultU64x32(gCPUStructure.CPUFrequency, 10),
                                         (gCPUStructure.MaxRatio == 0) ? 1 : gCPUStructure.MaxRatio);
  gCPUStructure.ExternalClock = (UINT32)DivU64x32(gCPUStructure.FSBFrequency, kilo);
  gCPUStructure.MaxSpeed = (UINT32)DivU64x32(gCPUStructure.TSCFrequency, Mega);
  
  //Second step. Load config.plist into gSettings	
	Status = GetUserSettings(SelfRootDir);  
 //       DBG("GetUserSettings OK\n");
  
  if (!gFirmwareClover && !gDriversFlags.EmuVariableLoaded &&
      GlobalConfig.Timeout == 0 && !ReadAllKeyStrokes()) {
// UEFI boot: get gEfiBootDeviceGuid from NVRAM.
// if present, ScanVolumes() will skip scanning other volumes
// in the first run.
// this speeds up loading of default OSX volume.
     GetEfiBootDeviceFromNvram();
//    DBG("GetEfiBootDeviceFromNvram()\n");
  }
  AfterTool = FALSE;
  do {
    //     PauseForKey(L"Enter main cycle");
    //    DBG("Enter main cycle\n");

    MainMenu.EntryCount = 0;
    OptionMenu.EntryCount = 0;
    ScanVolumes();
    //   DBG("ScanVolumes()\n");

    // as soon as we have Volumes, find latest nvram.plist and copy it to RT vars
    if (!AfterTool) {
      if (gFirmwareClover || gDriversFlags.EmuVariableLoaded) {
        PutNvramPlistToRtVars();
      }
    }

    if (!GlobalConfig.FastBoot) {
 //     GetListOfThemes();
      if (gThemeNeedInit) {
        InitTheme(TRUE);
        gThemeNeedInit = FALSE;
      } else if (gThemeChanged) {
        InitTheme(FALSE);
        FreeMenu(&OptionMenu);
      }
      gThemeChanged = FALSE;

      //now it is a time to set RtVariables
      SetVariablesFromNvram();
      FillInputs();
      // scan for loaders and tools, add then to the menu
      if (GlobalConfig.LegacyFirst){
        //DBG("scan legacy first\n");
        AddCustomLegacy();
        if (!GlobalConfig.NoLegacy) {
          ScanLegacy();
        }
      }
    }

    // Add custom entries
    AddCustomEntries();

    if (gSettings.DisableEntryScan) {
      DBG("Entry scan disabled\n");
    } else {
      //ScanUEFIBootOptions(FALSE);
      ScanLoader();
      //        DBG("ScanLoader OK\n");
    }

    if (!GlobalConfig.FastBoot) {

      if (!GlobalConfig.LegacyFirst) {
        //      DBG("scan legacy second\n");
        AddCustomLegacy();
        if (!GlobalConfig.NoLegacy) {
          ScanLegacy();
        }
        //      DBG("ScanLegacy()\n");
      }

      // fixed other menu entries
      //               DBG("FillInputs OK\n");

      if (!(GlobalConfig.DisableFlags & DISABLE_FLAG_TOOLS)) {
        AddCustomTool();
        if (!gSettings.DisableToolScan) {
          ScanTool();
        }
        //      DBG("ScanTool()\n");
      }

      if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_FUNCS)) {
        MenuEntryOptions.Image = BuiltinIcon(BUILTIN_ICON_FUNC_OPTIONS);
        AddMenuEntry(&MainMenu, &MenuEntryOptions);
        MenuEntryAbout.Image = BuiltinIcon(BUILTIN_ICON_FUNC_ABOUT);
        AddMenuEntry(&MainMenu, &MenuEntryAbout);
      }

      if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_FUNCS) || MainMenu.EntryCount == 0) {
        MenuEntryReset.Image = BuiltinIcon(BUILTIN_ICON_FUNC_RESET);
        //    DBG("Reset.Image->Width=%d\n", MenuEntryReset.Image->Width);
        AddMenuEntry(&MainMenu, &MenuEntryReset);
        MenuEntryShutdown.Image = BuiltinIcon(BUILTIN_ICON_FUNC_SHUTDOWN);
        //    DBG("Shutdown.Image->Width=%d\n", MenuEntryShutdown.Image->Width);
        AddMenuEntry(&MainMenu, &MenuEntryShutdown);
      }
    }
    // wait for user ACK when there were errors
    FinishTextScreen(FALSE);
    //   DBG("FinishTextScreen()\n");

    DefaultIndex = FindDefaultEntry();
    //    DBG("FindDefaultEntry()\n");
      DBG("DefaultIndex=%d and MainMenu.EntryCount=%d\n", DefaultIndex, MainMenu.EntryCount);
    if ((DefaultIndex >= 0) && (DefaultIndex < (INTN)MainMenu.EntryCount)) {
      DefaultEntry = MainMenu.Entries[DefaultIndex];
    } else {
      DefaultEntry = NULL;
    }

    //    MainMenu.TimeoutSeconds = GlobalConfig.Timeout >= 0 ? GlobalConfig.Timeout : 0;
    if (GlobalConfig.FastBoot && DefaultEntry) {
      StartLoader((LOADER_ENTRY *)DefaultEntry);
      MainLoopRunning = FALSE;
      GlobalConfig.FastBoot = FALSE; //Hmm... will never be here
    }
    MainAnime = GetAnime(&MainMenu);
    MainLoopRunning = TRUE;

    // PauseForKey(L"Enter main loop");

    AfterTool = FALSE;
    gEvent = 0; //clear to cancel loop
    while (MainLoopRunning) {
      if (GlobalConfig.Timeout == 0 && DefaultEntry != NULL && !ReadAllKeyStrokes()) {
        // go strait to DefaultVolume loading
        MenuExit = MENU_EXIT_TIMEOUT;
      } else {
        MainMenu.AnimeRun = MainAnime;
        MenuExit = RunMainMenu(&MainMenu, DefaultIndex, &ChosenEntry);
      }

      // disable default boot - have sense only in the first run
      GlobalConfig.Timeout = -1;

      //    DBG("out from menu\n");
      if ((DefaultEntry != NULL) && (MenuExit == MENU_EXIT_TIMEOUT)) {
        //      DBG("boot by timeout\n");
        if (DefaultEntry->Tag == TAG_LOADER) {
          StartLoader((LOADER_ENTRY *)DefaultEntry);
        } else if (DefaultEntry->Tag == TAG_LEGACY){
          StartLegacy((LEGACY_ENTRY *)DefaultEntry);
        }
        // if something goes wrong - break main loop to reinit volumes
        break;
      }

      if (MenuExit == MENU_EXIT_OPTIONS){
        OptionsMenu(&OptionEntry);
        continue;
      }

      if (MenuExit == MENU_EXIT_HELP){
        HelpRefit();
        continue;
      }

      // EjectVolume
      if (MenuExit == MENU_EXIT_EJECT){
        if ((ChosenEntry->Tag == TAG_LOADER) ||
            (ChosenEntry->Tag == TAG_LEGACY)) {
          Status = EjectVolume(((LOADER_ENTRY *)ChosenEntry)->Volume);
          if (!EFI_ERROR(Status)) {
            break; //main loop is broken so Reinit all
          }
        }
        continue;
      }

      // Hide toggle
      if (MenuExit == MENU_EXIT_HIDE_TOGGLE) {
        gSettings.ShowHiddenEntries = !gSettings.ShowHiddenEntries;
        AfterTool = TRUE;
        break;
      }

      // We don't allow exiting the main menu with the Escape key.
      if (MenuExit == MENU_EXIT_ESCAPE){
        AfterTool = TRUE;
        break;   //refresh main menu
        //           continue;
      }

      switch (ChosenEntry->Tag) {

        case TAG_RESET:    // Restart
          TerminateScreen();
          gRS->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
          MainLoopRunning = FALSE;   // just in case we get this far
          break;

        case TAG_SHUTDOWN: // It is not Shut Down, it is Exit from Clover
          TerminateScreen();
          //         gRS->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
          MainLoopRunning = FALSE;   // just in case we get this far
          ReinitDesktop = FALSE;
          AfterTool = TRUE;
          break;

        case TAG_OPTIONS:    // Options like KernelFlags, DSDTname etc.
          OptionsMenu(&OptionEntry);
          if (gThemeChanged) // If theme has changed reinit the desktop
            MainLoopRunning = FALSE;
          break;

        case TAG_ABOUT:    // About rEFIt
          AboutRefit();
          break;

        case TAG_LOADER:   // Boot OS via .EFI loader
          StartLoader((LOADER_ENTRY *)ChosenEntry);
          break;

        case TAG_LEGACY:   // Boot legacy OS
          StartLegacy((LEGACY_ENTRY *)ChosenEntry);
          break;

        case TAG_TOOL:     // Start a EFI tool
          StartTool((LOADER_ENTRY *)ChosenEntry);
          TerminateScreen(); //does not happen
          //   return EFI_SUCCESS;
          //  BdsLibConnectAllDriversToAllControllers();
          //    PauseForKey(L"Returned from StartTool\n");
          MainLoopRunning = FALSE;
          AfterTool = TRUE;
          break;

        case TAG_CLOVER:     // Clover options
          LoaderEntry = (LOADER_ENTRY *)ChosenEntry;
          if (LoaderEntry->LoadOptions != NULL) {

            if (gEmuVariableControl != NULL) {
              gEmuVariableControl->UninstallEmulation(gEmuVariableControl);
            }

            if (StrStr(LoaderEntry->LoadOptions, L"BO-ADD") != NULL) {
              PrintBootOptions(FALSE);
              Status = AddBootOptionForFile (LoaderEntry->Volume->DeviceHandle,
                                             LoaderEntry->LoaderPath,
                                             TRUE,
                                             L"Clover OS X Boot",
                                             0,
                                             NULL
                                             );
              PrintBootOptions(FALSE);
            } else if (StrStr(LoaderEntry->LoadOptions, L"BO-REMOVE-ALL") != NULL) {
              PrintBootOptions(FALSE);
              DeleteBootOptionsContainingFile (L"CLOVERX64.efi");
              DeleteBootOptionsContainingFile (L"CLOVERIA32.efi");
              PrintBootOptions(FALSE);
            } else if (StrStr(LoaderEntry->LoadOptions, L"BO-REMOVE") != NULL) {
              PrintBootOptions(FALSE);
              Status = DeleteBootOptionForFile (LoaderEntry->Volume->DeviceHandle,
                                                LoaderEntry->LoaderPath
                                                );
              PrintBootOptions(FALSE);
            } else if (StrStr(LoaderEntry->LoadOptions, L"BO-PRINT") != NULL) {
              PrintBootOptions(TRUE);
            }

            if (gEmuVariableControl != NULL) {
              gEmuVariableControl->InstallEmulation(gEmuVariableControl);
            }
          }
          MainLoopRunning = FALSE;
          AfterTool = TRUE;
          break;

      } //switch
    } //MainLoopRunning
    UninitRefitLib();
    if (!AfterTool) {
      //   PauseForKey(L"After uninit");
      //reconnectAll
      if (!gFirmwareClover) {
        BdsLibConnectAllEfi();
      }
      else {
        DBG("ConnectAll after refresh menu\n");
        BdsLibConnectAllDriversToAllControllers();
      }
      //  ReinitRefitLib();
      //    PauseForKey(L"After ReinitRefitLib");
    }
    ReinitSelfLib();
    //    PauseForKey(L"After ReinitSelfLib");
  } while (ReinitDesktop);
  
  // If we end up here, things have gone wrong. Try to reboot, and if that
  // fails, go into an endless loop.
  //Slice - NO!!! Return to EFI GUI
  //   gRS->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
  //   EndlessIdleLoop();

  // Unload EmuVariable before returning to EFI GUI, as it should not be present when booting other Operating Systems.
  // This seems critical in some UEFI implementations, such as Phoenix UEFI 2.0
  if (gEmuVariableControl != NULL) {
    gEmuVariableControl->UninstallEmulation(gEmuVariableControl);
  }  

  return EFI_SUCCESS;
}
