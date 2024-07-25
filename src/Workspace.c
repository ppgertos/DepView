
#include "App.h"
#include "Diagram.h"
#include "LogBook.h"

#include <gui_window_file_dialog.h>
#include <raygui.h>

#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Core {
  LogBook logBook;
  size_t currentLog;
  Diagram oldDiagram;
  Diagram currentDiagram;
} Core;

static void DrawGraph(const Core* core, float procent, const Vector2* scrollOffset);
// static void BuildAbsoluteLayout(Vector2* result, const Diagram* diagram);
static void BuildRelativeLayout(Vector2* result, const Diagram* diagram, const size_t selectedNode);
static void DrawEdge(const Vector2* coordinates, const Edge* edge, const Vector2* scrollOffset);

typedef struct DiagramStyle {
  const float VERT_PADDING;
  const float HORI_PADDING;
  const float NODE_H;
  const float NODE_W;
  const float MARGIN;
} DiagramStyle;

struct DiagramStyle DiagramStyle_Default() {
  return (DiagramStyle){
      .VERT_PADDING = 10,
      .HORI_PADDING = 15,
      .NODE_W = 140,
      .NODE_H = 30,
      .MARGIN = 10,
  };
}

void Workspace_Draw(const Core* core, float animationProgress, const Vector2* scrollOffset) {
  float deltaPosition = 1.0;
  if (animationProgress < 1.0) {
    deltaPosition = 0.5 + 0.5 * sinf(animationProgress * 3.14 - 1.57);
  }
  DrawGraph(core, deltaPosition, scrollOffset);
}

static void DrawGraph(const Core* core, float procent, const Vector2* scrollOffset) {
  DiagramStyle ds = DiagramStyle_Default();
  static size_t selectedNode = 0;
  const Diagram* diagram = &core->currentDiagram;
  const Diagram* oldDiagram = &core->oldDiagram;
  //  BuildAbsoluteLayout(diagram->coordinates, diagram);
  if (selectedNode >= diagram->nodesSize) {
    selectedNode = 0;
  }
  BuildRelativeLayout(diagram->coordinates, diagram, selectedNode);

  size_t nodesSize = diagram->nodesSize < oldDiagram->nodesSize ? oldDiagram->nodesSize : diagram->nodesSize;
  Vector2 coords[nodesSize];
  for (size_t i = 0; i != nodesSize; ++i) {
    if (i < oldDiagram->nodesSize && i < diagram->nodesSize) {
      coords[i].x = oldDiagram->coordinates[i].x * (1.0 - procent) + diagram->coordinates[i].x * procent;
      coords[i].y = oldDiagram->coordinates[i].y * (1.0 - procent) + diagram->coordinates[i].y * procent;
    } else if (i >= oldDiagram->nodesSize) {
      coords[i] = diagram->coordinates[i];
    } else {
      coords[i] = oldDiagram->coordinates[i];
    }
  }

  for (size_t i = 0; i < diagram->nodesSize; ++i) {
    Node* node = &diagram->nodes[i];
    if (i == selectedNode) {
      GuiSetState(STATE_PRESSED);
    } else {
      switch (node->status) {
        case EStatus_Finished:
          GuiSetState(STATE_DISABLED);
          break;
        case EStatus_Ongoing:
          GuiSetState(STATE_FOCUSED);
          break;
        default:
          GuiSetState(STATE_NORMAL);
          break;
      }
    }
    if (GuiButton((Rectangle){coords[i].x + scrollOffset->x, coords[i].y + scrollOffset->y, ds.NODE_W, ds.NODE_H},
                  LogBook_GetNodeName(&core->logBook, node->nodeName))) {
      selectedNode = i;
    };
  }

  for (size_t i = 0; i < diagram->edgesSize; ++i) {
    DrawEdge(coords, &diagram->edges[i], scrollOffset);
  }

  GuiSetState(STATE_NORMAL);
}
/*
static void BuildAbsoluteLayout(Vector2* result, const Diagram* diagram) {
  DiagramStyle ds = DiagramStyle_Default();
  int nodesInColumn[16];
  memset(&nodesInColumn, '\0', sizeof(nodesInColumn));
  int levelsOfDependency[diagram->nodesSize];
  memset(levelsOfDependency, -1, sizeof(levelsOfDependency));

  DynamicArray* stack = DynamicArray_Make(size_t);
  for (size_t i = 0; i < diagram->nodesSize; ++i) {
    if (levelsOfDependency[i] == -1) {
      DynamicArray_Push(stack, i);
      while (DynamicArray_Size(size_t, stack) != 0) {
        size_t j = *(DynamicArray_End(size_t, stack) - 1);
        if (diagram->nodes[j].dependencies[0] == (size_t)-1) {
          levelsOfDependency[j] = 0;
          DynamicArray_Pop(size_t, stack);
        } else {
          size_t max = 0;
          for (size_t k = 0; diagram->nodes[j].dependencies[k] != (size_t)-1; ++k) {
            size_t dependency = diagram->nodes[j].dependencies[k];
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

    result[i].x = ds.MARGIN + levelsOfDependency[i] * (ds.NODE_W + ds.HORI_PADDING);
    result[i].y = ds.MARGIN + nodesInColumn[levelsOfDependency[i]] * (ds.NODE_H + ds.VERT_PADDING);
    ++nodesInColumn[levelsOfDependency[i]];
  }
  DynamicArray_Destroy(stack);
  free(stack);
}
*/
static void BuildRelativeLayout(Vector2* result, const Diagram* diagram, const size_t selectedNode) {
  DiagramStyle ds = DiagramStyle_Default();
  int minLevel = 0;
  int levelsOfDependency[diagram->nodesSize];

  const int UNKNOWN = 1024 * 64 - 1;
  const int DURING_CALCULATION = UNKNOWN - 1;
  for (size_t i = 0; i < diagram->nodesSize; ++i) {
    levelsOfDependency[i] = UNKNOWN;
  }

  DynamicArray* stack = DynamicArray_Make(size_t);
  levelsOfDependency[selectedNode] = 0;
  DynamicArray_Push(stack, selectedNode);
  while (DynamicArray_Size(size_t, stack) != 0) {
    size_t current = *(DynamicArray_Pop(size_t, stack));
    for (size_t k = 0; diagram->nodes[current].dependencies[k] != (size_t)-1; ++k) {
      size_t dependency = diagram->nodes[current].dependencies[k];
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
  for (size_t i = 0; i < diagram->nodesSize; ++i) {
    if (levelsOfDependency[i] == UNKNOWN) {
      DynamicArray_Push(stack, i);
      while (DynamicArray_Size(size_t, stack) != 0) {
        size_t j = *(DynamicArray_End(size_t, stack) - 1);
        if (diagram->nodes[j].dependencies[0] == (size_t)-1) {
          levelsOfDependency[j] = 0;
          DynamicArray_Pop(size_t, stack);
        } else {
          size_t max = 0;
          for (size_t k = 0; diagram->nodes[j].dependencies[k] != (size_t)-1; ++k) {
            size_t dependency = diagram->nodes[j].dependencies[k];
            if (levelsOfDependency[dependency] == UNKNOWN) {
              DynamicArray_Push(stack, dependency);
              levelsOfDependency[dependency] = DURING_CALCULATION;
              max = (size_t)-1;
            } else if (levelsOfDependency[dependency] != DURING_CALCULATION) {
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
    result[i].x = ds.MARGIN + (levelsOfDependency[i] - minLevel) * (ds.NODE_W + ds.HORI_PADDING);
    result[i].y = ds.MARGIN + nodesInColumn[levelsOfDependency[i] - minLevel] * (ds.NODE_H + ds.VERT_PADDING);
    ++nodesInColumn[levelsOfDependency[i] - minLevel];
  }
  DynamicArray_Destroy(stack);
  free(stack);
}

static void DrawEdge(const Vector2* coordinates, const Edge* edge, const Vector2* scrollOffset) {
  DiagramStyle ds = DiagramStyle_Default();

  Vector2 strip[8];

  // middle of nodes bottom side
  {
    strip[0].x = scrollOffset->x + coordinates[edge->source].x + ds.NODE_W / 2;
    strip[0].y = scrollOffset->y + coordinates[edge->source].y + ds.NODE_H;
  }

  strip[1].x = strip[0].x;
  strip[1].y = strip[0].y + ds.VERT_PADDING / 5 + (edge->source % 9);  // here to add small jumps to distinguish lines

  // Line on "edge-bus"
  {
    strip[2].x = scrollOffset->x + coordinates[edge->destination].x - ds.HORI_PADDING / 5 -
                 (edge->source % 9);  // here to add small jumps to distinguish lines
    strip[2].y = strip[1].y;

    strip[3].x = strip[2].x;
    strip[3].y = scrollOffset->y + coordinates[edge->destination].y + ds.NODE_H / 2;
  }

  // middle point on nodes left side
  {
    strip[4].x = scrollOffset->x + coordinates[edge->destination].x;
    strip[4].y = strip[3].y;
  }

  // Arrowhead
  {
    strip[5].x = strip[4].x - ds.HORI_PADDING / 3;
    strip[5].y = strip[4].y - ds.VERT_PADDING / 4;
    strip[6].x = strip[4].x - ds.HORI_PADDING / 3;
    strip[6].y = strip[4].y + ds.VERT_PADDING / 4;
    strip[7] = strip[4];
  }

  DrawLineStrip(strip, 8, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
}
