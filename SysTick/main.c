/* User LED is on PC13 */

#include <stdint.h>
#include "../include/cortexm3.h"
#include "../include/STM32F103.h"
void delay(uint32_t dly)
{
    while(dly--);
}
uint32_t milliseconds=0;
void SysTick_Handler()
{
    milliseconds++;
    if (milliseconds>=1000)
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

int main()
{
    
    delay(100000000); // startup delay in case things go bad
    configurePins();
    initSysTick();
    enable_interrupts();
    while(1)
    {
    }
}
    
