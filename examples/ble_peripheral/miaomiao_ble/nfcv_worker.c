#include "utils.h"
#include "rfal_rf.h"
#include "rfal_nfcv.h"
#include "rfal_nfcDep.h"
#include "rfal_isoDep.h"
#include "nfcv_worker.h"

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

bool PollNFCV(void)
{
    ReturnCode err;
    rfalNfcvListenDevice nfcvDev;
    bool found = false;
    uint8_t devCnt = 0;
    uint8_t rxBuf[128];
    uint16_t rcvLen;
    uint8_t blockAddress = 4;
    uint8_t blockDataLen = 4;
    uint8_t flagDefault = 2;
    uint8_t numOfBlocks = 3;

    /*******************************************************************************/
    /* ISO15693/NFC_V_PASSIVE_POLL_MODE                                            */
    /*******************************************************************************/

    rfalNfcvPollerInitialize(); /* Initialize for NFC-V */
    rfalFieldOnAndStartGT();    /* Turns the Field On if not already and start GT timer */

    err = rfalNfcvPollerCollisionResolution(1, &nfcvDev, &devCnt);
    if ((err == ERR_NONE) && (devCnt > 0))
    {
        /******************************************************/
        /* NFC-V card found                                   */
        /* NFCID/UID is contained in: invRes.UID */
        err = rfalNfvSelect(2, nfcvDev.InvRes.UID); /* select card */

        REVERSE_BYTES(nfcvDev.InvRes.UID, RFAL_NFCV_UID_LEN);

        found = true;

        platformLog("ISO15693/NFC-V card found. UID: %s\r\n", hex2Str(nfcvDev.InvRes.UID, RFAL_NFCV_UID_LEN));

        if (err == ERR_NONE)
        {
            err = rfalNfvReadSingleBlock(flagDefault, NULL, blockAddress, rxBuf, sizeof(rxBuf), &rcvLen); /* read card block */
            if (err == ERR_NONE)
            {
                platformLog("ISO15693/NFC-V card block 4 data: %s\r\n", hex2Str(&rxBuf[1], rcvLen - 1));
                rxBuf[2]++;
                rxBuf[1]--;
            }

            err = rfalNfvWriteSingleBlock(flagDefault, NULL, blockAddress, &rxBuf[1], blockDataLen); /* write card block */
        }

        err = rfalNfvReadMultipleBlocks(flagDefault, NULL, blockAddress, numOfBlocks - 1, rxBuf, sizeof(rxBuf), &rcvLen); /* Step 5 read card blocks */
        if (err == ERR_NONE)
        {
            platformLog("ISO15693/NFC-V card blocks  data: %s\r\n", hex2Str(&rxBuf[1], rcvLen - 1));
            rxBuf[4 + ((numOfBlocks - 1) << 2)]++;
            rxBuf[3 + ((numOfBlocks - 1) << 2)]--;
            rfalNfvWriteSingleBlock(flagDefault, NULL, blockAddress + numOfBlocks - 1, &rxBuf[1 + ((numOfBlocks - 1) << 2)], blockDataLen); /* Step 5 write card block */
        }
    }

    return found;
}
