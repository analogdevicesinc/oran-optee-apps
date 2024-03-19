#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

/*
 * Function pointer definition for optee command handlers.
 */
typedef TEE_Result adi_optee_cmd_handler(uint32_t, TEE_Param *);
