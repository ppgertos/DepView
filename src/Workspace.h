typedef struct Core Core;
typedef struct LogBook LogBook;
typedef struct Diagram Diagram;
typedef struct Vector2 Vector2;

void Workspace_Draw(const Core* core,
                    float animationProgress,
                    const Vector2* scrollOffset);
