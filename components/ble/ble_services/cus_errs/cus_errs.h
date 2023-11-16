#ifndef _CUS_ERRS_H_
#define _CUS_ERRS_H_
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

// 26 38 xx xx - D8 27 - 46 FC - 9A 83 - 2C A1 3A 66 F5 84
#define ERRS_PROFILE_UUID_BASE                             \
    {                                                      \
        0x84, 0xF5, 0x66, 0x3A, 0xA1, 0x2C, 0x83, 0x9A,    \
            0xFC, 0x46, 0x27, 0xD8, 0x00, 0x00, 0x38, 0x26 \
    }
#define ERRS_PROFILE_UUID_SERVICE 0x0001
#define ERRS_PROFILE_UUID_CHAR 0X0002

// Length of Characteristic in bytes
#define ERRS_PROFILE_CHAR_LEN 20

    typedef struct errsProfileService_s ErrsProfile_t;

    typedef void (*errs_profile_evt_handler_t)(void);

    typedef struct
    {
        errs_profile_evt_handler_t errs_evt_handler;
    } ErrsProfileCallback_t;

    struct errsProfileService_s
    {
        uint8_t uuidType;
        uint16_t serviceHandle;
        uint16_t conn_handle;
        ble_gatts_char_handles_t errs_char_handler;
        errs_profile_evt_handler_t errs_evt_handler;
    };

    uint32_t ble_errs_on_data_send(ErrsProfile_t *p_errs, void *p_data, uint16_t length);
    uint32_t errs_profile_init(const ErrsProfileCallback_t *p_app_cb);
    void ble_errs_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);

#ifdef __cplusplus
}
#endif

#endif // !_CUS_ERRS_H_
