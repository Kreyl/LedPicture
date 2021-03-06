#if 1 // ============================ Includes =================================
#include "hal.h"
#include "MsgQ.h"
#include "shell.h"
#include "led.h"
//#include "usb_msd.h"
#include "Sequences.h"
#include "kl_i2c.h"
#include "radio_lvl1.h"
#include "ws2812b.h"
#endif
#if 1 // ======================== Variables & prototypes =======================
// Forever
bool OsIsInitialized = false;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{CmdUartParams};
void OnCmd(Shell_t *PShell);
void ITask();

LedSmooth_t Led{LED_PIN};
PinInput_t PinUsbDetect(USB_DETECT_PIN, pudPullDown);
PinOutput_t PwrEn(PWR_EN);

static const NeopixelParams_t LedParams(NPX_SPI, NPX_GPIO, NPX_PIN, NPX_AF,
        NPX_DMA, NPX_DMA_MODE(NPX_DMA_CHNL),
        NPX_LED_CNT,
        npxRGB);
Neopixels_t Npx(&LedParams);

#endif

void SetPix(uint8_t x, uint8_t y, Color_t Clr) {
    uint32_t Indx = 0;
    if     (x <=  9 and y <= 13) Indx = y * 10 + x;
    else if(x >= 10 and y <= 13) Indx = y * 10 + x + 130;
    else if(x <=  9 and y >= 14) Indx = y * 10 + x + 140;
    else if(x >= 10 and y >= 14) Indx = y * 10 + x + 270;
    Npx.ClrBuf[Indx] = Clr;
}


int main(void) {
    // Start Watchdog. Will reset in main thread by periodic 1 sec events.
//    Iwdg::InitAndStart(4500);
//    Iwdg::DisableInDebug();

#if 1 // ==== Iwdg, Clk, Os, EvtQ, Uart ====
    // Setup clock frequency
    Clk.SetupPllSrc(pllsrcHse);
    Clk.SetCoreClk(cclk48MHz);
    // 48MHz clock
    Clk.SetupSai1Qas48MhzSrc();
    Clk.UpdateFreqValues();
    // Init OS
    halInit();
    chSysInit();
    OsIsInitialized = true;

    // ==== Init hardware ====
    EvtQMain.Init();
    Uart.Init();
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();
#endif
    Led.Init();

    PwrEn.Init();
    PwrEn.SetHi();
//    PinUsbDetect.Init();

//    i2c3.Init();

    Radio.Init();

    Npx.Init();
    Npx.SetAll(clBlack);
    Npx.SetCurrentColors();

    Led.StartOrRestart(lsqOk);
    // UsbMsd.Init();

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdShellCmdRcvd:
                while(((CmdUart_t*)Msg.Ptr)->TryParseRxBuff() == retvOk) OnCmd((Shell_t*)((CmdUart_t*)Msg.Ptr));
                break;

            case evtIdEverySecond:
//                Printf("Second\r");
                Iwdg::Reload();
                break;

            case evtIdNewPix:
                Printf("NewPix: %u %u; %u %u %u\r", Msg.x, Msg.y, Msg.R, Msg.G, Msg.B);
                if(Msg.x == 0xFF) Npx.SetAll(Color_t(Msg.R, Msg.G, Msg.B));
                else SetPix(Msg.x, Msg.y, Color_t(Msg.R, Msg.G, Msg.B));
                Npx.SetCurrentColors();
                break;

#if 0 // ======= USB =======
            case evtIdUsbConnect:
                Printf("USB connect\r");
                Resume();
                UsbConnected = true;
                UsbMsd.Connect();
                Charger.Enable();
                break;
            case evtIdUsbDisconnect:
                Standby();
                Printf("USB disconnect\r");
                UsbConnected = false;
                UsbMsd.Disconnect();
                break;
            case evtIdUsbReady:
                Printf("USB ready\r");
                break;
#endif
            default: break;
        } // switch
    } // while true
}




#if 1 // ======================= Command processing ============================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ok();
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    else if(PCmd->NameIs("mem")) PrintMemoryInfo();

    else if(PCmd->NameIs("TX")) {
        uint8_t Pwr, BWIndx, SFIndx, CRIndx;
        if(PCmd->GetParams<uint8_t>(4, &Pwr, &BWIndx, &SFIndx, &CRIndx) == retvOk) {
            Radio.SetupTxConfigLora(Pwr, BWIndx, SFIndx, CRIndx);
            uint8_t b, Cnt=0;
            while(PCmd->GetNext<uint8_t>(&b) == retvOk) {
                Radio.TxBuf[Cnt++] = b;
                if(Cnt == LORA_FIFO_SZ) break;
            }
            if(Cnt) {
                Radio.TransmitBuf(Cnt);
                PShell->Ok();
            }
            else PShell->CmdError();
        }
        else PShell->CmdError();
    }

//    else if(PCmd->NameIs("RX")) {
//        uint8_t Try[LORA_FIFO_SZ] = {7};
//        uint8_t Len, BWIndx, SFIndx, CRIndx;
//        uint32_t Timeout_ms;
//        if(PCmd->GetParams<uint8_t>(4, &Len, &BWIndx, &SFIndx, &CRIndx) == retvOk) {
//            Lora.SetupRxConfigLora(bw[BWIndx], sf[SFIndx], CR[CRIndx], hdrmodeExplicit, Len);
//            if(PCmd->GetNext<uint32_t>(&Timeout_ms) == retvOk) {
//                uint8_t Rslt = Lora.ReceiveByLora(Try, &Len, Timeout_ms);
//                if(Rslt == retvOk) {
//                    Printf("SNR: %d; RSSI: %d; Len: %u\r", Lora.RxParams.SNR, Lora.RxParams.RSSI, Len);
//                    Printf("%A\r\n", Try, Len, ' ');
//                }
//                else if(Rslt == retvTimeout) Printf("Timeout\r");
//                else Printf("CRC Err\r");
//            }
//            else PShell->CmdError();
//        }
//        else PShell->CmdError();
//    }
    else if(PCmd->NameIs("Regs")) Lora.PrintRegs();
    else if(PCmd->NameIs("Sta")) Lora.PrintState();

    else if(PCmd->NameIs("SetAll")) {
        Color_t Clr;
        if(PCmd->GetClrRGB(&Clr) == retvOk) {
            Npx.SetAll(Clr);
            Npx.SetCurrentColors();
            PShell->Ok();
        }
        else PShell->CmdError();
    }

    else if(PCmd->NameIs("SetPix")) {
        uint8_t x, y;
        Color_t Clr;
        if(PCmd->GetParams<uint8_t>(5, &x, &y, &Clr.R, &Clr.G, &Clr.B) == retvOk) {
            SetPix(x, y, Clr);
            Npx.SetCurrentColors();
        }
        else PShell->CmdError();
    }

    else PShell->CmdUnknown();
}
#endif
