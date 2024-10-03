#include <App.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct Arguments {
  char* filePath;
} Arguments;

Arguments parseArgv(int argc, char** argv) {
  Arguments res;
  res.filePath = NULL;

  if (argc == 2) {
    if (access(argv[1], F_OK)) {
      printf("Cannot find file %s\n", argv[1]);
      return res;
    }
    res.filePath = argv[1];
  }

  return res;
}

int main(int argc, char** argv) {
  App* app = malloc(App_SizeOf());

  Arguments arguments = parseArgv(argc, argv);

  Config conf = {
      .filePath = arguments.filePath,
      .style = 4,
  };

  App_Init(app);
  App_Configure(app, conf);

  App_Run(app);

  App_Destroy(app);
  free(app);
  return 0;
}
