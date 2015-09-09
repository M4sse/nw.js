#include "clipboard_mac.h"

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include "content/nw/src/browser/native_window_mac.h"

using namespace std;

static NSWindow* window = nil;
static NSTimer* timer = nil;

@interface TimerFunction : NSObject

- (void)updatePosition;

@end

@implementation TimerFunction

- (void)updatePosition {
    const unsigned short kVK_Option = 0x3A;
    const CGFloat kDistance = 3.0f;
    
    if (window == nil) {
        return;
    }
    
    if (isPressed(kVK_Option)) {
        [window close];
        [timer invalidate];
        timer = nil;
        //window = nil;
    } else {
        NSPoint mouseLocation = [NSEvent mouseLocation];
        mouseLocation.x = mouseLocation.x + kDistance;
        mouseLocation.y = mouseLocation.y + kDistance;
        [window setFrameOrigin:mouseLocation];
    }
}

@end




bool isPressed(unsigned short inKeyCode) {
	unsigned char keyMap[16];
    GetKeys((BigEndianUInt32*) &keyMap);
    return (0 != ((keyMap[ inKeyCode >> 3] >> (inKeyCode & 7)) & 1));
}



void createNotificationWindow(string message) {
    const CGFloat kPadding = 3.0f;
    const CGFloat kUpdateIntervall = 0.01f;
    
    NSString* textInput = [NSString stringWithUTF8String:message.c_str()];
    
    NSPoint mouseLocation = [NSEvent mouseLocation];
    NSDictionary* attributes =
    [NSDictionary dictionaryWithObjectsAndKeys:
     [NSFont fontWithName:@"Helvetica" size:12],
     NSFontAttributeName, nil];
    
    NSAttributedString* text =
    [[NSAttributedString alloc] initWithString:textInput
                                    attributes:attributes];
    NSSize textSize = [text size];
    
    NSRect frame = NSMakeRect(mouseLocation.x,
                              mouseLocation.y,
                              textSize.width + 35 + kPadding * 2,
                              textSize.height + kPadding * 2);
    
    window = [[NSWindow alloc]
              initWithContentRect:frame
              styleMask:NSBorderlessWindowMask
              backing:NSBackingStoreBuffered
              defer:NO];
    
    [window setOpaque:NO];
    [window setLevel:NSFloatingWindowLevel];
    window.alphaValue = 0.8f;
    [window setBackgroundColor: [NSColor blackColor]];
    [NSApp activateIgnoringOtherApps:YES];
    
    NSTextField* textField = [[NSTextField alloc]
                              initWithFrame: NSMakeRect(kPadding,
                                                        kPadding,
                                                        textSize.width + 35,
                                                        textSize.height + 2)];
    
    [textField setBezeled:NO];
    [textField setDrawsBackground:NO];
    [textField setEditable:NO];
    [textField setSelectable:NO];
    [textField setTextColor: [NSColor whiteColor]];
    
    [textField setStringValue:textInput];
    [[window contentView] addSubview: textField];
    
    [window makeKeyAndOrderFront: window];
    
    TimerFunction* timerFunction = [TimerFunction alloc];
    
    timer = [NSTimer scheduledTimerWithTimeInterval:kUpdateIntervall
                                             target:timerFunction
                                           selector:@selector(updatePosition)
                                           userInfo:nil
                                            repeats:YES];
}

void DoDragAndDropCocoa(vector<string> files, content::Shell* shell) {
    
    NSWindow* window =
      static_cast<nw::NativeWindowCocoa*>(shell->window())->window();
    
    NSPoint position = [window mouseLocationOutsideOfEventStream];
    NSTimeInterval eventTime = [[NSProcessInfo processInfo] systemUptime];
    
    NSEvent* dragEvent = [NSEvent mouseEventWithType:NSLeftMouseDragged
                                            location:position
                                       modifierFlags:NSLeftMouseDraggedMask
                                           timestamp:eventTime
                                        windowNumber:[window windowNumber]
                                             context:nil
                                         eventNumber:0
                                          clickCount:1
                                            pressure:1.0];
    
    vector<NSString*> filesNS;
    for (string file : files) {
        NSString* fileNS = [NSString stringWithUTF8String:file.c_str()];
        filesNS.push_back(fileNS);
    }
    
    NSArray* fileList = [NSArray arrayWithObjects:&filesNS[0]
                                            count:filesNS.size()];
    
    NSImage* dragImage = [[NSWorkspace sharedWorkspace] iconForFiles:fileList];
    NSPoint dragPosition = position;
    
    NSPasteboard* pboard = [NSPasteboard pasteboardWithName:NSDragPboard];
    [pboard declareTypes: [NSArray arrayWithObject:NSFilenamesPboardType] owner:nil];
    [pboard setPropertyList:fileList forType:NSFilenamesPboardType];
    
    [window dragImage:dragImage
                   at:dragPosition
               offset:NSZeroSize
                event:dragEvent
           pasteboard:pboard
               source:window
            slideBack:YES];
}
