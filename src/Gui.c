#include "Gui.h"
#include "Core.h"
#include "FlowLayout.h"
#include "FrameList.h"
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
#include <stdio.h>

const float SELECTED_FILE_LABEL_W = 289;
const char* const GUI_STYLES_COMBOLIST =
    "default;Jungle;Candy;Lavanda;Cyber;Terminal;Ashes;Bluish;Dark;Cherry;Sunny;Enefete";

typedef struct Gui {
  Workspace* workspace;
  const char* loadFileText;
  GuiWindowFileDialogState fileDialogState;
  char displayedFileName[128];
  char* newFileName;
  int activeStyle;
  int prevStyle;
  float toolbarHeight;
  Vector2 windowMargins;
  Vector2 windowPaddings;
  int screenWidth;
  int screenHeight;
  char selectedTimestamp[25];
  Rectangle scrollPanelView;
  Vector2 scrollPanelScrollOffset;
  Vector2 scrollPanelBoundsOffset;
  bool graphNeedsToChange;
  bool showFrameList;
  float changeProcent;
} Gui;

static void Gui_HandleNewFileName(Gui* this);
static void Gui_HandleFileSelected(Gui* this, Core* core);
static void Gui_HandleGraphChange(Gui* this, Core* core);
static void Gui_DrawToolbar(Gui* this, Core* core);
static void Gui_DrawWorkspacePanel(Gui* this, Core* core, const Rectangle rect);

size_t Gui_SizeOf() {
  return sizeof(Gui);
}
void Gui_Init(Gui* this) {
  *this = (Gui){
      .workspace = malloc(Workspace_SizeOf()),
      .loadFileText = "LOAD FILE",
      .fileDialogState = InitGuiWindowFileDialog(NULL),
      .displayedFileName = "",
      .newFileName = NULL,
      .activeStyle = 4,
      .prevStyle = 0,
      .toolbarHeight = 24,
      .windowMargins = {.x = 10, .y = 10},
      .windowPaddings = {.x = 8, .y = 10},
      .screenWidth = 800,
      .screenHeight = 600,
      .selectedTimestamp = "2024-06-17T21:41:35+0200",
      .scrollPanelView = {.x = 0, .y = 0, .width = 0, .height = 0},
      .scrollPanelScrollOffset = {.x = 0, .y = 0},
      .scrollPanelBoundsOffset = {.x = 0, .y = 0},
      .graphNeedsToChange = false,
      .changeProcent = 1.0,
  };
  Workspace_Init(this->workspace);
  memset(this->displayedFileName, '\0', sizeof(this->displayedFileName) / sizeof(char));
}
void Gui_Destroy(Gui* this) {
  Workspace_Destroy(this->workspace);
}

void Gui_Loop(Gui* this, Core* core) {
  Gui_HandleNewFileName(this);
  Gui_HandleFileSelected(this, core);
  Gui_HandleGraphChange(this, core);
}

void Gui_SetActiveStyle(Gui* this, int activeStyle) {
  this->activeStyle = activeStyle;
}

void Gui_SetNewFileName(Gui* this, char* fileName) {
  this->newFileName = fileName;
}

static void Gui_HandleNewFileName(Gui* this) {
  if (this->newFileName != NULL && this->newFileName[0] != '\0') {
    float textWidth = MeasureText(this->newFileName, GuiGetFont().baseSize);
    if (textWidth > SELECTED_FILE_LABEL_W) {
      size_t textStartOffset = 3;
      while (textWidth > SELECTED_FILE_LABEL_W) {
        textWidth = MeasureText(this->newFileName + textStartOffset++, GuiGetFont().baseSize);
      }
      // core->displayedFileName = realloc(core->displayedFileName, strlen(core->selectedFileName)+1 -
      // textStartOffset + 3);
      snprintf(this->displayedFileName, 128, "...%s", this->newFileName + textStartOffset);
    } else {
      snprintf(this->displayedFileName, 128, "%.127s", this->newFileName);
    }
    this->newFileName = NULL;
  }
}

static void Gui_HandleFileSelected(Gui* this, Core* core) {
  if (this->fileDialogState.SelectFilePressed) {
    printf("File has been selected\n");
    snprintf(core->selectedFileName, 2048, "%s/%s", this->fileDialogState.dirPathText,
             this->fileDialogState.fileNameText);
    this->fileDialogState.SelectFilePressed = false;
    this->newFileName = core->selectedFileName;
  }
}

void Gui_TriggerGraphChange(Gui* this) {
  this->graphNeedsToChange = true;
}

static void Gui_HandleGraphChange(Gui* this, Core* core) {
  if (this->graphNeedsToChange) {
    this->changeProcent = 0.0;
    if (core->logBook.entriesSize > core->currentLog) {
      printf("Graph update! (%ld)\n", core->logBook.entries[core->currentLog].timestamp);
      Graph_Destroy(&core->oldGraph);
      Graph_Copy(&core->oldGraph, &core->currentGraph);
      Graph_Destroy(&core->currentGraph);
      core->currentGraph = Graph_Init(&core->logBook, core->currentLog);
      snprintf(this->selectedTimestamp, 24, "%ld", core->logBook.entries[core->currentLog].timestamp);
      Workspace_BuildLayout(this->workspace, &core->currentGraph);
    }
    this->graphNeedsToChange = false;
  }
}

void Gui_InitWindow(Gui* this, char* title) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(this->screenWidth, this->screenHeight, title);
  SetTargetFPS(60);
}

bool Gui_ShouldWindowClose() {
  return WindowShouldClose();
}

void Gui_Close() {
  CloseWindow();
}

static void Gui_ChangeStyle(Gui* this) {
  // Reset to default internal style
  // NOTE: Required to unload any previously loaded font texture
  GuiLoadStyleDefault();

  switch (this->activeStyle) {
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

  this->prevStyle = this->activeStyle;
}

void Gui_Draw(Gui* this, Core* core) {
  if (IsWindowResized()) {
    this->screenWidth = GetScreenWidth();
    this->screenHeight = GetScreenHeight();
  }

  if (this->activeStyle != this->prevStyle) {
    Gui_ChangeStyle(this);
  }

  BeginDrawing();
  ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

  Gui_DrawToolbar(this, core);

  this->showFrameList = true;  // TODO: Add button to toolbar to toggle this

  const float FRAMELIST_W = 140;
  const float FRAMELIST_X = this->screenWidth - this->windowMargins.x - FRAMELIST_W;
  const float PANEL_X = this->windowMargins.x;
  const float PANEL_Y = this->windowMargins.y + (this->toolbarHeight + this->windowPaddings.y) * 2;
  const float PANEL_W = this->showFrameList  // fmt
                            ? FRAMELIST_X - PANEL_X - this->windowPaddings.x - this->scrollPanelBoundsOffset.x
                            : this->screenWidth - PANEL_X - this->windowMargins.x - this->scrollPanelBoundsOffset.x;
  const float PANEL_H = this->screenHeight - PANEL_Y - this->windowMargins.y - this->scrollPanelBoundsOffset.y;

  Gui_DrawWorkspacePanel(this, core, (Rectangle){PANEL_X, PANEL_Y, PANEL_W, PANEL_H});
  if (this->showFrameList) {
    Rectangle framelist_rect = {FRAMELIST_X, PANEL_Y, FRAMELIST_W, PANEL_H};
    FrameList_Draw(framelist_rect, &core->currentLog, &this->graphNeedsToChange, &core->logBook);
  }
  GuiWindowFileDialog(&this->fileDialogState);

  EndDrawing();
}

static void Gui_DrawToolbar(Gui* this, Core* core) {
  const Vector2 TOOLBAR_POSITION = {.x = this->windowMargins.x, .y = this->windowMargins.y};
  const Vector2 TOOLBAR_PADDINGS = {.x = this->windowPaddings.x, .y = 0};
  FlowLayout toolbarLayout = FlowLayout_Init(TOOLBAR_POSITION, TOOLBAR_PADDINGS);

  const float TOOLBAR_H = this->toolbarHeight;
  if (GuiButton(FlowLayout_Add(&toolbarLayout, 72, TOOLBAR_H), this->loadFileText)) {
    this->fileDialogState = InitGuiWindowFileDialog(core->selectedFileName);
    this->fileDialogState.windowActive = true;
  }

  GuiLabel(FlowLayout_Add(&toolbarLayout, SELECTED_FILE_LABEL_W, TOOLBAR_H), this->displayedFileName);

  if (!this->showFrameList) {
    if (GuiButton(FlowLayout_Add(&toolbarLayout, TOOLBAR_H, TOOLBAR_H), "<")) {
      core->currentLog = 0 == core->currentLog ? core->currentLog : core->currentLog - 1;
      this->graphNeedsToChange = true;
    }
    GuiLabel(FlowLayout_Add(&toolbarLayout, 160, TOOLBAR_H), this->selectedTimestamp);
    if (GuiButton(FlowLayout_Add(&toolbarLayout, TOOLBAR_H, TOOLBAR_H), ">")) {
      core->currentLog =
          core->currentLog + 1 >= core->logBook.entriesSize ? core->logBook.entriesSize - 1 : core->currentLog + 1;
      this->graphNeedsToChange = true;
    }
  }

  GuiLabel(FlowLayout_Add(&toolbarLayout, 40, TOOLBAR_H), "Style:");
  GuiComboBox(FlowLayout_Add(&toolbarLayout, 120, TOOLBAR_H), GUI_STYLES_COMBOLIST, &this->activeStyle);

  FlowLayout_Destroy(&toolbarLayout);

  const Vector2 TOOLBAR2_POSITION = {.x = this->windowMargins.x,
                                     .y = this->windowMargins.y + TOOLBAR_H + this->windowPaddings.y};
  FlowLayout toolbar2Layout = FlowLayout_Init(TOOLBAR2_POSITION, TOOLBAR_PADDINGS);
  GuiLabel(FlowLayout_Add(&toolbar2Layout, 40, TOOLBAR_H), "Layout:");
  GuiComboBox(FlowLayout_Add(&toolbar2Layout, 120, TOOLBAR_H), "Absolute;Relative",
              Workspace_PointDiagramLayout(this->workspace));
}

static void Gui_DrawWorkspacePanel(Gui* this, Core* core, const Rectangle rect) {
  if (this->fileDialogState.windowActive) {
    GuiSetState(STATE_DISABLED);
  }

  Vector2 wsSize = Workspace_GetSpaceSize(this->workspace, &rect);

  GuiScrollPanel(rect, NULL, (Rectangle){.x = 0, .y = 0, .width = wsSize.x, .height = wsSize.y},
                 &this->scrollPanelScrollOffset, &this->scrollPanelView);

  GuiSetState(STATE_NORMAL);
  BeginScissorMode(this->scrollPanelView.x, this->scrollPanelView.y,
                   this->scrollPanelView.width - this->scrollPanelBoundsOffset.x,
                   this->scrollPanelView.height - this->scrollPanelBoundsOffset.y);
  if (core->logBook.entriesSize > 0 && this->workspace) {
    Vector2 workspaceOffset = {
        .x = this->scrollPanelView.x + this->scrollPanelScrollOffset.x,
        .y = this->scrollPanelView.y + this->scrollPanelScrollOffset.y,
    };
    Workspace_Draw(this->workspace, core, this->changeProcent, &workspaceOffset);
    this->changeProcent = this->changeProcent + 0.02 > 1.00 ? 1.0 : this->changeProcent + 0.02;
  }
  EndScissorMode();
}
