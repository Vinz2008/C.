
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