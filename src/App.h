#pragma once

#include <stddef.h>

typedef struct App App;

typedef struct Core Core;

typedef struct Config {
    char* filePath;
    int style;
} Config;

size_t App_SizeOf();
void App_Init(App* app);
void App_Configure(App* app, Config config);
void App_Destroy(App* this);

void App_Run(App* app);
int App_IsRunning(App* app);
