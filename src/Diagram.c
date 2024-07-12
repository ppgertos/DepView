#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Diagram.h"
#include "LogBook.h"

static Node* Diagram_FindNode(Diagram const* diagram, size_t nodeName);

void Diagram_Destroy(Diagram* this) {
  free(this->edges);
  this->edgesSize = 0;
  free(this->nodes);
  this->nodesSize = 0;
}

Diagram Diagram_Init(const struct LogBook* logBook, size_t currentLogIndex) {
  size_t nodesNumber = 0;
  size_t maxNodesNumber = 0;

  Diagram this;
  this.nodes = NULL;
  this.nodesSize = 0;
  this.edges = NULL;
  this.edgesSize = 0;
  if (!logBook) {
    return this;
  }
  if (currentLogIndex >= logBook->entriesSize) {
    puts("Runtime error: currentLogIndex > logBook.entriesSize");
    exit(10);
  }

  LogEntry* logBegin = logBook->entries;
  LogEntry* logEnd = logBook->entries + currentLogIndex + 1;
  for (LogEntry* log = logBegin; log < logEnd; ++log) {
    switch (log->operation) {
      case EOperation_Add:
        nodesNumber++;
        break;
      case EOperation_Remove:
        nodesNumber--;
        break;
      default:
        break;
    }
    if (maxNodesNumber < nodesNumber) {
      maxNodesNumber = nodesNumber;
    }
  }
  this.nodes = calloc(maxNodesNumber, sizeof(Node));
  this.edges = calloc(maxNodesNumber * (maxNodesNumber - 1), sizeof(Edge));
  printf("calloc(%ld, %ld) : %p \n", maxNodesNumber, sizeof(Node), (void*)this.nodes);
  size_t nextNode = 0;
  size_t nextEdge = 0;

  for (LogEntry* log = logBegin; log < logEnd; ++log) {
    switch (log->operation) {
      case EOperation_Add: {
        if (nextNode >= maxNodesNumber) {
          perror("More nodes than expected");
          exit(10);
        }
        Node* node = &this.nodes[nextNode++];
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
          Edge* e = &this.edges[nextEdge++];
          e->destination = node - this.nodes;
          e->source = node->dependencies[i];
          ++i;
          ++this.edgesSize;
        }
        ++this.nodesSize;
        break;
      }
      case EOperation_Remove: {
        Node* node = Diagram_FindNode(&this, log->nodeName);
        if (node == NULL) {
          fprintf(stderr, "Unable to find node %s at log %ld", LogBook_GetNodeName(logBook, log->nodeName),
                  log->timestamp);
          exit(10);
        }
        node->status = EStatus_Finished;
        break;
      }
      case EOperation_StatusChange: {
        Node* node = Diagram_FindNode(&this, log->nodeName);
        if (node == NULL) {
          fprintf(stderr, "Unable to find node %s at log %ld", LogBook_GetNodeName(logBook, log->nodeName),
                  log->timestamp);
          exit(10);
        }
        node->status = EStatus_Ongoing;
        break;
      }
      default:
        break;
    }
  }
  return this;
}

static Node* Diagram_FindNode(Diagram const* diagram, size_t nodeName) {
  for (Node* it = diagram->nodes; it < diagram->nodes + diagram->nodesSize; ++it) {
    if (nodeName == it->nodeName) {
      return it;
    }
  }
  return NULL;
}
