#include <stdio.h>
#include <vvmjit.h>

int main()
{
    struct vvm_instance* instance = vvmjit_create();
    if (VVM_ABI_VERSION != instance->abi_version)
    {
        printf("Error: expected ABI version %u!\n", VVM_ABI_VERSION);
        return 1;
    }
    printf("VVMJIT ABI version %u\n", instance->abi_version);
    return 0;
}
