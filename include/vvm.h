/// VVM-C -- C interface to Vapory Virtual Machine
///
/// ## High level design rules
///
/// 1. Pass function arguments and results by value.
///    This rule comes from modern C++ and tries to avoid costly alias analysis
///    needed for optimization. As the result we have a lots of complex structs
///    and unions. And variable sized arrays of bytes cannot be passed by copy.
/// 2. The VVM operates on integers so it prefers values to be host-endian.
///    On the other hand, LLVM can generate good code for byte swaping.
///    The interface also tries to match host application "natural" endianess.
///    I would like to know what endianess you use and where.
///
/// ## Terms
///
/// 1. VVM  -- an Vapory Virtual Machine instance/implementation.
/// 2. Host -- an entity controlling the VVM. The Host requests code execution
///            and responses to VVM queries by callback functions.
///
/// @defgroup VVMC VVM-C
/// @{
#ifndef VVM_H
#define VVM_H

#include <stdint.h>    // Definition of int64_t, uint64_t.
#include <stddef.h>    // Definition of size_t.

#if __cplusplus
extern "C" {
#endif

// BEGIN Python CFFI declarations

enum {
    /// The VVM-C ABI version number of the interface declared in this file.
    VVM_ABI_VERSION = 0
};

/// Big-endian 256-bit integer.
///
/// 32 bytes of data representing big-endian 256-bit integer. I.e. bytes[0] is
/// the most significant byte, bytes[31] is the least significant byte.
/// This type is used to transfer to/from the VM values interpreted by the user
/// as both 256-bit integers and 256-bit hashes.
struct vvm_uint256be {
    /// The 32 bytes of the big-endian integer or hash.
    uint8_t bytes[32];
};

/// Big-endian 160-bit hash suitable for keeping an Vapory address.
struct vvm_address {
    /// The 20 bytes of the hash.
    uint8_t bytes[20];
};

/// The kind of call-like instruction.
enum vvm_call_kind {
    VVM_CALL = 0,         ///< Request CALL.
    VVM_DELEGATECALL = 1, ///< Request DELEGATECALL. The value param ignored.
    VVM_CALLCODE = 2,     ///< Request CALLCODE.
    VVM_CREATE = 3,       ///< Request CREATE. Semantic of some params changes.
};

/// The flags for ::vvm_message.
enum vvm_flags {
    VVM_STATIC = 1        ///< Static call mode.
};

/// The message describing an VVM call,
/// including a zero-depth calls from a transaction origin.
struct vvm_message {
    struct vvm_address address;  ///< The destination of the message.
    struct vvm_address sender;   ///< The sender of the message.

    /// The amount of Vapor transferred with the message.
    struct vvm_uint256be value;

    const uint8_t* input;        ///< The message input data.
    size_t input_size;           ///< The size of the message input data.

    /// The optional hash of the code of the destination account.
    /// The null hash MUST be used when not specified.
    struct vvm_uint256be code_hash;

    int64_t gas;                 ///< The amount of gas for message execution.
    int32_t depth;               ///< The call depth.

    /// The kind of the call. For zero-depth calls ::VVM_CALL SHOULD be used.
    enum vvm_call_kind kind;

    ///< Additional flags modifying the call execution behavior.
    ///< In the current version the only valid values are ::VVM_STATIC or 0.
    uint32_t flags;
};


/// The transaction and block data for execution.
struct vvm_tx_context {
    struct vvm_uint256be tx_gas_price;      ///< The transaction gas price.
    struct vvm_address tx_origin;           ///< The transaction origin account.
    struct vvm_address block_coinbase;      ///< The miner of the block.
    int64_t block_number;                   ///< The block number.
    int64_t block_timestamp;                ///< The block timestamp.
    int64_t block_gas_limit;                ///< The block gas limit.
    struct vvm_uint256be block_difficulty;  ///< The block difficulty.
};

struct vvm_context;

/// Get transaction context callback function.
///
/// This callback function is used by an VVM to retrieve the transaction and
/// block context.
///
/// @param[out] result   The returned transaction context.
///                      @see ::vvm_tx_context.
/// @param      context  The pointer to the Host execution context.
///                      @see ::vvm_context.
typedef void (*vvm_get_tx_context_fn)(struct vvm_tx_context* result,
                                      struct vvm_context* context);

/// Get block hash callback function..
///
/// This callback function is used by an VVM to query the block hash of
/// a given block.
///
/// @param[out] result   The returned block hash value.
/// @param      context  The pointer to the Host execution context.
/// @param      number   The block number. Must be a value between
//                       (and including) 0 and 255.
typedef void (*vvm_get_block_hash_fn)(struct vvm_uint256be* result,
                                      struct vvm_context* context,
                                      int64_t number);

/// The execution status code.
enum vvm_status_code {
    VVM_SUCCESS = 0,               ///< Execution finished with success.
    VVM_FAILURE = 1,               ///< Generic execution failure.
    VVM_OUT_OF_GAS = 2,
    VVM_BAD_INSTRUCTION = 3,
    VVM_BAD_JUMP_DESTINATION = 4,
    VVM_STACK_OVERFLOW = 5,
    VVM_STACK_UNDERFLOW = 6,
    VVM_REVERT = 7,                ///< Execution terminated with REVERT opcode.

    /// The given type of executable code is not supported by the VVM.
    ///
    /// This error SHOULD be used to signal that the VVM is not able to execute
    /// given code type. If an VVM implementing _higher_ revision of the VVM
    /// specification returns the ::VVM_UNSUPPORTED_CODE_TYPE status code,
    /// the Client MAY try to fallback to other VVM implementing a _lower_
    /// revision of the VVM specification.
    /// For example, the Client tries running a code in the VVM 1.5. If the
    /// code is not supported there, the execution fallbacks to the VVM 1.0.
    VVM_UNSUPPORTED_CODE_TYPE = -1,

    /// VVM implementation internal error.
    ///
    /// @todo We should rethink reporting internal errors. One of the options
    ///       it to allow using any negative value to represent internal errors.
    VVM_INTERNAL_ERROR = -2,
};

struct vvm_result;  ///< Forward declaration.

/// Releases resources assigned to an execution result.
///
/// This function releases memory (and other resources, if any) assigned to the
/// specified execution result making the result object invalid.
///
/// @param result  The execution result which resource are to be released. The
///                result itself it not modified by this function, but becomes
///                invalid and user should discard it as well.
typedef void (*vvm_release_result_fn)(const struct vvm_result* result);

/// The VVM code execution result.
struct vvm_result {
    /// The execution status code.
    enum vvm_status_code status_code;

    /// The amount of gas left after the execution.
    ///
    /// If vvm_result::code is not ::VVM_SUCCESS nor ::VVM_REVERT
    /// the value MUST be 0.
    int64_t gas_left;

    /// The reference to output data.
    ///
    /// The output contains data coming from RETURN opcode (iff vvm_result::code
    /// field is ::VVM_SUCCESS) or from REVERT opcode.
    ///
    /// The memory containing the output data is owned by VVM and has to be
    /// freed with vvm_result::release().
    ///
    /// @todo Inconsistent name: output_data vs msg.input.
    uint8_t const* output_data;

    /// The size of the output data.
    size_t output_size;

    /// The pointer to a function releasing all resources associated with
    /// the result object.
    ///
    /// This function pointer is optional (MAY be NULL) and MAY be set by
    /// the VVM implementation. If set it MUST be used by the user to
    /// release memory and other resources associated with the result object.
    /// After the result resources are released the result object
    /// MUST NOT be used any more.
    ///
    /// The suggested code pattern for releasing VVM results:
    /// @code
    /// struct vvm_result result = ...;
    /// if (result.release)
    ///     result.release(&result);
    /// @endcode
    ///
    /// @note
    /// It works similarly to C++ virtual destructor. Attaching the release
    /// function to the result itself allows VVM composition.
    vvm_release_result_fn release;

    /// The address of the contract created by CREATE opcode.
    ///
    /// This field has valid value only if the result describes successful
    /// CREATE (vvm_result::status_code is ::VVM_SUCCESS).
    struct vvm_address create_address;

    /// Reserved data that MAY be used by a vvm_result object creator.
    ///
    /// This reserved 4 bytes together with 20 bytes from create_address form
    /// 24 bytes of memory called "optional data" within vvm_result struct
    /// to be optionally used by the vvm_result object creator.
    ///
    /// @see vvm_result_optional_data, vvm_get_optional_data().
    ///
    /// Also extends the size of the vvm_result to 64 bytes (full cache line).
    uint8_t padding[4];
};


/// The union representing vvm_result "optional data".
///
/// The vvm_result struct contains 24 bytes of optional data that can be
/// reused by the obejct creator if the object does not contain
/// vvm_result::create_address.
///
/// An VVM implementation MAY use this memory to keep additional data
/// when returning result from ::vvm_execute_fn.
/// The host application MAY use this memory to keep additional data
/// when returning result of performed calls from ::vvm_call_fn.
///
/// @see vvm_get_optional_data(), vvm_get_const_optional_data().
union vvm_result_optional_data
{
    uint8_t bytes[24];
    void* pointer;
};

/// Provides read-write access to vvm_result "optional data".
static inline union vvm_result_optional_data* vvm_get_optional_data(
    struct vvm_result* result)
{
    return (union vvm_result_optional_data*) &result->create_address;
}

/// Provides read-only access to vvm_result "optional data".
static inline const union vvm_result_optional_data* vvm_get_const_optional_data(
    const struct vvm_result* result)
{
    return (const union vvm_result_optional_data*) &result->create_address;
}


/// Check account existence callback function
///
/// This callback function is used by the VVM to check if
/// there exists an account at given address.
/// @param      context  The pointer to the Host execution context.
///                      @see ::vvm_context.
/// @param      address  The address of the account the query is about.
/// @return              1 if exists, 0 otherwise.
typedef int (*vvm_account_exists_fn)(struct vvm_context* context,
                                     const struct vvm_address* address);

/// Get storage callback function.
///
/// This callback function is used by an VVM to query the given contract
/// storage entry.
/// @param[out] result   The returned storage value.
/// @param      context  The pointer to the Host execution context.
///                      @see ::vvm_context.
/// @param      address  The address of the contract.
/// @param      key      The index of the storage entry.
typedef void (*vvm_get_storage_fn)(struct vvm_uint256be* result,
                                   struct vvm_context* context,
                                   const struct vvm_address* address,
                                   const struct vvm_uint256be* key);

/// Set storage callback function.
///
/// This callback function is used by an VVM to update the given contract
/// storage entry.
/// @param context  The pointer to the Host execution context.
///                 @see ::vvm_context.
/// @param address  The address of the contract.
/// @param key      The index of the storage entry.
/// @param value    The value to be stored.
typedef void (*vvm_set_storage_fn)(struct vvm_context* context,
                                   const struct vvm_address* address,
                                   const struct vvm_uint256be* key,
                                   const struct vvm_uint256be* value);

/// Get balance callback function.
///
/// This callback function is used by an VVM to query the balance of the given
/// address.
/// @param[out] result   The returned balance value.
/// @param      context  The pointer to the Host execution context.
///                      @see ::vvm_context.
/// @param      address  The address.
typedef void (*vvm_get_balance_fn)(struct vvm_uint256be* result,
                                   struct vvm_context* context,
                                   const struct vvm_address* address);

/// Get code callback function.
///
/// This callback function is used by an VVM to get the code of a contract of
/// given address.
///
/// @param[out] result_code  The pointer to the contract code. This argument is
///                          optional. If NULL is provided, the host MUST only
///                          return the code size. It will be freed by the Client.
/// @param      context      The pointer to the Host execution context.
///                          @see ::vvm_context.
/// @param      address      The address of the contract.
/// @return                  The size of the code.
typedef size_t (*vvm_get_code_fn)(const uint8_t** result_code,
                                  struct vvm_context* context,
                                  const struct vvm_address* address);

/// Selfdestruct callback function.
///
/// This callback function is used by an VVM to SELFDESTRUCT given contract.
/// The execution of the contract will not be stopped, that is up to the VVM.
///
/// @param context      The pointer to the Host execution context.
///                     @see ::vvm_context.
/// @param address      The address of the contract to be selfdestructed.
/// @param beneficiary  The address where the remaining VAP is going to be
///                     transferred.
typedef void (*vvm_selfdestruct_fn)(struct vvm_context* context,
                                    const struct vvm_address* address,
                                    const struct vvm_address* beneficiary);

/// Log callback function.
///
/// This callback function is used by an VVM to inform about a LOG that happened
/// during an VVM bytecode execution.
/// @param context       The pointer to the Host execution context.
///                      @see ::vvm_context.
/// @param address       The address of the contract that generated the log.
/// @param data          The pointer to unindexed data attached to the log.
/// @param data_size     The length of the data.
/// @param topics        The pointer to the array of topics attached to the log.
/// @param topics_count  The number of the topics. Valid values are between
///                      0 and 4 inclusively.
typedef void (*vvm_log_fn)(struct vvm_context* context,
                           const struct vvm_address* address,
                           const uint8_t* data,
                           size_t data_size,
                           const struct vvm_uint256be topics[],
                           size_t topics_count);

/// Pointer to the callback function supporting VVM calls.
///
/// @param[out] result  The result of the call. The result object is not
///                     initialized by the VVM, the Client MUST correctly
///                     initialize all expected fields of the structure.
/// @param      context The pointer to the Host execution context.
///                     @see ::vvm_context.
/// @param      msg     Call parameters.
typedef void (*vvm_call_fn)(struct vvm_result* result,
                            struct vvm_context* context,
                            const struct vvm_message* msg);

/// The context interface.
///
/// The set of all callback functions expected by VVM instances. This is C
/// realisation of vtable for OOP interface (only virtual methods, no data).
/// Host implementations SHOULD create constant singletons of this (similarly
/// to vtables) to lower the maintenance and memory management cost.
struct vvm_context_fn_table {
    vvm_account_exists_fn account_exists;
    vvm_get_storage_fn get_storage;
    vvm_set_storage_fn set_storage;
    vvm_get_balance_fn get_balance;
    vvm_get_code_fn get_code;
    vvm_selfdestruct_fn selfdestruct;
    vvm_call_fn call;
    vvm_get_tx_context_fn get_tx_context;
    vvm_get_block_hash_fn get_block_hash;
    vvm_log_fn log;
};


/// Execution context managed by the Host.
///
/// The Host MUST pass the pointer to the execution context to
/// ::vvm_execute_fn. The VVM MUST pass the same pointer back to the Host in
/// every callback function.
/// The context MUST contain at least the function table defining the context
/// callback interface.
/// Optionally, The Host MAY include in the context additional data.
struct vvm_context {

    /// Function table defining the context interface (vtable).
    const struct vvm_context_fn_table* fn_table;
};


struct vvm_instance;  ///< Forward declaration.

/// Destroys the VVM instance.
///
/// @param vvm  The VVM instance to be destroyed.
typedef void (*vvm_destroy_fn)(struct vvm_instance* vvm);


/// Configures the VVM instance.
///
/// Allows modifying options of the VVM instance.
/// Options:
/// - code cache behavior: on, off, read-only, ...
/// - optimizations,
///
/// @param vvm    The VVM instance to be configured.
/// @param name   The option name. NULL-terminated string. Cannot be NULL.
/// @param value  The new option value. NULL-terminated string. Cannot be NULL.
/// @return       1 if the option set successfully, 0 otherwise.
typedef int (*vvm_set_option_fn)(struct vvm_instance* vvm,
                                 char const* name,
                                 char const* value);


/// VVM revision.
///
/// The revision of the VVM specification based on the Vapory
/// upgrade / hard fork codenames.
enum vvm_revision {
    VVM_FRONTIER = 0,
    VVM_HOMESTEAD = 1,
    VVM_TANGERINE_WHISTLE = 2,
    VVM_SPURIOUS_DRAGON = 3,
    VVM_BYZANTIUM = 4,
    VVM_CONSTANTINOPLE = 5,
};


/// Generates and executes machine code for given VVM bytecode.
///
/// All the fun is here. This function actually does something useful.
///
/// @param instance    A VVM instance.
/// @param context     The pointer to the Host execution context to be passed
///                    to callback functions. @see ::vvm_context.
/// @param rev         Requested VVM specification revision.
/// @param code_hash   A hash of the bytecode, usually Keccak. The VVM uses it
///                    as the code identifier. A VVM implementation is able to
///                    hash the code itself if it requires it, but the host
///                    application usually has the hash already.
/// @param code        Reference to the bytecode to be executed.
/// @param code_size   The length of the bytecode.
/// @param gas         Gas for execution. Min 0, max 2^63-1.
/// @param input       Reference to the input data.
/// @param input_size  The size of the input data.
/// @param value       Call value.
/// @return            All execution results.
typedef struct vvm_result (*vvm_execute_fn)(struct vvm_instance* instance,
                                            struct vvm_context* context,
                                            enum vvm_revision rev,
                                            const struct vvm_message* msg,
                                            uint8_t const* code,
                                            size_t code_size);


/// Status of a code in VM. Useful for JIT-like implementations.
enum vvm_code_status {
    /// The code is uknown to the VM.
    VVM_UNKNOWN,

    /// The code has been compiled and is available in memory.
    VVM_READY,

    /// The compiled version of the code is available in on-disk cache.
    VVM_CACHED,
};


/// Get information the status of the code in the VM.
typedef enum vvm_code_status
(*vvm_get_code_status_fn)(struct vvm_instance* instance,
                          enum vvm_revision rev,
                          uint32_t flags,
                          struct vvm_uint256be code_hash);

/// Request preparation of the code for faster execution. It is not required
/// to execute the code but allows compilation of the code ahead of time in
/// JIT-like VMs.
typedef void (*vvm_prepare_code_fn)(struct vvm_instance* instance,
                                    enum vvm_revision rev,
                                    uint32_t flags,
                                    struct vvm_uint256be code_hash,
                                    uint8_t const* code,
                                    size_t code_size);

/// The VVM instance.
///
/// Defines the base struct of the VVM implementation.
struct vvm_instance {

    /// VVM-C ABI version implemented by the VVM instance.
    ///
    /// For future use to detect ABI incompatibilities. The VVM-C ABI version
    /// represented by this file is in ::VVM_ABI_VERSION.
    ///
    /// @todo Consider removing this field.
    const int abi_version;

    /// Pointer to function destroying the VVM instance.
    vvm_destroy_fn destroy;

    /// Pointer to function executing a code by the VVM instance.
    vvm_execute_fn execute;

    /// Optional pointer to function returning a status of a code.
    ///
    /// If the VM does not support this feature the pointer can be NULL.
    vvm_get_code_status_fn get_code_status;

    /// Optional pointer to function compiling  a code.
    ///
    /// If the VM does not support this feature the pointer can be NULL.
    vvm_prepare_code_fn prepare_code;

    /// Optional pointer to function modifying VM's options.
    ///
    /// If the VM does not support this feature the pointer can be NULL.
    vvm_set_option_fn set_option;
};

// END Python CFFI declarations

/// Example of a function creating an instance of an example VVM implementation.
///
/// Each VVM implementation MUST provide a function returning an VVM instance.
/// The function SHOULD be named `<vm-name>_create(void)`.
///
/// @return  VVM instance or NULL indicating instance creation failure.
struct vvm_instance* examplevm_create(void);


#if __cplusplus
}
#endif

#endif  // VVM_H
/// @}
