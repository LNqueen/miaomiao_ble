/*********************************************************************
 * INCLUDES
 */
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(BLE_DRS)
#include "ble_srv_common.h"
#include <stdlib.h>

#include "cus_drs.h"
#include "nrf_log.h"
#include "nrf_delay.h"

/*********************************************************************
 * LOCAL VARIABLES
 */
static DrsProfile_t m_drs;
NRF_SDH_BLE_OBSERVER(m_drs_obs, BLE_DRS_BLE_OBSERVER_PRIO, ble_drs_on_ble_evt, &m_drs);

static void on_write(DrsProfile_t *p_drs, ble_evt_t const *pBleEvent)
{
    ble_gatts_evt_write_t const *pEventWrite = &pBleEvent->evt.gatts_evt.params.write;

    if ((pEventWrite->handle == p_drs->drs_char_handler.value_handle) &&
        (p_drs->drs_evt_handler != NULL))
    {
        p_drs->drs_evt_handler();
        NRF_LOG_INFO("DRS characteristic written");
    }
}

void ble_drs_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context)
{
    DrsProfile_t *p_drs = (DrsProfile_t *)p_context;
    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GATTS_EVT_WRITE:
        on_write(p_drs, p_ble_evt);
        break;
    default:
        break;
    }
}
// drs数据上传函数
uint32_t ble_drs_on_data_send(DrsProfile_t *p_drs, void *p_data, uint16_t length)
{
    ble_gatts_hvx_params_t hvx_params;
    uint16_t data_length = length;
    uint32_t errCode;

    if (p_drs->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        memset(&hvx_params, 0, sizeof(hvx_params));
        hvx_params.handle = p_drs->drs_char_handler.value_handle;
        hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len = &data_length;
        hvx_params.p_data = p_data;

        return sd_ble_gatts_hvx(p_drs->conn_handle, &hvx_params);
    }
    else
    {
        errCode = BLE_ERROR_INVALID_CONN_HANDLE;
    }
    return errCode;
}
uint8_t drs_adv_libre_notify_send(DrsLibrePayload_s *data)
{
    static uint8_t tmp[172];

    memcpy(tmp, data, 172);

    ble_drs_on_data_send(&m_drs, (void *)tmp, 172);

    return 0;
}
// uint8_t drs_adv_his_notify_send(DrsHisPayload_s *data)
// {
//     uint8_t tmp[19];
//     static uint16_t cur_data_number = 0;
//     tmp[0] = data->type;
//     tmp[1] = data->len;
//     tmp[4] = data->totalDataNumber >> 8 & 0xff;
//     tmp[5] = data->totalDataNumber >> 0 & 0xff;
//     tmp[6] = data->curSensorSta;
//     tmp[7] = data->battery >> 8 & 0xff;
//     tmp[8] = data->battery >> 0 & 0xff;

//     if (cur_data_number != data->totalDataNumber)
//     {
//         cur_data_number++;
//         tmp[2] = cur_data_number >> 8 & 0xff;
//         tmp[3] = cur_data_number >> 0 & 0xff;
//         tmp[9] = data->data[cur_data_number - 1].bloodGluose >> 8 & 0xff;
//         tmp[10] = data->data[cur_data_number - 1].bloodGluose >> 0 & 0xff;
//         tmp[11] = data->data[cur_data_number - 1].libreLife >> 24 & 0xff;
//         tmp[12] = data->data[cur_data_number - 1].libreLife >> 16 & 0xff;
//         tmp[13] = data->data[cur_data_number - 1].libreLife >> 8 & 0xff;
//         tmp[14] = data->data[cur_data_number - 1].libreLife >> 0 & 0xff;
//         tmp[15] = data->data[cur_data_number - 1].timeStamp >> 24 & 0xff;
//         tmp[16] = data->data[cur_data_number - 1].timeStamp >> 16 & 0xff;
//         tmp[17] = data->data[cur_data_number - 1].timeStamp >> 8 & 0xff;
//         tmp[18] = data->data[cur_data_number - 1].timeStamp >> 0 & 0xff;
//         ble_drs_on_data_send(&m_drs, (void *)tmp, 19);
//     }

//     if (cur_data_number == data->totalDataNumber)
//     {
//         cur_data_number = 0;
//         return 0;
//     }

//     return 1;
// }

/*********************************************************************
 * EXTERN FUNCTIONS
 */
/**
    @brief 注册应用程序回调函数。只调用这个函数一次
    @param DrsProfile_t -[out] DRS初始化服务结构体
    @param DrsProfileCallback_t -[in] 指向应用程序的回调
    @return NRF_SUCCESS - 成功；其他值 - 失败
*/
uint32_t drs_profile_init(const DrsProfileCallback_t *p_app_cb)
{
    uint32_t errCode;
    ble_uuid_t bleUuid;
    ble_add_char_params_t addCharParams;

    VERIFY_PARAM_NOT_NULL(p_app_cb);

    // 初始化服务结构体
    m_drs.drs_evt_handler = p_app_cb->drs_evt_handler;

    /*--------------------- 服务 ---------------------*/
    ble_uuid128_t baseUuid = DRS_PROFILE_UUID_BASE;
    errCode = sd_ble_uuid_vs_add(&baseUuid, &m_drs.uuidType);
    VERIFY_SUCCESS(errCode);

    bleUuid.type = m_drs.uuidType;
    bleUuid.uuid = DRS_PROFILE_UUID_SERVICE;

    errCode = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &bleUuid, &m_drs.serviceHandle);
    VERIFY_SUCCESS(errCode);

    /*--------------------- 特征1 ---------------------*/
    memset(&addCharParams, 0, sizeof(addCharParams));
    addCharParams.uuid = DRS_PROFILE_UUID_CHAR;
    addCharParams.uuid_type = m_drs.uuidType;
    addCharParams.init_len = sizeof(uint8_t);
    addCharParams.max_len = DRS_PROFILE_CHAR_LEN; // 特征长度
    addCharParams.char_props.read = 1;            // 可读
    // addCharParams.char_props.write = 1;           // 可写
    addCharParams.char_props.notify = 1; // 可通知

    addCharParams.read_access = SEC_OPEN;
    // addCharParams.write_access = SEC_OPEN;
    addCharParams.cccd_write_access = SEC_OPEN;

    errCode = characteristic_add(m_drs.serviceHandle, &addCharParams, &m_drs.drs_char_handler);
    VERIFY_SUCCESS(errCode);

    return errCode;
}

#endif

/****************************************************END OF FILE****************************************************/
