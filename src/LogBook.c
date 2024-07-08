#define _XOPEN_SOURCE 500

#include "LogBook.h"

#include "StringContainer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static LogEntry LogBook_MakeEntryFromString(LogBook *this, char *line);
static void LogBook_MakeDependencies(LogBook *this, LogEntry *le,
                                     char *fieldBegin, char *fieldEnd);
static size_t LogBook_FindNodeNameOffset(LogBook *this, const char *name);

LogBook LogBook_Make(const char *fileName) {
  LogBook this;
  this.entriesSize = 0;
  FILE *fptr = fopen(fileName, "r");
  if (fptr == NULL) {
    perror("Cannot read file: ");
    perror(fileName);
    exit(1);
  }
  char buffer[128];
  memset(buffer, 0, 128);
  while (fgets(buffer, sizeof(buffer), fptr)) {
    ++this.entriesSize;
  }
  fclose(fptr);

  fptr = fopen(fileName, "r");
  if (fptr == NULL) {
    perror("Cannot read file: ");
    perror(fileName);
    exit(1);
  }

  this.nodeNames = StringContainer_Make();
  this.entries = calloc(sizeof(LogEntry), this.entriesSize);

  if (this.entries == NULL) {
    perror("Cannot calloc memory for LogBook entries");
    exit(10);
  }

  int i = 0;
  while (fgets(buffer, sizeof(buffer), fptr)) {
    this.entries[i++] = LogBook_MakeEntryFromString(&this, buffer);
  }
  StringContainer_Print(&this.nodeNames);
  fclose(fptr);
  return this;
}

LogEntry LogBook_MakeEntryFromString(LogBook *this, char *buffer) {
  char *fieldBegin = buffer;
  LogEntry le;
  memset(&le.dependencies, '\0', sizeof(le.dependencies));
  char *fieldEnd = strchr(buffer, ' ');
  *fieldEnd = '\0';
  struct tm tm;
  memset(&tm, '\0', sizeof(struct tm));
  char *isOk = strptime(fieldBegin, "%Y-%m-%dT%H:%M:%S%z", &tm);
  if (isOk == NULL) {
    perror("Unable to read timestamp at:\n");
    perror(buffer);
    exit(10);
  }
  le.timestamp = mktime(&tm);
  fieldBegin = fieldEnd + 1;
  fieldEnd = strchr(fieldBegin, ' ');
  *fieldEnd = '\0';
  le.operation = EOperation_None;
  if (*fieldBegin == '+') {
    le.operation = EOperation_Add;
    fieldBegin = fieldEnd + 1;
    fieldEnd = strchr(fieldBegin, ' ');
    *fieldEnd = '\0';
    StringContainer_Append(&this->nodeNames, fieldBegin);
    le.nodeName = LogBook_FindNodeNameOffset(this, fieldBegin);
    fieldBegin = fieldEnd + 1;
    fieldEnd = strchr(fieldBegin, ' ');
    *fieldEnd = '\0';
    switch (fieldBegin[0]) {
    case 'o':
      le.status = EStatus_Ongoing;
      break;
    case 'w':
    default:
      le.status = EStatus_Waiting;
      break;
    }
    fieldBegin = strchr(fieldEnd + 1, '[');
    fieldEnd = strchr(fieldBegin, ']');
    *fieldEnd = '\0';
    printf("dependencies of %s: %s\n", LogBook_GetNodeName(this, le.nodeName), fieldBegin);
    LogBook_MakeDependencies(this, &le, fieldBegin + 1, fieldEnd);
    return le;
  } else if (*fieldBegin == 's') {
    le.operation = EOperation_StatusChange;
    fieldBegin = fieldEnd + 1;
    fieldEnd = strchr(fieldBegin, ' ');
    *fieldEnd = '\0';
    le.nodeName = LogBook_FindNodeNameOffset(this, fieldBegin);
    fieldBegin = fieldEnd + 1;
    fieldEnd = strchr(fieldBegin, '\n');
    *fieldEnd = '\0';
    switch (fieldBegin[0]) {
    case 'o':
      le.status = EStatus_Ongoing;
      break;
    case 'w':
    default:
      le.status = EStatus_Waiting;
      break;
    }
    printf("new status of %s: %d\n", LogBook_GetNodeName(this, le.nodeName), le.status);
    return le;
  } else if (*fieldBegin == '-') {
    le.operation = EOperation_Remove;
    fieldBegin = fieldEnd + 1;
    fieldEnd = strchr(fieldBegin, '\n');
    *fieldEnd = '\0';
    memset(le.dependencies, '\0', sizeof(le.dependencies));
    le.status = EStatus_Finished;
    le.nodeName = LogBook_FindNodeNameOffset(this, fieldBegin);
    printf("removal of %s\n", fieldBegin);
    return le;
  }
  perror("Unknown operator in line:");
  perror(buffer);
  return le;
}

void LogBook_MakeDependencies(LogBook *this, LogEntry *logEntry, char *begin,
                              char *end) {
  const char *nameBegin = begin;
  size_t nextDependency = 0;
  while (nameBegin < end) {
    char *nameEnd = strchr(nameBegin, ',');
    if (nameEnd == NULL) {
      nameEnd = end;
    } else {
      *nameEnd = '\0';
    }

    fprintf(stdout, "dependencies[%ld]: %s\n", nextDependency, nameBegin);
    logEntry->dependencies[nextDependency++] =
        LogBook_FindNodeNameOffset(this, nameBegin);
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
  logEntry->dependencies[nextDependency] = (size_t)-1;
}

char *LogBook_GetNodeName(const LogBook *this, const size_t offset) {
  return &this->nodeNames.begin[offset];
}

static size_t LogBook_FindNodeNameOffset(LogBook *this, const char *name) {
  for (size_t i = 0; i < SizeTContainer_Used(&this->nodeNames.offsets); ++i) {
    const char *current = StringContainer_At(&this->nodeNames, i);
    if (0 == strcmp(current, name)) {
      return current - this->nodeNames.begin;
    }
  }
  return StringContainer_Used(&this->nodeNames);
}
