#include "Diagram.h"
#include "LogBook.h"
#include "raygui.h"
#include <raygui.h>

#define RAYGUI_IMPLEMENTATION
#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include <gui_window_file_dialog.h>

#include <raylib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
  LogBook logBook;
  size_t logsSize;
  Diagram currentDiagram;
  Diagram nextDiagram;
} AppContext;

typedef struct DiagramStyle {
  const float VERT_PADDING;
  const float HORI_PADDING;
  const float NODE_H;
  const float NODE_W;
  const float MARGIN;
} DiagramStyle;

struct DiagramStyle DiagramStyle_Default() {
  DiagramStyle ds = {.VERT_PADDING = 10,
                     .HORI_PADDING = 20,
                     .NODE_H = 40,
                     .NODE_W = 120,
                     .MARGIN = 20};
  return ds;
}

//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------
static void LoadFileButtonPressed(GuiWindowFileDialogState *fileDialogState);
static AppContext AppContext_Init();
static void AppContext_Destroy(AppContext *this);
static void AppContext_LoadLog(AppContext *appContext, char *fileName);
static void BuildAppContextDiagram(AppContext *ctx,
                                   const LogEntry *const currentLog);
static void DrawGraph(const LogBook *logBook, const Diagram *diagram,
                      const Vector2 *scrollOffset);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main() {
  // Initialization
  //---------------------------------------------------------------------------------------
  int screenWidth = 800;
  int screenHeight = 600;

  InitWindow(screenWidth, screenHeight, "DepView");

  AppContext appContext = AppContext_Init();
  size_t currentLog = 0;

  char selectedTimestamp[25] = "2024-06-17T21:41:35+0200";
  const char *LoadFileText = "LOAD FILE";

  char selectedFileName[2048] = "FilePath";
  Rectangle scrollPanelView = {0, 0, 0, 0};
  Vector2 scrollPanelScrollOffset = {0, 0};
  Vector2 scrollPanelBoundsOffset = {0, 0};
  GuiWindowFileDialogState fileDialogState =
      InitGuiWindowFileDialog(selectedFileName);
  bool diagramNeedsToChange = true;

  SetTargetFPS(60);

  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    // Update
    {
      if (fileDialogState.SelectFilePressed) {
        printf("File has been selected\n");
        snprintf(selectedFileName, 2048, "%s/%s", fileDialogState.dirPathText,
                 fileDialogState.fileNameText);
        printf("Loading log from %s\n", selectedFileName);
        AppContext_LoadLog(&appContext, selectedFileName);
        fileDialogState.SelectFilePressed = false;
        diagramNeedsToChange = true;
      }

      if (diagramNeedsToChange) {
        printf("DIagram update!\n");
        if (appContext.logsSize > currentLog) {
          Diagram_Destroy(&appContext.currentDiagram);
          appContext.currentDiagram =
              Diagram_Make(&appContext.logBook, currentLog);
          snprintf(selectedTimestamp, 24, "%ld",
                   appContext.logBook.entries[currentLog].timestamp);
        }
        diagramNeedsToChange = false;
      }
    }

    // Draw
    BeginDrawing();
    {
      ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
      if (GuiButton((Rectangle){16, 16, 32, 24}, "<")) {
        currentLog = 0 > currentLog - 1 ? currentLog : currentLog - 1;
        diagramNeedsToChange = true;
      }
      GuiLabel((Rectangle){52, 16, 144, 24}, selectedTimestamp);
      if (GuiButton((Rectangle){200, 16, 32, 24}, ">")) {
        currentLog = currentLog + 1 >= appContext.logBook.entriesSize
                         ? appContext.logBook.entriesSize - 1
                         : currentLog + 1;
        diagramNeedsToChange = true;
      }
      GuiLabel((Rectangle){360, 16, 304, 24}, selectedFileName);
      if (GuiButton((Rectangle){662, 16, 104, 24}, LoadFileText)) {
        LoadFileButtonPressed(&fileDialogState);
      }
      if (fileDialogState.windowActive) {
        GuiSetState(STATE_DISABLED);
      }
      GuiScrollPanel((Rectangle){16, 50, 768 - scrollPanelBoundsOffset.x,
                                 536 - scrollPanelBoundsOffset.y},
                     "", (Rectangle){32, 50, 768, 536},
                     &scrollPanelScrollOffset, &scrollPanelView);
      GuiSetState(STATE_NORMAL);
      BeginScissorMode(scrollPanelView.x, scrollPanelView.y,
                       scrollPanelView.width - scrollPanelBoundsOffset.x,
                       scrollPanelView.height - scrollPanelBoundsOffset.y);
      {
        if (appContext.logsSize > 0) {
          Vector2 subviewOffset;
          subviewOffset.x = scrollPanelView.x + scrollPanelScrollOffset.x;
          subviewOffset.y = scrollPanelView.y + scrollPanelScrollOffset.y;

          DrawGraph(&appContext.logBook, &appContext.currentDiagram,
                    &subviewOffset);
        }
      }
      EndScissorMode();
      GuiWindowFileDialog(&fileDialogState);
    }
    EndDrawing();
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  AppContext_Destroy(&appContext);
  //--------------------------------------------------------------------------------------

  return 0;
}

//------------------------------------------------------------------------------------
// Controls Functions Definitions (local)
//------------------------------------------------------------------------------------
static AppContext AppContext_Init() {
  AppContext ctx;
  ctx.logsSize = 0;
  ctx.currentDiagram = Diagram_Make(NULL, 0);
  return ctx;
}

static void AppContext_Destroy(AppContext *this) {
  Diagram_Destroy(&this->currentDiagram);
  this->logsSize = 0;
}
static void LoadFileButtonPressed(GuiWindowFileDialogState *fileDialogState) {
  fileDialogState->windowActive = true;
}

static void AppContext_LoadLog(AppContext *appContext, char *fileName) {
  appContext->logsSize = 0;
  appContext->logBook = LogBook_Make(fileName);
  appContext->logsSize = appContext->logBook.entriesSize;
  printf("Loaded %zu logs\n", appContext->logsSize);
}

static void BuildLayout(Vector2 *result, const Diagram *diagram) {
  DiagramStyle ds = DiagramStyle_Default();
  int nodesInColumn[16];
  memset(&nodesInColumn, '\0', sizeof(nodesInColumn));
  int nodeToColumn[diagram->nodesSize];
  for (size_t i = 0; i < diagram->nodesSize; ++i) {
    if (diagram->nodes[i].dependencies[0] == (size_t)-1) {
      nodeToColumn[i] = 0;
    } else {
      size_t j = 0;
      size_t maxColumn = 0;
      size_t sourceIndex = diagram->nodes[i].dependencies[j];
      while (sourceIndex != (size_t)-1) {
        if (maxColumn < nodeToColumn[sourceIndex]) {
          maxColumn = nodeToColumn[sourceIndex];
        }
        sourceIndex = diagram->nodes[i].dependencies[++j];
      }
      nodeToColumn[i] = maxColumn + 1;
    }
    result[i].x = ds.MARGIN + nodeToColumn[i] * (ds.NODE_W + ds.HORI_PADDING);
    result[i].y =
        ds.MARGIN + nodesInColumn[nodeToColumn[i]] * (ds.NODE_H + ds.VERT_PADDING);
    ++nodesInColumn[nodeToColumn[i]];
  }
}

static void DrawEdge(const Vector2 *nodesCoords, const Edge *edge,
                     const Vector2 *scrollOffset) {
  DiagramStyle ds = DiagramStyle_Default();

  Vector2 strip[8];

  // middle of nodes bottom side
  {
    strip[0].x = scrollOffset->x + nodesCoords[edge->source].x + ds.NODE_W / 2;
    strip[0].y = scrollOffset->y + nodesCoords[edge->source].y + ds.NODE_H;
  }

  strip[1].x = strip[0].x;
  strip[1].y = strip[0].y + ds.VERT_PADDING / 2;

  // Line on "edge-bus"
  {
    strip[2].x =
        strip[1].x + ds.NODE_W / 2 +
        ds.HORI_PADDING / 5 + (edge->source % 9); // here to add small jumps to distinguish lines
    strip[2].y = strip[1].y;

    strip[3].x = strip[2].x;
    strip[3].y =
        scrollOffset->y + nodesCoords[edge->destination].y + ds.NODE_H / 2;
  }

  // middle point on nodes left side
  {
    strip[4].x = scrollOffset->x + nodesCoords[edge->destination].x;
    strip[4].y = strip[3].y;
  }

  // Arrowhead
  {
    strip[5].x = strip[4].x - ds.HORI_PADDING / 3;
    strip[5].y = strip[4].y - ds.VERT_PADDING / 4;
    strip[6].x = strip[4].x - ds.HORI_PADDING / 3;
    strip[6].y = strip[4].y + ds.VERT_PADDING / 4;
    strip[7] = strip[4];
  }

  DrawLineStrip(strip, 8, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
}

static void DrawGraph(const LogBook *logBook, const Diagram *diagram,
                      const Vector2 *scrollOffset) {
  Vector2 nodesCoords[diagram->nodesSize];
  BuildLayout(nodesCoords, diagram);

  for (size_t i = 0; i < diagram->nodesSize; ++i) {
    Node *node = &diagram->nodes[i];
    switch (node->status) {
    case EStatus_Finished:
      GuiSetState(STATE_DISABLED);
      break;
    case EStatus_Ongoing:
      GuiSetState(STATE_FOCUSED);
      break;
    default:
      GuiSetState(STATE_NORMAL);
      break;
    }
    GuiButton((Rectangle){nodesCoords[i].x + scrollOffset->x,
                          nodesCoords[i].y + scrollOffset->y, 120, 40},
              LogBook_GetNodeName(logBook, node->nodeName));
  }
  for (size_t i = 0; i < diagram->edgesSize; ++i) {
    DrawEdge(nodesCoords, &diagram->edges[i], scrollOffset);
  }
  GuiSetState(STATE_NORMAL);
}
