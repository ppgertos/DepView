#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Container.h"
#include "StringContainer.h"

static size_t StringContainer_Allocated(StringContainer* this);
static size_t StringContainer_Available(StringContainer* this);

StringContainer StringContainer_Init() {
  StringContainer this;
  size_t container_size = 1;
  this.begin = malloc(container_size);
  this.end = this.begin;
  this.alloc_end = this.begin + container_size;
  this.offsets = DynamicArray_Make(size_t);
  return this;
}

void StringContainer_Destroy(StringContainer* this) {
  if (this->begin) {
    free(this->begin);
  }
  this->begin = NULL;
  this->end = NULL;
  this->alloc_end = NULL;
  if (this->offsets) {
    DynamicArray_Destroy(this->offsets);
    free(this->offsets);
  }
  this->offsets = NULL;
}

const char* StringContainer_At(const StringContainer* this, size_t index) {
  return &this->begin[DynamicArray_Begin(size_t, this->offsets)[index]];
}

size_t StringContainer_Append(StringContainer* this, char* newString) {
  const size_t newStringLength = strlen(newString) + 1;
  while (StringContainer_Available(this) < newStringLength) {
    const size_t allocatedDoubled = StringContainer_Allocated(this) * 2;
    const size_t used = StringContainer_Used(this);
    this->begin = realloc(this->begin, allocatedDoubled);
    if (this->begin == NULL) {
      perror("Unable to realloc StringContainer");
      exit(10);
    }
    this->alloc_end = this->begin + allocatedDoubled;
    this->end = this->begin + used;
    memset(this->end, '\0', StringContainer_Available(this));
  }
  printf("Added %s name\n", newString);
  strcpy(this->end, newString);
  size_t used = StringContainer_Used(this);
  DynamicArray_Push(this->offsets, used);
  this->end = this->begin + used + newStringLength;
  return used;
}

void StringContainer_Print(const StringContainer* this) {
  size_t* offsetsBegin = DynamicArray_Begin(size_t, this->offsets);
  size_t arraySize = DynamicArray_End(size_t, this->offsets) - offsetsBegin;
  for (size_t i = 0; i < arraySize; ++i) {
    printf("%ld (%zu): %s\n", i, offsetsBegin[i], StringContainer_At(this, i));
  }
}

static size_t StringContainer_Allocated(StringContainer* this) {
  return this->alloc_end - this->begin;
}

size_t StringContainer_Used(StringContainer* this) {
  return this->end - this->begin;
}

static size_t StringContainer_Available(StringContainer* this) {
  return this->alloc_end - this->end;
}
