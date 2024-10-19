## Build Statically the compiler

Run ```STATIC_LLVM=TRUE make```

If the installed llvm on your system doesn't includes static libraries,  build LLVM manually and specify its path like this : ```STATIC_LLVM=TRUE LLVM_PREFIX=/usr/local/llvm_static/sysroot/usr make```

You can use the ```ADDITIONAL_LDFLAGS``` var for the Makefile to specify a sysroot to search dependencies of LLVM : ```NO_MOLD=TRUE STATIC_LLVM=TRUE LLVM_PREFIX=/usr/local/llvm_static/sysroot/usr ADDITIONAL_LDFLAGS=-L/usr/local/llvm_static/dep-sysroot/usr/lib STATIC_LLVM_DEPENDENCIES=TRUE make```