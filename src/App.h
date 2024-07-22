#pragma once

#include <stddef.h>

typedef struct App App;

typedef struct Core Core;

size_t App_SizeOf();
void App_Init(App* app);
void App_Destroy(App* this);

void App_Run(App* app);
int App_IsRunning(App* app);
