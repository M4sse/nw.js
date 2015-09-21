#ifndef CONTENT_NW_SRC_API_CLIPBOARD_CLIPBOARD_MAC_H_
#define CONTENT_NW_SRC_API_CLIPBOARD_CLIPBOARD_MAC_H_

#include <vector>
#include <string>

#include "content/nw/src/nw_shell.h"

bool isPressed(unsigned short inKeyCode);

void DoDragAndDropCocoa(std::vector<std::string> files,
	content::Shell* shell);

void registerNotificationWindow(std::string message_, 
	content::Shell* shell);

void createNotificationWindow();
void closeNotificationWindow();


#endif
