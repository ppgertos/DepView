#pragma once

#include "LogBook.h"

#include <raylib.h>

void FrameList_Draw(const Rectangle rect, size_t* currentLog, bool* refreshNeeded, const LogBook* logBook);

