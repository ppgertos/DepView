#pragma once

#include <stddef.h>

typedef struct DynamicArray DynamicArray;

DynamicArray* DynamicArray_Make_(size_t item_size);
#define DynamicArray_Make(T) (DynamicArray_Make_(sizeof(T)))

void DynamicArray_Destroy(DynamicArray* this);

size_t DynamicArray_Size_(DynamicArray* this, size_t elementSize);
#define DynamicArray_Size(T, this) (DynamicArray_Size_(this, sizeof(T)))

void* DynamicArray_Begin_(DynamicArray* this);
#define DynamicArray_Begin(T, this) ((T*) DynamicArray_Begin_(this))

void* DynamicArray_End_(DynamicArray* this);
#define DynamicArray_End(T, this) ((T*) DynamicArray_End_(this))

void DynamicArray_Push_(DynamicArray* this, const void* item, size_t item_size);
#define DynamicArray_Push(this, item) (DynamicArray_Push_(this, &item, sizeof(item)))

