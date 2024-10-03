#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct Gui Gui;
typedef struct Core Core;

size_t Gui_SizeOf();

void Gui_Init(Gui* gui);
void Gui_Destroy(Gui* gui);
void Gui_Loop(Gui* gui, Core* core);

void Gui_TriggerGraphChange(Gui* gui);

void Gui_SetActiveStyle(Gui* gui, int activeStyle);
void Gui_SetNewFileName(Gui* gui, char* fileName);
void Gui_InitWindow(Gui* gui, char* title);
void Gui_Draw(Gui* app, Core* core);
bool Gui_ShouldWindowClose();
void Gui_Close();
