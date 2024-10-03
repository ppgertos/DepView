#include "App.h"
#include "Core.h"
#include "DynamicArray.h"
#include "Graph.h"
#include "Gui.h"
#include "LogBook.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct App {
  bool running;
  Core core;
  Gui* gui;
} App;

static void App_Loop(App* this);
static void App_LoadSelectedLogBook(App* this);

size_t App_SizeOf() {
  return sizeof(App);
}

void App_Init(App* this) {
  *this = (App){.running = true,
               .core =
                   {
                       .selectedFileName = "",
                       .logBook = LogBook_Init(),
                       .currentLog = 0,
                       .oldGraph = Graph_Init(NULL, 0),
                       .currentGraph = Graph_Init(NULL, 0),
                   },
               .gui = malloc(Gui_SizeOf())};
  Gui_Init(this->gui);
}

void Core_Destroy(Core* this) {
  LogBook_Destroy(&this->logBook);
  Graph_Destroy(&this->oldGraph);
  Graph_Destroy(&this->currentGraph);
}

void App_Destroy(App* this) {
  Gui_Destroy(this->gui);
  Core_Destroy(&this->core);
}

void App_Configure(App* this, Config config) {
  if (config.filePath) {
    strncpy(this->core.selectedFileName, config.filePath, sizeof(this->core.selectedFileName));
    Gui_SetNewFileName(this->gui, config.filePath);
    App_LoadSelectedLogBook(this);
  }
  Gui_SetActiveStyle(this->gui, config.style);
}

void App_Run(App* this) {
  Gui_InitWindow(this->gui, "DepView");

  while (this->running) {
    App_Loop(this);
    Gui_Draw(this->gui, &this->core);
    this->running = !Gui_ShouldWindowClose();
  }

  Gui_Close();
}

static void App_Loop(App* this) {
  Gui_Loop(this->gui, &this->core);

  if (this->core.selectedFileName[0] != '\0'){
    Gui_SetNewFileName(this->gui, this->core.selectedFileName);
    App_LoadSelectedLogBook(this);
  }
}

static void App_LoadSelectedLogBook(App* this) {
  printf("Loading log from %s\n", this->core.selectedFileName);
  if (LogBook_IsLoaded(&this->core.logBook)) {
    LogBook_Destroy(&this->core.logBook);
    this->core.logBook = LogBook_Init();
  }
  LogBook_Load(&this->core.logBook, this->core.selectedFileName);
  printf("Loaded %zu logs\n", this->core.logBook.entriesSize);
  this->core.currentLog = 0;
  LogBook_Print(&this->core.logBook);
  Gui_TriggerGraphChange(this->gui);
  this->core.selectedFileName[0] = '\0';
}
