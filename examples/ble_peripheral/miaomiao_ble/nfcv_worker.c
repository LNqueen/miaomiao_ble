#include "utils.h"
#include "rfal_rf.h"
#include "rfal_nfcv.h"
#include "rfal_nfcDep.h"
#include "rfal_isoDep.h"
#include "nfcv_worker.h"
#include "cus_drs.h"

#define FIELD_OFF 0
#define POLL_ACTIVE_TECH 1
#define POLL_PASSIV_TECH 2
#define WAIT_WAKEUP 3

/* macro to cycle through states */
#define NEXT_STATE()                 \
    {                                \
        state++;                     \
        state %= sizeof(stateArray); \
    }

static uint8_t stateArray[] = {FIELD_OFF,
                               POLL_ACTIVE_TECH,
                               POLL_PASSIV_TECH,
                               WAIT_WAKEUP};

static uint8_t NFCID3[] = {0x01, 0xFE, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
static uint8_t GB[] = {0x46, 0x66, 0x6d, 0x01, 0x01, 0x11, 0x02, 0x02, 0x07, 0x80, 0x03, 0x02, 0x00, 0x03, 0x04, 0x01, 0x32, 0x07, 0x01, 0x03};

/*! Receive buffers union, only one interface is used at a time                                                             */
static union
{
    rfalIsoDepDevice isoDepDev; /* ISO-DEP Device details                          */
    rfalNfcDepDevice nfcDepDev; /* NFC-DEP Device details                          */
} gDevProto;

char hexStr[4][128];
uint8_t hexStrIdx = 0;

static bool doWakeUp = false;     /*!< by default do not perform Wake-Up               */
static uint8_t state = FIELD_OFF; /*!< Actual state, starting with RF field turned off */

bool PollNFCV(void);
bool demoPollAP2P(void);

static char *hex2Str(unsigned char *data, size_t dataLen)
{
    unsigned char *pin = data;
    const char *hex = "0123456789ABCDEF";
    char *pout = hexStr[hexStrIdx];
    uint8_t i = 0;
    uint8_t idx = hexStrIdx;
    if (dataLen == 0)
    {
        pout[0] = 0;
    }
    else
    {
        for (; i < dataLen - 1; ++i)
        {
            *pout++ = hex[(*pin >> 4) & 0xF];
            *pout++ = hex[(*pin++) & 0xF];
        }
        *pout++ = hex[(*pin >> 4) & 0xF];
        *pout++ = hex[(*pin) & 0xF];
        *pout = 0;
    }

    hexStrIdx++;
    hexStrIdx %= 4;

    return hexStr[idx];
}

void workCycle(void)
{
    bool found = false;

    switch (stateArray[state])
    {
    case FIELD_OFF:
        rfalFieldOff();
        rfalWakeUpModeStop();
        platformDelay(300);

        /* If WakeUp is to be executed, enable Wake-Up mode */
        if (doWakeUp)
        {
            platformLog("Going to Wakeup mode.\r\n");

            rfalWakeUpModeStart(NULL);
            state = WAIT_WAKEUP;
            break;
        }
        NEXT_STATE();
        break;

    case POLL_ACTIVE_TECH:
        demoPollAP2P();
        platformDelay(40);
        NEXT_STATE();
        break;

    case POLL_PASSIV_TECH:
        found |= PollNFCV();

        platformDelay(300);
        state = FIELD_OFF;
        break;

    case WAIT_WAKEUP:

        /* Check if Wake-Up Mode has been awaked */
        if (rfalWakeUpModeHasWoke())
        {
            platformLog("start poll\r\n");
            /* If awake, go directly to Poll */
            rfalWakeUpModeStop();
            state = POLL_ACTIVE_TECH;
        }
        break;

    default:
        break;
    }
}
/*!
 *****************************************************************************
 * \brief Activate P2P
 *
 * Configures NFC-DEP layer and executes the NFC-DEP/P2P activation (ATR_REQ
 * and PSL_REQ if applicable)
 *
 * \param[in] nfcid      : nfcid to be used
 * \param[in] nfcidLen   : length of nfcid
 * \param[in] isActive   : Active or Passive communiccation
 * \param[out] nfcDepDev : If activation successful, device's Info
 *
 *  \return ERR_PARAM    : Invalid parameters
 *  \return ERR_TIMEOUT  : Timeout error
 *  \return ERR_FRAMING  : Framing error detected
 *  \return ERR_PROTO    : Protocol error detected
 *  \return ERR_NONE     : No error, activation successful
 *
 *****************************************************************************
 */
ReturnCode demoActivateP2P(uint8_t *nfcid, uint8_t nfidLen, bool isActive, rfalNfcDepDevice *nfcDepDev)
{
    rfalNfcDepAtrParam nfcDepParams;

    nfcDepParams.nfcid = nfcid;
    nfcDepParams.nfcidLen = nfidLen;
    nfcDepParams.BS = RFAL_NFCDEP_Bx_NO_HIGH_BR;
    nfcDepParams.BR = RFAL_NFCDEP_Bx_NO_HIGH_BR;
    nfcDepParams.LR = RFAL_NFCDEP_LR_254;
    nfcDepParams.DID = RFAL_NFCDEP_DID_NO;
    nfcDepParams.NAD = RFAL_NFCDEP_NAD_NO;
    nfcDepParams.GBLen = sizeof(GB);
    nfcDepParams.GB = GB;
    nfcDepParams.commMode = ((isActive) ? RFAL_NFCDEP_COMM_ACTIVE : RFAL_NFCDEP_COMM_PASSIVE);
    nfcDepParams.operParam = (RFAL_NFCDEP_OPER_FULL_MI_EN | RFAL_NFCDEP_OPER_EMPTY_DEP_DIS | RFAL_NFCDEP_OPER_ATN_EN | RFAL_NFCDEP_OPER_RTOX_REQ_EN);

    /* Initialize NFC-DEP protocol layer */
    rfalNfcDepInitialize();

    /* Handle NFC-DEP Activation (ATR_REQ and PSL_REQ if applicable) */
    return rfalNfcDepInitiatorHandleActivation(&nfcDepParams, RFAL_BR_424, nfcDepDev);
}

/*!
 *****************************************************************************
 * \brief Poll NFC-AP2P
 *
 * Configures the RFAL to AP2P communication and polls for a nearby
 * device. If a device is found turns On a LED and logs its UID.
 * If the Device supports NFC-DEP protocol (P2P) it will activate
 * the device and try to send an URI record.
 *
 * This methid first tries to establish communication at 424kb/s and if
 * failed, tries also at 106kb/s
 *
 *
 *  \return true    : AP2P device found
 *  \return false   : No device found
 *
 *****************************************************************************
 */
bool demoPollAP2P(void)
{
    ReturnCode err;
    bool try106 = false;

    while (!try106)
    {
        /*******************************************************************************/
        /* NFC_ACTIVE_POLL_MODE                                                        */
        /*******************************************************************************/
        /* Initialize RFAL as AP2P Initiator NFC BR 424 */
        err = rfalSetMode(RFAL_MODE_POLL_ACTIVE_P2P, ((try106) ? RFAL_BR_106 : RFAL_BR_424), ((try106) ? RFAL_BR_106 : RFAL_BR_424));

        rfalSetErrorHandling(RFAL_ERRORHANDLING_NFC);
        rfalSetFDTListen(RFAL_FDT_LISTEN_AP2P_POLLER);
        rfalSetFDTPoll(RFAL_TIMING_NONE);

        rfalSetGT(RFAL_GT_AP2P_ADJUSTED);
        err = rfalFieldOnAndStartGT();

        err = demoActivateP2P(NFCID3, RFAL_NFCDEP_NFCID3_LEN, true, &gDevProto.nfcDepDev);
        if (err == ERR_NONE)
        {
            /****************************************************************************/
            /* Active P2P device activated                                              */
            /* NFCID / UID is contained in : nfcDepDev.activation.Target.ATR_RES.NFCID3 */
            platformLog("NFC Active P2P device found. NFCID3: %s\r\n", hex2Str(gDevProto.nfcDepDev.activation.Target.ATR_RES.NFCID3, RFAL_NFCDEP_NFCID3_LEN));
            return true;
        }

        /* AP2P at 424kb/s didn't found any device, try at 106kb/s */
        try106 = true;
        rfalFieldOff();
    }

    return false;
}

S_sensor_t sensor_d;

// extern uint32_t libreLife;
// extern DrsRTPayload_s rt_payload;
// extern DrsHisPayload_s his_payload;
// extern uint32_t cur_TZ;
#define PLATFORM_LOG 0
void libre_data_parsing(void)
{
    sensor_d.oldNextTrend = sensor_d.nextTrend;
    sensor_d.nextTrend = sensor_d.fram[26];
    sensor_d.oldNextHistory = sensor_d.nextHistory;
    sensor_d.nextHistory = sensor_d.fram[27];

    // rt_payload.curSensorSta = sensor_d.fram[4];
#if PLATFORM_LOG
    platformLog("oldNTrend:0x%x\nnTrend:0x%x\noldNHistory:0x%x\nnHistory:0x%x\n", sensor_d.oldNextTrend, sensor_d.nextTrend, sensor_d.oldNextHistory, sensor_d.nextHistory);
#endif
    sensor_d.oldMinutesSinceStart = sensor_d.minutesSinceStart;
    sensor_d.minutesSinceStart = ((uint16_t)sensor_d.fram[317] << 8) + (uint16_t)sensor_d.fram[316]; // bytes swapped
    uint16_t minutesSinceLastReading = sensor_d.minutesSinceStart - sensor_d.oldMinutesSinceStart;
    // libreLife = (MAX_LIBRE_LIFE - sensor_d.minutesSinceStart * 60) > MAX_LIBRE_LIFE ? 0 : (MAX_LIBRE_LIFE - sensor_d.minutesSinceStart * 60); // min to sec
#if PLATFORM_LOG
    platformLog("oldmSinceStart:0x%x\nmSinceStart:0x%x\nmSinceLastRead:0x%x\n", sensor_d.oldMinutesSinceStart, sensor_d.minutesSinceStart, minutesSinceLastReading);
#endif
    int8_t trendDelta = sensor_d.nextTrend - sensor_d.oldNextTrend;
    int8_t trendsSinceLastReading = (trendDelta >= 0) ? trendDelta : trendDelta + 16; // compensate ring buffer
    if (minutesSinceLastReading > 15)
        trendsSinceLastReading = 16; // Read all 16 trend data if more than 15 minute have passed
#if PLATFORM_LOG
    platformLog("trdDelta:0x%x\ntrdSinceLastRead:0x%x\n", trendDelta, trendsSinceLastReading);
#endif
    if (trendsSinceLastReading > 0)
    {
        int firstTrend = sensor_d.oldNextTrend;
        int firstTrendByte = 28 + (6 * firstTrend);
        // Does not consider the ring buffer (this is considered later in the reading loop)
        int lastTrendByte = firstTrendByte + (6 * trendsSinceLastReading) - 1;

        uint8_t firstTrendBlock = firstTrendByte / 8; // integer division without remainder
        uint8_t lastTrendBlock = lastTrendByte / 8;   // integer division without remainder
#if PLATFORM_LOG
        platformLog("fstTrd:0x%x\nfstTrdByte:0x%x\nlstTrdByte:0x%x\nfstTrdBlock:0x%x\nlstTrdBlock:0x%x\n", firstTrend, firstTrendByte, lastTrendByte, firstTrendBlock, lastTrendBlock);
#endif
        // read the trend blocks that are new since last reading, but no more than 100 times to avoid endless loop in error case
        uint8_t trendBlock = firstTrendBlock;
        uint8_t count = 0;
        while (count <= 100)
        {
            if (trendBlock != 3)
            {
                // block 3 was read already and can be skipped
                // sensor_d.resultCodes[trendBlock] = readSingleBlock(trendBlock, maxTrials, RXBuffer, sensor_d.fram, SS_PIN);
            }
            if (trendBlock == lastTrendBlock)
                break;
            trendBlock++;
            if (trendBlock > 15)
                trendBlock = trendBlock - 13; // block 16 -> start over at block 3
            count++;
        }
    }

    int8_t historyDelta = sensor_d.nextHistory - sensor_d.oldNextHistory;
    int8_t historiesSinceLastReading = (historyDelta >= 0) ? historyDelta : historyDelta + 32; // compensate ring buffer
#if PLATFORM_LOG
    platformLog("hisDelta:0x%x\nhisSinceLstRead:0x%x\n", historyDelta, historiesSinceLastReading);
    //    platformLog("historiesSinceLastReading: 0x%x\n", historiesSinceLastReading);
#endif
    // if there is new trend data, read the new trend data
    if (historiesSinceLastReading > 0)
    {
        int firstHistory = sensor_d.oldNextHistory;
        int firstHistoryByte = 124 + (6 * firstHistory);
        // Does not consider the ring buffer (this is considered later in the reading loop)
        int lastHistoryByte = firstHistoryByte + (6 * historiesSinceLastReading) - 1;

        uint8_t firstHistoryBlock = firstHistoryByte / 8; // integer division without remainder
        uint8_t lastHistoryBlock = lastHistoryByte / 8;   // integer division without remainder
#if PLATFORM_LOG
        platformLog("fstHis:0x%x\nfstHisByte:0x%x\nlstHisByte:0x%x\nfstHisBlock:0x%x\nlstHisBlock:0x%x\n", firstHistory, firstHistoryByte, lastHistoryByte, firstHistoryBlock, lastHistoryBlock);
#endif
        // read the history blocks that are new since last reading, but no more than 100 times
        uint8_t historyBlock = firstHistoryBlock;
        uint8_t count = 0;
        while (count <= 100)
        {
            if (historyBlock != 39)
            {
                // block 39 was read already and can be skipped
                //                sensor_d.resultCodes[historyBlock] = readSingleBlock(historyBlock, maxTrials, RXBuffer, sensor_d.fram, SS_PIN);
            }
            if (historyBlock == lastHistoryBlock)
                break;
            historyBlock++;
            if (historyBlock > 39)
                historyBlock = historyBlock - 25; // block 40 -> start over at block 15
            count++;
        }
    }

    for (int i = 0; i < 15; i++)
    {
        uint8_t index = 4 + (14 - i) * 6;
        // var index = 4 + (nextTrendBlock - 1 - blockIndex) * 6 // runs backwards
        if (index < 4)
        {
            index += 96;
        }
        uint16_t glucose = (((uint16_t)sensor_d.fram[index + 25]) << 8) | sensor_d.fram[index + 24];

        // if (i == 0)
        // {
        //     rt_payload.data.bloodGluose = glucose;
        //     rt_payload.data.timeStamp = cur_TZ - 60 * i;
        //     rt_payload.data.libreLife = libreLife + 60 * i;
        // }

        // let measurementDate = date.addingTimeInterval(Double(-60 * blockIndex))
#if PLATFORM_LOG
        platformLog("trdrange:%d~%d,time:%d,glucose:%d\n", index + 24, index + 30, -60 * i, glucose);
#endif
    }
    for (int i = 0; i < 32; i++)
    {
        uint16_t index = 100 + (31 - i) * 6;
        // var index = 100 + (nextHistoryBlock - 1 - blockIndex) * 6 // runs backwards
        if (index < 100)
        {
            index += 192;
        }
        uint16_t glucose = (((uint16_t)sensor_d.fram[index + 25]) << 8) | sensor_d.fram[index + 24];
        //        his_payload.data[i].bloodGluose = glucose;
        //        his_payload.data[i].timeStamp = cur_TZ - 900 * i;
        //        his_payload.data[i].libreLife = libreLife + 900 * i;
        //
        // let measurementDate = mostRecentHistoryDate.addingTimeInterval(Double(-900 * blockIndex)) // 900 = 60 * 15
#if PLATFORM_LOG
        platformLog("hisrange:%d~%d,time:%d,glucose:%d\n", index + 24, index + 30, -900 * i, glucose);
#endif
    }
    //    his_payload.totalDataNumber = 32;
}

static uint8_t read_flag = 0;
uint32_t last_count = 0;
extern uint32_t my_systick;
uint8_t sensor_state = 0;
bool PollNFCV(void)
{
    ReturnCode err;
    rfalNfcvListenDevice nfcvDev;
    bool found = false;
    uint8_t devCnt = 0;
    uint8_t rxBuf[128];
    uint16_t rcvLen;
    uint8_t blockAddress = 0;
    uint8_t flagDefault = 2;

    /*******************************************************************************/
    /* ISO15693/NFC_V_PASSIVE_POLL_MODE                                            */
    /*******************************************************************************/

    rfalNfcvPollerInitialize(); /* Initialize for NFC-V */
    rfalFieldOnAndStartGT();    /* Turns the Field On if not already and start GT timer */

    err = rfalNfcvPollerCollisionResolution(1, &nfcvDev, &devCnt);
    if ((err == ERR_NONE) && (devCnt > 0))
    {
        sensor_state = 1;
        if (read_flag == 0)
        {
            read_flag = 1;
            last_count = my_systick;
            /******************************************************/
            /* NFC-V card found                                   */
            /* NFCID/UID is contained in: invRes.UID */
            err = rfalNfvSelect(2, nfcvDev.InvRes.UID); /* select card */

            REVERSE_BYTES(nfcvDev.InvRes.UID, RFAL_NFCV_UID_LEN);

            found = true;

            platformLog("ISO15693/NFC-V card found. UID: %s\r\n", hex2Str(nfcvDev.InvRes.UID, RFAL_NFCV_UID_LEN));

            memcpy(sensor_d.oldUid, sensor_d.uid, sizeof(sensor_d.uid));
            memcpy(sensor_d.uid, nfcvDev.InvRes.UID, sizeof(nfcvDev.InvRes.UID));

            if (err == ERR_NONE)
            {
                for (int i = 0; i < 43; i++)
                {
                    err = rfalNfvReadSingleBlock(flagDefault, NULL, blockAddress + i, rxBuf, sizeof(rxBuf), &rcvLen); /* read card block */
                    if (err == ERR_NONE)
                    {
                        //                    platformLog("ISO15693/NFC-V card block[%d]: %s\r\n", i,hex2Str(&rxBuf[1], rcvLen-1));
                        for (int j = 0; j < 8; j++)
                        {
                            sensor_d.fram[i * 8 + j] = rxBuf[j + 1];
                        }
                    }
                }

                libre_data_parsing();
            }
        }
    }
    else
    {
        read_flag = 0;
        sensor_state = 0;
    }

    if (my_systick - last_count > 5 * 60 * 1000)
    {
        read_flag = 0;
    }

    return found;
}
