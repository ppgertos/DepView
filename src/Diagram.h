#pragma once

#include <raylib.h>
#include <time.h>

#include <LogBook.h>

typedef struct Node {
  time_t timestamp;
  EStatus status;
  size_t nodeName;
  size_t dependencies[128];
} Node;

typedef struct Edge {
  size_t source;
  size_t destination;
} Edge;

typedef struct Diagram {
  Node* nodes;
  size_t nodesSize;
  Edge* edges;
  size_t edgesSize;
} Diagram;

void Diagram_Destroy(Diagram* this);
Diagram Diagram_Make(const struct LogBook* logBook, size_t currentLogIndex);
