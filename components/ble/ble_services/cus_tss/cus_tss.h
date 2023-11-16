#ifndef _CUS_TSS_H_
#define _CUS_TSS_H_
/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

/*********************************************************************
 * DEFINITIONS
 */
#ifdef __cplusplus
extern "C"
{
#endif

// 48 AA xx xx - BC 9C - 41 95 - 9B E6 - BD 81 81 54 20 33
#define TSS_PROFILE_UUID_BASE                              \
    {                                                      \
        0x33, 0x20, 0x54, 0x81, 0x81, 0xBD, 0xE6, 0x9B,    \
            0x95, 0x41, 0x9C, 0xBC, 0x00, 0x00, 0xAA, 0x48 \
    }
#define TSS_PROFILE_UUID_SERVICE 0x0001
#define TSS_PROFILE_UUID_CHAR 0X0002

// Length of Characteristic in bytes
#define TSS_PROFILE_CHAR_LEN 20

    typedef struct tssProfileService_s TssProfile_t;

    typedef void (*tss_profile_evt_handler_t)(uint32_t timestamp);

    typedef struct
    {
        tss_profile_evt_handler_t tss_evt_handler;
    } TssProfileCallback_t;

    struct tssProfileService_s
    {
        uint8_t uuidType;
        uint16_t serviceHandle;
        uint16_t conn_handle;
        ble_gatts_char_handles_t tss_char_handler;
        tss_profile_evt_handler_t tss_evt_handler;
    };

    uint32_t tss_profile_init(const TssProfileCallback_t *p_app_cb);
    void ble_tss_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);

#ifdef __cplusplus
}
#endif

#endif // !_CUS_TSS_H_
