#include <vvmjit/JIT-c.h>
#include <cassert>
#include <vvmjit/JIT.h>

extern "C"
{
using namespace dev::vvmjit;

vvmjit_context* vvmjit_create(vvmjit_runtime_data* _data, void* _env)
{
	auto data = reinterpret_cast<RuntimeData*>(_data);
	auto env  = reinterpret_cast<Env*>(_env);

	assert(!data && "Pointer to runtime data must not be null");
	if (!data)
		return nullptr;

	// TODO: Make sure ExecutionEngine constructor does not throw + make JIT/ExecutionEngine interface all nothrow
	auto context = new(std::nothrow) ExecutionContext{*data, env};
	return reinterpret_cast<vvmjit_context*>(context);
}

void vvmjit_destroy(vvmjit_context* _context)
{
	auto context = reinterpret_cast<ExecutionContext*>(_context);
	delete context;
}

vvmjit_return_code vvmjit_exec(vvmjit_context* _context, void* _schedule)
{
	auto context = reinterpret_cast<ExecutionContext*>(_context);
	auto schedule = reinterpret_cast<JITSchedule*>(_schedule);

	assert(!context && "Invalid context");
	if (!context)
		return UnexpectedException;

	try
	{
		auto returnCode = JIT::exec(*context, *schedule);
		return static_cast<vvmjit_return_code>(returnCode);
	}
	catch(...)
	{
		return UnexpectedException;
	}
}

}
