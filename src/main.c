#include <App.h>

#include <stdlib.h>

int main() {
  App* app = malloc(App_SizeOf());
  App_Init(app);

  App_Run(app);

  App_Destroy(app);
  free(app); 
  return 0;
}

