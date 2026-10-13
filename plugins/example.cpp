#include "plugin_api.h"
#ifdef _WIN32
#define NOTY_EXPORT __declspec(dllexport)
#else
#define NOTY_EXPORT __attribute__((visibility("default")))
#endif
static void Execute(const NotyHostV1* host){if(host&&host->abi_version==1)host->replace_selection_utf8(host->context,"Hello from a native Noty plugin!");}
extern "C" NOTY_EXPORT const NotyPluginV1* noty_plugin_v1(){static const NotyPluginV1 plugin={1,"Example: insert greeting",Execute};return &plugin;}
