## Build the compiler for wasm

For now  it doesn't completely work because it uses popen.
(TODO : add popen as an extern when the compiler is compiled for wasm)

You will need to compile it statically
Run ```CC="clang --target=wasm32-wasi  --sysroot=/usr/share/wasi-sysroot -D__GNU__" CXX="clang++ --target=wasm32-unknown-wasi --sysroot=/usr/share/wasi-sysroot -D__GNU__" NO_MOLD=TRUE STATIC_LLVM=TRUE LLVM_PREFIX=/usr/local/llvm_static/sysroot/usr ADDITIONAL_LDFLAGS=-L/usr/local/llvm_static/dep-sysroot/usr/lib STATIC_LLVM_DEPENDENCIES=TRUE make```
