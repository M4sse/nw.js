// Copyright (c) 2012 Intel Corp
// Copyright (c) 2012 The Chromium Authors
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell co
// pies of the Software, and to permit persons to whom the Software is furnished
//  to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in al
// l copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IM
// PLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNES
// S FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
//  OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WH
// ETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "content/nw/src/api/clipboard/clipboard.h"

#include "base/values.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/string16.h"
#include "content/nw/src/api/dispatcher_host.h"
#include "ui/base/clipboard/clipboard.h"
#include "content/public/browser/browser_thread.h"
#include "content/nw/src/nw_shell.h"

#ifdef OS_MACOSX
#include "clipboard_mac.h"
#endif

#ifdef OS_WIN
#include "clipboard_win.h"
#endif

namespace nwapi {

Clipboard::Clipboard(int id,
           const base::WeakPtr<DispatcherHost>& dispatcher_host,
           const base::DictionaryValue& option)
    : Base(id, dispatcher_host, option) {
}

Clipboard::~Clipboard() {
}

	void DoDragAndDrop(std::vector<std::string> files, content::Shell* shell) {
		DCHECK(content::BrowserThread::CurrentlyOn(
			content::BrowserThread::UI));
#ifdef OS_MACOSX
		DoDragAndDropCocoa(files, shell);
#endif

#ifdef OS_WIN
		DoDragAndDropWin32(files, shell);
#endif
    
    }
    
void Clipboard::Call(const std::string& method,
                     const base::ListValue& arguments) {
  if (method == "Set") {
    std::string text, type;
    arguments.GetString(0, &text);
    arguments.GetString(1, &type);
    SetText(text);
  } else if (method == "Clear") {
    Clear();
  } else if (method == "Drag") {

#ifdef OS_WIN
      if (!GetAsyncKeyState(VK_SHIFT)) {
          return;
      }
#endif
#ifdef OS_MACOSX
      unsigned short kVK_Shift = 0x38;
      if (!isPressed(kVK_Shift))  {
          return;
      }
#endif

      content::Shell* shell_ =
        content::Shell::FromRenderViewHost(
            dispatcher_host()->render_view_host());
     
      const base::ListValue* fileList = NULL;
      arguments.GetList(0, &fileList);
      
      std::vector<std::string> files;
      
      for (size_t i = 0; i < fileList->GetSize(); i++) {
          std::string file;
          fileList->GetString(i, &file);
          files.push_back(file);
      }
      
      content::BrowserThread::PostTask(
            content::BrowserThread::UI, FROM_HERE,
                base::Bind(&DoDragAndDrop, files, shell_));
      
  } else if (method == "ShowNotification") {
      std::string message;
      arguments.GetString(0, &message);
      
      content::Shell* shell_ = content::Shell::FromRenderViewHost(
                                    dispatcher_host()->render_view_host());
      
      content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                       base::Bind(&registerNotificationWindow, message, shell_));
  } else if (method == "CloseNotification") {
      content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                       base::Bind(&closeNotificationWindow));
  } else {
    NOTREACHED() << "Invalid call to Clipboard method:" << method
                 << " arguments:" << arguments;
  }
}

void Clipboard::CallSync(const std::string& method,
                         const base::ListValue& arguments,
                         base::ListValue* result) {
  if (method == "Get") {
    result->AppendString(GetText());
  } else {
    NOTREACHED() << "Invalid call to Clipboard method:" << method
                 << " arguments:" << arguments;
  }
}

void Clipboard::SetText(std::string& text) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::ObjectMap map;
  map[ui::Clipboard::CBF_TEXT].push_back(
      std::vector<char>(text.begin(), text.end()));
  clipboard->WriteObjects(ui::CLIPBOARD_TYPE_COPY_PASTE, map);
}

std::string Clipboard::GetText() {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  base::string16 text;
  clipboard->ReadText(ui::CLIPBOARD_TYPE_COPY_PASTE, &text);
  return base::UTF16ToUTF8(text);
}

void Clipboard::Clear() {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->Clear(ui::CLIPBOARD_TYPE_COPY_PASTE);
}

}  // namespace nwapi
