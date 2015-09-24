#include "clipboard_mac.h"

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include "content/nw/src/browser/native_window_mac.h"

using namespace std;

static NSWindow* notificationWindow = nil;
static NSWindow* mainWindow = nil;
static NSTimer* timer = nil;
static string message = "";
static BOOL wndIsOpen = false;

@interface TimerFunction : NSObject

- (void)checkMousePosition;

@end

@implementation TimerFunction

- (void)checkMousePosition {
    const CGFloat kDistance = 3.0f;
    
    if (mainWindow == nil) {
        return;
    }
    
    NSRect windowFrame = [mainWindow frame];
    NSPoint mouseLocation = [NSEvent mouseLocation];
    BOOL isInside = NSPointInRect(mouseLocation, windowFrame);
    
    const NSUInteger pressedButtonMask = [NSEvent pressedMouseButtons];
    const BOOL leftMouseDown = (pressedButtonMask & (1 << 0)) != 0;
    
    if (isInside) {
        if (isPressed(kVK_Shift || !leftMouseDown)) {
            if (timer != nil) {
                [timer invalidate];
                timer = nil;
            }
        }
        closeNotificationWindow();
    } else {
        createNotificationWindow();
       
        if (isPressed(kVK_Shift) || !leftMouseDown) {
            closeNotificationWindow();
            
            if (timer != nil) {
                [timer invalidate];
                timer = nil;
            }
        } else {
            NSPoint mouseLocation = [NSEvent mouseLocation];
            mouseLocation.x = mouseLocation.x + kDistance;
            mouseLocation.y = mouseLocation.y + kDistance;
            [notificationWindow setFrameOrigin:mouseLocation];
        }
    }
}
@end

static TimerFunction* timerFunction = nil;

bool isPressed(unsigned short inKeyCode) {
	unsigned char keyMap[16];
    GetKeys((BigEndianUInt32*) &keyMap);
    return (0 != ((keyMap[ inKeyCode >> 3] >> (inKeyCode & 7)) & 1));
}

void registerNotificationWindow(string message_, content::Shell* shell) {
    const CGFloat kUpdateIntervall = 0.01f;

    message = message_;
    mainWindow =
        static_cast<nw::NativeWindowCocoa*>(shell->window())->window();
    
    if (timer == nil) {
        
        if (timerFunction == nil) {
            timerFunction = [TimerFunction alloc];
        }
            
        timer = [NSTimer scheduledTimerWithTimeInterval:kUpdateIntervall
                                             target:timerFunction
                                           selector:@selector(checkMousePosition)
                                           userInfo:nil
                                            repeats:YES];
    }
}

void closeNotificationWindow() {
    if (wndIsOpen) {
        [notificationWindow close];
        notificationWindow = nil;
        wndIsOpen = false;
    }
}

void createNotificationWindow() {
    if (wndIsOpen) {
        return;
    }
    wndIsOpen = true;
    
    const CGFloat kPaddingLeft = 0;
    const CGFloat kPaddingTop = 0;
    const CGFloat kPaddingRight = 0;
    const CGFloat kPaddingBottom = 0;
    NSColor* kBorderColor = [NSColor colorWithSRGBRed:0.8f green:0.8f blue:0.8f alpha:1.0f];
    NSColor* kBackgroundColor = [NSColor colorWithSRGBRed:1.0f green:1.0f blue:1.0f alpha:1.0f];
    NSColor* kTextColor = [NSColor colorWithSRGBRed:0.4392f green:0.4352f blue:0.4352f alpha:1.0f];
    NSString* kFontName = @"Helvetica-Bold";
    const int kFontSize = 12;
    const int kLayoutWidth = 6;
    const int kBorderThickness = 1;
    
    NSPoint mouseLocation = [NSEvent mouseLocation];
    
    // Calculate Textsize
    NSFont* font = [NSFont fontWithName:kFontName size:kFontSize];
    NSDictionary* attributes = [NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, nil];
    NSString* textInput = [NSString stringWithUTF8String:message.c_str()];
    NSSize textSize = [textInput sizeWithAttributes:attributes];
    
    NSRect frame = NSMakeRect(mouseLocation.x, mouseLocation.y,
                              textSize.width + kLayoutWidth + kBorderThickness + kPaddingLeft*2 + kPaddingRight*2,
                              textSize.height + kBorderThickness + kPaddingTop*2 + kPaddingBottom*2);
    
    notificationWindow = [[NSWindow alloc]
              initWithContentRect:frame
              styleMask:NSBorderlessWindowMask
              backing:NSBackingStoreBuffered
              defer:NO];

    [notificationWindow setOpaque:NO];
    [notificationWindow setLevel:NSFloatingWindowLevel];
    notificationWindow.alphaValue = 0.9f;
    [notificationWindow setBackgroundColor: kBorderColor];
    [NSApp activateIgnoringOtherApps:YES];
    [notificationWindow setReleasedWhenClosed:YES];
    
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
    [[notificationWindow contentView] addSubview: textField];
    
    [notificationWindow makeKeyAndOrderFront: notificationWindow];
    
}

void DoDragAndDropCocoa(vector<string> files, content::Shell* shell) {
    
    
    mainWindow =
        static_cast<nw::NativeWindowCocoa*>(shell->window())->window();

    
    NSPoint position = [mainWindow mouseLocationOutsideOfEventStream];
    NSTimeInterval eventTime = [[NSProcessInfo processInfo] systemUptime];
    
    NSEvent* dragEvent = [NSEvent mouseEventWithType:NSLeftMouseDragged
                                            location:position
                                       modifierFlags:NSLeftMouseDraggedMask
                                           timestamp:eventTime
                                        windowNumber:[mainWindow windowNumber]
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
    
    [mainWindow dragImage:dragImage
                   at:dragPosition
               offset:NSZeroSize
                event:dragEvent
           pasteboard:pboard
               source:mainWindow
            slideBack:YES];
    
    // Send global shift press to fix the bug, where you have to hit shift twice
    CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, kVK_Shift, YES);
    CGEventRef keyUp   = CGEventCreateKeyboardEvent(NULL, kVK_Shift, NO);
    
    CGEventPost(kCGSessionEventTap, keyDown);
    CGEventPost(kCGSessionEventTap, keyUp);
    
    CFRelease(keyDown);
    CFRelease(keyUp);
}
