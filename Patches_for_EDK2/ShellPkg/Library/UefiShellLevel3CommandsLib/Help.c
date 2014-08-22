/** @file
  Main file for Help shell level 3 function.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel3CommandsLib.h"

#include <Library/ShellLib.h>
#include <Library/HandleParsingLib.h>

#include <Protocol/EfiShellDynamicCommand.h>

/**
  Attempt to print help from a dynamically added command.

  @param[in]  CommandToGetHelpOn  The unicode name of the command that help is requested on.

  @retval EFI_SUCCESS             The help was displayed
  @retval FI_NOT_FOUND            The command name could not be found
**/
STATIC
EFI_STATUS
EFIAPI
PrintDynamicCommandHelp(
  IN  CHAR16 *CommandToGetHelpOn
  )
{
  EFI_STATUS                          Status;
  BOOLEAN                             Found;
  EFI_HANDLE                          *CommandHandleList;
  EFI_HANDLE                          *NextCommand;
  EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *DynamicCommand;
  CHAR16                              *OutText;
  
  Found = FALSE;
  CommandHandleList = NULL;
  OutText = NULL;

  CommandHandleList = GetHandleListByProtocol(&gEfiShellDynamicCommandProtocolGuid);

  if (CommandHandleList == NULL) {
    //
    // not found or out of resources
    //
    return FALSE;
  }

  for (NextCommand = CommandHandleList; *NextCommand != NULL; NextCommand++) {
    Status = gBS->HandleProtocol(
      *NextCommand,
      &gEfiShellDynamicCommandProtocolGuid,
      (VOID **)&DynamicCommand
      );

    if (EFI_ERROR(Status)) {
      continue;
    }

    if ((gUnicodeCollation->MetaiMatch(gUnicodeCollation, (CHAR16 *)DynamicCommand->CommandName, CommandToGetHelpOn)) ||
      (gEfiShellProtocol->GetAlias(CommandToGetHelpOn, NULL) != NULL && (gUnicodeCollation->MetaiMatch(gUnicodeCollation, (CHAR16 *)DynamicCommand->CommandName, (CHAR16*)(gEfiShellProtocol->GetAlias(CommandToGetHelpOn, NULL)))))) {
      //
      // TODO: how to get proper language?
      //
      OutText = DynamicCommand->GetHelp(DynamicCommand, "en");

      if (OutText == NULL) {
        continue;
      }

      //
      // Trim extra characters from the end the the string before printing
      //
      while (StrLen(OutText) > 0 
        && (OutText[StrLen(OutText) - 1] == L'\r' || OutText[StrLen(OutText) - 1] == L'\n' || OutText[StrLen(OutText) - 1] == L' ')) {
        OutText[StrLen(OutText) - 1] = CHAR_NULL;
      }

      //
      // Make sure we have something to print still.
      //
      if (StrLen(OutText) == 0) {
        FreePool(OutText);
        OutText = NULL;
        continue;
      }

      //
      // Print and move on.
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_HELP_COMMAND), gShellLevel3HiiHandle, DynamicCommand->CommandName, OutText);
      FreePool(OutText);
      OutText = NULL;
      Found = TRUE;
      break;
    }

  }

  FreePool(CommandHandleList);

  return Found ? EFI_SUCCESS : EFI_NOT_FOUND;
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-usage", TypeFlag},
  {L"-section", TypeMaxValue},
  {L"-verbose", TypeFlag},
  {L"-v", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'help' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunHelp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CONST COMMAND_LIST  *CommandList;
  CONST COMMAND_LIST  *Node;
  CHAR16              *CommandToGetHelpOn;
  CHAR16              *SectionToGetHelpOn;
  CHAR16              *HiiString;
  BOOLEAN             Found;
  BOOLEAN             PrintCommandText;

  PrintCommandText    = TRUE;
  Package             = NULL;
  ProblemParam        = NULL;
  ShellStatus         = SHELL_SUCCESS;
  CommandToGetHelpOn  = NULL;
  SectionToGetHelpOn  = NULL;
  Found               = FALSE;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
//  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = CommandInit();
  //  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
     // ASSERT(FALSE);
      return Status;
    }
  } else {
    //
    // Check for conflicting parameters.
    //
    if (ShellCommandLineGetFlag(Package, L"-usage") &&
        ShellCommandLineGetFlag(Package, L"-section") &&
        (ShellCommandLineGetFlag(Package, L"-verbose") ||
         ShellCommandLineGetFlag(Package, L"-v"))) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CON), gShellLevel3HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetRawValue(Package, 2) != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // Get the command name we are getting help on
      //
  //    ASSERT(CommandToGetHelpOn == NULL);
      StrnCatGrow(&CommandToGetHelpOn, NULL, ShellCommandLineGetRawValue(Package, 1), 0);
      if (CommandToGetHelpOn == NULL && ShellCommandLineGetFlag(Package, L"-?")) {
        //
        // If we dont have a command and we got a simple -?
        // we are looking for help on help command.
        //
        StrnCatGrow(&CommandToGetHelpOn, NULL, L"help", 0);
      }

      if (CommandToGetHelpOn == NULL) {
        StrnCatGrow(&CommandToGetHelpOn, NULL, L"*", 0);
  //      Print(L"if NULL then *\n");
 //       ASSERT(SectionToGetHelpOn == NULL);
        StrnCatGrow(&SectionToGetHelpOn, NULL, L"NAME", 0);
      } else {
        PrintCommandText = FALSE;
//        ASSERT(SectionToGetHelpOn == NULL);
        //
        // Get the section name for the given command name
        //
        if (ShellCommandLineGetFlag(Package, L"-section")) {
          StrnCatGrow(&SectionToGetHelpOn, NULL, ShellCommandLineGetValue(Package, L"-section"), 0);
        } else if (ShellCommandLineGetFlag(Package, L"-usage")) {
          StrnCatGrow(&SectionToGetHelpOn, NULL, L"NAME,SYNOPSIS", 0);
        } else if (ShellCommandLineGetFlag(Package, L"-verbose") || ShellCommandLineGetFlag(Package, L"-v")) {
        } else {
          //
          // The output of help <command> will display NAME, SYNOPSIS, OPTIONS, DESCRIPTION, and EXAMPLES sections.
          //
          StrnCatGrow (&SectionToGetHelpOn, NULL, L"NAME,SYNOPSIS,OPTIONS,DESCRIPTION,EXAMPLES", 0);
        }
      }

      if (gUnicodeCollation->StriColl(gUnicodeCollation, CommandToGetHelpOn, L"special") == 0) {
        //
        // we need info on the special characters
        //
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_SC_HEADER), gShellLevel3HiiHandle);
        HiiString = HiiGetString(gShellLevel3HiiHandle, STRING_TOKEN(STR_HELP_SC_DATA), NULL);
        ShellPrintEx(-1, -1, L"%s", HiiString);
        FreePool(HiiString);
        Found = TRUE;
      } else {
        CommandList = ShellCommandGetCommandList(TRUE);
 //       ASSERT(CommandList != NULL);
        for ( Node = (COMMAND_LIST*)GetFirstNode(&CommandList->Link)
            ; CommandList != NULL && !IsListEmpty(&CommandList->Link) && !IsNull(&CommandList->Link, &Node->Link)
            ; Node = (COMMAND_LIST*)GetNextNode(&CommandList->Link, &Node->Link)
           ){
          //
          // Checking execution break flag when print multiple command help information.
          //
          if (ShellGetExecutionBreakFlag ()) {
            break;
          } 
          if ((gUnicodeCollation->MetaiMatch(gUnicodeCollation, Node->CommandString, CommandToGetHelpOn)) ||
             (gEfiShellProtocol->GetAlias(CommandToGetHelpOn, NULL) != NULL && (gUnicodeCollation->MetaiMatch(gUnicodeCollation, Node->CommandString, (CHAR16*)(gEfiShellProtocol->GetAlias(CommandToGetHelpOn, NULL)))))) {
     //       Print(L"found node %s\n", Node->CommandString);
            //
            // We have a command to look for help on.
            //
            Status = ShellPrintHelp(Node->CommandString, SectionToGetHelpOn, PrintCommandText);
            if (Status == EFI_DEVICE_ERROR) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_INV), gShellLevel3HiiHandle, Node->CommandString);
            } else if (EFI_ERROR(Status)) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_NF), gShellLevel3HiiHandle, Node->CommandString);
            } else {
                Found = TRUE;
            }
          }
        }
        //
        // Search the .man file for Shell applications (Shell external commands).
        //
        if (!Found) {
            Status = ShellPrintHelp(CommandToGetHelpOn, SectionToGetHelpOn, FALSE);
            if (Status == EFI_DEVICE_ERROR) {
               ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_INV), gShellLevel3HiiHandle, CommandToGetHelpOn);
            } else if (EFI_ERROR(Status)) {
               ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_NF), gShellLevel3HiiHandle, CommandToGetHelpOn);
            } else {
              Found = TRUE;
            }
        }

        //
        // now try to match against the dynamic command list and print help
        //
        Status = PrintDynamicCommandHelp(CommandToGetHelpOn);
        if (Status == EFI_SUCCESS) {
          Found = TRUE;
        }
        
      }

      if (!Found) {
        ShellStatus = SHELL_NOT_FOUND;
      }

      //
      // free the command line package
      //
      ShellCommandLineFreeVarList (Package);
    }
  }

  if (CommandToGetHelpOn != NULL && StrCmp(CommandToGetHelpOn, L"*") == 0){
    //
    // If '*' then the command entered was 'Help' without qualifiers, This footer
    // provides additional info on help switches
    //
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HELP_FOOTER), gShellLevel3HiiHandle);
  }
  if (CommandToGetHelpOn != NULL) {
    FreePool(CommandToGetHelpOn);
  }
  if (SectionToGetHelpOn != NULL) {
    FreePool(SectionToGetHelpOn);
  }

  return (ShellStatus);
}
