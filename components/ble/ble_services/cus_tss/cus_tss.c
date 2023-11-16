/*********************************************************************
 * INCLUDES
 */
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(BLE_DRS)
#include "ble_srv_common.h"

#include "cus_tss.h"
#include "nrf_log.h"

/*********************************************************************
 * LOCAL VARIABLES
 */
static TssProfile_t m_tss;
NRF_SDH_BLE_OBSERVER(m_tss_obs, BLE_TSS_BLE_OBSERVER_PRIO, ble_tss_on_ble_evt, &m_tss);

static void on_write(TssProfile_t *p_tss, ble_evt_t const *pBleEvent)
{
    ble_gatts_evt_write_t const *pEventWrite = &pBleEvent->evt.gatts_evt.params.write;
    uint32_t timestamp = 0;

    if ((pEventWrite->handle == p_tss->tss_char_handler.value_handle) &&
        (p_tss->tss_evt_handler != NULL))
    {
        // 组合成32位的数据
        timestamp = pEventWrite->data[0] << 24 | pEventWrite->data[1] << 16 |
                    pEventWrite->data[2] << 8 | pEventWrite->data[3];
        p_tss->tss_evt_handler(timestamp);
        NRF_LOG_INFO("TSS characteristic written");
    }
}

void ble_tss_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context)
{
    TssProfile_t *p_tss = (TssProfile_t *)p_context;
    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GATTS_EVT_WRITE:
        on_write(p_tss, p_ble_evt);
        break;
    default:
        break;
    }
}

/*********************************************************************
 * EXTERN FUNCTIONS
 */
/**
    @brief 注册应用程序回调函数。只调用这个函数一次
    @param TssProfile_t -[out] DRS初始化服务结构体
    @param TssProfileCallback_t -[in] 指向应用程序的回调
    @return NRF_SUCCESS - 成功；其他值 - 失败
*/
uint32_t tss_profile_init(const TssProfileCallback_t *p_app_cb)
{
    uint32_t errCode;
    ble_uuid_t bleUuid;
    ble_add_char_params_t addCharParams;

    VERIFY_PARAM_NOT_NULL(p_app_cb);

    // 初始化服务结构体
    m_tss.tss_evt_handler = p_app_cb->tss_evt_handler;

    /*--------------------- 服务 ---------------------*/
    ble_uuid128_t baseUuid = TSS_PROFILE_UUID_BASE;
    errCode = sd_ble_uuid_vs_add(&baseUuid, &m_tss.uuidType);
    VERIFY_SUCCESS(errCode);

    bleUuid.type = m_tss.uuidType;
    bleUuid.uuid = TSS_PROFILE_UUID_SERVICE;

    errCode = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &bleUuid, &m_tss.serviceHandle);
    VERIFY_SUCCESS(errCode);

    /*--------------------- 特征1 ---------------------*/
    memset(&addCharParams, 0, sizeof(addCharParams));
    addCharParams.uuid = TSS_PROFILE_UUID_CHAR;
    addCharParams.uuid_type = m_tss.uuidType;
    addCharParams.init_len = sizeof(uint8_t);
    addCharParams.max_len = TSS_PROFILE_CHAR_LEN; // 特征长度
    // addCharParams.char_props.read = 1;            // 可读
    addCharParams.char_props.write = 1; // 可写
    // addCharParams.char_props.notify = 1;          // 可通知

    // addCharParams.read_access = SEC_OPEN;
    addCharParams.write_access = SEC_OPEN;
    // addCharParams.cccd_write_access = SEC_OPEN;

    errCode = characteristic_add(m_tss.serviceHandle, &addCharParams, &m_tss.tss_char_handler);
    VERIFY_SUCCESS(errCode);

    return errCode;
}

#endif

/****************************************************END OF FILE****************************************************/
