#include "Diagram.h"
#include "DynamicArray.h"
#include "LogBook.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Node* Diagram_FindNode(Diagram const* diagram, size_t nodeName);
static Edge* Diagram_FindEdge(Diagram const* diagram, size_t source, size_t destination);

void Diagram_Destroy(Diagram* this) {
  free(this->edges);
  this->edgesSize = 0;
  free(this->coordinates);
  free(this->nodes);
  this->nodesSize = 0;
}

Diagram Diagram_Init(const struct LogBook* logBook, size_t currentLogIndex) {
  Diagram this = {
      .nodes = NULL,
      .nodesSize = 0,
      .coordinates = NULL,
      .edges = NULL,
      .edgesSize = 0,
  };

  if (!logBook) {
    return this;
  }

  if (currentLogIndex >= logBook->entriesSize) {
    puts("Runtime error: currentLogIndex > logBook.entriesSize");
    exit(10);
  }

  const size_t maxNodesNumber = DynamicArray_Size(size_t, logBook->nodeNames.offsets);
  this.nodes = calloc(maxNodesNumber, sizeof(Node));
  this.coordinates = calloc(maxNodesNumber, sizeof(Node));
  this.edges = calloc(maxNodesNumber * (maxNodesNumber - 1), sizeof(Edge));
  printf("calloc(%ld, %ld) : %p \n", maxNodesNumber, sizeof(Node), (void*)this.nodes);

  LogEntry* logBegin = logBook->entries;
  LogEntry* logEnd = logBegin + currentLogIndex + 1;
  for (LogEntry* log = logBegin; log < logEnd; ++log) {
    switch (log->operation) {
      case EOperation_Add: {
        if (this.nodesSize >= maxNodesNumber) {
          perror("More nodes than expected");
          exit(10);
        }
        Node* node = &this.nodes[this.nodesSize++];
        node->timestamp = log->timestamp;
        node->nodeName = log->nodeName;
        node->status = log->status;

        size_t i = 0;
        while (log->dependencies[i] != 0) {
          node->dependencies[i] = Diagram_FindNode(&this, log->dependencies[i]) - this.nodes;
          ++i;
        }
        node->dependencies[i] = (size_t)-1;

        i = 0;
        while (node->dependencies[i] != (size_t)-1) {
          Edge* e = &this.edges[this.edgesSize++];
          e->destination = node - this.nodes;
          e->source = node->dependencies[i];
          ++i;
        }
        break;
      }

      case EOperation_Remove: {
        Node* node = Diagram_FindNode(&this, log->nodeName);
        if (node == NULL) {
          fprintf(stderr, "Unable to find node '%s' at log %ld", LogBook_GetNodeName(logBook, log->nodeName),
                  log->timestamp);
          exit(10);
        }
        node->status = EStatus_Finished;
        break;
      }

      case EOperation_StatusChange: {
        Node* node = Diagram_FindNode(&this, log->nodeName);
        if (node == NULL) {
          fprintf(stderr, "Unable to find node '%s' at log %ld", LogBook_GetNodeName(logBook, log->nodeName),
                  log->timestamp);
          exit(10);
        }
        node->status = EStatus_Ongoing;
        break;
      }

      case EOperation_AddDependency: {
        Node* node = Diagram_FindNode(&this, log->nodeName);
        if (node == NULL) {
          fprintf(stderr, "Unable to find node '%s' at log %ld", LogBook_GetNodeName(logBook, log->nodeName),
                  log->timestamp);
          exit(10);
        }

        size_t* nodeDepsEnd = node->dependencies;
        while (*nodeDepsEnd != (size_t)-1) {
          ++nodeDepsEnd;
        }

        for (size_t i = 0; log->dependencies[i] != 0; ++i) {
          size_t dependencyIndex = Diagram_FindNode(&this, log->dependencies[i]) - this.nodes;
          if (node == NULL) {
            fprintf(stderr, "Unable to find node '%s' at log %ld", LogBook_GetNodeName(logBook, dependencyIndex),
                    log->timestamp);
            exit(10);
          }
          *nodeDepsEnd = dependencyIndex;
          ++nodeDepsEnd;

          Edge* e = &this.edges[this.edgesSize++];
          e->destination = node - this.nodes;
          e->source = dependencyIndex;
        }
        *nodeDepsEnd = (size_t)-1;
        break;
      }

      case EOperation_RemoveDependency: {
        Node* node = Diagram_FindNode(&this, log->nodeName);
        if (node == NULL) {
          fprintf(stderr, "Unable to find node '%s' at log %ld", LogBook_GetNodeName(logBook, log->nodeName),
                  log->timestamp);
          exit(10);
        }

        size_t* nodeDependenciesLast = node->dependencies;
        while (*nodeDependenciesLast != (size_t)-1) {
          ++nodeDependenciesLast;
        }
        --nodeDependenciesLast;

        for (const size_t* logDep = log->dependencies; *logDep != 0; ++logDep) {
          size_t logDependencyIndex = Diagram_FindNode(&this, *logDep) - this.nodes;
          size_t* nodeDependencyIndex = node->dependencies;
          while (*nodeDependencyIndex != (size_t)-1) {
            if (*nodeDependencyIndex == logDependencyIndex) {
              *nodeDependencyIndex = *nodeDependenciesLast;
              *nodeDependenciesLast = (size_t)-1;
              --nodeDependenciesLast;
              Edge* unwantedEdge = Diagram_FindEdge(&this, logDependencyIndex, node - this.nodes);
              *unwantedEdge = this.edges[--this.edgesSize];
            } else {
              ++nodeDependencyIndex;
            }
          }
        }
        break;
      }
      default:
        break;
    }
  }
  return this;
}

void Diagram_Copy(Diagram* target, const Diagram* source)
{
  memcpy(target, source, sizeof(Diagram));
  target->nodes = calloc(sizeof(Node), target->nodesSize);
  memcpy(target->nodes, source->nodes, sizeof(Node)*target->nodesSize);
  target->coordinates = calloc(sizeof(Vector2), target->nodesSize);
  memcpy(target->coordinates, source->coordinates, sizeof(Vector2)*target->nodesSize);
  target->edges = calloc(sizeof(Edge), target->edgesSize);
  memcpy(target->edges, source->edges, sizeof(Edge)*target->edgesSize);
}

static Node* Diagram_FindNode(Diagram const* diagram, size_t nodeName) {
  for (Node* it = diagram->nodes; it < diagram->nodes + diagram->nodesSize; ++it) {
    if (nodeName == it->nodeName) {
      return it;
    }
  }
  return NULL;
}

static Edge* Diagram_FindEdge(Diagram const* diagram, size_t source, size_t destination) {
  for (Edge* it = diagram->edges; it < diagram->edges + diagram->edgesSize; ++it) {
    if (source == it->source && destination == it->destination) {
      return it;
    }
  }
  return NULL;
}
