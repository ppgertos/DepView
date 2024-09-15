#include "Workspace.h"

#include "App.h"
#include "Graph.h"
#include "LogBook.h"

#include <raygui.h>

#include <limits.h>
#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Workspace {
  size_t selectedNode;
  int diagramLayout;
  Vector2* coordinates;
  Vector2* previousCoordinates;
} Workspace;

typedef struct Core {
  LogBook logBook;
  size_t currentLog;
  Graph oldGraph;
  Graph currentGraph;
  Workspace* workspace;
} Core;

static void DrawDiagram(const Core* core, float procent, const Vector2* scrollOffset);
static void BuildAbsoluteLayout(Vector2* result, const Graph* graph);
static void BuildRelativeLayout(Vector2* result, const Graph* graph, const size_t selectedNode);
static void DrawEdge(const Vector2* coordinates, const Edge* edge, const Vector2* scrollOffset);

typedef struct DiagramStyle {
  const float VERT_PADDING;
  const float HORI_PADDING;
  const float NODE_H;
  const float NODE_W;
  const float MARGIN;
} DiagramStyle;

DiagramStyle DiagramStyle_Default() {
  return (DiagramStyle){
      .VERT_PADDING = 10,
      .HORI_PADDING = 15,
      .NODE_W = 140,
      .NODE_H = 30,
      .MARGIN = 10,
  };
}

size_t Workspace_SizeOf() {
  return sizeof(Workspace);
}

void Workspace_Init(Workspace* this) {
  this->selectedNode = 0;
  this->diagramLayout = 0;
  this->coordinates = NULL;
  this->previousCoordinates = NULL;
}

void Workspace_Destroy(Workspace* this) {
  this->selectedNode = 0;
  this->diagramLayout = 0;

  if (this->coordinates) {
    free(this->coordinates);
  }
  if (this->previousCoordinates) {
    free(this->previousCoordinates);
  }
}

void Workspace_SetDiagramLayout(Workspace* this, int diagramLayout) {
  this->diagramLayout = diagramLayout;
}

int* Workspace_PointDiagramLayout(Workspace* this) {
  return &this->diagramLayout;
}

void Workspace_Draw(const Core* core, float animationProgress, const Vector2* scrollOffset) {
  float deltaPosition = 1.0;
  if (animationProgress < 1.0) {
    deltaPosition = 0.5 + 0.5 * sinf(animationProgress * 3.14 - 1.57);
  }
  DrawDiagram(core, deltaPosition, scrollOffset);
}

void Workspace_BuildLayout(Workspace* workspace, const Graph* graph) {
  if (workspace->previousCoordinates) {
    free(workspace->previousCoordinates);
    workspace->previousCoordinates = NULL;
  }
  if (workspace->coordinates != NULL)
  {
    workspace->previousCoordinates = workspace->coordinates;
  }
  else {
    workspace->previousCoordinates = calloc(graph->nodesSize, sizeof(Vector2));
  }
  workspace->coordinates = calloc(graph->nodesSize, sizeof(Vector2));
  if (workspace->diagramLayout == 0) {
    BuildAbsoluteLayout(workspace->coordinates, graph);
  } else {
    size_t centralNode = workspace->selectedNode;
    if (centralNode >= graph->nodesSize) {
      centralNode = (size_t)-1;
    }
    BuildRelativeLayout(workspace->coordinates, graph, centralNode);
  }
//  if (! workspace->previousCoordinates)
//  {
//      workspace->previousCoordinates = workspace->coordinates;
//  }
}

static void DrawDiagram(const Core* core, float procent, const Vector2* scrollOffset) {
  if (!core || !core->workspace) {
    return;
  }

  DiagramStyle ds = DiagramStyle_Default();
  const Graph* graph = &core->currentGraph;
  const Graph* oldGraph = &core->oldGraph;
  const Vector2* oldCoords = core->workspace->previousCoordinates;
  const Vector2* newCoords = core->workspace->coordinates;

  size_t nodesSize = graph->nodesSize < oldGraph->nodesSize ? oldGraph->nodesSize : graph->nodesSize;
  Vector2 drawCoords[nodesSize];
  for (size_t i = 0; i != nodesSize; ++i) {
    if (i >= oldGraph->nodesSize) {
      drawCoords[i] = newCoords[i];
    } else if (i >= graph->nodesSize){
      drawCoords[i] = oldCoords[i];
    } else {
      drawCoords[i].x = oldCoords[i].x * (1.0 - procent) + newCoords[i].x * procent;
      drawCoords[i].y = oldCoords[i].y * (1.0 - procent) + newCoords[i].y * procent;
    }
  }

  for (size_t i = 0; i < graph->nodesSize; ++i) {
    Node* node = &graph->nodes[i];

    switch (node->status) {
      case EStatus_Finished:
        GuiSetState(STATE_DISABLED);
        break;
      case EStatus_Ongoing:
        GuiSetState(STATE_FOCUSED);
        break;
      default:
        if (i == core->workspace->selectedNode) {
          GuiSetState(STATE_PRESSED);
        } else {
          GuiSetState(STATE_NORMAL);
        }
        break;
    }

    if (GuiButton((Rectangle){drawCoords[i].x + scrollOffset->x, drawCoords[i].y + scrollOffset->y, ds.NODE_W, ds.NODE_H},
                  LogBook_GetNodeName(&core->logBook, node->nodeName))) {
      core->workspace->selectedNode = i;
    };
  }

  GuiSetState(STATE_NORMAL);

  for (size_t i = 0; i < graph->edgesSize; ++i) {
    DrawEdge(drawCoords, &graph->edges[i], scrollOffset);
  }
}

static void BuildAbsoluteLayout(Vector2* result, const Graph* graph) {
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

    result[i].x = ds.MARGIN + levelsOfDependency[i] * (ds.NODE_W + ds.HORI_PADDING);
    result[i].y = ds.MARGIN + nodesInColumn[levelsOfDependency[i]] * (ds.NODE_H + ds.VERT_PADDING);
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

static void DrawEdge(const Vector2* coordinates, const Edge* edge, const Vector2* scrollOffset) {
  DiagramStyle ds = DiagramStyle_Default();

  Vector2 strip[8];

  // middle on bottom side of source node
  {
    strip[0].x = scrollOffset->x + coordinates[edge->source].x + ds.NODE_W / 2;
    strip[0].y = scrollOffset->y + coordinates[edge->source].y + ds.NODE_H;
  }

  // Line on "edge-bus"
  {
    strip[1].x = strip[0].x;
    strip[1].y = strip[0].y + ds.VERT_PADDING / 5 + (edge->source % 9);  // here to add small jumps to distinguish lines

    strip[2].x = scrollOffset->x + coordinates[edge->destination].x - ds.HORI_PADDING / 5 -
                 (edge->source % 9);  // here to add small jumps to distinguish lines
    strip[2].y = strip[1].y;

    strip[3].x = strip[2].x;
    strip[3].y = scrollOffset->y + coordinates[edge->destination].y + ds.NODE_H / 2;
  }

  // middle point on left side of destination node 
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
