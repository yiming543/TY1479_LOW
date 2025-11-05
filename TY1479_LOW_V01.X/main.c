/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules
  selected in the GUI. Generation Information : Product Revision  :  PIC10 /
  PIC12 / PIC16 / PIC18 MCUs - 1.81.8 Device            :  PIC16F1936 Driver
  Version    :  2.00
 */

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software
   and any derivatives exclusively with Microchip products. It is your
   responsibility to comply with third party license terms applicable to your
   use of third party software (including open source software) that may
   accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
 */
//**********************************************************************************
//                        PIC16F1936 Pinout for this example
//                                  ------------
//                             MCLR |1       28| RB7 ->
//                              RA0 |2       27| RB6 ->
//                          ->  RA1 |3       26| RB5 ->
//                    HB_EN ->  RA2 |4       25| RB4 ->
//                          ->  RA3 |5       24| RB3 ->
//                          ->  RA4 |6       23| RB2 ->
//                          ->  RA5 |7       22| RB1 ->
//                              GND |8       21| RB0 -> LB_EN (CCP4)
//                          ->  RA7 |9       20| VDD
//                          ->  RA6 |10      19| VSS
//                          ->  RC0 |11      18| RC7
//                          ->  RC1 |12      17| RC6 -> ECCP3 IN
//                   POS_EN ->  RC2 |13      16| RC5 ->
//                          ->  RC3 |14      15| RC4 ->
//                                  ------------
//
//***********************************************************************************
// 20251031 V04 CS:ED41
// 不用柯松資料
// 抓到所有資料(包括車子沒有的信號)
// 修正不符合KD件的亮燈方式

//20251105 V01 CS:DCCF
//共用高配PCB
//畫行，近/遠燈共用光源
//畫行 40%，近燈 100%
//沒有P2(RA1) 
//CCP1不用PWM，改成GPIO
//不使用LampSideSelect(RB3)
//處理所有編譯時警告訊息
//HB_EN RA0，改RA2
//RB0 ，改CCP4(PWM)
//POS RA6取消，改RC2

#include "mcc_generated_files/mcc.h"

void POS_ON(void);
void POS_OFF(void);
void LoBeam_ON(void);
void LoBeam_OFF(void);
void DRL_ON(void);
void LED_output(void);
void check_input(void);
void getFrameData(void);

typedef union {
  struct {
    _Bool b0 : 1;
    _Bool b1 : 1;
    _Bool POS : 1;
    _Bool DRL : 1;
    _Bool LoBeam : 1;
    _Bool HiBeam : 1;
    _Bool b6 : 1;
    _Bool b7 : 1;
  };
  uint8_t Byte;
} flag_D2;
typedef union {
  struct {
    _Bool b0 : 1;
    _Bool b1 : 1;
    _Bool b2 : 1;
    _Bool b3 : 1;
    _Bool b4 : 1;
    _Bool turnLight_L : 1;
    _Bool turnLight_R : 1;
    _Bool b7 : 1;
  };
  uint8_t Byte;
} flag_D3;
typedef union {
  struct {
    _Bool turnLight_R : 1;
    _Bool turnLight_L : 1;
    _Bool HiBeam : 1;
    _Bool LoBeam : 1;
    _Bool DRL : 1;
    _Bool POS : 1;
    _Bool b6 : 1;
    _Bool b7 : 1;
  };
  uint8_t Byte;
} flag_CS_index;

uint8_t rx_data = 0;
volatile bool data_1byte_OK = 0;
bool fFrame_OK = 0;
uint8_t data_buf[11];
uint8_t step = 0;

flag_D2 D2;
flag_D3 D3;
flag_CS_index csFlag;

volatile uint8_t T10MS_CNT = 0; // 單位:10ms
bool fException = 0;
bool fException2 = 0;
bool fLampSide = 0;

#define LOGIC_LEVEL (16)
#define HiBeam_ON() HB_EN_SetHigh() // active-high
#define HiBeam_OFF() HB_EN_SetLow()

#define POS_EN_ON() POS_EN_SetHigh()
#define POS_EN_OFF() POS_EN_SetLow()

#define PWM_DUTY_100  ((201<<2))
#define PWM_DUTY_40   ((81<<2))
#define PWM_DUTY_0    0

// #define LoBeam_OFF() DRL_OFF

void LoBeam_ON(void) {
  //CCPR4L = 201;
  PWM4_LoadDutyValue(PWM_DUTY_100);
}

void DRL_ON(void) {
  //CCPR4L = 81;
  PWM4_LoadDutyValue(PWM_DUTY_40);
}

void LoBeam_OFF(void) {
  //CCPR4L =0;
  PWM4_LoadDutyValue(PWM_DUTY_0);
}
static uint8_t CS[64] = {
    // 行車,晝行,近燈,遠燈,左方,右方
    0x4C, // 000000 關
    0x9F, // 000001 右方
    0xED, // 000010 左方
    0x3E, // 000011 右方+左方(警示)

    // 0x9A, // 000100 遠燈 (KOSO)
    0x66, // 000100 遠燈
    0xB5, // 000101 遠燈+右方
    0xC7, // 000110 遠燈+左方
    0x14, // 000111 遠燈+右方+左方(警示)

    //     0x27, // 001000 近燈 (KOSO)
    0x59, // 001000 近燈
    0x8A, // 001001 近燈+右方
    0xF8, // 001010 近燈+左方
    0x2B, // 001011 近燈+右方+左方(警示)

    //    0xF1, // 001100 遠燈+近燈(超車) (KOSO)
    0x73, // 001100 遠燈+近燈(超車)
    0xA0, // 001101 遠燈+近燈(超車)+右方
    0xD2, // 001110 遠燈+近燈(超車)+左方
    0x01, // 001111 遠燈+近燈(超車)+右方+左方(警示)

    // 0xB1, // 010000 晝行 (KOSO)
    0x8E, // 010000 晝行
    0x5D, // 010001 晝行+右方
    0x2F, // 010010 晝行+左方
    0xFC, // 010011 晝行+右方+左方(警示)

    //    0x67, // 010100 晝行+遠燈 (KOSO)
    0xA4, // 010100 晝行+遠燈
    0x77, // 010101 晝行+遠燈+右方
    0x05, // 010110 晝行+遠燈+左方
    0xD6, // 010111 晝行+遠燈+右方+左方(警示)

    //    0xDA, // 011000 晝行+近燈 (KOSO)
    0x9B, // 011000 晝行+近燈
    0x48, // 011001 晝行+近燈+右方
    0x3A, // 011010 晝行+近燈+左方
    0xE9, // 011011 晝行+近燈+右方+左方(警示)

    //    0x0C, // 011100 畫行+遠燈+近燈(超車) (KOSO)
    0xB1, // 011100 畫行+遠燈+近燈(超車)
    0x62, // 011101 畫行+遠燈+近燈(超車)+右方
    0x10, // 011110 畫行+遠燈+近燈(超車)+左方
    0xC3, // 011111 晝行+遠燈+近燈(超車)+右方+左方(警示)

    // 0xFA, // 100000 行車 (KOSO)
    0x2D, // 100000 行車
    0xFE, // 100001 行車+右方
    0x8C, // 100010 行車+左方
    0x5F, // 100011 行車+右方+左方(警示)

    //    0x2C, // 100100 行車+遠燈 (KOSO)
    0x07, // 100100 行車+遠燈
    0xD4, // 100101 行車+遠燈+右方
    0xA6, // 100110 行車+遠燈+左方
    0x75, // 100111 行車+遠燈+右方+左方(警示)

    // 0x91, // 101000 行車+近燈 (KOSO)
    0x38, // 101000 行車+近燈
    0xEB, // 101001 行車+近燈+右方
    0x99, // 101010 行車+近燈+左方
    0x4A, // 101011 行車+近燈+右方+左方(警示)

    //    0x47, // 101100 行車+遠燈+近燈(超車) (KOSO)
    0x12, // 101100 行車+遠燈+近燈(超車)
    0xC1, // 101101 行車+遠燈+近燈(超車)+右方
    0xB3, // 101110 行車+遠燈+近燈(超車)+左方
    0x60, // 101111 行車+遠燈+近燈(超車)+右方+左方(警示)

    // 0x07, // 110000 行車+晝行 (KOSO)
    0xEF, // 110000 行車+晝行
    0x3C, // 110001 行車+晝行+右方
    0x4E, // 110010 行車+晝行+左方
    0x9D, // 110011 行車+晝行+右方+左方(警示)

    //    0xD1, // 110100 行車+晝行+遠燈 (KOSO)
    0xC5, // 110100 行車+晝行+遠燈
    0x16, // 110101 行車+晝行+遠燈+右方
    0x64, // 110110 行車+晝行+遠燈+左方
    0xB7, // 110111 行車+晝行+遠燈+右方+左方(警示)

    //    0x6C, // 111000 行車+晝行+近燈 (KOSO)
    0xFA, // 111000 行車+晝行+近燈
    0x29, // 111001 行車+晝行+近燈+右方
    0x5B, // 111010 行車+晝行+近燈+左方
    0x88, // 111011 行車+晝行+近燈+右方+左方(警示)

    //    0xBA, // 111100 行車+晝行+遠燈+近燈 (KOSO)
    0xD0, // 111100 行車+晝行+遠燈+近燈
    0x03, // 111101 行車+晝行+遠燈+近燈+右方
    0x71, // 111110 行車+晝行+遠燈+近燈+左方
    0xA2, // 111111 行車+晝行+遠燈+近燈(超車)+右方+左方(警示)
};

// 特殊信號(unlock) 亮P2
void Exception_handling(void) {
  if ((data_buf[2] == 6) && (data_buf[10] == 0xD5)) {
    // P2_ON(); // P2亮
    fException = 1;
    // EPWM1_LoadDutyValue(PWM_DUTY_0_PERCENT); // 0%
    LoBeam_OFF();  // 遠燈 OFF
    HiBeam_OFF();  // 遠燈 OFF
    POS_ON();      // 亮POS
    T10MS_CNT = 0; // 重置2000ms計數器
  } else {
    fException = 0;
  }
}

void POS_ON(void) {
  POS_EN_ON();
}

void POS_OFF(void) {
  POS_EN_OFF();
}

void LED_output(void) {
  uint8_t crc8 = CS[csFlag.Byte];
  // CRC檢查
  if (data_buf[10] != crc8)
    return;
  if (data_buf[0] != 0xC8)
    return; // CRC檢查
  if (data_buf[1] != 0x8B)
    return; // CRC檢查

  T10MS_CNT = 0; // 重置2000ms計數器

  if (csFlag.HiBeam == 0 && csFlag.LoBeam == 0 && csFlag.DRL == 0 && csFlag.POS == 0) { // 0
    // ALL OFF
    HiBeam_OFF(); // 遠燈 OFF
    LoBeam_OFF(); // 近燈 OFF
    POS_OFF();    // 晝行 OFF
  } else if (csFlag.HiBeam == 0 && csFlag.LoBeam == 0 && csFlag.DRL == 0 && csFlag.POS == 1) { // 1
    // POS
    HiBeam_OFF(); // 遠燈 OFF
    LoBeam_OFF(); // 近燈 OFF
    POS_ON();     // POS ON
  } else if (csFlag.HiBeam == 0 && csFlag.LoBeam == 0 && csFlag.DRL == 1 && csFlag.POS == 0) { // 2
    // DRL
    HiBeam_OFF(); // 遠燈 OFF
    // LoBeam_OFF(); // 近燈 OFF
    DRL_ON();     // 晝行 ON
    POS_OFF();    // 晝行 OFF
  } else if (csFlag.HiBeam == 0 && csFlag.LoBeam == 0 && csFlag.DRL == 1 && csFlag.POS == 1) { // 3
    // DRL+POS
    HiBeam_OFF(); // 遠燈 OFF
    // LoBeam_OFF(); // 近燈 OFF
    DRL_ON();     // DRL ON
    POS_ON();     // POS ON
  } else if (csFlag.HiBeam == 0 && csFlag.LoBeam == 1 && csFlag.DRL == 0 && csFlag.POS == 0) { // 4
    // LOBEAM
    HiBeam_OFF(); // 遠燈 OFF
    LoBeam_ON();  // 近燈 ON
    POS_OFF();    // 晝行 OFF
  } else if (csFlag.HiBeam == 0 && csFlag.LoBeam == 1 && csFlag.DRL == 0 && csFlag.POS == 1) { // 5
    // LOBEAM + POS
    HiBeam_OFF(); // 遠燈 OFF
    LoBeam_ON();  // 近燈 ON
    POS_ON();     // POS ON
  } else if (csFlag.HiBeam == 0 && csFlag.LoBeam == 1 && csFlag.DRL == 1 && csFlag.POS == 0) { // 6
    // LOBEAM + DRL
    HiBeam_OFF(); // 遠燈 OFF
    LoBeam_ON();  // 近燈 ON
    POS_OFF();    // 晝行 OFF
  } else if (csFlag.HiBeam == 0 && csFlag.LoBeam == 1 && csFlag.DRL == 1 && csFlag.POS == 1) { // 7
    // LOBEAM + DRL+POS
    HiBeam_OFF(); // 遠燈 OFF
    LoBeam_ON();  // 近燈 ON
    POS_ON();     // POS ON
  } else if (csFlag.HiBeam == 1 && csFlag.LoBeam == 0 && csFlag.DRL == 0 && csFlag.POS == 0) { // 8
    // HiBeam
    HiBeam_ON();  // 遠燈 ON
    LoBeam_OFF(); // 近燈 OFF
    POS_OFF();    // 晝行 OFF
  } else if (csFlag.HiBeam == 1 && csFlag.LoBeam == 0 && csFlag.DRL == 0 && csFlag.POS == 1) { // 9
    // HIBeam + POS
    HiBeam_OFF(); // 遠燈 OFF
    LoBeam_OFF(); // 近燈 OFF
    POS_ON();     // POS ON
  } else if (csFlag.HiBeam == 1 && csFlag.LoBeam == 0 && csFlag.DRL == 1 && csFlag.POS == 0) { // 10
    // HIBeam + DRL
    HiBeam_OFF(); // 遠燈 OFF
    LoBeam_OFF(); // 近燈 OFF
    DRL_ON();     // 晝行 ON
  } else if (csFlag.HiBeam == 1 && csFlag.LoBeam == 0 && csFlag.DRL == 1 && csFlag.POS == 1) { // 11
    // HIBeam + DRL+POS
    HiBeam_OFF(); // 遠燈 OFF
    DRL_ON();     // 晝行 ON
    POS_ON();     // POS ON
  } else if (csFlag.HiBeam == 1 && csFlag.LoBeam == 1 && csFlag.DRL == 0 && csFlag.POS == 0) { // 12
    // HIBeam + LOBEAM
    HiBeam_ON();  // 遠燈 ON
    LoBeam_ON();  // 近燈 ON
    POS_OFF();    // POS OFF
  } else if (csFlag.HiBeam == 1 && csFlag.LoBeam == 1 && csFlag.DRL == 0 && csFlag.POS == 1) { // 13
    // HIBeam + LOBEAM+POS
    HiBeam_ON();  // 遠燈 ON
    LoBeam_ON();  // 近燈 ON
    POS_ON();     // POS ON
  } else if (csFlag.HiBeam == 1 && csFlag.LoBeam == 1 && csFlag.DRL == 1 && csFlag.POS == 0) { // 14
    // HIBeam + LOBEAM+DRL
    HiBeam_ON();  // 遠燈 ON
    LoBeam_ON();  // 近燈 ON
    POS_OFF();    // POS OFF
  } else if (csFlag.HiBeam == 1 && csFlag.LoBeam == 1 && csFlag.DRL == 1 && csFlag.POS == 1) { // 15
    // HIBeam + LOBEAM+DRL+POS
    HiBeam_ON();  // 遠燈 ON
    LoBeam_ON();  // 近燈 ON
    POS_ON();     // POS ON
  }

  for (uint8_t i = 0; i < 11; i++) {
    data_buf[i] = 0; // 清除資料緩衝區
  }

}

void check_input(void) {
  D2.Byte = data_buf[2];
  D3.Byte = data_buf[3];
  csFlag.Byte = 0;

  // 行車
  if (D2.POS == 1)
    csFlag.POS = 1;
  else
    csFlag.POS = 0;

  // 晝行
  if (D2.DRL == 1)
    csFlag.DRL = 1;
  else
    csFlag.DRL = 0;

  // 近燈
  if (D2.LoBeam == 1)
    csFlag.LoBeam = 1;
  else
    csFlag.LoBeam = 0;

  // 遠燈
  if (D2.HiBeam == 1)
    csFlag.HiBeam = 1;
  else
    csFlag.HiBeam = 0;

  // 左方向
  if (D3.turnLight_L == 1)
    csFlag.turnLight_L = 1;
  else
    csFlag.turnLight_L = 0;

  // 右方向
  if (D3.turnLight_R == 1)
    csFlag.turnLight_R = 1;
  else
    csFlag.turnLight_R = 0;
}

void getFrameData(void) {
  if (data_1byte_OK == 1) {
    data_1byte_OK = 0;

    switch (step) {
    case 0:
      if (rx_data == 0xC8) {
        step = 1;
        data_buf[0] = rx_data;
      } else {
        step = 0;
      }
      break;
    case 1:
      if (rx_data == 0x8B) {
        data_buf[1] = rx_data;
        step = 2;
      } else {
        step = 0;
      }
      break;
    case 2:
      data_buf[2] = rx_data;
      step = 3;
      break;
    case 3:
      data_buf[3] = rx_data;
      step = 4;
      break;
    case 4:
      data_buf[4] = rx_data;
      step = 5;
      break;
    case 5:
      data_buf[5] = rx_data;
      step = 6;
      break;
    case 6:
      data_buf[6] = rx_data;
      step = 7;
      break;
    case 7:
      data_buf[7] = rx_data;
      step = 8;
      break;
    case 8:
      data_buf[8] = rx_data;
      step = 9;
      break;
    case 9:
      data_buf[9] = rx_data;
      step = 10;
      break;
    case 10:
      data_buf[10] = rx_data;
      fFrame_OK = 1;
      step = 0;
      break;
    default:
      step = 0;
      break;
    }
  }
}

void ECCP3_CallBack(uint16_t capturedValue) {
  static uint16_t pluse_width_LO = 0;
  static uint16_t pluse_width_HI = 0;
  static uint8_t HI_us = 0;
  static uint8_t LO_us = 0;
  static uint8_t diff_us = 0;
  static uint16_t falling_edge_time = 0;
  static uint16_t rising_edge_time = 0;
  static bool rise_edge_flag = 0;
  static bool fHead = 0;
  static uint8_t data_cnt = 0;

  // falling edge tringgered
  if (CCP3CON == 0x04) {
    falling_edge_time = capturedValue;
    CCP3CON = 0x05; // set to rising edge trigger
    if (rise_edge_flag == 1) {
      rise_edge_flag = 0;

      if (falling_edge_time >= rising_edge_time) {
        pluse_width_HI = falling_edge_time - rising_edge_time;
      } else {
        pluse_width_HI = (0xffff - rising_edge_time) + falling_edge_time;
      }
      LO_us = (pluse_width_LO >> 3) & 0xff;
      HI_us = (pluse_width_HI >> 3) & 0xff;
      if (HI_us > LO_us)
        diff_us = HI_us - LO_us;
      else
        diff_us = LO_us - HI_us;

      if (fHead == 0) {
        UART_TX_SetHigh();
        // init
        if (diff_us < LOGIC_LEVEL) {
          // logical '0'
          fHead = 1;
          data_cnt = 0;
          UART_TX_SetLow();
        }
      } else {
        // data
        if (diff_us < LOGIC_LEVEL) {
          // logical '0'
          UART_TX_SetLow();
          rx_data &= ~(1 << data_cnt);
        } else {
          // logical '1'
          UART_TX_SetHigh();
          rx_data |= (1 << data_cnt);
        }

        data_cnt++;
        if (data_cnt > 7) {
          fHead = 0;
          data_1byte_OK = 1;
        }
      }
    }

  } // rising edge tringgered
  else if (CCP3CON == 0x05) {
    rising_edge_time = capturedValue;
    CCP3CON = 0x04; // set to falling edge trigger
    // 計算低電位脈衝寬度
    if (rising_edge_time >= falling_edge_time) {
      pluse_width_LO = rising_edge_time - falling_edge_time;
    } else {
      pluse_width_LO = (0xffff - falling_edge_time) + rising_edge_time;
    }
    rise_edge_flag = 1;
  }
}

void TMR0_EvenHandler(void) {
  T10MS_CNT++;
  if (T10MS_CNT >= 200) { // 2000ms
    fException2 = 1;      // 設定異常狀態
    T10MS_CNT = 0;
  }
}

/*
                         Main application
 */
int main(void) {
  // initialize the device
  SYSTEM_Initialize();
  TMR0_SetInterruptHandler(TMR0_EvenHandler);
  LoBeam_OFF();

  // When using interrupts, you need to set the Global and Peripheral
  // Interrupt Enable bits Use the following macros to:

  // Enable the Global Interrupts
  INTERRUPT_GlobalInterruptEnable();

  // Enable the Peripheral Interrupts
  INTERRUPT_PeripheralInterruptEnable();

  // Disable the Global Interrupts
  // INTERRUPT_GlobalInterruptDisable();

  // Disable the Peripheral Interrupts
  // INTERRUPT_PeripheralInterruptDisable();
  
    while (1) {
    getFrameData();
    check_input();
    // LED_output();
    Exception_handling();
    if (fException2 == 0) {
      if (fException == 1) {
        // 異常處理
        fException = 0; // 重置異常狀態
      } else
        LED_output();
    } else {
      // 2秒沒收到正確的資料開啟近燈+P2
      fException2 = 0;
      // P2_ON();      // P2亮
      HiBeam_OFF(); // 遠燈 OFF
      LoBeam_ON();  // 近燈 ON
      POS_ON();     // POS ON
    }
  }
}

/**
 End of File
 */