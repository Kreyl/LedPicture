#if 1 // ============================ Includes =================================
#include "hal.h"
#include "MsgQ.h"
#include "shell.h"
#include "led.h"
//#include "usb_msd.h"
#include "Sequences.h"
#include "kl_i2c.h"
#include "Lora.h"
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
#endif

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

    Lora.Init();
    Lora.SetChannel(868000000);
    Lora.SetupTxConfigLora(2, bwLora125kHz, sprfact64chipsPersym, coderate4s5, true);
    Lora.SetupRxConfigLora(bwLora125kHz, sprfact64chipsPersym, coderate4s5, true, 9);

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
        uint8_t Try[10] = {1,2,3,4,5,6,7,8,9, 0};
        Lora.TransmitByLora(Try, 10);
        PShell->Ok();
    }

    else if(PCmd->NameIs("RX")) {
        uint32_t Timeout_ms;
        uint8_t Try[9] = {0};
        if(PCmd->GetNext<uint32_t>(&Timeout_ms) == retvOk) {
            uint8_t Rslt = Lora.ReceiveByLora(Try, 9, Timeout_ms);
            if(Rslt == retvOk) {
                Printf("SNR: %d; RSSI: %d\r", Lora.RxParams.SNR, Lora.RxParams.RSSI);
                Printf("%A\r\n", Try, 9, ' ');
            }
            else if(Rslt == retvTimeout) Printf("Timeout\r");
            else Printf("CRC Err\r");
        }
        else PShell->CmdError();
    }


    else if(PCmd->NameIs("Regs")) Lora.PrintRegs();

    else if(PCmd->NameIs("Sta")) Lora.PrintState();

    else PShell->CmdUnknown();
}
#endif
