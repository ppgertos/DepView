#include "Diagram.h"
#include "LogBook.h"

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#undef RAYGUI_IMPLEMENTATION

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include <gui_window_file_dialog.h>

#include <raylib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct DiagramStyle {
  const float VERT_PADDING;
  const float HORI_PADDING;
  const float NODE_H;
  const float NODE_W;
  const float MARGIN;
} DiagramStyle;

struct DiagramStyle DiagramStyle_Default() {
  return (DiagramStyle){
      .VERT_PADDING = 10,
      .HORI_PADDING = 20,
      .NODE_H = 40,
      .NODE_W = 120,
      .MARGIN = 20,
  };
}

typedef struct Gui {
  int screenWidth;
  int screenHeight;
  const char* loadFileText;
  char selectedTimestamp[25];
  char selectedFileName[2048];
  Rectangle scrollPanelView;
  Vector2 scrollPanelScrollOffset;
  Vector2 scrollPanelBoundsOffset;
  GuiWindowFileDialogState fileDialogState;
  bool diagramNeedsToChange;
} Gui;

typedef struct AppContext {
  LogBook logBook;
  size_t currentLog;
  Diagram currentDiagram;
  Diagram nextDiagram;
  Gui gui;
} AppContext;

//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------
static AppContext AppContext_Init();
static void AppContext_Loop(AppContext* app);
static void AppContext_Draw(AppContext* app);
static void AppContext_Destroy(AppContext* this);
static void AppContext_LoadLog(AppContext* appContext, char* fileName);
static void DrawGraph(const LogBook* logBook, const Diagram* diagram, const Vector2* scrollOffset);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main() {
  AppContext app = AppContext_Init();

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(app.gui.screenWidth, app.gui.screenHeight, "DepView");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    AppContext_Loop(&app);
    AppContext_Draw(&app);
  }

  // De-Initialization
  CloseWindow();
  AppContext_Destroy(&app);

  return 0;
}

//------------------------------------------------------------------------------------
// Controls Functions Definitions (local)
//------------------------------------------------------------------------------------
static AppContext AppContext_Init() {
  AppContext a = {.logBook = LogBook_Init(NULL),
                  .currentLog = 0,
                  .currentDiagram = Diagram_Init(NULL, 0),
                  .gui = {.screenWidth = 800,
                          .screenHeight = 600,
                          .selectedTimestamp = "2024-06-17T21:41:35+0200",
                          .loadFileText = "LOAD FILE",
                          .selectedFileName = "FilePath",
                          .scrollPanelView = {.x = 0, .y = 0, .width = 0, .height = 0},
                          .scrollPanelScrollOffset = {.x = 0, .y = 0},
                          .scrollPanelBoundsOffset = {.x = 0, .y = 0},
                          .fileDialogState = InitGuiWindowFileDialog(NULL),
                          .diagramNeedsToChange = false}};
  return a;
}

static void AppContext_Loop(AppContext* app) {
  if (app->gui.fileDialogState.SelectFilePressed) {
    printf("File has been selected\n");
    snprintf(app->gui.selectedFileName, 2048, "%s/%s", app->gui.fileDialogState.dirPathText,
             app->gui.fileDialogState.fileNameText);
    printf("Loading log from %s\n", app->gui.selectedFileName);
    AppContext_LoadLog(app, app->gui.selectedFileName);
    app->gui.fileDialogState.SelectFilePressed = false;
    app->gui.diagramNeedsToChange = true;
  }

  if (app->gui.diagramNeedsToChange) {
    printf("DIagram update!\n");
    if (app->logBook.entriesSize > app->currentLog) {
      Diagram_Destroy(&app->currentDiagram);
      app->currentDiagram = Diagram_Init(&app->logBook, app->currentLog);
      snprintf(app->gui.selectedTimestamp, 24, "%ld", app->logBook.entries[app->currentLog].timestamp);
    }
    app->gui.diagramNeedsToChange = false;
  }
}

static void AppContext_Draw(AppContext* app) {
  if (IsWindowResized()) {
    app->gui.screenWidth = GetScreenWidth();
    app->gui.screenHeight = GetScreenHeight();
  }
  BeginDrawing();
  ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

  const float MARGIN = 10;

  // Toolbar
  const float TOOLBAR_HEIGHT = 24;
  if (GuiButton((Rectangle){MARGIN, MARGIN, TOOLBAR_HEIGHT, TOOLBAR_HEIGHT}, "<")) {
    app->currentLog = 0 > app->currentLog - 1 ? app->currentLog : app->currentLog - 1;
    app->gui.diagramNeedsToChange = true;
  }
  GuiLabel((Rectangle){50, MARGIN, 180, TOOLBAR_HEIGHT}, app->gui.selectedTimestamp);
  if (GuiButton((Rectangle){190, MARGIN, TOOLBAR_HEIGHT, TOOLBAR_HEIGHT}, ">")) {
    app->currentLog =
        app->currentLog + 1 >= app->logBook.entriesSize ? app->logBook.entriesSize - 1 : app->currentLog + 1;
    app->gui.diagramNeedsToChange = true;
  }
  GuiLabel((Rectangle){360, MARGIN, 304, TOOLBAR_HEIGHT}, app->gui.selectedFileName);
  if (GuiButton((Rectangle){app->gui.screenWidth - MARGIN - 72, MARGIN, 72, TOOLBAR_HEIGHT}, app->gui.loadFileText)) {
    app->gui.fileDialogState = InitGuiWindowFileDialog(app->gui.selectedFileName);
    app->gui.fileDialogState.windowActive = true;
  }

  // Scroll Panel
  if (app->gui.fileDialogState.windowActive) {
    GuiSetState(STATE_DISABLED);
  }
  const float PANEL_X = MARGIN;
  const float PANEL_Y = MARGIN + TOOLBAR_HEIGHT + 10;
  GuiScrollPanel((Rectangle){.x = PANEL_X,
                             .y = PANEL_Y,
                             .width = (app->gui.screenWidth - PANEL_X - MARGIN) - app->gui.scrollPanelBoundsOffset.x,
                             .height = (app->gui.screenHeight - MARGIN - PANEL_Y) - app->gui.scrollPanelBoundsOffset.y},
                 NULL, (Rectangle){.x = 0, .y = 0, .width = 800, .height = 600}, &app->gui.scrollPanelScrollOffset,
                 &app->gui.scrollPanelView);
  GuiSetState(STATE_NORMAL);
  BeginScissorMode(app->gui.scrollPanelView.x, app->gui.scrollPanelView.y,
                   app->gui.scrollPanelView.width - app->gui.scrollPanelBoundsOffset.x,
                   app->gui.scrollPanelView.height - app->gui.scrollPanelBoundsOffset.y);
  {
    if (app->logBook.entriesSize > 0) {
      Vector2 subviewOffset = {
          .x = app->gui.scrollPanelView.x + app->gui.scrollPanelScrollOffset.x,
          .y = app->gui.scrollPanelView.y + app->gui.scrollPanelScrollOffset.y,
      };

      DrawGraph(&app->logBook, &app->currentDiagram, &subviewOffset);
    }
  }
  EndScissorMode();

  // Modal window
  GuiWindowFileDialog(&app->gui.fileDialogState);

  EndDrawing();
}

static void AppContext_Destroy(AppContext* this) {
  Diagram_Destroy(&this->currentDiagram);
  LogBook_Destroy(&this->logBook);
}

static void AppContext_LoadLog(AppContext* this, char* fileName) {
  LogBook_Load(&this->logBook, fileName);
  printf("Loaded %zu logs\n", this->logBook.entriesSize);
}

static void BuildLayout(Vector2* result, const Diagram* diagram) {
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
    result[i].y = ds.MARGIN + nodesInColumn[nodeToColumn[i]] * (ds.NODE_H + ds.VERT_PADDING);
    ++nodesInColumn[nodeToColumn[i]];
  }
}

static void DrawEdge(const Vector2* nodesCoords, const Edge* edge, const Vector2* scrollOffset) {
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
    strip[2].x = strip[1].x + ds.NODE_W / 2 + ds.HORI_PADDING / 5 +
                 (edge->source % 9);  // here to add small jumps to distinguish lines
    strip[2].y = strip[1].y;

    strip[3].x = strip[2].x;
    strip[3].y = scrollOffset->y + nodesCoords[edge->destination].y + ds.NODE_H / 2;
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

static void DrawGraph(const LogBook* logBook, const Diagram* diagram, const Vector2* scrollOffset) {
  Vector2 nodesCoords[diagram->nodesSize];
  BuildLayout(nodesCoords, diagram);

  for (size_t i = 0; i < diagram->nodesSize; ++i) {
    Node* node = &diagram->nodes[i];
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
    GuiButton((Rectangle){nodesCoords[i].x + scrollOffset->x, nodesCoords[i].y + scrollOffset->y, 120, 40},
              LogBook_GetNodeName(logBook, node->nodeName));
  }

  for (size_t i = 0; i < diagram->edgesSize; ++i) {
    DrawEdge(nodesCoords, &diagram->edges[i], scrollOffset);
  }

  GuiSetState(STATE_NORMAL);
}
