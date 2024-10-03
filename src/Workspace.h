typedef struct Workspace Workspace;

typedef struct LogBook LogBook;
typedef struct Graph Graph;
typedef struct Vector2 Vector2;

#include "Core.h"

#include <stddef.h>

void Workspace_Init(Workspace* this);
size_t Workspace_SizeOf();
void Workspace_Destroy(Workspace* this);

Vector2 Workspace_GetSpaceSize(Workspace* this);

void Workspace_SetDiagramLayout(Workspace* this, int diagramLayout);
int* Workspace_PointDiagramLayout(Workspace* this);

void Workspace_BuildLayout(Workspace* workspace, const Graph* graph);

void Workspace_Draw(Workspace* this, const Core* core, float animationProgress, const Vector2* scrollOffset);
