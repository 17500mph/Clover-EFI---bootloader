//
//  CloverPrefpane.h
//  CloverPrefpane
//
//  Created by JrCs on 03/05/13.
//  Copyright (c) 2013 ProjectOSX. All rights reserved.
//

#import <PreferencePanes/PreferencePanes.h>
#import <ServiceManagement/ServiceManagement.h>
#import <SecurityInterface/SFAuthorizationView.h>

@interface CloverPrefpane : NSPreferencePane {
    io_registry_entry_t _ioRegEntryRef;
    NSString* agentPlistPath;

    IBOutlet NSPopUpButton *popUpCheckInterval;
    IBOutlet NSTextField *LastRunDate;
    IBOutlet NSButton *checkNowButton;
    IBOutlet SFAuthorizationView *authView;
    IBOutlet NSTextField *lastBootedRevision;
    IBOutlet NSTextField *lastInstalledRevision;

    IBOutlet NSTextField *_themeTextField;
    IBOutlet NSTextField *_logLineCountTextField;
    IBOutlet NSTextField *_logEveryBootTextField;
    IBOutlet NSTextField *_mountEFITextField;
    IBOutlet NSTextField *_nvRamDiskTextField;
}

- (id) initWithBundle:(NSBundle *)bundle;
- (void) mainViewDidLoad;
- (IBAction) checkNow:(id)sender;
- (IBAction) configureAutomaticUpdates:(id)sender;
- (IBAction) simpleNvramVariableChanged:(id)sender;

- (BOOL)isUnlocked;

- (unsigned int) getUIntPreferenceKey:(CFStringRef)key
                             forAppID:(CFStringRef)appID
                          withDefault:(unsigned int)defaultValue;
- (void) setPreferenceKey:(CFStringRef)key
                 forAppID:(CFStringRef)appID
                  fromInt:(int)value;
@end
