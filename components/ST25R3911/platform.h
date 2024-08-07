
/******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
 *
 * Licensed under ST MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/myliberty
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
 * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/
/*! \file
 *
 *  \author
 *
 *  \brief Platform header file. Defining platform independent functionality.
 *
 */

/*
 *      PROJECT:
 *      $Revision: $
 *      LANGUAGE:  ISO C99
 */

/*! \file platform.h
 *
 *  \author Gustavo Patricio
 *
 *  \brief Platform specific definition layer
 *
 *  This should contain all platform and hardware specifics such as
 *  GPIO assignment, system resources, locks, IRQs, etc
 *
 *  Each distinct platform/system/board must provide this definitions
 *  for all SW layers to use
 *
 */

#ifndef PLATFORM_H
#define PLATFORM_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/

#include "stdint.h"
#include "stdbool.h"
#include "limits.h"

#include "timer.h"

#include "bsp_config.h"
#include "boards.h"

#include "nrf.h"
#include "nrf_log.h"
#include "nrf_delay.h"

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
#define ST25R391X_SS_PIN BSP_SPI_CS    /*!< GPIO pin used for ST25R3911 SPI SS                */
#define ST25R391X_INT_PIN BSP_3911_INT /*!< GPIO pin used for ST25R3911 External Interrupt    */

#ifdef LED_FIELD_Pin
#define PLATFORM_LED_FIELD_PIN LED_FIELD_Pin /*!< GPIO pin used as field LED                        */
#endif

#ifdef LED_FIELD_GPIO_Port
#define PLATFORM_LED_FIELD_PORT LED_FIELD_GPIO_Port /*!< GPIO port used as field LED                       */
#endif

// #define PLATFORM_LED_A_PIN LED_A_Pin              /*!< GPIO pin used for LED A    */
// #define PLATFORM_LED_A_PORT LED_A_GPIO_Port       /*!< GPIO port used for LED A   */
// #define PLATFORM_LED_B_PIN LED_B_Pin              /*!< GPIO pin used for LED B    */
// #define PLATFORM_LED_B_PORT LED_B_GPIO_Port       /*!< GPIO port used for LED B   */
// #define PLATFORM_LED_F_PIN LED_F_Pin              /*!< GPIO pin used for LED F    */
// #define PLATFORM_LED_F_PORT LED_F_GPIO_Port       /*!< GPIO port used for LED F   */
// #define PLATFORM_LED_V_PIN LED_V_Pin              /*!< GPIO pin used for LED V    */
// #define PLATFORM_LED_V_PORT LED_V_GPIO_Port       /*!< GPIO port used for LED V   */
// #define PLATFORM_LED_AP2P_PIN LED_AP2P_Pin        /*!< GPIO pin used for LED AP2P */
// #define PLATFORM_LED_AP2P_PORT LED_AP2P_GPIO_Port /*!< GPIO port used for LED AP2P*/

// #define PLATFORM_USER_BUTTON_PIN B1_Pin        /*!< GPIO pin user button       */
// #define PLATFORM_USER_BUTTON_PORT B1_GPIO_Port /*!< GPIO port user button      */

/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/
extern void IRQ_Enable(void);
extern void IRQ_Disable(void);
#define platformProtectST25R391xComm() IRQ_Disable()  /*!< Protect unique access to ST25R391x communication channel - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment      */
#define platformUnprotectST25R391xComm() IRQ_Enable() /*!< Unprotect unique access to ST25R391x communication channel - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment */

#define platformIrqST25R3911SetCallback(cb)
#define platformIrqST25R3911PinInitialize()

#define platformIrqST25R3916SetCallback(cb)
#define platformIrqST25R3916PinInitialize()

#define platformProtectST25R391xIrqStatus() platformProtectST25R391xComm()     /*!< Protect unique access to IRQ status var - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment */
#define platformUnprotectST25R391xIrqStatus() platformUnprotectST25R391xComm() /*!< Unprotect the IRQ status var - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment         */

#define platformLedsInitialize() /*!< Initializes the pins used as LEDs to outputs*/

#define platformLedOff(pin) platformGpioClear(pin)     /*!< Turns the given LED Off                     */
#define platformLedOn(pin) platformGpioSet(pin)        /*!< Turns the given LED On                      */
#define platformLedToogle(pin) platformGpioToogle(pin) /*!< Toogle the given LED                        */

#define platformGpioSet(pin) nrf_gpio_pin_set(pin)            /*!< Turns the given GPIO High                   */
#define platformGpioClear(pin) nrf_gpio_pin_clear(pin)        /*!< Turns the given GPIO Low                    */
#define platformGpioToogle(pin) nrf_gpio_pin_toggle(pin)      /*!< Toogles the given GPIO                      */
#define platformGpioIsHigh(pin) (nrf_gpio_pin_read(pin) == 1) /*!< Checks if the given LED is High             */
#define platformGpioIsLow(pin) (!platformGpioIsHigh(pin))     /*!< Checks if the given LED is Low              */

#define platformTimerCreate(t) timerCalculateTimer(t)       /*!< Create a timer with the given time (ms)     */
#define platformTimerIsExpired(timer) timerIsExpired(timer) /*!< Checks if the given timer is expired        */
#define platformDelay(t) nrf_delay_ms(t)                    /*!< Performs a delay for the given time (ms)    */

extern uint32_t get_sysTick(void);
#define platformGetSysTick() get_sysTick() /*!< Get System Tick ( 1 tick = 1 ms)            */
extern void spi0_cs_enable(void);
extern void spi0_cs_disable(void);
#define platformSpiSelect() spi0_cs_enable()/*platformGpioClear(ST25R391X_SS_PIN) !< SPI SS\CS: Chip|Slave Select                */
#define platformSpiDeselect() spi0_cs_disable()/*platformGpioSet(ST25R391X_SS_PIN) !< SPI SS\CS: Chip|Slave Deselect              */
extern uint8_t nrf_spi_tx_rx(const uint8_t *txData, uint8_t *rxData, uint8_t len);
#define platformSpiTxRx(txBuf, rxBuf, len) nrf_spi_tx_rx(txBuf, rxBuf, len) /*!< SPI transceive                              */

#define platformI2CTx(txBuf, len)   /*!< I2C Transmit                                */
#define platformI2CRx(txBuf, len)   /*!< I2C Receive                                 */
#define platformI2CStart()          /*!< I2C Start condition                         */
#define platformI2CStop()           /*!< I2C Stop condition                          */
#define platformI2CRepeatStart()    /*!< I2C Repeat Start                            */
#define platformI2CSlaveAddrWR(add) /*!< I2C Slave address for Write operation       */
#define platformI2CSlaveAddrRD(add) /*!< I2C Slave address for Read operation        */

#define platformLog(...) NRF_LOG_INTERNAL_INFO(__VA_ARGS__) /*!< Log  method                                 */

/*
******************************************************************************
* RFAL FEATURES CONFIGURATION
******************************************************************************
*/

#define RFAL_FEATURE_NFCA true                  /*!< Enable/Disable RFAL support for NFC-A (ISO14443A)                         */
#define RFAL_FEATURE_NFCB true                  /*!< Enable/Disable RFAL support for NFC-B (ISO14443B)                         */
#define RFAL_FEATURE_NFCF true                  /*!< Enable/Disable RFAL support for NFC-F (FeliCa)                            */
#define RFAL_FEATURE_NFCV true                   /*!< Enable/Disable RFAL support for NFC-V (ISO15693)                          */
#define RFAL_FEATURE_T1T true                   /*!< Enable/Disable RFAL support for T1T (Topaz)                               */
#define RFAL_FEATURE_ST25TB true                /*!< Enable/Disable RFAL support for ST25TB                                    */
#define RFAL_FEATURE_DYNAMIC_ANALOG_CONFIG false /*!< Enable/Disable Analog Configs to be dynamically updated (RAM)             */
#define RFAL_FEATURE_DYNAMIC_POWER false         /*!< Enable/Disable RFAL dynamic power support                                 */
#define RFAL_FEATURE_ISO_DEP true               /*!< Enable/Disable RFAL support for ISO-DEP (ISO14443-4)                      */
#define RFAL_FEATURE_NFC_DEP true               /*!< Enable/Disable RFAL support for NFC-DEP (NFCIP1/P2P)                      */

#define RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN 256 /*!< ISO-DEP I-Block max length. Please use values as defined by rfalIsoDepFSx */
#define RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN 1024  /*!< ISO-DEP APDU max length. Please use multiples of I-Block max length       */

#endif /* PLATFORM_H */
