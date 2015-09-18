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
    const CGFloat kDistance = 3.0f;
    
    if (window == nil) {
        return;
    }
    
    NSRect windowFrame = [window frame];
    NSPoint mouseLocation = [NSEvent mouseLocation];
    BOOL isInside = NSPointInRect(mouseLocation, windowFrame);
    
    [window setIsVisible:!isInside];
    
    const NSUInteger pressedButtonMask = [NSEvent pressedMouseButtons];
    const BOOL leftMouseDown = (pressedButtonMask & (1 << 0)) != 0;
    
    if (isPressed(kVK_Control) || !leftMouseDown) {
        [window close];
        [timer invalidate];
        timer = nil;
        //window = nil;
    } else {
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

void closeNotificationWindow() {
    [window close];
    [timer invalidate];
}

void createNotificationWindow(string message) {
    const CGFloat kPaddingLeft = 0;
    const CGFloat kPaddingTop = 0;
    const CGFloat kPaddingRight = 0;
    const CGFloat kPaddingBottom = 0;
    NSColor* kBorderColor = [NSColor colorWithSRGBRed:0.8f green:0.8f blue:0.8f alpha:1.0f];
    NSColor* kBackgroundColor = [NSColor colorWithSRGBRed:1.0f green:1.0f blue:1.0f alpha:1.0f];
    NSColor* kTextColor = [NSColor colorWithSRGBRed:0.4392f green:0.4352f blue:0.4352f alpha:1.0f];
    const CGFloat kUpdateIntervall = 0.01f;
    NSString* kFontName = @"Helvetica-Bold";
    const int kFontSize = 12;
    const int kLayoutWidth = 6;
    const int kBorderThickness = 1;
    
    NSString* textInput = [NSString stringWithUTF8String:message.c_str()];
    NSPoint mouseLocation = [NSEvent mouseLocation];
    
    // Calculate Textsize
    NSFont* font = [NSFont fontWithName:kFontName size:kFontSize];
    NSDictionary* attributes = [NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, nil];
    NSSize textSize = [textInput sizeWithAttributes:attributes];
    
    NSRect frame = NSMakeRect(mouseLocation.x, mouseLocation.y,
                              textSize.width + kLayoutWidth + kBorderThickness + kPaddingLeft*2 + kPaddingRight*2,
                              textSize.height + kBorderThickness + kPaddingTop*2 + kPaddingBottom*2);
    
    window = [[NSWindow alloc]
              initWithContentRect:frame
              styleMask:NSBorderlessWindowMask
              backing:NSBackingStoreBuffered
              defer:NO];
    
    [window setOpaque:NO];
    [window setLevel:NSFloatingWindowLevel];
    window.alphaValue = 0.9f;
    [window setBackgroundColor: kBorderColor];
    [NSApp activateIgnoringOtherApps:YES];
    
    NSTextField* textField = [[NSTextField alloc] initWithFrame:NSMakeRect(
        kBorderThickness + kPaddingLeft,
        kBorderThickness + kPaddingBottom,
        textSize.width + kLayoutWidth + kPaddingRight,
        textSize.height + kPaddingTop)];
    
    [textField setBezeled:NO];
    //[textField setDrawsBackground:NO];
    [textField setEditable:NO];
    [textField setSelectable:NO];
    [textField setFont:font];
    [textField setTextColor: kTextColor];
    [textField setBackgroundColor: kBackgroundColor];
    
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
