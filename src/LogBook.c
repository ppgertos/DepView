#define _XOPEN_SOURCE 500

#include "LogBook.h"
#include "DynamicArray.h"
#include "StringContainer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static LogEntry LogBook_MakeEntryFromString(LogBook* this, const char* line, const size_t lineNumber);
static void LogBook_MakeDependencies(LogBook* this, LogEntry* le, char* fieldBegin, char* fieldEnd);
static size_t LogBook_FindNodeNameOffset(LogBook* this, const char* name);

LogBook LogBook_Init(const char* fileName) {
  LogBook this = (LogBook){
      .entries = NULL,
      .entriesSize = 0,
      .nodeNames = StringContainer_Init(),
  };
  return this;
}

void LogBook_Load(LogBook* this, const char* fileName) {
  if (!fileName) {
    perror("filename is NULL");
    exit(10);
  }

  FILE* fptr = fopen(fileName, "r");
  if (fptr == NULL) {
    perror("Cannot read file: ");
    perror(fileName);
    exit(1);
  }
  char buffer[128];
  memset(buffer, '\0', 128);
  while (fgets(buffer, sizeof(buffer), fptr)) {
    ++this->entriesSize;
  }
  fclose(fptr);

  fptr = fopen(fileName, "r");
  if (fptr == NULL) {
    perror("Cannot read file: ");
    perror(fileName);
    exit(1);
  }

  this->entries = calloc(sizeof(LogEntry), this->entriesSize);
  if (this->entries == NULL) {
    perror("Cannot calloc memory for LogBook entries");
    exit(10);
  }

  StringContainer_Append(&this->nodeNames, "<UnknownNode>");
  size_t i = 0;
  while (fgets(buffer, sizeof(buffer), fptr)) {
    this->entries[i] = LogBook_MakeEntryFromString(this, buffer, i + 1);
    ++i;
  }
  StringContainer_Print(&this->nodeNames);
  fclose(fptr);
}

void LogBook_Destroy(LogBook* this) {
  StringContainer_Destroy(&this->nodeNames);
  free(this->entries);
  this->entriesSize = 0;
}

LogEntry LogBook_SyntaxWrongTimestamp(LogEntry* le, const char* timestamp, const size_t lineNumber) {
  le->timestamp = 0;
  char reason[512];
  snprintf(reason, 511, "Wrong timestamp '%s' at line %ld. Should be in '%%Y-%%m-%%dT%%H:%%M:%%S%%z' format.",
           timestamp, lineNumber);
  printf("%s\n", reason);
  return *le;
}

LogEntry LogBook_SyntaxWrongOperation(LogEntry* le, const char* operation, const size_t lineNumber) {
  le->operation = EOperation_None;
  char reason[512];
  snprintf(reason, 511, "Wrong operation '%s' at line %ld", operation, lineNumber);
  printf("%s\n", reason);
  return *le;
}

LogEntry LogBook_SyntaxUnknownNodeName(LogEntry* le, const char* nodeName, const size_t lineNumber) {
  char reason[512];
  snprintf(reason, 511, "Unknown nodeName '%s' at line %ld", nodeName, lineNumber);
  printf("%s\n", reason);
  return *le;
}

LogEntry LogBook_SyntaxWrongStatus(LogEntry* le, const char* status, const size_t lineNumber) {
  le->status = EStatus_Waiting;
  char reason[512];
  snprintf(reason, 511, "Wrong status '%s' at line %ld", status, lineNumber);
  printf("%s\n", reason);
  return *le;
}

LogEntry LogBook_MakeEntryFromString(LogBook* this, const char* buffer, const size_t lineNumber) {
  char timestamp[20];
  char operation[3];
  char nodeName[128];
  char operSpecific[512];

  memset(nodeName, '\0', sizeof(nodeName));
  memset(operation, '\0', sizeof(operation));
  memset(timestamp, '\0', sizeof(timestamp));
  memset(operSpecific, '\0', sizeof(operSpecific));
  sscanf(buffer, "%s %s %s%[][0-9a-zA-Z, \n]", timestamp, operation, nodeName, operSpecific);
  LogEntry le = {
      .operation = EOperation_None,
      .status = EStatus_Finished,
      .timestamp = 0,
      .nodeName = 0,
  };
  memset(le.dependencies, '\0', sizeof(le.dependencies));

  struct tm tm;
  memset(&tm, '\0', sizeof(struct tm));
  char* isOk = strptime(timestamp, "%Y-%m-%dT%H:%M:%S%z", &tm);
  if (!isOk) {
    return LogBook_SyntaxWrongTimestamp(&le, timestamp, lineNumber);
  }
  le.timestamp = mktime(&tm);

  switch (operation[0]) {  // clang-format off
    case 's': le.operation = EOperation_StatusChange; break;
    case '+': switch (operation[1]) {
        case '\0': le.operation = EOperation_Add; break;
        case 'd': le.operation = EOperation_AddDependency; break;
        default: return LogBook_SyntaxWrongOperation(&le, operation, lineNumber);
      } break;
    case '-': switch (operation[1]) {
        case '\0': le.operation = EOperation_Remove; break;
		case 'd': le.operation = EOperation_RemoveDependency; break;
        default: return LogBook_SyntaxWrongOperation(&le, operation, lineNumber);
      } break;
    default: return LogBook_SyntaxWrongOperation(&le, operation, lineNumber);
  }  // clang-format on

  switch (le.operation) {
    case EOperation_Add: {
      le.nodeName = StringContainer_Append(&this->nodeNames, nodeName);
      char status[10];
      char dependencies[512];
      sscanf(operSpecific, " %s %[][0-9a-zA-Z, \n]", status, dependencies);

      switch (status[0]) {  // clang-format off
        case 'o': le.status = EStatus_Ongoing; break;
        case 'w': le.status = EStatus_Waiting; break;
        case 'f': le.status = EStatus_Finished; break;
        default: return LogBook_SyntaxWrongStatus(&le, status, lineNumber);
      }  // clang-format on

      char* dependenciesEnd = strchr(dependencies, ']');
      printf("dependencies of %s: %s", LogBook_GetNodeName(this, le.nodeName), dependencies);
      LogBook_MakeDependencies(this, &le, dependencies + 1, dependenciesEnd - 1);
      return le;
    }
    case EOperation_StatusChange: {
      le.nodeName = LogBook_FindNodeNameOffset(this, nodeName);
      if (!le.nodeName) {
        return LogBook_SyntaxUnknownNodeName(&le, nodeName, lineNumber);
      }

      char status[10];
      sscanf(operSpecific, " %s", status);

      switch (status[0]) {  // clang-format off
        case 'o': le.status = EStatus_Ongoing; break;
        case 'w': le.status = EStatus_Waiting; break;
        default: return LogBook_SyntaxWrongStatus(&le, status, lineNumber);
      }  // clang-format on

      printf("new status of %s: %d\n", LogBook_GetNodeName(this, le.nodeName), le.status);
      return le;
    }
    case EOperation_Remove: {
      le.nodeName = LogBook_FindNodeNameOffset(this, nodeName);
      if (!le.nodeName) {
        return LogBook_SyntaxUnknownNodeName(&le, nodeName, lineNumber);
      }
      le.status = EStatus_Finished;
      printf("removal of %s\n", nodeName);
      return le;
    }
    case EOperation_AddDependency:
    case EOperation_RemoveDependency: {
      le.nodeName = LogBook_FindNodeNameOffset(this, nodeName);
      if (!le.nodeName) {
        return LogBook_SyntaxUnknownNodeName(&le, nodeName, lineNumber);
      }
      char dependencies[512];
      sscanf(operSpecific, " %[][0-9a-zA-Z, \n]", dependencies);
      char* dependenciesEnd = strchr(dependencies, ']');
      printf("dependencies of %s: %s\n", LogBook_GetNodeName(this, le.nodeName), dependencies);
      LogBook_MakeDependencies(this, &le, dependencies + 1, dependenciesEnd - 1);
      return le;
    }
    default:
      return le;
  }
}

void LogBook_MakeDependencies(LogBook* this, LogEntry* logEntry, char* begin, char* end) {
  const char* nameBegin = begin;
  size_t nextDependency = 0;
  while (nameBegin < end) {
    char* nameEnd = strpbrk(nameBegin, ",]");
    if (nameEnd) {
      *nameEnd = '\0';
    } else {
      nameEnd = end;
    }

    fprintf(stdout, "dependencies[%ld]: %s\n", nextDependency, nameBegin);
    logEntry->dependencies[nextDependency++] = LogBook_FindNodeNameOffset(this, nameBegin);
    if (nameEnd + 1 < end) {
      nameBegin = strchr(nameEnd + 1, ' ');
      if (nameBegin == NULL) {
        nameBegin = end;
        continue;
      }
      nameBegin += 1;
      continue;
    }
    nameBegin = end;
  }
}

char* LogBook_GetNodeName(const LogBook* this, const size_t offset) {
  return &this->nodeNames.begin[offset];
}

void LogBook_Print(LogBook* this) {
  for (const LogEntry* le = this->entries; le < this->entries + this->entriesSize; ++le) {
    char deps[1024];
    memset(deps, '\0', sizeof(deps));
    size_t end = 0;
    for (const size_t* dep = le->dependencies; *dep != 0; ++dep) {
      end += snprintf(deps + end, 1023 - end, "%s, ", &this->nodeNames.begin[*dep]);
    }
    printf("%ld %d %s %d {%s}\n", le->timestamp, le->operation, &this->nodeNames.begin[le->nodeName], le->status, deps);
  }
}

static size_t LogBook_FindNodeNameOffset(LogBook* this, const char* name) {
  size_t array_size =
      DynamicArray_End(size_t, this->nodeNames.offsets) - DynamicArray_Begin(size_t, this->nodeNames.offsets);
  for (size_t i = 1; i < array_size; ++i) {
    const char* current = StringContainer_At(&this->nodeNames, i);
    if (0 == strcmp(current, name)) {
      return current - this->nodeNames.begin;
    }
  }
  return 0;
}
