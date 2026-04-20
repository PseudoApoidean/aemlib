#ifndef AEMLIB_STATUS_H
#define AEMLIB_STATUS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * Status code layout:
 *
 *   0xLLLLCCCC
 *
 *   LLLL = layer (aemlib_status_layer_t)
 *   CCCC = code  (aemlib_status_code_t)
 */

/* ------------------------------
    * High-level layer classification
    * ------------------------------ */
typedef enum aemlib_status_layer
{
    AEMLIB_LAYER_GENERAL   = 0x0001,  /* common / shared */
    AEMLIB_LAYER_SYSTEM    = 0x0002,  /* OS, platform, time, memory */
    AEMLIB_LAYER_IO        = 0x0003,  /* transport, storage, files */
    AEMLIB_LAYER_PROTOCOL  = 0x0004,  /* parsing, framing, validation */
    AEMLIB_LAYER_USER      = 0x00FF   /* reserved for user extensions */
} aemlib_status_layer_t;

/* ------------------------------
    * Low-level error codes
    * ------------------------------ */
typedef enum aemlib_status_code
{
    /* Success */
    AEMLIB_CODE_OK                  = 0,

    /* Errors */
    AEMLIB_CODE_UNKNOWN             = 1,
    AEMLIB_CODE_INVALID_ARG         = 2,
    AEMLIB_CODE_NOT_INITIALIZED     = 3,
    AEMLIB_CODE_OUT_OF_MEMORY       = 4,
    AEMLIB_CODE_BUFFER_TOO_SMALL    = 5,
    AEMLIB_CODE_NOT_SUPPORTED       = 6,
    AEMLIB_CODE_STATE_INVALID       = 7,
    AEMLIB_CODE_WOULD_BLOCK         = 8,
    AEMLIB_CODE_INTERNAL            = 9,
    AEMLIB_CODE_CLOSED              = 10, 
    AEMLIB_CODE_IO                  = 11,  /* generic read/write failure */
    AEMLIB_CODE_TIMEOUT             = 12,
    AEMLIB_CODE_CONNECT             = 13,  /* connection failure */
    AEMLIB_CODE_PROTOCOL            = 14,  /* protocol violation at I/O boundary */
    AEMLIB_CODE_MALFORMED           = 15,  
    AEMLIB_CODE_UNSUPPORTED         = 16,  
    AEMLIB_CODE_UNEXPECTED          = 17,  
    AEMLIB_CODE_UNAVAILABLE         = 18,  

} aemlib_status_code_t;

/* ------------------------------
    * Combined status type
    * ------------------------------ */
typedef uint32_t aemlib_status_t;

/* Construct a status */
#define AEMLIB_STATUS(layer, code) \
    ((((uint32_t)(layer) & 0xFFFF) << 16) | ((uint32_t)(code) & 0xFFFF))

/* Extract layer */
#define AEMLIB_STATUS_LAYER(status) \
    ((aemlib_status_layer_t)(((status) >> 16) & 0xFFFF))

/* Extract code */
#define AEMLIB_STATUS_CODE(status) \
    ((aemlib_status_code_t)((status) & 0xFFFF))

/* Convenience success constant */
#define AEMLIB_STATUS_OK AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_OK)

/* Helper for returning the current status if it is not OK. */
#define AEMLIB_CHECK_STATUS(status)             \
    do {                                        \
        if ((status) != AEMLIB_STATUS_OK) {     \
            return (status);                    \
        }                                       \
    } while (0)

/**
 * Convert a status to a human-readable string.
 * Todo: Implemented in status.c. (Stubbed for now.)
 */
const char *aemlib_status_str(aemlib_status_t status);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_STATUS_H */
