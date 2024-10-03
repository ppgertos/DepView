#pragma once

#include "LogBook.h"
#include "Graph.h"

typedef struct Core {
  char selectedFileName[2048];
  LogBook logBook;
  size_t currentLog;
  Graph oldGraph;
  Graph currentGraph;
} Core;
