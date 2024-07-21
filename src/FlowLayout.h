#pragma once

#include "DynamicArray.h"

#include <raylib.h>

#include <stddef.h>
#include <stdlib.h>

typedef struct FlowLayout {
  const Vector2 start;
  const Vector2 padding;
  DynamicArray* rects;
} FlowLayout;

FlowLayout FlowLayout_Init(const Vector2 position, const Vector2 paddings) {
  return (FlowLayout){.start = position, .padding = paddings, .rects = DynamicArray_Make(Rectangle)};
}
void FlowLayout_Destroy(FlowLayout* this) {
  DynamicArray_Destroy(this->rects);
  this->rects = NULL;
}

Rectangle FlowLayout_Add(FlowLayout* this, float w, float h) {
  const size_t index = DynamicArray_Size(Rectangle, this->rects);
  if (index == 0) {
    DynamicArray_Push(this->rects, ((Rectangle){
                                       .x = this->start.x,
                                       .y = this->start.y,
                                       .width = w,
                                       .height = h,
                                   }));
  } else {
    const Rectangle* last = DynamicArray_Begin(Rectangle, this->rects) + index - 1;
    DynamicArray_Push(this->rects, ((Rectangle){
                                       .x = last->x + last->width + this->padding.x,
                                       .y = this->start.y,
                                       .width = w,
                                       .height = h,
                                   }));
  }
  return *(DynamicArray_Begin(Rectangle, this->rects) + index);
}

Rectangle FlowLayout_AddV(FlowLayout* this, const Vector2 dimensions) {
  return FlowLayout_Add(this, dimensions.x, dimensions.y);
}
