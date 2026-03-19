# Contributing to Rive Code Generator

## Cloning

```sh
git clone --recurse-submodules https://github.com/rive-app/rive-code-generator-wip
```

or

```sh
git clone https://github.com/rive-app/rive-code-generator-wip
cd rive-code-generator-wip
git submodule update --init --recursive
```

## Build Instructions

### Prerequisites

- C++ compiler with C++17 support
- Rive C++ Runtime library (included as a git submodule)
- `make` (or `ninja` if using the ninja build system)

Premake5 is built from source automatically on first build.

### Building

Debug build:
```sh
cd build && ./build.sh
```

Release build:
```sh
cd build && ./build.sh release
```

Clean build:
```sh
cd build && ./build.sh clean           # clean debug build
cd build && ./build.sh release clean   # clean release build
```

Rebuild an existing output directory (reuses previous premake args):
```sh
cd build && ./build.sh rebuild out/debug
```

The executable will be created in `build/out/debug` or `build/out/release`.

### Alternative Build Systems

```sh
cd build && ./build.sh ninja           # use ninja instead of make
cd build && ./build.sh xcode           # generate and build an Xcode workspace
```

### IDE Integration (compile_commands.json)

Generate `compile_commands.json` for clangd / IDE code completion:
```sh
cd build && ./build.sh compdb
```

This copies `compile_commands.json` to the project root for automatic IDE pickup.

### Running

```sh
cd build && ./build.sh run             # build and run --help
cd build && ./build.sh dev             # build and run with dev sample args
```

## Testing

To run the tests:
```sh
cd test && ./test.sh
```

Add additional tests to the `test.sh` script.

To regenerate the test data:
```sh
cd test && ./test.sh --update-expected
```
