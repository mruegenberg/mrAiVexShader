#ifdef _WIN32
#  ifdef MODULE_API_EXPORTS
#    define MODULE_API __declspec(dllexport)
#  else
#    define MODULE_API __declspec(dllimport)
#  endif
#else
#  define MODULE_API
#endif

// prepend public functions with MODULE_API and do
//
// #define MODULE_API_EXPORTS
// #include "vexvol_util.h"
//
// in cpp files
// see http://stackoverflow.com/questions/19418908/creating-a-portable-library-to-run-on-both-linux-and-windows
