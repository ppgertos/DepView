#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SizeTContainer.h"
#include "StringContainer.h"

static size_t StringContainer_Allocated(StringContainer *this);
static size_t StringContainer_Available(StringContainer *this);

StringContainer StringContainer_Make() {
  StringContainer this;
  this.begin = malloc(1);
  this.end = this.begin;
  this.alloc_end = this.begin + 1;
  this.offsets = SizeTContainer_Make();
  return this;
}

void StringContainer_Destroy(StringContainer *this) {
  if (this->begin) {
    free(this->begin);
  }
  this->begin = NULL;
  this->end = NULL;
  this->alloc_end = NULL;
  SizeTContainer_Destroy(&this->offsets);
}

const char *StringContainer_At(const StringContainer *this, size_t index) {
  return &this->begin[this->offsets.begin[index]];
}

void StringContainer_Append(StringContainer *this, char *newString) {
  char *newStringEnd = strchr(newString, '\0');
  if (!newStringEnd) {
    perror("Cannot find NULL char in newString");
    exit(10);
  }

  const size_t newStringLength = newStringEnd - newString;
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
    memset(this->end, StringContainer_Available(this), '\0');
  }

  printf("Added %s name\n", newString);
  strcpy(this->end, newString);
  SizeTContainer_Append(&this->offsets, StringContainer_Used(this));
  this->end = this->begin + StringContainer_Used(this) + newStringLength + 1;
}

void StringContainer_Print(const StringContainer *this) {
  for (size_t i = 0; i < SizeTContainer_Used(&this->offsets); ++i) {
    printf("%ld (%zu): %s\n", i, this->offsets.begin[i],
           StringContainer_At(this, i));
  }
}

static size_t StringContainer_Allocated(StringContainer *this) {
  return this->alloc_end - this->begin;
}

size_t StringContainer_Used(StringContainer *this) {
  return this->end - this->begin;
}

static size_t StringContainer_Available(StringContainer *this) {
  return this->alloc_end - this->end;
}
