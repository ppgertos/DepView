# DepView

A minimalistic dependency diagram viewer.

![Screenshot](https://raw.githubusercontent.com/ppgertos/DepView/master/doc/depView.gif)

## Key Features

* Parses logs in specific syntax. Sample log file to be found in `test` directory;
* Draws graph of dependency between nodes. For different "snapshots" based on log file;
* Nodes do not change their place when user traverses logs;
* Least 3rdparty dependecies possible;

## How To Use

To clone and run this application, you'll need [Git](https://git-scm.com) and [CMake](https://cmake.org/download/) at least in version 3.28. From your command line:

```bash
# Clone this repository
$ git clone https://github.com/ppgertos/DepView

# Go into the repository
$ cd DepView

# Build app
$ mkdir build && cmake -B build -S . && cmake --build build

# Run the app
$ ./build/DepView
```

In opened window click "LOAD FILE" and choose file with logs for which you want to see dependency graph. For example `DepView/test/sample_list`

## Further improvements

* Replace text logs file with binary data;
* Add simple configuration file to keep recently choosen path, and gui theme;

## Credits

This software uses the following open source packages:

- [Raylib](https://github.com/raysan5/raylib)
- [Raygui](https://github.com/raysan5/raygui)

## License

MIT

