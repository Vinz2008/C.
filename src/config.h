#ifndef DEFAULT_PREFIX_PATH
#ifndef _WIN32
#define DEFAULT_PREFIX_PATH "/usr/local"
#else
#define DEFAULT_PREFIX_PATH "C:/cpoint"
#endif
#endif

#define DEFAULT_PREFIX_LIB_PATH DEFAULT_PREFIX_PATH "/lib"
#define DEFAULT_STD_PATH DEFAULT_PREFIX_LIB_PATH "/cpoint/std"
#define DEFAULT_PACKAGE_PATH DEFAULT_PREFIX_LIB_PATH "/cpoint/packages"
#define DEFAULT_GC_PATH DEFAULT_PREFIX_LIB_PATH "/cpoint/bdwgc"
#define DEFAULT_BUILD_INSTALL_PATH DEFAULT_PREFIX_LIB_PATH "/cpoint/build_install"

#define ENABLE_JIT 0
#define ENABLE_FILE_AST 1
#define ENABLE_CIR 0 // only works when ENABLE_FILE_AST is enabled


// comment this if you want to disable it (it is not like the other flags because it is needed to be known in the build system)
#define ENABLE_LLVM_TOOLS_EMBEDDED_COMPILER

#define BUILD_TIMESTAMP __TIMESTAMP__

#ifndef TARGET
// TARGET is defined in CXXFLAGS of Makefile but it is useful to remove errors in ide and so it compiles even with overriden cxxflags (which would break the infos menu)
#define TARGET ""
#endif

#define INTERNAL_FUNC_PREFIX "cpoint_internal_"