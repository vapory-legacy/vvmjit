
#include <stdint.h>

#ifdef _MSC_VER
#ifdef vvmjit_EXPORTS
	#define VVMJIT_API __declspec(dllexport)
#else
	#define VVMJIT_API __declspec(dllimport)
#endif

#define _ALLOW_KEYWORD_MACROS
#define noexcept throw()
#else
#define VVMJIT_API [[gnu::visibility("default")]]
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vvmjit_i256
{
	uint64_t words[4];
} vvmjit_i256;

typedef struct vvmjit_runtime_data
{
	int64_t 	gas;
	int64_t 	gasPrice;
	char const* callData;
	uint64_t 	callDataSize;
	vvmjit_i256 address;
	vvmjit_i256 caller;
	vvmjit_i256 origin;
	vvmjit_i256 transferredValue;
	vvmjit_i256 apparentValue;
	vvmjit_i256 coinBase;
	vvmjit_i256 difficulty;
	vvmjit_i256 gasLimit;
	uint64_t 	number;
	int64_t 	timestamp;
	char const* code;
	uint64_t 	codeSize;
	vvmjit_i256	codeHash;
} vvmjit_runtime_data;

typedef enum vvmjit_return_code
{
	// Success codes
	Stop    = 0,
	Return  = 1,
	Suicide = 2,

	// Standard error codes
	OutOfGas           = -1,

	// Internal error codes
	LLVMError           = -101,
	UnexpectedException = -111
} vvmjit_return_code;

typedef struct vvmjit_context vvmjit_context;

VVMJIT_API vvmjit_context* vvmjit_create(vvmjit_runtime_data* _data, void* _env);

VVMJIT_API vvmjit_return_code vvmjit_exec(vvmjit_context* _context, void* _schedule);

VVMJIT_API void vvmjit_destroy(vvmjit_context* _context);


inline char const* vvmjit_get_output(vvmjit_runtime_data* _data) { return _data->callData; }
inline uint64_t vvmjit_get_output_size(vvmjit_runtime_data* _data) { return _data->callDataSize; }

#ifdef __cplusplus
}
#endif
