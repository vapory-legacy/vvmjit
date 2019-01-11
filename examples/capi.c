#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vvm.h"


struct vvm_uint256be balance(struct vvm_context* context,
                             const struct vvm_address* address)
{
    struct vvm_uint256be ret = {.bytes = {1, 2, 3, 4}};
    return ret;
}

struct vvm_address address(struct vvm_context* context)
{
    struct vvm_address ret = {.bytes = {1, 2, 3, 4}};
    return ret;
}

static void print_address(const struct vvm_address* address)
{
    int i = 0;
    for (i = 0; i < sizeof(address->bytes); ++i)
        printf("%x", address->bytes[i] & 0xff);
}

static int account_exists(struct vvm_context* context,
                           const struct vvm_address* address) {
    printf("VVM-C: EXISTS @");
    print_address(address);
    printf("\n");
    return 0;
}

static void get_storage(struct vvm_uint256be* result,
                        struct vvm_context* context,
                        const struct vvm_address* address,
                        const struct vvm_uint256be* key)
{
    printf("VVM-C: SLOAD @");
    print_address(address);
    printf("\n");
}

static void set_storage(struct vvm_context* context,
                        const struct vvm_address* address,
                        const struct vvm_uint256be* key,
                        const struct vvm_uint256be* value)
{
    printf("VVM-C: SSTORE @");
    print_address(address);
    printf("\n");
}

static void get_balance(struct vvm_uint256be* result,
                        struct vvm_context* context,
                        const struct vvm_address* address)
{
    printf("VVM-C: BALANCE @");
    print_address(address);
    printf("\n");
    *result = balance(context, address);
}

static size_t get_code(const uint8_t** code,
                       struct vvm_context* context,
                       const struct vvm_address* address)
{
    printf("VVM-C: CODE @");
    print_address(address);
    printf("\n");
    return 0;
}

static void selfdestruct(struct vvm_context* context,
                         const struct vvm_address* address,
                         const struct vvm_address* beneficiary)
{
    printf("VVM-C: SELFDESTRUCT ");
    print_address(address);
    printf(" -> ");
    print_address(beneficiary);
    printf("\n");
}

static void call(struct vvm_result* result,
                 struct vvm_context* context,
                 const struct vvm_message* msg)
{
    printf("VVM-C: CALL (depth: %d)\n", msg->depth);
    result->status_code = VVM_FAILURE;
}

static void get_tx_context(struct vvm_tx_context* result, struct vvm_context* context)
{

}

static void get_block_hash(struct vvm_uint256be* result, struct vvm_context* context,
                           int64_t number)
{

}

/// VVM log callback.
///
/// @note The `vvm_log` name is used to avoid conflict with `log()` C function.
static void vvm_log(struct vvm_context* context, const struct vvm_address* address,
                    const uint8_t* data, size_t data_size,
                    const struct vvm_uint256be topics[], size_t topics_count)
{
    printf("VVM-C: LOG%d\n", (int)topics_count);
}

static const struct vvm_context_fn_table ctx_fn_table = {
    account_exists,
    get_storage,
    set_storage,
    get_balance,
    get_code,
    selfdestruct,
    call,
    get_tx_context,
    get_block_hash,
    vvm_log
};

/// Example how the API is supposed to be used.
int main(int argc, char *argv[]) {
    struct vvm_instance* jit = examplvvm_create();
    if (jit->abi_version != VVM_ABI_VERSION)
        return 1;  // Incompatible ABI version.

    uint8_t const code[] = "Place some VVM bytecode here";
    const size_t code_size = sizeof(code);
    struct vvm_uint256be code_hash = {.bytes = {1, 2, 3,}};
    uint8_t const input[] = "Hello World!";
    struct vvm_uint256be value = {{1, 0,}};
    struct vvm_address addr = {{0, 1, 2,}};
    int64_t gas = 200000;

    struct vvm_context ctx = {&ctx_fn_table};

    struct vvm_message msg = {addr, addr, value, input, sizeof(input),
                              code_hash, gas, 0};

    struct vvm_result result =
        jit->execute(jit, &ctx, VVM_HOMESTEAD, &msg, code, code_size);

    printf("Execution result:\n");
    if (result.status_code != VVM_SUCCESS) {
      printf("  VVM execution failure: %d\n", result.status_code);
    } else {
        printf("  Gas used: %" PRId64 "\n", gas - result.gas_left);
        printf("  Gas left: %" PRId64 "\n", result.gas_left);
        printf("  Output size: %zd\n", result.output_size);

        printf("  Output: ");
        size_t i = 0;
        for (i = 0; i < result.output_size; i++) {
            printf("%02x ", result.output_data[i]);
        }
        printf("\n");
    }

    if (result.release)
        result.release(&result);
    jit->destroy(jit);

    return 0;
}
