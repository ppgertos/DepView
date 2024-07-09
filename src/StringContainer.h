#pragma once

#include <SizeTContainer.h>
#include <stddef.h>

typedef struct StringContainer {
  char* begin;
  char* end;
  char* alloc_end;
  struct SizeTContainer offsets;
  size_t count;
} StringContainer;

StringContainer StringContainer_Make();
void StringContainer_Destroy(StringContainer* this);
const char* StringContainer_At(const StringContainer* this, size_t index);
void StringContainer_Append(StringContainer* this, char* newString);
void StringContainer_Print(const StringContainer* this);
size_t StringContainer_Used(StringContainer* this);
