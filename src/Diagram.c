#include "DynamicArray.h"

#include <raygui.h>
#include <raylib.h>
#include <string.h>

typedef struct DiagramStyle {
  const float EDGE_SPACING;
  const float EDGE_W;
  const float HORI_PADDING;
  const float MARGIN;
  const float NODE_H;
  const float NODE_W;
  const float VERT_PADDING;
} DiagramStyle;

typedef struct Diagram {
  DiagramStyle diagramStyle;
  Vector2* nodesCoordinates;
  Vector2* edgeOffsets;
  DynamicArray* horizontalBusesHeights;
  DynamicArray* verticalBusesWitdths;
} Diagram;

DiagramStyle DiagramStyle_Default() {
  return (DiagramStyle){
      .EDGE_SPACING = 2,
      .EDGE_W = 1,
      .HORI_PADDING = 5,
      .MARGIN = 10,
      .NODE_H = 30,
      .NODE_W = 140,
      .VERT_PADDING = 5,
  };
}

size_t Diagram_SizeOf() {
  return sizeof(Diagram);
}

static void Diagram_BuildAbsoluteLayout(Diagram* this, const Graph* graph) {
  DiagramStyle ds = DiagramStyle_Default();
  int nodesInColumn[16];
  memset(&nodesInColumn, '\0', sizeof(nodesInColumn));
  int levelsOfDependency[graph->nodesSize];
  memset(levelsOfDependency, -1, sizeof(levelsOfDependency));

  DynamicArray* stack = DynamicArray_Make(size_t);
  for (size_t i = 0; i < graph->nodesSize; ++i) {
    if (levelsOfDependency[i] == -1) {
      DynamicArray_Push(stack, i);
      while (DynamicArray_Size(size_t, stack) != 0) {
        size_t j = *(DynamicArray_End(size_t, stack) - 1);
        if (graph->nodes[j].dependencies[0] == (size_t)-1) {
          levelsOfDependency[j] = 0;
          DynamicArray_Pop(size_t, stack);
        } else {
          size_t max = 0;
          for (size_t k = 0; graph->nodes[j].dependencies[k] != (size_t)-1; ++k) {
            size_t dependency = graph->nodes[j].dependencies[k];
            if (levelsOfDependency[dependency] == -1) {
              DynamicArray_Push(stack, dependency);
              levelsOfDependency[dependency] = -2;
              max = (size_t)-1;
            } else if (levelsOfDependency[dependency] != -2) {
              if (max != (size_t)-1 && max < levelsOfDependency[dependency]) {
                max = levelsOfDependency[dependency];
              }
            }
          }
          if (max != (size_t)-1) {
            levelsOfDependency[j] = max + 1;
            DynamicArray_Pop(size_t, stack);
          }
        }
      }
    }

    this->nodesCoordinates[i].x = ds.MARGIN + levelsOfDependency[i] * (ds.NODE_W + ds.HORI_PADDING);
    this->nodesCoordinates[i].y = ds.MARGIN + nodesInColumn[levelsOfDependency[i]] * (ds.NODE_H + ds.VERT_PADDING);
    ++nodesInColumn[levelsOfDependency[i]];
  }
  DynamicArray_Destroy(stack);
  free(stack);
}

static void BuildRelativeLayout(Vector2* result, const Graph* graph, const size_t startNode) {
  DiagramStyle ds = DiagramStyle_Default();
  int minLevel = 0;
  int levelsOfDependency[graph->nodesSize];

  const int UNKNOWN = 1024 * 64 - 1;
  const int DURING_CALCULATION = UNKNOWN - 1;
  for (size_t i = 0; i < graph->nodesSize; ++i) {
    levelsOfDependency[i] = UNKNOWN;
  }
  size_t centralNode = startNode == (size_t) -1 ? 0: startNode;
  DynamicArray* stack = DynamicArray_Make(size_t);
  levelsOfDependency[centralNode] = 0;
  DynamicArray_Push(stack, centralNode);
  while (DynamicArray_Size(size_t, stack) != 0) {
    size_t current = *(DynamicArray_Pop(size_t, stack));
    for (size_t k = 0; graph->nodes[current].dependencies[k] != (size_t)-1; ++k) {
      size_t dependency = graph->nodes[current].dependencies[k];
      if (levelsOfDependency[dependency] == UNKNOWN) {
        DynamicArray_Push(stack, dependency);
        levelsOfDependency[dependency] = levelsOfDependency[current] - 1;
        if (minLevel > levelsOfDependency[dependency]) {
          minLevel = levelsOfDependency[dependency];
        }
      }
    }
  }

  int nodesInColumn[16];
  memset(&nodesInColumn, '\0', sizeof(nodesInColumn));
  for (size_t i = 0; i < graph->nodesSize; ++i) {
    if (levelsOfDependency[i] == UNKNOWN) {
      DynamicArray_Push(stack, i);
      while (DynamicArray_Size(size_t, stack) != 0) {
        size_t j = *(DynamicArray_End(size_t, stack) - 1);
        if (graph->nodes[j].dependencies[0] == (size_t)-1) {
          levelsOfDependency[j] = 0;
          DynamicArray_Pop(size_t, stack);
        } else {
          int max = INT_MIN;
          for (size_t k = 0; graph->nodes[j].dependencies[k] != (size_t)-1; ++k) {
            size_t dependency = graph->nodes[j].dependencies[k];
            if (levelsOfDependency[dependency] == UNKNOWN) {
              DynamicArray_Push(stack, dependency);
              levelsOfDependency[dependency] = DURING_CALCULATION;
              max = INT_MAX;
            } else if (levelsOfDependency[dependency] != DURING_CALCULATION) {
              if (max != INT_MAX && max < levelsOfDependency[dependency]) {
                max = levelsOfDependency[dependency];
              }
            }
          }
          if (max != INT_MAX) {
            levelsOfDependency[j] = max + 1;
            DynamicArray_Pop(size_t, stack);
          }
        }
      }
    }
    result[i].x = ds.MARGIN + (levelsOfDependency[i] - minLevel) * (ds.NODE_W + ds.HORI_PADDING);
    result[i].y = ds.MARGIN + nodesInColumn[levelsOfDependency[i] - minLevel] * (ds.NODE_H + ds.VERT_PADDING);
    ++nodesInColumn[levelsOfDependency[i] - minLevel];
  }
  DynamicArray_Destroy(stack);
  free(stack);
}

static void DrawEdge(const Vector2 edgeOffsets,
                     const Vector2 source,
                     const Vector2 destination,
                     const Vector2 scrollOffset) {
  DiagramStyle ds = DiagramStyle_Default();

  Vector2 strip[8];

  // middle on bottom side of source node
  {
    strip[0].x = scrollOffset.x + source.x + ds.NODE_W / 2;
    strip[0].y = scrollOffset.y + source.y + ds.NODE_H;
  }

  // Line on "edge-bus"
  {
    strip[1].x = strip[0].x;
    strip[1].y = strip[0].y + edgeOffsets.y + ds.VERT_PADDING;

    strip[2].x = scrollOffset.x + destination.x - edgeOffsets.x - ds.HORI_PADDING;
    strip[2].y = strip[1].y;

    strip[3].x = strip[2].x;
    strip[3].y = scrollOffset.y + destination.y + ds.NODE_H / 2;
  }

  // middle point on left side of destination node
  {
    strip[4].x = scrollOffset.x + destination.x;
    strip[4].y = strip[3].y;
  }

  // Arrowhead
  {
    strip[5].x = strip[4].x - ds.HORI_PADDING;
    strip[5].y = strip[4].y - ds.NODE_H / 3;
    strip[6].x = strip[4].x - ds.HORI_PADDING;
    strip[6].y = strip[4].y + ds.NODE_H / 3;
    strip[7] = strip[4];
  }

  DrawLineStrip(strip, 8, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
}
