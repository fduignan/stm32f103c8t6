/* User LED is on PC13 */

#include <stdint.h>
#include "../include/cortexm3.h"
#include "../include/STM32F103.h"
void delay(uint32_t dly)
{
    while(dly--);
}
int main()
{

    // Turn on GPIO C
    RCC->APB2ENR |= BIT4;
    // Configure PC13 as an output
    GPIOC->CRH |= BIT20;
    GPIOC->CRH &= ~(BIT23 | BIT22 | BIT21);
    while(1)
    {
        GPIOC->ODR |= BIT13;
        delay(1000000);
        GPIOC->ODR &= ~BIT13;
        delay(1000000);
    }
}
    
