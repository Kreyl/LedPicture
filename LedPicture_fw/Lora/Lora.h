/*
 * Radio.h
 *
 *  Created on: 18 апр. 2020 г.
 *      Author: layst
 */

#pragma once

#include <inttypes.h>
#include "kl_lib.h"
#include "sx1276Regs-Fsk.h"
#include "sx1276Regs-LoRa.h"

#define SX_MAX_BAUDRATE_HZ  10000000
#define SX_RF_OUT_PABOOST   TRUE  // Set to TRUE if RF out is PA_BOOST, and to FALSE if RFO_HF

// When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
enum SXLoraBW_t : uint8_t {
    bwLora125kHz = 0b0111, // their ID: 7
    bwLora250kHz = 0b1000, // their ID: 8
    bwLora500kHz = 0b1001  // their ID: 9
};

enum SXCodingRate_t : uint8_t {
    coderate4s5 = 0b001,
    coderate4s6 = 0b010,
    coderate4s7 = 0b011,
    coderate4s8 = 0b100
};

enum SXSpreadingFactor_t : uint8_t {
    sprfact64chipsPersym = 6,
    sprfact128chipsPersym = 7,
    sprfact256chipsPersym = 8,
    sprfact512chipsPersym = 9,
    sprfact1024chipsPersym = 10,
    sprfact2048chipsPersym = 11,
    sprfact4096chipsPersym = 12
};

void SX_DIO0IrqHandler();

class Lora_t {
private:
    const PinIrq_t DIO0{SX_DIO0_GPIO, SX_DIO0_PIN, pudNone, SX_DIO0IrqHandler};
    void WriteReg(uint8_t RegAddr, uint8_t Value);
    uint8_t ReadReg (uint8_t RegAddr);
    void WriteFifo(uint8_t *ptr, uint8_t Sz);
    void ReadFifo(uint8_t *ptr, uint8_t Sz);
    // Inner use
//    virtual_timer_t ITmr;
    thread_reference_t ThdRef = nullptr;

    // Middle level
    void RxChainCalibration();
    void SetOpMode(uint8_t opMode);
    void SetTxPower(int8_t power);
    void SetLoraModem();
    // Modes
    void EnterStandby() { SetOpMode(RF_OPMODE_STANDBY); }
    void EnterSleep()   { SetOpMode(RF_OPMODE_SLEEP); }
public:
    struct {
        uint32_t Channel;
        SXLoraBW_t Bandwidth;
        bool IqInverted = false;
        bool FskFreqHopOn = false;
        uint8_t FreqHopPeriod = 0;
        ftVoidVoid RxCallback;
        int16_t SNR, RSSI;
    } Settings;

    void SetChannel(uint32_t freq);
    bool IsChannelFreeLora(uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime);
    void SetupTxConfigLora(int8_t power, SXLoraBW_t bandwidth,
            SXSpreadingFactor_t SpreadingFactor, SXCodingRate_t coderate,
            bool FixLen, uint16_t preambleLen);
    void SetupRxConfigLora(SXLoraBW_t bandwidth,
            SXSpreadingFactor_t SpreadingFactor, SXCodingRate_t coderate,
            uint16_t preambleLen, uint16_t symbTimeout,
            bool FixLen, uint8_t payloadLen);

    uint8_t Init();
    void TransmitByLora(uint8_t *ptr, uint8_t Sz);
    uint8_t ReceiveByLora(uint8_t *ptr, uint8_t Sz, uint32_t Timeout_ms);
    void PrintState();
    void PrintRegs();

    // Inner use
    void IIrqHandler();
};

extern Lora_t Lora;
