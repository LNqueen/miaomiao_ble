#include "utils.h"
#include "rfal_rf.h"
#include "rfal_nfcv.h"
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

char hexStr[4][128];
uint8_t hexStrIdx = 0;

static bool doWakeUp = false;     /*!< by default do not perform Wake-Up               */
static uint8_t state = FIELD_OFF; /*!< Actual state, starting with RF field turned off */

bool PollNFCV(void);

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
        // if (doWakeUp)
        // {
        platformLog("Going to Wakeup mode.\r\n");

        rfalWakeUpModeStart(NULL);
        state = WAIT_WAKEUP;
        break;
        // }

        // NEXT_STATE();
        // break;

    case POLL_ACTIVE_TECH:
        platformDelay(40);
        NEXT_STATE();
        break;

    case POLL_PASSIV_TECH:
        found |= PollNFCV();

        platformDelay(300);
        // state = FIELD_OFF;
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
