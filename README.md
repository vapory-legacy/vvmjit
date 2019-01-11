# The Vapory VVM JIT

[![Join the chat at https://gitter.im/vapory/vvmjit](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/vapory/vvmjit?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

VVM JIT is a library for just-in-time compilation of Vapory VVM code.
It can be used to substitute classic interpreter-like VVM Virtual Machine in Vapory client.

## Build

The VVMJIT project uses **CMake** tool to configure the build and depends only on the LLVM library.
LLVM installation is not needed, because CMake will download and build LLVM from source.
However, LLVM requires **Python** interpreter to be built.

```sh
mkdir build
cd build
cmake ..
cmake --build . --config RelWithDebInfo
```

## Options

Options to vvmjit library can be passed by environmental variable, e.g. `VVMJIT="-help" testvap --jit`.
