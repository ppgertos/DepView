#pragma once

#include "StringContainer.h"
#include <time.h>

typedef enum {
  EOperation_Add,
  EOperation_StatusChange,
  EOperation_AddDependency,
  EOperation_RemoveDependency,
  EOperation_Remove,
  EOperation_None
} EOperation;

typedef enum { EStatus_Waiting, EStatus_Ongoing, EStatus_Finished } EStatus;

typedef struct LogEntry{
  EOperation operation;
  EStatus status;
  time_t timestamp;
  size_t nodeName;
  size_t dependencies[128];
} LogEntry;

typedef struct LogBook{
	LogEntry* entries;
	size_t entriesSize;
	StringContainer nodeNames;
} LogBook;

LogBook LogBook_Init(const char *fileName);
void LogBook_Destroy(LogBook* this);
void LogBook_Load(LogBook* this, const char* fileName);
void LogBook_Print(LogBook* this);
char* LogBook_GetNodeName(const LogBook *this, const size_t offset);

