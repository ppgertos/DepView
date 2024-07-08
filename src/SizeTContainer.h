#pragma once

#include <stddef.h>

typedef struct SizeTContainer {
  size_t *begin;
  size_t *end;
  size_t *alloc_end;
} SizeTContainer;

SizeTContainer SizeTContainer_Make();
void SizeTContainer_Destroy(SizeTContainer *this);
void SizeTContainer_Append(SizeTContainer *this, size_t newVal);
size_t SizeTContainer_Used(const SizeTContainer *this);
