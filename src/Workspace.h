typedef struct Workspace Workspace;

typedef struct Core Core;
typedef struct LogBook LogBook;
typedef struct Graph Graph;
typedef struct Vector2 Vector2;

#include <stddef.h>

void Workspace_Init(Workspace* this);
size_t Workspace_SizeOf();
void Workspace_Destroy(Workspace* this);

void Workspace_SetDiagramLayout(Workspace* this, int diagramLayout);
int* Workspace_PointDiagramLayout(Workspace* this);

void Workspace_BuildLayout(Workspace* workspace, const Graph* graph);

void Workspace_Draw(const Core* core, float animationProgress, const Vector2* scrollOffset);
