/**
 * Copyright (c) 2014 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_app_template_main main.c
 * @{
 * @ingroup ble_sdk_app_template
 * @brief Template project main file.
 *
 * This file contains a template for creating a new application. It has the code necessary to wakeup
 * from button, advertise, get a connection restart advertising on disconnect and if no new
 * connection created go back to system-off mode.
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'YOUR_JOB' indicates where and how you can customize.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "fds.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "bsp_btn_ble.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_dfu_ble_svci_bond_sharing.h"
#include "nrf_svci_async_function.h"
#include "nrf_svci_async_handler.h"
#include "ble_dfu.h"
#include "nrf_power.h"
#include "cus_drs.h"
#include "cus_errs.h"
#include "cus_tss.h"
#include "cus_bat.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"
#include "nrf_drv_spi.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_saadc.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "rfal_rf.h"
#include "rfal_analogConfig.h"
#include "nfcv_worker.h"

#include "st25R3911_interrupt.h"
#include "st25r3911_com.h"

#define DEVICE_NAME "Diameter_"                            /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME "NordicSemiconductor"            /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL MSEC_TO_UNITS(300, UNIT_0_625_MS) /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */

#define APP_ADV_DURATION 18000  /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
#define APP_BLE_OBSERVER_PRIO 3 /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG 1  /**< A tag identifying the SoftDevice BLE configuration. */

#define MIN_CONN_INTERVAL MSEC_TO_UNITS(100, UNIT_1_25_MS) /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL MSEC_TO_UNITS(200, UNIT_1_25_MS) /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY 0                                    /**< Slave latency. */
#define CONN_SUP_TIMEOUT MSEC_TO_UNITS(4000, UNIT_10_MS)   /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(5000) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY APP_TIMER_TICKS(30000) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT 3                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND 1                               /**< Perform bonding. */
#define SEC_PARAM_MITM 0                               /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC 0                               /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS 0                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES BLE_GAP_IO_CAPS_NONE /**< No I/O capabilities. */
#define SEC_PARAM_OOB 0                                /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE 7                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE 16                      /**< Maximum encryption key size. */

#define DEAD_BEEF 0xDEADBEEF /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define SPI_INSTANCE 0

NRF_BLE_GATT_DEF(m_gatt);           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising); /**< Advertising module instance. */

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */

static volatile bool spi_xfer_done;
const nrf_drv_rtc_t m_rtc = NRF_DRV_RTC_INSTANCE(2);

static const nrf_drv_spi_t m_spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);

uint32_t cur_TZ = 1704038400UL;

static uint8_t tx[256];
static uint8_t rx[256];

// 0: disconnected; 1: charging; 2: foc;
uint8_t charge_status = 0;

uint8_t device_connect_sta = 0;

/* Flag to check fds initialization. */
bool volatile m_fds_initialized;

// tz data save
#define CONFIG_TZ_FILE 0x1f00
#define CONFIG_TZ_KEY 0x1f1f

static fds_record_t const m_dummy_tz_record = {
    .file_id = CONFIG_TZ_FILE,
    .key = CONFIG_TZ_KEY,
    .data.p_data = (uint32_t *)&cur_TZ,
    .data.length_words = (sizeof(cur_TZ) + 3) / sizeof(uint32_t),
};

uint8_t fds_gc_flag = 0;

DrsLibrePayload_s libre_payload;
extern S_sensor_t sensor_d;
extern uint8_t sensor_state;
uint16_t battery_value = 0;

static void advertising_start(bool erase_bonds);
static void adv_data_refresh(void);
static void bat_data_refresh(void);
static void adv_fake_data_refresh(void);

static void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    st25r3911Isr();
}

static void power_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    NRF_LOG_INFO("system restart\r\n");
    NVIC_SystemReset();
}

static void gpio_init(void)
{
    ret_code_t err_code;
    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    // riseing edge trigger
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    in_config.pull = NRF_GPIO_PIN_NOPULL;

    err_code = nrf_drv_gpiote_in_init(IRQ_PIN, &in_config, in_pin_handler);
    APP_ERROR_CHECK(err_code);

    // riseing edge trigger
    nrf_drv_gpiote_in_config_t power_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    power_config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_in_init(PPR_PIN, &power_config, power_pin_handler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(PPR_PIN, true);

    nrf_gpio_cfg_input(CHG_PIN, NRF_GPIO_PIN_PULLUP);
    // nrf_gpio_cfg_input(PPR_PIN, NRF_GPIO_PIN_PULLUP);
}

void IRQ_Enable(void)
{
    // 使能GPIOTE
    nrf_drv_gpiote_in_event_enable(IRQ_PIN, true);
}

void IRQ_Disable(void)
{
    // 失能GPIOTE
    nrf_drv_gpiote_in_event_disable(IRQ_PIN);
}

void spi_event_handler(nrf_drv_spi_evt_t const *p_event, void *p_context)
{
    spi_xfer_done = true;
}

static void hal_spi_init(void)
{
    nrf_gpio_cfg_output(SPIM0_SS_PIN);
    nrf_gpio_pin_set(SPIM0_SS_PIN);

    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin = NRF_DRV_SPI_PIN_NOT_USED;
    spi_config.miso_pin = SPIM0_MISO_PIN;
    spi_config.mosi_pin = SPIM0_MOSI_PIN;
    spi_config.sck_pin = SPIM0_SCK_PIN;
    spi_config.mode = NRF_DRV_SPI_MODE_1;
    spi_config.frequency = NRF_DRV_SPI_FREQ_2M;
    APP_ERROR_CHECK(nrf_drv_spi_init(&m_spi, &spi_config, spi_event_handler, NULL));
}

void spi0_cs_enable(void)
{
    nrf_gpio_pin_clear(SPIM0_SS_PIN);
}

void spi0_cs_disable(void)
{
    nrf_gpio_pin_set(SPIM0_SS_PIN);
}

uint8_t nrf_spi_tx_rx(const uint8_t *txData, uint8_t *rxData, uint8_t len)
{
    if (txData != NULL)
    {
        memcpy(tx, txData, len);
    }

    spi_xfer_done = false;
    if (len == 1)
    {
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&m_spi, tx, len, rx, 0));
    }
    else
    {
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&m_spi, tx, len, (rxData != NULL) ? rxData : rx, len));
    }
    while (!spi_xfer_done)
    {
    }

    return 0;
}

void saadc_callback(nrf_drv_saadc_evt_t const *p_event)
{
}

void saadc_init(void)
{
    ret_code_t err_code;
    nrf_saadc_channel_config_t channel_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN4);

    err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);
}

void saadc_start(void)
{
    nrf_saadc_value_t saadc_val;
    float value = 0;
    nrf_drv_saadc_sample_convert(0, &saadc_val);

    value = (saadc_val * 3.6 / 1024) * 3;
    battery_value = (uint16_t)(value * 1000); // mv
}
/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const *p_evt)
{
    // pm_handler_on_pm_evt(p_evt);
    // pm_handler_flash_clean(p_evt);
    ret_code_t err_code;
    switch (p_evt->evt_id)
    {
    case PM_EVT_BONDED_PEER_CONNECTED:
    {
        NRF_LOG_INFO("Connected to a previously bonded device.\r\n");
    }
    break;
    case PM_EVT_CONN_SEC_SUCCEEDED:
    {
        NRF_LOG_INFO("Connection secured. Role: %d. conn_handle: %d, Procedure: %d\r\n",
                     ble_conn_state_role(p_evt->conn_handle),
                     p_evt->conn_handle,
                     p_evt->params.conn_sec_succeeded.procedure);
    }
    break;
    case PM_EVT_CONN_SEC_FAILED:
        break;
    case PM_EVT_CONN_SEC_CONFIG_REQ:
    {
        pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
        pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
    }
    break;
    case PM_EVT_STORAGE_FULL:
    {
        err_code = fds_gc();
        if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
        {
            // Retry.
        }
        else
        {
            APP_ERROR_CHECK(err_code);
        }
    }
    break;
    case PM_EVT_PEERS_DELETE_SUCCEEDED:
    {
        advertising_start(false);
    }
    break;
    case PM_EVT_PEER_DATA_UPDATE_FAILED:
    {
        APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
    }
    break;
    case PM_EVT_PEER_DELETE_FAILED:
    {
        APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
    }
    break;
    case PM_EVT_PEERS_DELETE_FAILED:
    {
        APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
    }
    break;
    case PM_EVT_ERROR_UNEXPECTED:
    {
        APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
    }
    break;
    case PM_EVT_CONN_SEC_START:
    case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
    case PM_EVT_PEER_DELETE_SUCCEEDED:
    case PM_EVT_LOCAL_DB_CACHE_APPLIED:
    case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
    case PM_EVT_SERVICE_CHANGED_IND_SENT:
    case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
    default:
        break;
    }
}
// fds处理事件回调
static void fds_evt_handler(fds_evt_t const *p_evt)
{
    switch (p_evt->id)
    {
    case FDS_EVT_INIT:
        if (p_evt->result == NRF_SUCCESS)
        {
            m_fds_initialized = true;
        }
        break;

    case FDS_EVT_WRITE:
    {
        if (p_evt->result == NRF_SUCCESS)
        {
            NRF_LOG_INFO(":\t0x%04x", p_evt->write.file_id);
        }
    }
    break;

    case FDS_EVT_DEL_RECORD:
    {
        if (p_evt->result == NRF_SUCCESS)
        {
            NRF_LOG_INFO("Delete File ID:\t0x%04x", p_evt->del.file_id);
        }
    }
    break;

    default:
        break;
    }
}

void fds_init_function(void)
{
    ret_code_t err_code;

    (void)fds_register(fds_evt_handler);
    err_code = fds_init();
    APP_ERROR_CHECK(err_code);
    while (!m_fds_initialized)
    {
        sd_app_evt_wait();
    }
    fds_stat_t stat = {0};
    err_code = fds_stat(&stat);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("Found %d valid records.", stat.valid_records);
    NRF_LOG_INFO("Found %d dirty records (ready to be garbage collected).", stat.dirty_records);
    if (stat.dirty_records >= 10)
    {
        fds_gc();
        NRF_LOG_INFO("Too many dirty records, garbage collecting.");
    }
}

// bool fds_tz_record_delete(void)
// {
//     ret_code_t err_code;
//     fds_find_token_t tok = {0};
//     fds_record_desc_t desc = {0};
//     err_code = fds_record_find(CONFIG_TZ_FILE, CONFIG_TZ_KEY, &desc, &tok);
//     if (err_code == NRF_SUCCESS)
//     {
//         err_code = fds_record_delete(&desc);
//         if (err_code != NRF_SUCCESS)
//         {
//             return false;
//         }

//         return true;
//     }
//     else
//     {
//         /* No records left to delete. */
//         return false;
//     }
// }

void fds_tz_update(void)
{
    static uint8_t update_count = 0;
    ret_code_t err_code;
    fds_record_desc_t desc = {0}; // 用来操作记录的描述符结构清零
    fds_find_token_t tok = {0};   // 保存秘钥的令牌清零
    err_code = fds_record_find(CONFIG_TZ_FILE, CONFIG_TZ_KEY, &desc, &tok);
    if (err_code == NRF_SUCCESS)
    {
        err_code = fds_record_update(&desc, &m_dummy_tz_record);
        if ((err_code != NRF_SUCCESS) && (err_code == FDS_ERR_NO_SPACE_IN_FLASH))
        {
            NRF_LOG_INFO("No space in flash, delete some records to update the config file.");
        }
        else
        {
            APP_ERROR_CHECK(err_code);
        }
    }
    update_count++;
    if (err_code == FDS_ERR_NO_SPACE_IN_FLASH || update_count >= 5)
    {
        // fds_gc();
        update_count = 0;
        fds_gc_flag = 1;
        NRF_LOG_INFO("fds_gc()");
        // err_code = fds_record_write(&desc, &m_dummy_tz_record); // 写记录和数据
    }
    APP_ERROR_CHECK(err_code);
}

void fds_tz_read(void)
{
    ret_code_t err_code;
    fds_record_desc_t desc = {0}; // 用来操作记录的描述符结构清零
    fds_find_token_t tok = {0};   // 保存秘钥的令牌清零
    uint32_t *data = 0;

    err_code = fds_record_find(CONFIG_TZ_FILE, CONFIG_TZ_KEY, &desc, &tok);
    if (err_code == NRF_SUCCESS)
    {
        fds_flash_record_t config = {0};

        err_code = fds_record_open(&desc, &config);
        APP_ERROR_CHECK(err_code);

        data = (uint32_t *)config.p_data;

        cur_TZ = data[0];

        /* Close the record when done reading. */
        err_code = fds_record_close(&desc); // 关闭记录
        APP_ERROR_CHECK(err_code);
    }
    else
    {
        err_code = fds_record_write(&desc, &m_dummy_tz_record); // 重新写数据
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
APP_TIMER_DEF(m_app_timer_id);
#define TIME_LEVEL_MEAS_INTERVAL APP_TIMER_TICKS(1000)
uint8_t scan_flag = 0;
uint8_t libre_send_step = 0;
uint8_t fake_libre_send_step = 0;
static void time_update(void)
{
    static uint32_t pre_tick = 0;
    //    uint8_t end_code = 0;
    ret_code_t err_code;

    cur_TZ++;

    if (cur_TZ - pre_tick >= 60)
    {
        pre_tick = cur_TZ;
        if (sensor_state == 1)
        {
            adv_data_refresh();
        }
        else
        {
            adv_fake_data_refresh();
        }
        bat_data_refresh();
    }
    else if (cur_TZ - pre_tick >= 59)
    {
        if (sensor_state == 1)
        {
            adv_data_refresh();
        }
        else
        {
            adv_fake_data_refresh();
        }
    }
    else if (cur_TZ - pre_tick >= 55)
    {
        scan_flag = 1;
        libre_send_step = 0;
        fake_libre_send_step = 0;
    }
    else
    {
        scan_flag = 0;
    }
    // NRF_LOG_INFO("scan_flag:%d", scan_flag);

    // NVIC_SystemReset();
    switch (charge_status)
    {
    case 0:
        if (nrf_gpio_pin_read(CHG_PIN) == 0)
        {
            charge_status = 1;
            err_code = bsp_indication_set(BSP_INDICATE_USER_STATE_0);
            APP_ERROR_CHECK(err_code);
            NRF_LOG_INFO("DEVICE IN CHARGING...");
        }
        break;
    case 1:
        if (nrf_gpio_pin_read(CHG_PIN) != 0)
        {
            charge_status = 2;
            err_code = bsp_indication_set(BSP_INDICATE_USER_STATE_3);
            APP_ERROR_CHECK(err_code);
            NRF_LOG_INFO("DEVICE FULL OF CHARGE");
        }
        break;
    case 2:
        break;
    default:
        break;
    }
}
static void timer_timeout_handler(void *p_context)
{
    UNUSED_PARAMETER(p_context);
    time_update();
}

// APP_TIMER_DEF(m_systick_id);
// #define TIME_SYSTICK_INTERVAL APP_TIMER_TICKS(1)
uint32_t my_systick = 0;

static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create timers.

    /* YOUR_JOB: Create any timers to be used by the application.
                 Below is an example of how to create a timer.
                 For every new timer needed, increase the value of the macro APP_TIMER_MAX_TIMERS by
                 one.
       ret_code_t err_code;
       err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
       APP_ERROR_CHECK(err_code); */
    err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if (int_type == NRF_DRV_RTC_INT_TICK)
    {
        my_systick++;
    }
}

uint32_t get_sysTick(void)
{
    return my_systick;
}

static void lfclk_config(void)
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}

static void rtc_config(void)
{
    uint32_t err_code;

    nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;
    config.prescaler = 31;

    err_code = nrf_drv_rtc_init(&m_rtc, &config, rtc_handler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_rtc_tick_enable(&m_rtc, true);
    nrf_drv_rtc_enable(&m_rtc);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t err_code;
    ble_gap_conn_params_t gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;
    ble_gap_addr_t addr;
    char *dev_name;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_addr_get(&addr);
    APP_ERROR_CHECK(err_code);

    dev_name = (char *)malloc(strlen(DEVICE_NAME) + 4);
    sprintf(dev_name, "%s%02x%02x", DEVICE_NAME, addr.addr[1], addr.addr[0]);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)dev_name,
                                          strlen(dev_name));
    APP_ERROR_CHECK(err_code);

    /* YOUR_JOB: Use an appearance value matching the application's use case.
       err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_);
       APP_ERROR_CHECK(err_code); */

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

#if (BLE_DFU_ENABLED == 1)
static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
    switch (event)
    {
    case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE:
    {
        NRF_LOG_INFO("Device is preparing to enter bootloader mode.");
        break;
    }

    case BLE_DFU_EVT_BOOTLOADER_ENTER:
        // YOUR_JOB: Write app-specific unwritten data to FLASH, control finalization of this
        //           by delaying reset by reporting false in app_shutdown_handler
        NRF_LOG_INFO("Device will enter bootloader mode.");
        break;

    case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED:
        NRF_LOG_ERROR("Request to enter bootloader mode failed asynchroneously.");
        // YOUR_JOB: Take corrective measures to resolve the issue
        //           like calling APP_ERROR_CHECK to reset the device.
        break;

    case BLE_DFU_EVT_RESPONSE_SEND_ERROR:
        NRF_LOG_ERROR("Request to send a response to client failed.");
        // YOUR_JOB: Take corrective measures to resolve the issue
        //           like calling APP_ERROR_CHECK to reset the device.
        APP_ERROR_CHECK(false);
        break;

    default:
        NRF_LOG_ERROR("Unknown event from ble_dfu_buttonless.");
        break;
    }
}
#endif

static void drs_profile_evt_handler()
{
}

static void errs_profile_evt_handler()
{
}

static void tss_profile_evt_handler(uint32_t timestamp)
{
    cur_TZ = timestamp;
    fds_tz_update();
    NRF_LOG_INFO("TSS: %d", timestamp);
}

static void bat_profile_evt_handler()
{
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    ret_code_t err_code;
    // ble_nus_init_t nus_init;
    nrf_ble_qwr_init_t qwr_init = {0};
    ble_dfu_buttonless_init_t dfus_init = {0};
    DrsProfileCallback_t drs_init = {0};
    ErrsProfileCallback_t errs_init = {0};
    TssProfileCallback_t tss_init = {0};
    BatProfileCallback_t bat_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // create drs profile
    drs_init.drs_evt_handler = drs_profile_evt_handler;
    err_code = drs_profile_init(&drs_init);
    APP_ERROR_CHECK(err_code);

    // create errs profile
    errs_init.errs_evt_handler = errs_profile_evt_handler;
    err_code = errs_profile_init(&errs_init);
    APP_ERROR_CHECK(err_code);

    // create tss profile
    tss_init.tss_evt_handler = tss_profile_evt_handler;
    err_code = tss_profile_init(&tss_init);
    APP_ERROR_CHECK(err_code);

    // create bat profile
    bat_init.bat_evt_handler = bat_profile_evt_handler;
    err_code = bat_profile_init(&bat_init);
    APP_ERROR_CHECK(err_code);

#if (BLE_DFU_ENABLED == 1)
    err_code = ble_dfu_buttonless_async_svci_init();
    APP_ERROR_CHECK(err_code);

    dfus_init.evt_handler = ble_dfu_evt_handler;

    err_code = ble_dfu_buttonless_init(&dfus_init);
    APP_ERROR_CHECK(err_code);
#endif
}

unsigned short DO_CRC16(unsigned char *ptr, int len)
{
    unsigned short crc = 0xFFFF;
    for (int i = 0; i < len; i++)
    {
        crc ^= ptr[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return ((crc >> 8) | (crc << 8));
}

// 5 min intervel
static void adv_data_refresh(void)
{
    uint16_t half_fram_size = sizeof(sensor_d.fram) / 2;
    uint16_t checkcrc = 0;

    if (libre_send_step == 0)
    {
        memcpy(libre_payload.data + 1, sensor_d.fram, half_fram_size);
        libre_payload.data[0] = 0x00;
        checkcrc = DO_CRC16(libre_payload.data, half_fram_size + 1);
        libre_payload.data[half_fram_size + 1] = (checkcrc >> 8) & 0xFF;
        libre_payload.data[half_fram_size + 2] = (checkcrc >> 0) & 0xFF;
        drs_adv_libre_notify_send(&libre_payload);
        NRF_LOG_INFO("step: %d, checkcrc:%04x", libre_send_step, checkcrc);
        libre_send_step = 1;
    }
    else if (libre_send_step == 1)
    {
        memcpy(libre_payload.data + 1, sensor_d.fram + half_fram_size, half_fram_size);
        libre_payload.data[0] = 0x01;
        checkcrc = DO_CRC16(libre_payload.data, half_fram_size + 1);
        libre_payload.data[half_fram_size + 1] = (checkcrc >> 8) & 0xFF;
        libre_payload.data[half_fram_size + 2] = (checkcrc >> 0) & 0xFF;
        drs_adv_libre_notify_send(&libre_payload);
        NRF_LOG_INFO("step: %d, checkcrc:%04x", libre_send_step, checkcrc);
    }
}

uint8_t fake_data[344] = {0xF9, 0x8D, 0x50, 0x77, 0x06, 0x00, 0x1F, 0x00,
                          0x00, 0x13, 0x1B, 0x50, 0x17, 0x1B, 0x50, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x4F, 0xDB, 0x0B, 0x17, 0xFD, 0x04, 0xC8, 0x68,
                          0x56, 0x01, 0x00, 0x05, 0xC8, 0x08, 0x17, 0x01,
                          0xD5, 0x04, 0xC8, 0x88, 0x17, 0x01, 0xC4, 0x04,
                          0xC8, 0x9C, 0xD8, 0x00, 0xA5, 0x04, 0xC8, 0x94,
                          0x99, 0x00, 0x8A, 0x04, 0xC8, 0x3C, 0x5A, 0x00,
                          0x84, 0x04, 0xC8, 0x88, 0x5A, 0x00, 0x8C, 0x04,
                          0xC8, 0xF0, 0x99, 0x00, 0x88, 0x04, 0xC8, 0x3C,
                          0x9A, 0x00, 0x7E, 0x04, 0xC8, 0x14, 0x9A, 0x00,
                          0x88, 0x04, 0xC8, 0x40, 0x9A, 0x00, 0xF2, 0x04,
                          0xC8, 0x94, 0x56, 0x01, 0xEE, 0x04, 0xC8, 0x9C,
                          0x16, 0x01, 0xF1, 0x04, 0xC8, 0x94, 0x16, 0x01,
                          0xE3, 0x04, 0xC8, 0xB4, 0x56, 0x01, 0xEA, 0x04,
                          0xC8, 0x74, 0x16, 0x01, 0xF1, 0x04, 0xC8, 0xB0,
                          0x17, 0x01, 0xD3, 0x04, 0xC8, 0x10, 0x18, 0x01,
                          0x58, 0x04, 0xC8, 0xE8, 0x17, 0x01, 0x95, 0x04,
                          0xC8, 0x94, 0x17, 0x01, 0xBE, 0x04, 0xC8, 0x3C,
                          0x17, 0x01, 0xE7, 0x04, 0xC8, 0xA4, 0x16, 0x01,
                          0xC7, 0x04, 0xC8, 0x7C, 0x56, 0x01, 0xDC, 0x04,
                          0xC8, 0x4C, 0x56, 0x01, 0x68, 0x04, 0xC8, 0xD4,
                          0x17, 0x01, 0x2D, 0x04, 0xC8, 0x8C, 0x16, 0x01,
                          0x99, 0x04, 0xC8, 0x74, 0x56, 0x01, 0x99, 0x04,
                          0xC8, 0x38, 0x56, 0x01, 0xA4, 0x04, 0xC8, 0x28,
                          0x56, 0x01, 0x2B, 0x04, 0xC8, 0x5C, 0x56, 0x01,
                          0xC7, 0x03, 0xC8, 0x50, 0x16, 0x01, 0xEF, 0x03,
                          0xC8, 0x54, 0x16, 0x01, 0xDE, 0x03, 0xC8, 0xB4,
                          0x16, 0x01, 0x42, 0x04, 0xC8, 0x04, 0x17, 0x01,
                          0x1F, 0x05, 0xC8, 0xEC, 0x16, 0x01, 0x84, 0x05,
                          0xC8, 0xB0, 0x16, 0x01, 0x49, 0x05, 0xC8, 0x9C,
                          0x56, 0x01, 0xFD, 0x04, 0xC8, 0x94, 0x16, 0x01,
                          0x86, 0x04, 0xC8, 0x3C, 0x9A, 0x00, 0x1D, 0x03,
                          0xC8, 0xA8, 0x17, 0x01, 0x54, 0x03, 0xC8, 0x88,
                          0x17, 0x01, 0x71, 0x03, 0xC8, 0x60, 0x17, 0x01,
                          0x25, 0x03, 0xC8, 0xF8, 0x99, 0x00, 0x1E, 0x03,
                          0xC8, 0xE4, 0xD9, 0x00, 0x97, 0x03, 0xC8, 0x2C,
                          0x9B, 0x00, 0xE4, 0x04, 0xC8, 0xAC, 0xD8, 0x00,
                          0xB0, 0x05, 0xC8, 0x38, 0xD8, 0x00, 0x6C, 0x05,
                          0xC8, 0xE0, 0x17, 0x01, 0x1B, 0x50, 0x00, 0x00,
                          0x5B, 0x17, 0x00, 0x08, 0xE7, 0x0D, 0x61, 0x51,
                          0x14, 0x07, 0x96, 0x80, 0x5A, 0x00, 0xED, 0xA6,
                          0x06, 0x88, 0x1A, 0xC8, 0x04, 0x28, 0xB8, 0x5F};

static void adv_fake_data_refresh(void)
{
    uint16_t half_size = 172;
    uint16_t checkcrc = 0;

    if (fake_libre_send_step == 0)
    { // make fake rt data
        for (int i = 0; i < 15; i++)
        {
            uint8_t index = 4 + (14 - i) * 6;

            if (index < 4)
            {
                index += 96;
            }

            uint16_t glucose = rand() % 200 + 20;

            fake_data[index + 25] = (glucose << 8) & 0xff;
            fake_data[index + 24] = (glucose << 0) & 0xff;
        }

        // make fake his data
        for (int j = 0; j < 32; j++)
        {
            uint16_t index = 100 + (31 - j) * 6;
            if (index < 100)
            {
                index += 192;
            }

            uint16_t glucose = rand() % 200 + 20;

            fake_data[index + 25] = (glucose << 8) & 0xff;
            fake_data[index + 24] = (glucose << 0) & 0xff;
        }
    }

    if (fake_libre_send_step == 0)
    {
        memcpy(libre_payload.data + 1, fake_data, half_size);
        libre_payload.data[0] = 0x00;
        checkcrc = DO_CRC16(libre_payload.data, half_size + 1);
        libre_payload.data[half_size + 1] = (checkcrc >> 8) & 0xFF;
        libre_payload.data[half_size + 2] = (checkcrc >> 0) & 0xFF;
        drs_adv_libre_notify_send(&libre_payload);
        NRF_LOG_INFO("fake step: %d, checkcrc:%04x", fake_libre_send_step, checkcrc);
        fake_libre_send_step = 1;
    }
    else if (fake_libre_send_step == 1)
    {
        memcpy(libre_payload.data + 1, fake_data + half_size, half_size);
        libre_payload.data[0] = 0x01;
        checkcrc = DO_CRC16(libre_payload.data, half_size + 1);
        libre_payload.data[half_size + 1] = (checkcrc >> 8) & 0xFF;
        libre_payload.data[half_size + 2] = (checkcrc >> 0) & 0xFF;
        drs_adv_libre_notify_send(&libre_payload);
        NRF_LOG_INFO("fake step: %d, checkcrc:%04x", fake_libre_send_step, checkcrc);
    }
}

static void bat_data_refresh(void)
{
    saadc_start();
    bat_adv_notify_send(&battery_value); // mv
}

// static void err_code_update(void)
//{
//     static uint16_t err_payload = 0, pre_err_code = 0;

//    err_payload = (sensor_state == 0) ? 1 : 0;

//    if (pre_err_code != err_payload)
//    {
//        pre_err_code = err_payload;
//        errs_adv_notify_send(&err_payload);
//    }
//}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t *p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail = false;
    cp_init.evt_handler = on_conn_params_evt;
    cp_init.error_handler = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting timers.
 */
static void application_timers_start(void)
{
    uint32_t err_code;

    err_code = app_timer_start(m_app_timer_id, TIME_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    // ret_code_t err_code;

    switch (ble_adv_evt)
    {
    case BLE_ADV_EVT_FAST:
        NRF_LOG_INFO("Fast advertising.");
        // err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
        // APP_ERROR_CHECK(err_code);
        break;

    case BLE_ADV_EVT_SLOW:
        NRF_LOG_INFO("Slow advertising.");
        // err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
        // APP_ERROR_CHECK(err_code);
        break;

    case BLE_ADV_EVT_IDLE:
        NRF_LOG_INFO("Sleep Mode.");
        // sleep_mode_enter();
        break;

    default:
        break;
    }
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_DISCONNECTED:
        NRF_LOG_INFO("Disconnected.");
        device_connect_sta = 0;
        // LED indication will be changed when advertising starts.
        break;

    case BLE_GAP_EVT_CONNECTED:
        NRF_LOG_INFO("Connected.");
        device_connect_sta = 1;
        // err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
        // APP_ERROR_CHECK(err_code);
        m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
    {
        NRF_LOG_DEBUG("PHY update request.");
        ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
        err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
        APP_ERROR_CHECK(err_code);
    }
    break;

    case BLE_GATTC_EVT_TIMEOUT:
        // Disconnect on GATT Client timeout event.
        NRF_LOG_DEBUG("GATT Client Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

    case BLE_GATTS_EVT_TIMEOUT:
        // Disconnect on GATT Server timeout event.
        NRF_LOG_DEBUG("GATT Server Timeout.");
        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;

    default:
        // No implementation needed.
        break;
    }
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond = SEC_PARAM_BOND;
    sec_param.mitm = SEC_PARAM_MITM;
    sec_param.lesc = SEC_PARAM_LESC;
    sec_param.keypress = SEC_PARAM_KEYPRESS;
    sec_param.io_caps = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob = SEC_PARAM_OOB;
    sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc = 1;
    sec_param.kdist_own.id = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated when button is pressed.
 */
static void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;

    switch (event)
    {
    case BSP_EVENT_SLEEP:
        sleep_mode_enter();
        break; // BSP_EVENT_SLEEP

    case BSP_EVENT_DISCONNECT:
        err_code = sd_ble_gap_disconnect(m_conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
        break; // BSP_EVENT_DISCONNECT

    case BSP_EVENT_WHITELIST_OFF:
        if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
        {
            err_code = ble_advertising_restart_without_whitelist(&m_advertising);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
        }
        break; // BSP_EVENT_KEY_0
    case BSP_EVENT_KEY_2:
        nrf_gpio_pin_toggle(LED_3);
        NRF_LOG_INFO("bsp_event_key_2");
        break;
    default:
        break;
    }
}

/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t err_code;
    ble_advertising_init_t init;
    uint8_t my_adv_manuf_data[6];
    ble_gap_addr_t addr;

    memset(&init, 0, sizeof(init));

    err_code = sd_ble_gap_addr_get(&addr);
    APP_ERROR_CHECK(err_code);

    for (int i = 0; i < 6; i++)
    {
        my_adv_manuf_data[i] = addr.addr[5 - i];
    }

    ble_advdata_manuf_data_t manuf_data;
    manuf_data.company_identifier = 0x0033;
    manuf_data.data.p_data = my_adv_manuf_data;
    manuf_data.data.size = sizeof(my_adv_manuf_data);

    init.advdata.name_type = BLE_ADVDATA_FULL_NAME;
    init.advdata.p_manuf_specific_data = &manuf_data;
    init.advdata.include_appearance = false;
    init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    // init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    // init.advdata.uuids_complete.p_uuids = m_adv_uuids;

    init.config.ble_adv_fast_enabled = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout = APP_ADV_DURATION;

    init.config.ble_adv_slow_enabled = true;
    init.config.ble_adv_slow_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_slow_timeout = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool *p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LEDS /*| BSP_INIT_BUTTONS*/, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

/**@brief Function for starting advertising.
 */
static void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETED_SUCEEDED event
    }
    else
    {
        ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);

        APP_ERROR_CHECK(err_code);
        NRF_LOG_DEBUG("Advertising started.");
    }
}

static void device_start(void)
{
    ret_code_t err_code = NRF_SUCCESS;

    NRF_LOG_INFO("diameter device started.");
    err_code = bsp_indication_set(BSP_INDICATE_USER_STATE_1);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for application main entry.
 */
int main(void)
{
    bool erase_bonds;

    // Initialize.
    log_init();
    timers_init();
    lfclk_config();
    rtc_config();
    hal_spi_init();
    gpio_init();
    buttons_leds_init(&erase_bonds);
    power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    advertising_init();
    services_init();
    conn_params_init();
    peer_manager_init();

    fds_init_function();
    fds_tz_read();
    // Start execution.
    device_start();
    application_timers_start();

    advertising_start(erase_bonds);

    saadc_init();

    rfalAnalogConfigInitialize();
    if (rfalInitialize() != ERR_NONE)
    {
        platformLog("RFAL initialization failed..\r\n");
    }
    else
    {
        platformLog("RFAL initialization succeeded..\r\n");
    }
    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
        rfalWorker();
        workCycle(scan_flag);
    }
}

/**
 * @}
 */
