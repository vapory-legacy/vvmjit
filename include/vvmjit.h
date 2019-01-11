#pragma once

#include <vvm.h>

#ifdef _MSC_VER
#ifdef vvmjit_EXPORTS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#else
#define EXPORT __attribute__ ((visibility ("default")))
#endif

#if __cplusplus
extern "C" {
#endif

/// Create VVMJIT instance.
///
/// @return  The VVMJIT instance.
EXPORT struct vvm_instance* vvmjit_create(void);

#if __cplusplus
}
#endif
