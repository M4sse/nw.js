#ifndef CONTENT_NW_SRC_API_CLIPBOARD_CLIPBOARD_MAC_H_
#define CONTENT_NW_SRC_API_CLIPBOARD_CLIPBOARD_MAC_H_

#include <vector>
#include <string>

#include "content/nw/src/nw_shell.h"

void DoDragAndDropCocoa(std::vector<std::string> files,
	content::Shell* shell);

#endif
