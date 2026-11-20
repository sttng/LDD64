// LEGO Digital Designer — restored source
// Objective-C helpers of the Platform.mm file dialogs (GetOpenPath / GetSavePath). TU 267, Mac
// 0x282450-0x28275f; file name inferred. Compiled only with Objective-C.
//   LEGOGetPathSheetDelegate: sheet end → *(bool*)context = OK, stops the modal loop.
//   LEGOGetPathAccessoryViewOwner (instance size 20): owner of nib "SavePanelAccessoryView",
//     whose `filter` pop-up picks the save type; the index goes to the caller's variable.
#if !defined(_WIN32) && defined(__OBJC__)
#import <Cocoa/Cocoa.h>

// PORT64: the selected index is the caller's UInt32 (unsigned long on i386, unsigned int on
// 64-bit, where an unsigned long* would write 8 bytes into it; Platform.mm has the same typedef).
#if defined(__LP64__)
typedef unsigned int LEGOTypeIndex;
#else
#define LEGOTypeIndex unsigned long   // (spelled out: clang encodes the typedef as unsigned int)
#endif

@interface LEGOGetPathSheetDelegate : NSObject
+ (void)savePanelDidEnd:(NSSavePanel*)panel returnCode:(int)code contextInfo:(void*)info;
+ (void)openPanelDidEnd:(NSOpenPanel*)panel returnCode:(int)code contextInfo:(void*)info;
@end

@interface LEGOGetPathAccessoryViewOwner : NSObject {
    IBOutlet NSPopUpButton* filter;      // +4 (nib outlet)
    NSSavePanel*            panel;       // +8
    NSMutableArray*         types;       // +12 (retained)
    LEGOTypeIndex*          selectedIndex; // +16
}
- (id)init;
- (void)dealloc;
- (IBAction)filterChanged:(id)sender;
- (void)setUpPanel:(NSSavePanel*)aPanel withFilterNames:(NSArray*)names types:(NSArray*)someTypes
  andSelectedIndex:(LEGOTypeIndex*)index;
@end

@implementation LEGOGetPathSheetDelegate

// Mac 0x282450
+ (void)savePanelDidEnd:(NSSavePanel*)panel returnCode:(int)code contextInfo:(void*)info
{
    (void)panel;
    *(bool*)info = code == NSOKButton;
    [NSApp stopModal];
}

// Mac 0x282490
+ (void)openPanelDidEnd:(NSOpenPanel*)panel returnCode:(int)code contextInfo:(void*)info
{
    (void)panel;
    *(bool*)info = code == NSOKButton;
    [NSApp stopModal];
}
@end

@implementation LEGOGetPathAccessoryViewOwner

// Mac 0x2824d0: loads the accessory view nib with self as its owner (sets `filter`).
- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    if (![NSBundle loadNibNamed:@"SavePanelAccessoryView" owner:self]) {
        NSLog(@"Warning! Could not load myNib file.\n");
        return nil;   // NOTE: self is not released
    }
    return self;
}

// Mac 0x282550
- (void)dealloc
{
    [types release];
    [super dealloc];
}

// Mac 0x2825a0: the chosen type becomes the panel's required file type.
// NOTE: indexOfSelectedItem is sent twice (the first result is unused).
- (IBAction)filterChanged:(id)sender
{
    (void)sender;
    [filter indexOfSelectedItem];
    NSInteger index = [filter indexOfSelectedItem];
    *selectedIndex = (LEGOTypeIndex)index;
#if defined(__LP64__)
    // PORT64: setRequiredFileType: is deprecated since 10.6
    [panel setAllowedFileTypes:[NSArray arrayWithObject:[types objectAtIndex:index]]];
#else
    [panel setRequiredFileType:[types objectAtIndex:index]];
#endif
}

// Mac 0x282610: the pop-up's superview (the nib's view) becomes the accessory view.
// NOTE: the counts of `names` and `someTypes` are read and ignored (an assertion of a debug
// build); the first type is selected regardless of *index.
- (void)setUpPanel:(NSSavePanel*)aPanel withFilterNames:(NSArray*)names types:(NSArray*)someTypes
  andSelectedIndex:(LEGOTypeIndex*)index
{
    [names count];
    [someTypes count];
    selectedIndex = index;
    [types release];
    types = (NSMutableArray*)[someTypes retain];
    panel = aPanel;
    [aPanel setAccessoryView:[filter superview]];
    [filter removeAllItems];
    [filter addItemsWithTitles:names];
    [filter selectItemAtIndex:0];
    NSInteger selected = [filter indexOfSelectedItem];
    *selectedIndex = (LEGOTypeIndex)selected;
#if defined(__LP64__)
    [panel setAllowedFileTypes:[NSArray arrayWithObject:[types objectAtIndex:selected]]];
#else
    [panel setRequiredFileType:[types objectAtIndex:selected]];
#endif
}
@end
#endif
