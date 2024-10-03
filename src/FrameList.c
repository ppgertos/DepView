#include "FrameList.h"

#include <raygui.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>


void FrameList_Draw(const Rectangle rect, size_t* currentLog, bool* refreshNeeded, const LogBook* logBook) {
  const float CHAR_SIZE = 14;  // TODO: placeholder - should be taken from theme parameters
  static Vector2 offsetVec = {0, 0};
  static struct Rectangle visibleRect = {0, 0, 0, 0};
  GuiScrollPanel(rect, NULL,
                 (Rectangle){.x = 0, .y = 0, .width = rect.width - 15, .height = logBook->entriesSize * CHAR_SIZE},
                 &offsetVec, &visibleRect);
  BeginScissorMode(visibleRect.x, visibleRect.y, visibleRect.width, visibleRect.height);
  for (int i = 0; i < logBook->entriesSize; ++i) {
    char label[32];
    struct tm* timeInfo = gmtime(&logBook->entries[i].timestamp);
    strftime(label, 32, "%H:%M:%S %d-%m-%Y", timeInfo);
    if (*currentLog == i) {
      GuiSetState(STATE_PRESSED);
    }

    if (GuiLabelButton((Rectangle){visibleRect.x, visibleRect.y + CHAR_SIZE * i + offsetVec.y, rect.width, CHAR_SIZE},
                       label)) {
      *currentLog = i;
      *refreshNeeded = true;
    }
    GuiSetState(STATE_NORMAL);
  }
  EndScissorMode();
}
