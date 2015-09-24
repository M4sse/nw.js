#ifndef CONTENT_NW_SRC_API_CLIPBOARD_CLIPBOARD_WIN_H_
#define CONTENT_NW_SRC_API_CLIPBOARD_CLIPBOARD_WIN_H_

#include <vector>
#include <string>

#include "content/nw/src/nw_shell.h"

void DoDragAndDropWin32(std::vector<std::string> files,
	content::Shell* shell);

void registerNotificationWindow(std::string message_,
	content::Shell* shell);

void createNotificationWindow();
void closeNotificationWindow();

#endif
