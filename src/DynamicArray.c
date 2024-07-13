#include <DynamicArray.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct DynamicArray {
  size_t size;
  size_t capacity;
  size_t item_size;
  void* buffer;
} DynamicArray;

DynamicArray* DynamicArray_Make_(size_t item_size) {
  DynamicArray* da = malloc(sizeof(DynamicArray));
  *da = (DynamicArray){
      .size = 0,
      .capacity = 0,
      .item_size = item_size,
      .buffer = NULL,
  };
  return da;
}

void DynamicArray_Destroy(DynamicArray* this) {
  if (this->buffer) {
    free(this->buffer);
    this->buffer = NULL;
  }
}

void* DynamicArray_Begin_(DynamicArray* this) {
  return ((char*)this->buffer);
}

void* DynamicArray_End_(DynamicArray* this) {
  return ((char*)this->buffer) + this->size;
}

size_t DynamicArray_Size_(DynamicArray* this, size_t elementSize)
{
  return this->size / elementSize;
}

void DynamicArray_Push_(DynamicArray* this, const void* item, size_t item_size) {
  if (item_size != this->item_size) {
    perror("Incompatible item sizes!");
    exit(10);
  }
  while (this->capacity - this->size < 1) {
    size_t newCapacity = this->capacity;
    newCapacity = newCapacity == 0 ? 1 : newCapacity * 2;
    newCapacity *= this->item_size;
    this->buffer = realloc(this->buffer, newCapacity);
    if (this->buffer == NULL) {
      perror("Unable to realloc DynamicArray");
      exit(10);
    }
    this->capacity = newCapacity;
    memset(((char*)this->buffer) + this->size, '\0', this->capacity - this->size);
  }
  memcpy(((char*)this->buffer) + this->size, (char*)item, item_size);
  this->size += item_size;
}
