/* User LED is on PC13 */

#include <stdint.h>
#include "../include/cortexm3.h"
#include "../include/STM32F103.h"
#include "serial.h"
#include "nvm.h"
void delay(uint32_t dly)
{
    while(dly--);
}
uint32_t milliseconds=0;
void SysTick_Handler()
{
    milliseconds++;
    if (milliseconds>=9000)
    {
        GPIOC->ODR ^= BIT13;
        milliseconds=0;
    }
}
void configurePins()
{
    // Turn on GPIO C
    RCC->APB2ENR |= BIT4;
    // Configure PC13 as an output
    GPIOC->CRH |= BIT20;
    GPIOC->CRH &= ~(BIT23 | BIT22 | BIT21);
}
void initSysTick()
{
    SysTick->CTRL |= ( BIT2 | BIT1 | BIT0); // enable systick, source = cpu clock, enable interrupt
    SysTick->LOAD=SysTick->CALIB;    
    SysTick->VAL = 1; // don't want long wait for counter to count down from initial high unknown value
}
void initClocks()
{
    // Set the clock to 72MHz
    // An external 8MHz crystal is on board
    /* Excerpt from reference manual
Several prescalers allow the configuration of the AHB frequency, the high speed APB
(APB2) and the low speed APB (APB1) domains. The maximum frequency of the AHB and
the APB2 domains is 72 MHz. The maximum allowed frequency of the APB1 domain is
36 MHz. The SDIO AHB interface is clocked with a fixed frequency equal to HCLK/2
The RCC feeds the Cortex ® System Timer (SysTick) external clock with the AHB clock
(HCLK) divided by 8. The SysTick can work either with this clock or with the Cortex ® clock
(HCLK), configurable in the SysTick Control and Status Register. The ADCs are clocked by
the clock of the High Speed domain (APB2) divided by 2, 4, 6 or 8.
    */
    RCC->CR &= ~BIT18; // clear HSEBYP 
    RCC->CR |= BIT16;  // set (turn on) HSE
    while ( (RCC->CR | BIT17)         == 0); // wait for HSE Ready

    // set PLL multiplier to 9 giving 72MHz as PLL output
    RCC->CFGR &= ~(BIT21);
    RCC->CFGR |= BIT20 | BIT19 | BIT18;      
    // Limit APB1 to 36MHz
    RCC->CFGR |= BIT10;
    RCC->CFGR &= ~(BIT9+BIT8);
    RCC->CFGR |= BIT16; // switch PLL clock source to HSE
    
    RCC->CR |= BIT24;         // turn on PLL
    while ( (RCC->CR | BIT24) == 0); // wait for PLL lock
    // Insert wait states for FLASH : need 2 at 72MHz
    FLASH->ACR |= BIT1;
    FLASH->ACR &= ~(BIT2+BIT0);
    // Turn on pre-fetch buffer to speed things up
    FLASH->ACR |= BIT4;
    // Switch to PLL output for system clock
    RCC->CFGR &= BIT0;
    RCC->CFGR |= BIT1;
}
void printNumber(uint16_t Number)
{
    // This function converts the supplied number into a character string and then calls on puText to
    // write it to the display
    char Buffer[6]; // Maximum value = 65535
    Buffer[5] = 0; // terminate the string
    Buffer[4] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[3] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[2] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[1] = Number % 10 + '0';
    Number = Number / 10;
    Buffer[0] = Number % 10 + '0';
    eputs(Buffer);
    eputs("\r\n");
}
int main()
{
    uint16_t values[]={1,2,3,4,5,6};
    int count=0;
    char RXString[10];
    initClocks();
    configurePins();    
    initSysTick();    
    initUART(9600);  
    delay(10000000);
    disable_interrupts();
    eraseSector(0x800fc00);
    writeSector(0x800fc00,values,sizeof(values));
    readSector(0x800fc00,values,sizeof(values));
    enable_interrupts();       
    eputs("\r\nvalues\r\n");
    while (count < 6)
    {
        printNumber(values[count]);        
        count++;
        delay(100000);
    }        
    while(1);
    {
        eputs("Press Reset to repeat the test\r\n");                
    }
}
    
