#include <stdio.h>
#include "lcd.h"
#include "screen.h"
#include "define.h"

#include "FreeRTOS.h"
#include "timers.h"

#define xDelay25   ((TickType_t)25 / portTICK_PERIOD_MS)
#define xDelay100  ((TickType_t)100 / portTICK_PERIOD_MS)


void delayuS(uint32_t uS)
{
    LPC_TIM1->TCR = 0x02;                // reset timer
    LPC_TIM1->PR  = 0x00;                // set prescaler to zero
    LPC_TIM1->MR0 = uS * (SystemCoreClock / 1000000) - 1;
    LPC_TIM1->IR  = 0xff;                // reset all interrrupts
    LPC_TIM1->MCR = 0x04;                // stop timer on match
    LPC_TIM1->TCR = 0x01;                // start timer
    // wait until delay time has elapsed
    while (LPC_TIM1->TCR & 0x01);
}


void spi_out(uint8_t data)
{
    LPC_SPI->SPDR = data;
    while (((LPC_SPI->SPSR >> 7) & 1) == 0);
}

void u8g_Delay(uint16_t val)
{
    delayuS(1000UL * (uint32_t)val);
}

void u8g_MicroDelay(void)
{
    delayuS(1);
}

void u8g_10MicroDelay(void)
{
    delayuS(10);
}

uint8_t u8g_com_hw_spi_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr)
{
    switch (msg) {
        case U8G_COM_MSG_STOP:
            break;

        case U8G_COM_MSG_INIT:
            // init spi and ports
            u8g_MicroDelay();
            break;

        case U8G_COM_MSG_ADDRESS:                     /* define cmd (arg_val = 0) or data mode (arg_val = 1) */
            //u8g_10MicroDelay();
            GPIO_SET_PIN_VAL(LCD_A0, arg_val);
            u8g_MicroDelay();
            break;

        case U8G_COM_MSG_CHIP_SELECT:
            GPIO_SET_PIN_VAL(LCD_CSB, !arg_val);

            u8g_MicroDelay();
            break;

        case U8G_COM_MSG_RESET:
            GPIO_SET_PIN_VAL(LCD_RSTB, arg_val);
            u8g_MicroDelay();
            break;

        case U8G_COM_MSG_WRITE_BYTE:
            spi_out(arg_val);
            u8g_MicroDelay();
            break;

        case U8G_COM_MSG_WRITE_SEQ:
        case U8G_COM_MSG_WRITE_SEQ_P: {
                register uint8_t *ptr = arg_ptr;
                while (arg_val > 0) {
                    spi_out(*ptr++);
                    arg_val--;
                }
            }
            break;
    }
    return 1;
}

void task_LCD(void *pvParameters)
{
    TickType_t xLastTime;
    xLastTime = xTaskGetTickCount();

    // Configure LCD backligt
    GPIO_DIR_OUT(LCD_BACKLIGHT);
    GPIO_SET_PIN(LCD_BACKLIGHT);

    GPIO_DIR_OUT(LCD_RSTB);
    GPIO_DIR_OUT(LCD_CSB);
    GPIO_DIR_OUT(LCD_A0);

    // Configure Timer1 used for µs delay in lcd
    LPC_SC->PCONP |= PCONP_PCTIM1;              // power up Timer (def on)
    LPC_SC->PCLKSEL0 |= PCLK_TIMER1(CCLK_DIV1); // set Timer0 clock1

    // Configure SPI (LCD)
    LPC_SC->PCONP |= PCONP_PCSPI;               // power up SPI
    LPC_SC->PCLKSEL0 |= PCLK_SPI(CCLK_DIV1);    // set SPI CCLK

    LPC_PINCON->PINSEL0 |= ((uint32_t)3 << 30); // p0.15 -> sck
    LPC_PINCON->PINSEL1 |= (0xc | 0x30);        // p0.17 & p0.18 miso / mosi (no miso??)

    LPC_SPI->SPCR |= SPCR_MSTR;                 // SPI operates in Master mode.

    LCDInit();

    for (;;) {
        vTaskDelayUntil(&xLastTime, xDelay100);
        lcdUpdate();
#if LOWSTACKWARNING
        int stack = uxTaskGetStackHighWaterMark(NULL);
        if (stack < 50) printf("Task task_LCD has %u words left in stack.\r\n", stack);
#endif

    }
}
