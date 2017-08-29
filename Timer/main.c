/* User LED is on PC13 */
// Experimenets in sound production.  

#include <stdint.h>
#include "../include/cortexm3.h"
#include "../include/STM32F103.h"
uint32_t MelodyNotes[]={329,329,329,329,329,329,329,392,261,293,329,0,349,349,349,349,349,329,329,329,329,293,293,329,293,392};
uint32_t MelodyTimes[]={100, 100, 200, 100, 100, 200, 100, 100, 100, 100, 200, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 200, 200};

volatile int32_t tone_time=0;
volatile uint32_t milliseconds=0; // used for delay routines
uint32_t uptime=0;                // measure how long the system has been running
void delay(uint32_t dly)
{
    if (dly)
    {        
        milliseconds = 0;
        while(milliseconds<dly);
            //cpu_sleep(); // may as well sleep while waiting for millisecond to elapse (saves power)
    }    
}


// The sound system needs to share a variable with the SysTick timer
// This variable is the number of milliseconds the sound is to play for.
// The SysTick counter decrements this variable and, when it reaches zero
// turns off the sound output

void initSound()
{
    // Output is a piezo speaker on B0 which is Timer 3, Channel 3
    // Turn on Timer 3
    RCC->APB1ENR |= BIT1; 
    // Ensure that Port B is enabled
    RCC->APB2ENR |= BIT3;
    // Set Timer 3 to default values
    TIM3->CR1 = 0; 
    // Enable PWM mode on channel 3
    TIM3->CCMR2_Output = BIT6+BIT5; 
    // Connect up Timer 3 output
    TIM3->CCER |= BIT8;
    // Configure PB0 as Timer3 output
    GPIOB->CRL &=0xfffffff0;
    GPIOB->CRL |= BIT3+BIT1;
    
}
void playTone(uint32_t Frequency, uint32_t ms)
{
    // Will assume a 72MHz input frequency
    // The auto-reload register has a maximum value of 65536.  
    // This should map to the lowest frequency we would be interested in.
    // 72MHz/65536 = 1098Hz.  This is too high for a 'lowest' frequency so 
    // some pre-scaling of the input frequency is required.
    TIM3->CR1 &= ~BIT0; // disable the timer    
    TIM3->PSC = 72000000UL/65536UL;  // prescale so that the lowest frequency is 1Hz.
    TIM3->ARR = (72000000UL/(uint32_t)(TIM3->PSC))/(Frequency);
    TIM3->CCR3 = TIM3->ARR/2; // 50 % Duty cycle
    TIM3->CNT = 0;
    TIM3->CR1 |= BIT0; // enable the timer        
    tone_time = ms;
}
void stopTone()
{
    TIM3->CR1 &= ~BIT0; // disable the timer
}
int isPlaying()
{
    return (tone_time != 0);
}
void playMelody(uint32_t *Tones, uint32_t *Times,int Len)
{
    int Index;
    for (Index = 0; Index < Len; Index++)
    {
        while(isPlaying()); // wait for previous note to complete
        delay(100);
        playTone(Tones[Index],Times[Index]);        
    }
}
void SysTick_Handler()
{
    uptime++;
    milliseconds++;    
    GPIOC->ODR ^= BIT13; // Debug output : allows you to check how fast SysTick is really happening
    if (tone_time)
    {
        tone_time--;
        if (tone_time==0)
            stopTone();
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
    // Assuming a 72MHz clock speed
    SysTick->CTRL |= ( BIT2 | BIT1 | BIT0); // enable systick, source = cpu clock, enable interrupt
    SysTick->LOAD=72000;  // Divide 72000000 by 72000 to get tick frequency of 1000Hz
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
int main()
{
    initClocks();
    configurePins();    
    initSysTick();    
    initSound();
    enable_interrupts();          
    while(1)
    {
        playMelody(MelodyNotes,MelodyTimes,26);
        delay(500);
    }
}
    
