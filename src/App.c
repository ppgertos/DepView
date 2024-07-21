#include "App.h"
#include "Diagram.h"
#include "DynamicArray.h"
#include "FlowLayout.h"
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

typedef struct App {
  LogBook logBook;
  size_t currentLog;
  Diagram oldDiagram;
  Diagram currentDiagram;
  float changeProcent;
  Gui gui;
} App;

static void App_Loop(App* app);
static void App_Draw(App* app);

size_t App_SizeOf() {
  return sizeof(App);
}

void App_Init(App* app) {
  *app = (App){.logBook = LogBook_Init(NULL),
               .currentLog = 0,
               .oldDiagram = Diagram_Init(NULL, 0),
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
}

void App_Destroy(App* this) {
  LogBook_Destroy(&this->logBook);
  Diagram_Destroy(&this->oldDiagram);
  Diagram_Destroy(&this->currentDiagram);
}

void App_Run(App* app) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(app->gui.screenWidth, app->gui.screenHeight, "DepView");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    App_Loop(app);
    App_Draw(app);
  }

  CloseWindow();
}

static void DrawGraph(const LogBook* logBook,
                      float procent,
                      const Diagram* oldDiagram,
                      const Diagram* diagram,
                      const Vector2* scrollOffset);
static void BuildLayout(Vector2* result, const Diagram* diagram);
static void DrawEdge(const Vector2* coordinates, const Edge* edge, const Vector2* scrollOffset);

static void App_Loop(App* app) {
  if (app->gui.fileDialogState.SelectFilePressed) {
    printf("File has been selected\n");
    snprintf(app->gui.selectedFileName, 2048, "%s/%s", app->gui.fileDialogState.dirPathText,
             app->gui.fileDialogState.fileNameText);
    printf("Loading log from %s\n", app->gui.selectedFileName);
    LogBook_Load(&app->logBook, app->gui.selectedFileName);
    printf("Loaded %zu logs\n", app->logBook.entriesSize);
    LogBook_Print(&app->logBook);
    app->gui.fileDialogState.SelectFilePressed = false;
    app->gui.diagramNeedsToChange = true;
  }

  if (app->gui.diagramNeedsToChange) {
    printf("Diagram update!\n");
    if (app->logBook.entriesSize > app->currentLog) {
      Diagram_Destroy(&app->currentDiagram);
      app->currentDiagram = Diagram_Init(&app->logBook, app->currentLog);
      snprintf(app->gui.selectedTimestamp, 24, "%ld", app->logBook.entries[app->currentLog].timestamp);
    }
    app->gui.diagramNeedsToChange = false;
  }
}

static void App_Draw(App* app) {
  if (IsWindowResized()) {
    app->gui.screenWidth = GetScreenWidth();
    app->gui.screenHeight = GetScreenHeight();
  }
  BeginDrawing();
  ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

  const float MARGIN = 10;

  // Toolbar
  const float TOOLBAR_HEIGHT = 24;
  FlowLayout toolbarLayout = FlowLayout_Init((Vector2){.x = MARGIN, .y = MARGIN}, (Vector2){.x = MARGIN, .y = 0});

  if (GuiButton(FlowLayout_Add(&toolbarLayout, 72, TOOLBAR_HEIGHT), app->gui.loadFileText)) {
    app->gui.fileDialogState = InitGuiWindowFileDialog(app->gui.selectedFileName);
    app->gui.fileDialogState.windowActive = true;
  }
  GuiLabel(FlowLayout_Add(&toolbarLayout, 304, TOOLBAR_HEIGHT), app->gui.selectedFileName);

  if (GuiButton(FlowLayout_Add(&toolbarLayout, TOOLBAR_HEIGHT, TOOLBAR_HEIGHT), "<")) {
    app->currentLog = 0 == app->currentLog ? app->currentLog : app->currentLog - 1;
    app->gui.diagramNeedsToChange = true;
    Diagram_Destroy(&app->oldDiagram);
    Diagram_Copy(&app->oldDiagram, &app->currentDiagram);
    app->changeProcent = 0.0;
  }
  GuiLabel(FlowLayout_Add(&toolbarLayout, 160, TOOLBAR_HEIGHT), app->gui.selectedTimestamp);
  if (GuiButton(FlowLayout_Add(&toolbarLayout, TOOLBAR_HEIGHT, TOOLBAR_HEIGHT), ">")) {
    app->currentLog =
        app->currentLog + 1 >= app->logBook.entriesSize ? app->logBook.entriesSize - 1 : app->currentLog + 1;
    app->gui.diagramNeedsToChange = true;
    Diagram_Destroy(&app->oldDiagram);
    Diagram_Copy(&app->oldDiagram, &app->currentDiagram);
    app->changeProcent = 0.0;
  }

  FlowLayout_Destroy(&toolbarLayout);

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
      Vector2 workspaceOffset = {
          .x = app->gui.scrollPanelView.x + app->gui.scrollPanelScrollOffset.x,
          .y = app->gui.scrollPanelView.y + app->gui.scrollPanelScrollOffset.y,
      };
      
      float deltaPosition = 1.0;
      if (app->changeProcent < 1.0)
      {
          deltaPosition = 0.5 + 0.5*sinf(app->changeProcent * 3.14 - 1.57);
      }
      DrawGraph(&app->logBook, deltaPosition, &app->oldDiagram, &app->currentDiagram, &workspaceOffset);
      app->changeProcent = app->changeProcent + 0.02 > 1.00 ? 1.0 : app->changeProcent + 0.02;
    }
  }
  EndScissorMode();

  // Modal window
  GuiWindowFileDialog(&app->gui.fileDialogState);

  EndDrawing();
}

static void DrawGraph(const LogBook* logBook,
                      float procent,
                      const Diagram* oldDiagram,
                      const Diagram* diagram,
                      const Vector2* scrollOffset) {
  BuildLayout(diagram->coordinates, diagram);

  size_t nodesSize = diagram->nodesSize < oldDiagram->nodesSize ? oldDiagram->nodesSize : diagram->nodesSize;
  Vector2 coords[nodesSize];
  for (size_t i = 0; i != nodesSize; ++i) {
    if (i < oldDiagram->nodesSize && i < diagram->nodesSize) {
      coords[i].x = oldDiagram->coordinates[i].x * (1.0 - procent) + diagram->coordinates[i].x * procent;
      coords[i].y = oldDiagram->coordinates[i].y * (1.0 - procent) + diagram->coordinates[i].y * procent;
    } else if (i >= oldDiagram->nodesSize) {
      coords[i] = diagram->coordinates[i];
    } else {
      coords[i] = oldDiagram->coordinates[i];
    }
  }

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
    GuiButton((Rectangle){coords[i].x + scrollOffset->x, coords[i].y + scrollOffset->y, 120, 40},
              LogBook_GetNodeName(logBook, node->nodeName));
  }

  for (size_t i = 0; i < diagram->edgesSize; ++i) {
    DrawEdge(coords, &diagram->edges[i], scrollOffset);
  }

  GuiSetState(STATE_NORMAL);
}

static void BuildLayout(Vector2* result, const Diagram* diagram) {
  DiagramStyle ds = DiagramStyle_Default();
  int nodesInColumn[16];
  memset(&nodesInColumn, '\0', sizeof(nodesInColumn));
  int levelsOfDependency[diagram->nodesSize];
  memset(levelsOfDependency, -1, sizeof(levelsOfDependency));

  DynamicArray* stack = DynamicArray_Make(size_t);
  for (size_t i = 0; i < diagram->nodesSize; ++i) {
    if (levelsOfDependency[i] == -1) {
      DynamicArray_Push(stack, i);
      while (DynamicArray_Size(size_t, stack) != 0) {
        size_t j = *(DynamicArray_End(size_t, stack) - 1);
        size_t max = 0;
        if (diagram->nodes[j].dependencies[0] == -1) {
          levelsOfDependency[j] = 0;
          DynamicArray_Pop(size_t, stack);
        } else {
          for (size_t k = 0; diagram->nodes[j].dependencies[k] != (size_t)-1; ++k) {
            size_t dependency = diagram->nodes[j].dependencies[k];
            if (levelsOfDependency[dependency] == -1) {
              DynamicArray_Push(stack, dependency);
              levelsOfDependency[dependency] = -2;
              max = (size_t)-1;
            } else if (levelsOfDependency[dependency] != -2) {
              if (max != (size_t)-1 && max < levelsOfDependency[dependency]) {
                max = levelsOfDependency[dependency];
              }
            }
          }
          if (max != (size_t)-1) {
            levelsOfDependency[j] = max + 1;
            DynamicArray_Pop(size_t, stack);
          }
        }
      }
    }

    result[i].x = ds.MARGIN + levelsOfDependency[i] * (ds.NODE_W + ds.HORI_PADDING);
    result[i].y = ds.MARGIN + nodesInColumn[levelsOfDependency[i]] * (ds.NODE_H + ds.VERT_PADDING);
    ++nodesInColumn[levelsOfDependency[i]];
  }
  DynamicArray_Destroy(stack);
  free(stack);
}

static void DrawEdge(const Vector2* coordinates, const Edge* edge, const Vector2* scrollOffset) {
  DiagramStyle ds = DiagramStyle_Default();

  Vector2 strip[8];

  // middle of nodes bottom side
  {
    strip[0].x = scrollOffset->x + coordinates[edge->source].x + ds.NODE_W / 2;
    strip[0].y = scrollOffset->y + coordinates[edge->source].y + ds.NODE_H;
  }

  strip[1].x = strip[0].x;
  strip[1].y = strip[0].y + ds.VERT_PADDING / 5 + (edge->source % 9);  // here to add small jumps to distinguish lines

  // Line on "edge-bus"
  {
    strip[2].x = scrollOffset->x + coordinates[edge->destination].x - ds.HORI_PADDING / 5 -
                 (edge->source % 9);  // here to add small jumps to distinguish lines
    strip[2].y = strip[1].y;

    strip[3].x = strip[2].x;
    strip[3].y = scrollOffset->y + coordinates[edge->destination].y + ds.NODE_H / 2;
  }

  // middle point on nodes left side
  {
    strip[4].x = scrollOffset->x + coordinates[edge->destination].x;
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
