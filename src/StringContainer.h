#pragma once

#include "Container.h"

#include <stddef.h>

typedef struct StringContainer {
  char* begin;
  char* end;
  char* alloc_end;
  DynamicArray* offsets;
  size_t count;
} StringContainer;

StringContainer StringContainer_Init();
void StringContainer_Destroy(StringContainer* this);
const char* StringContainer_At(const StringContainer* this, size_t index);
size_t StringContainer_Append(StringContainer* this, char* newString);
void StringContainer_Print(const StringContainer* this);
size_t StringContainer_Used(StringContainer* this);
