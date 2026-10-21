#include "stm32f103xb.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_cortex.h"
#include "stm32f1xx_hal_exti.h"
#include <stdint.h>
#include <sys/_intsup.h>

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// HCLK = 72 MHz
// APB1, APB2 prescalers = /4
// APB1 timers → 18 × 2 = 36 MHz
// APB2 timers → 18 × 2 = 36 MHz
void initialize_mcu(void) {
  FLASH->ACR &= ~0x7; // clear LATENCY bits
  FLASH->ACR |= 0x12; // Enable PRFTBE + 2 wait steps for LATENCY

  RCC->CR |= 0x00010001; // Set HSION and HSEON bits to 1
  while ((RCC->CR & 0x00000002) == 0); // wait until HSIRDY becomes 1
  RCC->CFGR = 0x00000000; // Set HSI as system clock
  while ((RCC->CFGR & 0x0000000C) != 0); // wait until SYSCLK becomes HSI

  // Previously we just changed HSION and HSEON bits to 1(|=), 
  // but this time completely modifying the CR register(=).
  RCC->CR = 0x00010001;
  // Set division, multipilication factors for PLL
  RCC->CFGR = 0x001D0000;

  RCC->CR = 0x01010001; // Set HSION, HSEON, PLL bits to 1
  while((RCC->CR & 0x02000000) == 0); // wait until PLLRDY becomes 1

  RCC->CFGR = 0x061D2D02;
  while((RCC->CFGR & 0x0C) != 0x08); // wait until PLL becomes the system clock

  RCC->CR |= 0x00080000; // enable CSSON(clock security system)
}

// Setup PB6(D10) as push-pull output, 2 MHz
void setup_LED_With_PWM() {
    GPIOB->CRL &= ~(0b1111 << 24);
    GPIOB->CRL |=  (0b1010 << 24);


    RCC->APB1ENR |= (1 << 2); // Enable TIM4EN(Timer 4 clock) 
    TIM4->PSC = 35; // 36MHz / 35 = 1MHz
    TIM4->ARR = 1000 - 1; // 1KHz
    TIM4->CCR1 = 1000 / 2; // 50% brightness
    TIM4->EGR = 0x0001; // UG = 1

    TIM4->CCMR1 |= (0b110 << 4); // Enable PWM mode 1
    TIM4->CCER |= 0x1; // Enable Capture/Compare output

    TIM4->CR1 |= 0x1; // Start the counter
}

/// Setup PA7 for ADC for Photoresistor
void setup_ADC() {
  GPIOA->CRL &= ~(0xF0000000); // clear the bits for PA7
  // GPIOA->CRL = 0x00000000; // set bits[31:28] to 0000 for analog input mode

  // Enable ADC1 interface clock(ADC1EN)
  RCC->APB2ENR |= (1 << 9);
  // Clear ADC prescaler bits
  RCC->CFGR &= ~(0b11 << 14);
  // This sets the ADC prescaler to PCLK2 divided by 4.
  // We can actually keep the bits as 00, which is divided by 2,
  // but doing like this for just testing.
  // Final goal is to limit the frequency for ADC clock,
  // for example for STM32F103, it must be <= 14 MHz.
  // In my current project setup, PCLK2 is operating at 18 Mhz. 
  // 18 / 4 = 4.5 Mhz
  RCC->CFGR |= (0b01 << 14);

  ADC1->CR2 |= (1 << 0); // Enable A/D converter(ADON)
  ADC1->SMPR2 |= (0b101 << 21); // Set sample time to 55.5 cycles for PA7
  // Put ADC channel 7 as the 1st conversion the regular sequence.
  // Like, when ADC starts regular conversion,
  // first read ADC channel 7
  ADC1->SQR3 = 0x7;

  // Select SWSTART
  ADC1->CR2 |= (0b111 << 17);
  // Enable external trigger conversion for regular channels
  ADC1->CR2 |= (1 << 20);
}

void setup_ADC_Interrupt() {
  ADC1->CR1 |= (1 << 5);
  ADC1->CR2 |= (1 << 22);
  ADC1->CR2 |= (1 << 1);
  HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
}

int main(void) {
   initialize_mcu();

  RCC->APB2ENR |= (0b11 << 2); // Enable IO port A and B clock

  setup_LED_With_PWM();
  setup_ADC();
  setup_ADC_Interrupt();

  while(1) {
    
  }
}

void ADC1_2_IRQHandler() {
    uint16_t value = ADC1->DR;
    uint16_t scaled_value = map(value, 0, 4095, 0, 300);
    scaled_value = scaled_value > 250 ? 300 : scaled_value;
    TIM4->CCR1 = (300 - scaled_value);
}