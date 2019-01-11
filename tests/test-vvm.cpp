#include <vvm.h>

vvm_result test_call_failure(int64_t gas)
{
    vvm_result result{};
    result.status_code = VVM_FAILURE;
    result.gas_left = gas;
    return result;
}
