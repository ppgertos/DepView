#pragma once

#include <raylib.h>
#include <time.h>

#include <LogBook.h>

typedef struct Node {
  time_t timestamp;
  EStatus status;
  size_t nodeName;
  size_t dependencies[128]; //indexes of nodes in Graph.nodes
} Node;

typedef struct Edge {
  size_t source;
  size_t destination;
} Edge;

typedef struct Graph {
  Node* nodes;
  size_t nodesSize;
  Vector2* coordinates;
  Edge* edges;
  size_t edgesSize;
} Graph;

void Graph_Destroy(Graph* this);
Graph Graph_Init(const struct LogBook* logBook, size_t currentLogIndex);
void Graph_Copy(Graph* target, const Graph* source);
