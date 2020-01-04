/* User LED is on PC13 */
// Experimenets in sound production.  
// This program sets up PWM at 281kHz which gives a PWM resolution of 8 bits(72MHz/256 = 281kHz)
// Version 1: A single cycle of an 8 bit sine wave is stored in memory.  The sine wave is notionally
// 1kHz sampled at a frequency of 281500Hz i.e 281 samples.
// A sine-wave lookup table holds the duties necessary to produce the Sinusoidal PWM output.  
// In order to reduce the number of interrupts per second the system will make use of DMA.  
// The DMA buffer will hold 1ms worth of samples (281) which happens to coincide with the
// length of the test sine wave in this example.  

// The following C-code was used to produce the sine-lookup table:
/*
#include <stdio.h>
#include <math.h>
int main()
{
    double SignalFrequency=1000.0;
    double SignalAmplitude=127.0;
    double SampleRate=281500.0;
    int StepCount = SampleRate / SignalFrequency;
    double AngleStep = 2.0*M_PI / (double) StepCount;
    double Angle=0;
    double TableValue;    
    int Index=0;
    printf("uint8_t SineArray[] = {");
    for (Index = 0; Index < StepCount; Index++)
    {
        TableValue = 127.0*sin(Angle);
        TableValue = TableValue + 127.0; // Offset to rangel 0 to 254
        printf("%d,",(int)TableValue);
        Angle = Angle + AngleStep;
    }
    printf("};");
}
Compile with gcc sinetable.c -o sinetable -lm
This writes an array definition to the console which is copied and pasted below:

 */
#include <stdint.h>
#include "../include/cortexm3.h"
#include "../include/STM32F103.h"

const uint8_t SineArray[] = {127,129,132,135,138,141,143,146,149,152,155,157,160,163,166,168,171,174,176,179,181,184,186,189,191,194,196,199,201,203,205,208,210,212,214,216,218,220,222,224,226,227,229,231,232,234,235,237,238,239,241,242,243,244,245,246,247,248,249,250,250,251,251,252,252,253,253,253,253,253,253,253,253,253,253,253,252,252,252,251,250,250,249,248,248,247,246,245,244,243,241,240,239,237,236,235,233,231,230,228,226,225,223,221,219,217,215,213,211,209,207,204,202,200,197,195,193,190,188,185,183,180,178,175,172,170,167,164,162,159,156,153,150,148,145,142,139,136,134,131,128,125,122,119,117,114,111,108,105,103,100,97,94,91,89,86,83,81,78,75,73,70,68,65,63,60,58,56,53,51,49,46,44,42,40,38,36,34,32,30,28,27,25,23,22,20,18,17,16,14,13,12,10,9,8,7,6,5,5,4,3,3,2,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,2,2,3,3,4,5,6,7,8,9,10,11,12,14,15,16,18,19,21,22,24,26,27,29,31,33,35,37,39,41,43,45,48,50,52,54,57,59,62,64,67,69,72,74,77,79,82,85,87,90,93,96,98,101,104,107,110,112,115,118,121,124,};
volatile uint32_t SampleCounter = 0;
volatile uint32_t milliseconds=0; // used for delay routines
uint32_t uptime=0;                // measure how long the system has been running
void delay(uint32_t dly)
{
    if (dly)
    {        
        milliseconds = 0;
        while(milliseconds<dly);
            cpu_sleep(); // may as well sleep while waiting for millisecond to elapse (saves power)
    }    
}

void SysTick_Handler()
{
    uptime++;
    milliseconds++;    
  

}
void configurePins()
{
    // Turn on GPIO C
    RCC->APB2ENR |= BIT4;
    // Configure PC13 as an output
    GPIOC->CRH |= BIT20;
    GPIOC->CRH &= ~(BIT23 | BIT22 | BIT21);        
    GPIOC->ODR |= BIT13; // Debug output : LED off start with (High = off)
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
void initPWM()
{
    // Attempting to do audio output using PWM 
    // The base clock frequency is 72MHz.
    // Upper limit of audio is 20kHz
    // If PWM resolution is 8 bits then the switching
    // frequency will be 72^6/256 = 281500Hz.
    // At 20kHz this means that there will be 281500/20000 switches
    // per cycle i.e. 14.0625 switches per cycle.
    // This is certainly above the nyquist frequency so if there
    // is not too much intermodulation distortion then it should sound
    // pretty reasonable.
    // The PWM out could go directly to a transistor/amplifier or
    // be filtered (simple RC filter with a cutoff of 20kHz should be OK-ish)
    // Will use TIM1 and PA8,PA9 (compare channels 1,2)
    RCC->APB2ENR |= BIT11 + BIT2;  // turn on clocks for TIM1 and port A
    // Select Alternate function push/pull @50MHz for PA8,PA9
    GPIOA->CRH |= BIT0 + BIT1 + BIT3 + BIT4 + BIT5 + BIT7;
    GPIOA->CRH &= ~(BIT2+BIT6);
    TIM1->ARR = 255;
    TIM1->CCR1 = 128;
    TIM1->CCR2 = 64;
    TIM1->CCMR1_Output = BIT14 + BIT13 + BIT11 + BIT10 + BIT6 + BIT5 + BIT3 + BIT2; // PWM mode 1 for CH1,CH2, Fast mode enable, preload enable
    TIM1->CCER |= BIT0 + BIT4;    // Enable OC1,OC2 outputs.
    TIM1->BDTR |= BIT15; // Main output enable
    TIM1->CR1 |= BIT7; // Set the ARPE bit
    TIM1->EGR |= BIT0; // Force update of registers    
    // DMA setup
    RCC->AHBENR |= BIT0; // Enable clock for DMA1
    // Page 281 of Reference guide (Figure 50)
    // Timer 1 update event (TIM1_UP) is routed to DMA Channel 5 on DMA Controller 1
    DMA1->CNDTR5 = sizeof(SineArray);  // Specify the amount of bytes to write
    DMA1->CPAR5 = (uint32_t)&(TIM1->CCR1); // Target peripheral register (Just doing CCR1 for now)
    DMA1->CMAR5 = (uint32_t)&SineArray; // Address of source of data
    DMA1->IFCR = BIT17; // Clear transfer complete flag for channel 5
    DMA1->CCR5 = BIT8 +  BIT7 + BIT4 + BIT1 + BIT0; // Channel 5, Memory increment mode, Read from memory, Transfer complete interrupt enable, Enable Channel        
    
    TIM1->DIER |= BIT8; // Tell TIM1 to generate a DMA request on update event. 
    
    NVIC->ISER0 |= BIT15; // Enable interrupts from DMA1,Channel 5 in the NVIC
    TIM1->CR1 |= BIT0; // enable the timer
}
void DMA1_CH5_Handler()
{
    DMA1->CCR5 &= ~BIT0; // disable DMA1 during re-configuration
    DMA1->CNDTR5 = sizeof(SineArray);  // Specify the amount of bytes to write
    DMA1->CPAR5 = (uint32_t)&(TIM1->CCR1); // Target peripheral register (Just doing CCR1 for now)
    DMA1->CMAR5 = (uint32_t)&SineArray; // Address of source of data
    DMA1->IFCR = BIT17; // Clear transfer complete flag for channel 5
    DMA1->CCR5 |= BIT0; // re-enable DMA1
    
    GPIOC->ODR ^= BIT13; // Debug output 
}
int main()
{
    uint32_t Left,Right;
    Left = 0;
    Right = 255;
    initClocks();
    configurePins();    
    initSysTick();
    initPWM();
    enable_interrupts();          
    
    while(1)
    {
        

    }
}
    
