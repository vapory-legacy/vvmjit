#include <vvm.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct examplevm
{
    struct vvm_instance instance;
    int verbose;
};

static void vvm_destroy(struct vvm_instance* vvm)
{
    free(vvm);
}

/// Example options.
///
/// VMs are allowed to omit this function implementation.
int vvm_set_option(struct vvm_instance* instance,
                   char const* name,
                   char const* value)
{
    struct examplvvm* vm = (struct examplvvm*)instance;
    if (strcmp(name, "verbose") == 0) {
        long int v = strtol(value, NULL, 0);
        if (v > INT_MAX || v < INT_MIN)
            return 0;
        vm->verbose = (int)v;
        return 1;
    }

    return 0;
}

static void vvm_release_result(struct vvm_result const* result)
{
    (void)result;
}

static void free_result_output_data(struct vvm_result const* result)
{
    free((uint8_t*)result->output_data);
}

static struct vvm_result execute(struct vvm_instance* instance,
                                 struct vvm_context* context,
                                 enum vvm_revision rev,
                                 const struct vvm_message* msg,
                                 const uint8_t* code,
                                 size_t code_size)
{
    struct vvm_result ret = {};
    if (code_size == 0) {
        // In case of empty code return a fancy error message.
        const char* error = rev == VVM_BYZANTIUM ?
                            "Welcome to Byzantium!" : "Hello Vapory!";
        ret.output_data = (const uint8_t*)error;
        ret.output_size = strlen(error);
        ret.status_code = VVM_FAILURE;
        ret.release = NULL;  // We don't need to release the constant messages.
        return ret;
    }

    struct examplevm* vm = (struct examplevm*)instance;

    // Simulate executing by checking for some code patterns.
    // Solidity inline assembly is used in the examples instead of VVM bytecode.

    // Assembly: `{ mstore(0, address()) return(0, msize()) }`.
    const char return_address[] = "30600052596000f3";

    // Assembly: `{ sstore(0, add(sload(0), 1)) }`
    const char counter[] = "600160005401600055";

    if (code_size == strlen(return_address) &&
        strncmp((const char*)code, return_address, code_size) == 0) {
        static const size_t address_size = sizeof(msg->address);
        uint8_t* output_data = (uint8_t*)malloc(address_size);
        if (!output_data) {
            // malloc failed, report internal error.
            ret.status_code = VVM_INTERNAL_ERROR;
            return ret;
        }
        memcpy(output_data, &msg->address, address_size);
        ret.status_code = VVM_SUCCESS;
        ret.output_data = output_data;
        ret.output_size = address_size;
        ret.release = &free_result_output_data;
        return ret;
    }
    else if (code_size == strlen(counter) &&
        strncmp((const char*)code, counter, code_size) == 0) {
        struct vvm_uint256be value;
        const struct vvm_uint256be index = {{0,}};
        context->fn_table->get_storage(&value, context, &msg->address, &index);
        value.bytes[31] += 1;
        context->fn_table->set_storage(context, &msg->address, &index, &value);
        ret.status_code = VVM_SUCCESS;
        return ret;
    }

    ret.release = vvm_release_result;
    ret.status_code = VVM_FAILURE;
    ret.gas_left = 0;

    if (vm->verbose)
        printf("Execution done.\n");

    return ret;
}

struct vvm_instance* examplevm_create()
{
    struct vvm_instance init = {
        .abi_version = VVM_ABI_VERSION,
        .destroy = vvm_destroy,
        .execute = execute,
        .set_option = vvm_set_option
    };
    struct examplevm* vm = calloc(1, sizeof(struct examplevm));
    struct vvm_instance* interface = &vm->instance;
    memcpy(interface, &init, sizeof(init));
    return interface;
}
