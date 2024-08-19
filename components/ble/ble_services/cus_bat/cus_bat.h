#ifndef _CUS_BAT_H_
#define _CUS_BAT_H_
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

// 90 5F xx xx - 26 44 - 44 19 - 91 05 - BA D6 83 46 DC D4
#define BAT_PROFILE_UUID_BASE                             \
    {                                                      \
        0xD4, 0xDC, 0x46, 0x83, 0xD6, 0xBA, 0x05, 0x91,    \
            0x19, 0x44, 0x44, 0x26, 0x00, 0x00, 0x5F, 0x90 \
    }
#define BAT_PROFILE_UUID_SERVICE 0x0001
#define BAT_PROFILE_UUID_CHAR 0X0002

// Length of Characteristic in bytes
#define BAT_PROFILE_CHAR_LEN 2

#define BAT_NORMAL 0
#define BAT_BATTERY_LOW 1
#define BAT_LIBRE_TIMEOUT 2
#define BAT_LOW_BLOOD_GLUCOSE 3

    typedef struct batProfileService_s BatProfile_t;

    typedef void (*bat_profile_evt_handler_t)(void);

    typedef struct
    {
        bat_profile_evt_handler_t bat_evt_handler;
    } BatProfileCallback_t;

    struct batProfileService_s
    {
        uint8_t uuidType;
        uint16_t serviceHandle;
        uint16_t conn_handle;
        ble_gatts_char_handles_t bat_char_handler;
        bat_profile_evt_handler_t bat_evt_handler;
    };

    uint32_t ble_bat_on_data_send(BatProfile_t *p_errs, void *p_data, uint16_t length);
    uint32_t bat_profile_init(const BatProfileCallback_t *p_app_cb);
    void ble_bat_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);
    uint8_t bat_adv_notify_send(uint16_t *bat_number);

#ifdef __cplusplus
}
#endif

#endif // !_CUS_ERRS_H_
