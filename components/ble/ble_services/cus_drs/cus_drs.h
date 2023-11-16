#ifndef _CUS_DRS_H_
#define _CUS_DRS_H_
/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

/*********************************************************************
 * DEFINITIONS
 */
#ifdef __cplusplus
extern "C"
{
#endif

// DF 07 xx xx -D9 4D - 44 F8 - 86 ED - B1 29 D2 8E 1D DC
#define DRS_PROFILE_UUID_BASE                                  \
    {                                                          \
        {                                                      \
            0xDC, 0x1D, 0x8E, 0xD2, 0x29, 0xB1, 0xED, 0x86,    \
                0xF8, 0x44, 0x4D, 0xD9, 0x00, 0x00, 0x07, 0xDF \
        }                                                      \
    }
#define DRS_PROFILE_UUID_SERVICE 0x0001
#define DRS_PROFILE_UUID_CHAR 0X0002

// Length of Characteristic in bytes
#define DRS_PROFILE_CHAR_LEN 20

    typedef struct drsProfileService_s DrsProfile_t;

    typedef void (*drs_profile_evt_handler_t)(void);

    typedef struct
    {
        drs_profile_evt_handler_t drs_evt_handler;
    } DrsProfileCallback_t;

    struct drsProfileService_s
    {
        uint8_t uuidType;
        uint16_t serviceHandle;
        uint16_t conn_handle;
        ble_gatts_char_handles_t drs_char_handler;
        drs_profile_evt_handler_t drs_evt_handler;
    };

    uint32_t ble_drs_on_data_send(DrsProfile_t *p_drs, void *p_data, uint16_t length);
    uint32_t drs_profile_init(const DrsProfileCallback_t *p_app_cb);
    void ble_drs_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);
#ifdef __cplusplus
}
#endif

#endif // !_CUS_DRS_H_
