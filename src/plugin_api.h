#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NotyHostV1 {
    uint32_t abi_version;
    void* context;
    const char* (*document_utf8)(void* context);
    void (*replace_selection_utf8)(void* context,const char* text);
} NotyHostV1;
typedef struct NotyPluginV1 {
    uint32_t abi_version;
    const char* name_utf8;
    void (*execute)(const NotyHostV1* host);
} NotyPluginV1;
typedef const NotyPluginV1* (*NotyPluginEntryV1)(void);
#ifdef __cplusplus
}
#endif
