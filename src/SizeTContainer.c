#include "SizeTContainer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t SizeTContainer_Allocated(const SizeTContainer* this);
static size_t SizeTContainer_Available(const SizeTContainer* this);

SizeTContainer SizeTContainer_Make() {
  SizeTContainer this;
  this.begin = NULL;
  this.end = NULL;
  this.alloc_end = NULL;
  return this;
}

void SizeTContainer_Destroy(SizeTContainer* this) {
  if (this->begin) {
    free(this->begin);
  }
  this->begin = NULL;
  this->end = NULL;
  this->alloc_end = NULL;
}

void SizeTContainer_Append(SizeTContainer* this, size_t newVal) {
  while (SizeTContainer_Available(this) < 1) {
    size_t capacityDoubled = SizeTContainer_Allocated(this);
    capacityDoubled = capacityDoubled == 0 ? 1 : capacityDoubled * 2;
    const size_t used = SizeTContainer_Used(this);
    this->begin = realloc(this->begin, capacityDoubled * sizeof(size_t));
    if (this->begin == NULL) {
      perror("Unable to realloc SizeTContainer");
      exit(10);
    }
    this->alloc_end = this->begin + capacityDoubled;
    this->end = this->begin + used;
    memset(this->end, SizeTContainer_Available(this), '\0');
  }

  *this->end = newVal;
  this->end = this->begin + SizeTContainer_Used(this) + 1;
  printf("Added %ld\n", newVal);
}

static size_t SizeTContainer_Allocated(const SizeTContainer* this) {
  return this->alloc_end - this->begin;
}

size_t SizeTContainer_Used(const SizeTContainer* this) {
  return this->end - this->begin;
}

static size_t SizeTContainer_Available(const SizeTContainer* this) {
  return this->alloc_end - this->end;
}
