#include "App.h"
#include "Diagram.h"
#include "DynamicArray.h"
#include "FlowLayout.h"
#include "LogBook.h"
#include "Workspace.h"

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#undef RAYGUI_IMPLEMENTATION

#define MAX_GUI_STYLES_AVAILABLE 12                 // NOTE: Included light style
#include <raygui/styles/ashes/style_ashes.h>        // raygui style: ashes
#include <raygui/styles/bluish/style_bluish.h>      // raygui style: bluish
#include <raygui/styles/candy/style_candy.h>        // raygui style: candy
#include <raygui/styles/cherry/style_cherry.h>      // raygui style: cherry
#include <raygui/styles/cyber/style_cyber.h>        // raygui style: cyber
#include <raygui/styles/dark/style_dark.h>          // raygui style: dark
#include <raygui/styles/enefete/style_enefete.h>    // raygui style: enefete
#include <raygui/styles/jungle/style_jungle.h>      // raygui style: jungle
#include <raygui/styles/lavanda/style_lavanda.h>    // raygui style: lavanda
#include <raygui/styles/sunny/style_sunny.h>        // raygui style: sunny
#include <raygui/styles/terminal/style_terminal.h>  // raygui style: terminal

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include <gui_window_file_dialog.h>
#undef GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION

#include <raylib.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const GUI_STYLES_COMBOLIST = "default;Jungle;Candy;Lavanda;Cyber;Terminal;Ashes;Bluish;Dark;Cherry;Sunny;Enefete";

typedef struct Core {
  LogBook logBook;
  size_t currentLog;
  Diagram oldDiagram;
  Diagram currentDiagram;
} Core;

typedef struct Gui {
  const char* loadFileText;
  int activeStyle;
  int prevStyle;
  float toolbarHeight;
  Vector2 windowMargins;
  Vector2 windowPaddings;
  int screenWidth;
  int screenHeight;
  char selectedTimestamp[25];
  char selectedFileName[2048];
  Rectangle scrollPanelView;
  Vector2 scrollPanelScrollOffset;
  Vector2 scrollPanelBoundsOffset;
  GuiWindowFileDialogState fileDialogState;
  bool diagramNeedsToChange;
} Gui;

typedef struct App {
  bool running;
  float changeProcent;
  Core core;
  Gui gui;
} App;

static void App_Loop(App* app);
static void App_Draw(App* app);
static void Gui_ChangeStyle(Gui* gui);

size_t App_SizeOf() {
  return sizeof(App);
}

void App_Init(App* app) {
  *app = (App){
      .running = true,
      .core =
          {
              .logBook = LogBook_Init(NULL),
              .currentLog = 0,
              .oldDiagram = Diagram_Init(NULL, 0),
              .currentDiagram = Diagram_Init(NULL, 0),
          },
      .changeProcent = 1.0,
      .gui =
          {
              .loadFileText = "LOAD FILE",
              .activeStyle = 4,
              .prevStyle = 0,
              .toolbarHeight = 24,
              .windowMargins = {.x = 10, .y = 10},
              .windowPaddings = {.x = 7, .y = 7},
              .screenWidth = 800,
              .screenHeight = 600,
              .selectedTimestamp = "2024-06-17T21:41:35+0200",
              .selectedFileName = "FilePath",
              .scrollPanelView = {.x = 0, .y = 0, .width = 0, .height = 0},
              .scrollPanelScrollOffset = {.x = 0, .y = 0},
              .scrollPanelBoundsOffset = {.x = 0, .y = 0},
              .fileDialogState = InitGuiWindowFileDialog(NULL),
              .diagramNeedsToChange = false,
          },
  };
}

void Core_Destroy(Core* this) {
  LogBook_Destroy(&this->logBook);
  Diagram_Destroy(&this->oldDiagram);
  Diagram_Destroy(&this->currentDiagram);
}

void App_Destroy(App* this) {
  Core_Destroy(&this->core);
}


void App_Run(App* app) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(app->gui.screenWidth, app->gui.screenHeight, "DepView");
  SetTargetFPS(60);


  while (app->running) {
    App_Loop(app);
    App_Draw(app);
    app->running = !WindowShouldClose();
  }

  CloseWindow();
}

static void App_Loop(App* app) {
  if (app->gui.fileDialogState.SelectFilePressed) {
    printf("File has been selected\n");
    snprintf(app->gui.selectedFileName, 2048, "%s/%s", app->gui.fileDialogState.dirPathText,
             app->gui.fileDialogState.fileNameText);
    printf("Loading log from %s\n", app->gui.selectedFileName);
    LogBook_Load(&app->core.logBook, app->gui.selectedFileName);
    printf("Loaded %zu logs\n", app->core.logBook.entriesSize);
    LogBook_Print(&app->core.logBook);
    app->gui.fileDialogState.SelectFilePressed = false;
    app->gui.diagramNeedsToChange = true;
  }

  if (app->gui.diagramNeedsToChange) {
    printf("Diagram update!\n");
    if (app->core.logBook.entriesSize > app->core.currentLog) {
      Diagram_Destroy(&app->core.currentDiagram);
      app->core.currentDiagram = Diagram_Init(&app->core.logBook, app->core.currentLog);
      snprintf(app->gui.selectedTimestamp, 24, "%ld", app->core.logBook.entries[app->core.currentLog].timestamp);
    }
    app->gui.diagramNeedsToChange = false;
  }
}

static void Gui_ChangeStyle(Gui* gui) {
  // Reset to default internal style
  // NOTE: Required to unload any previously loaded font texture
  GuiLoadStyleDefault();

  switch (gui->activeStyle) {
    // clang-format off
    case 1: GuiLoadStyleJungle(); break;
    case 2: GuiLoadStyleCandy(); break;
    case 3: GuiLoadStyleLavanda(); break;
    case 4: GuiLoadStyleCyber(); break;
    case 5: GuiLoadStyleTerminal(); break;
    case 6: GuiLoadStyleAshes(); break;
    case 7: GuiLoadStyleBluish(); break;
    case 8: GuiLoadStyleDark(); break;
    case 9: GuiLoadStyleCherry(); break;
    case 10: GuiLoadStyleSunny(); break;
    case 11: GuiLoadStyleEnefete(); break;
    default: break;
    // clang-format on
  }

  gui->prevStyle = gui->activeStyle;
}

static void DrawToolbar(App* app);
static void DrawPanel(App* app);

static void App_Draw(App* app) {
  if (IsWindowResized()) {
    app->gui.screenWidth = GetScreenWidth();
    app->gui.screenHeight = GetScreenHeight();
  }

  if (app->gui.activeStyle != app->gui.prevStyle) {
    Gui_ChangeStyle(&app->gui);
  }

  BeginDrawing();
  ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

  DrawToolbar(app);
  DrawPanel(app);
  GuiWindowFileDialog(&app->gui.fileDialogState);

  EndDrawing();
}

static void DrawToolbar(App* app) {
  const Vector2 TOOLBAR_POSITION = {.x = app->gui.windowMargins.x, .y = app->gui.windowMargins.y};
  const float TOOLBAR_H = app->gui.toolbarHeight;
  const Vector2 TOOLBAR_PADDINGS = {.x = app->gui.windowPaddings.x, .y = 0};

  FlowLayout toolbarLayout = FlowLayout_Init(TOOLBAR_POSITION, TOOLBAR_PADDINGS);

  if (GuiButton(FlowLayout_Add(&toolbarLayout, 72, TOOLBAR_H), app->gui.loadFileText)) {
    app->gui.fileDialogState = InitGuiWindowFileDialog(app->gui.selectedFileName);
    app->gui.fileDialogState.windowActive = true;
  }
  GuiLabel(FlowLayout_Add(&toolbarLayout, 298, TOOLBAR_H), app->gui.selectedFileName);

  if (GuiButton(FlowLayout_Add(&toolbarLayout, TOOLBAR_H, TOOLBAR_H), "<")) {
    app->core.currentLog = 0 == app->core.currentLog ? app->core.currentLog : app->core.currentLog - 1;
    app->gui.diagramNeedsToChange = true;
    Diagram_Destroy(&app->core.oldDiagram);
    Diagram_Copy(&app->core.oldDiagram, &app->core.currentDiagram);
    app->changeProcent = 0.0;
  }
  GuiLabel(FlowLayout_Add(&toolbarLayout, 160, TOOLBAR_H), app->gui.selectedTimestamp);
  if (GuiButton(FlowLayout_Add(&toolbarLayout, TOOLBAR_H, TOOLBAR_H), ">")) {
    app->core.currentLog = app->core.currentLog + 1 >= app->core.logBook.entriesSize ? app->core.logBook.entriesSize - 1
                                                                                     : app->core.currentLog + 1;
    app->gui.diagramNeedsToChange = true;
    Diagram_Destroy(&app->core.oldDiagram);
    Diagram_Copy(&app->core.oldDiagram, &app->core.currentDiagram);
    app->changeProcent = 0.0;
  }

  GuiLabel(FlowLayout_Add(&toolbarLayout, 40, TOOLBAR_H ), "Style:");
  GuiComboBox(FlowLayout_Add(&toolbarLayout, 120, TOOLBAR_H ), GUI_STYLES_COMBOLIST, &app->gui.activeStyle);

  FlowLayout_Destroy(&toolbarLayout);
}

static void DrawPanel(App* app) {
  if (app->gui.fileDialogState.windowActive) {
    GuiSetState(STATE_DISABLED);
  }
  const float PANEL_X = app->gui.windowMargins.x;
  const float PANEL_Y = app->gui.windowMargins.y + app->gui.toolbarHeight + app->gui.windowPaddings.y;
  const float PANEL_W = app->gui.screenWidth - PANEL_X - app->gui.windowMargins.x - app->gui.scrollPanelBoundsOffset.x;
  const float PANEL_H = app->gui.screenHeight - PANEL_Y - app->gui.windowMargins.y - app->gui.scrollPanelBoundsOffset.y;
  GuiScrollPanel((Rectangle){PANEL_X, PANEL_Y, PANEL_W, PANEL_H}, NULL,
                 (Rectangle){.x = 0, .y = 0, .width = 800, .height = 600}, &app->gui.scrollPanelScrollOffset,
                 &app->gui.scrollPanelView);

  GuiSetState(STATE_NORMAL);
  BeginScissorMode(app->gui.scrollPanelView.x, app->gui.scrollPanelView.y,
                   app->gui.scrollPanelView.width - app->gui.scrollPanelBoundsOffset.x,
                   app->gui.scrollPanelView.height - app->gui.scrollPanelBoundsOffset.y);
  if (app->core.logBook.entriesSize > 0) {
    Vector2 workspaceOffset = {
        .x = app->gui.scrollPanelView.x + app->gui.scrollPanelScrollOffset.x,
        .y = app->gui.scrollPanelView.y + app->gui.scrollPanelScrollOffset.y,
    };
    Workspace_Draw(&app->core, app->changeProcent, &workspaceOffset);
    app->changeProcent = app->changeProcent + 0.02 > 1.00 ? 1.0 : app->changeProcent + 0.02;
  }
  EndScissorMode();
}
