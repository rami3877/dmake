# dmake.h

A lightweight single-header build system written entirely in C.

`builder.h` provides a simple native build automation engine capable of compiling executables, static libraries, and shared libraries with automatic dependency handling and incremental builds.

Unlike traditional Makefiles, Builder allows you to describe your build directly in C structures.

---

## Features

* Single-header library
* Build executables (`B_EXEC`)
* Build static libraries (`B_STATIC`)
* Build shared libraries (`B_SHARED`)
* Automatic dependency resolution
* Incremental compilation
* Wildcard source expansion (`*.c`)
* Self-rebuilding build scripts
* Configurable global compiler settings
* Runtime environment variable overrides
* Automatic rpath generation for shared libraries
* No external dependencies

---

## Quick Start

### 1. Create a build script

```c
#define BUILDER_IMPLEMENTATION
#include "dmake.h"

int main(int argc, char **argv)
{
    check_and_rebuild_self(argc, argv, __FILE__);

    setGlobalCompiler("gcc");
    setGlobalFlags("-Wall -Wextra -O2");
    setBuildDir("build");

    const i8 *sources[] = {
        "src/*.c",
        null
    };

    BObject app = {
        .name = "my_program",
        .type = B_EXEC,
        .files = sources
    };

    objectAdd(&app);

    return 0;
}
```

---

### 2. Build the build script

```bash
gcc build.c -o build
```

---

### 3. Run it

```bash
./build
```

---

## Object Types

### Executable

```c
BObject app = {
    .name = "app",
    .type = B_EXEC,
    .files = sources
};
```

Produces:

```text
build/app.elf
```

---

### Static Library

```c
BObject math = {
    .name = "math",
    .type = B_STATIC,
    .files = math_sources
};
```

Produces:

```text
build/libmath.a
```

---

### Shared Library

```c
BObject math = {
    .name = "math",
    .type = B_SHARED,
    .files = math_sources
};
```

Produces:

```text
build/libmath.so
```

---

## Dependencies

Libraries and targets may depend on other targets.

```c
BObject *deps[] = {
    &math,
    null
};

BObject app = {
    .name = "app",
    .type = B_EXEC,
    .files = sources,
    .object = deps
};
```

Builder automatically builds dependencies before the main target.

---

## Includes

```c
const i8 *includes[] = {
    "include",
    "vendor/include",
    null
};

BObject app = {
    .includes = includes
};
```

Generated compiler arguments:

```bash
-I include -I vendor/include
```

---

## Libraries

```c
const i8 *libs[] = {
    "m",
    "pthread",
    null
};

BObject app = {
    .librs = libs
};
```

Generated linker arguments:

```bash
-lm -lpthread
```

---

## Wildcards

Builder supports Unix glob patterns.

```c
const i8 *files[] = {
    "src/*.c",
    "src/core/*.c",
    "src/utils/*.c",
    null
};
```

---

## Global Configuration

```c
setGlobalCompiler("clang");

setGlobalFlags(
    "-Wall -Wextra -O3"
);

setGlobalIncludes(
    "include vendor/include"
);

setGlobalLibRpaths(
    "libs build"
);

setBuildDir("build");
```

---

## Environment Variables

Runtime options may be extended without modifying source code.

### Extra Compiler Flags

```bash
export BUILDER_EXTRA_FLAGS="-g -fsanitize=address"
```

### Extra Include Directories

```bash
export BUILDER_EXTRA_INCLUDES="thirdparty/include"
```

### Extra Library Paths

```bash
export BUILDER_EXTRA_LIBRSPATH="libs external/lib"
```

---

## Self-Rebuilding Build Scripts

Builder can automatically rebuild itself when the build script changes.

```c
check_and_rebuild_self(
    argc,
    argv,
    __FILE__
);
```

Whenever the build script is modified:

```text
[Self-Rebuild] Code changed.
[Self-Rebuild] Success! Reloading builder...
```

---

## Cleaning

Remove build directory:

```bash
./build clean
```

Remove build directory and build executable:

```bash
./build cleanall
```

---

## Build Output Layout

Example:

```text
project/
├── build/
│   ├── app.elf
│   ├── libmath.a
│   ├── libgraphics.so
│   └── app/
│       ├── src/main.c.o
│       └── src/window.c.o
├── src/
├── include/
├── builder.h
└── build.c
```

---

## Why Builder?

Builder was created for developers who want:

* A Makefile alternative written in C.
* Full programmatic control over the build process.
* Simple dependency management.
* Fast incremental compilation.
* Single-header distribution.
* Zero configuration files.

---

