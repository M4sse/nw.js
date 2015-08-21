#include "clipboard_mac.h"

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include "content/nw/src/browser/native_window_mac.h"

using namespace std;

bool isPressed(unsigned short inKeyCode) {
	unsigned char keyMap[16];
    GetKeys((BigEndianUInt32*) &keyMap);
    return (0 != ((keyMap[ inKeyCode >> 3] >> (inKeyCode & 7)) & 1));
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
