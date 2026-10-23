#include "stm32f103xb.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_cortex.h"
#include "stm32f1xx_hal_exti.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/_intsup.h>

void write_byte(uint8_t command) {
  void write_bit(uint8_t value) {
  GPIOA->BRR = (1 << 1);

  TIM3->CNT = 0;
  uint8_t delay_value = value ? 10 : 60;
  while(TIM3->CNT < delay_value);
  
  GPIOA->BSRR = (1 << 1);
  while(TIM3->CNT < 70); // writing process should be minimum 60 ms
}

  for(int i=0; i<8; i++) {
    write_bit(command & 1);
    command >>= 1;
  }
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

int main(void) {
  initialize_mcu();

  RCC->APB2ENR |= (0b11 << 2); // Enable IO port A and B clock

  GPIOA->CRL &= ~(0b1111 << 4);
  GPIOA->CRL |= (0b0110 << 4);
  GPIOA->BSRR |= (1 << 1);


  RCC->APB1ENR |= (11 << 1); // enable timer 3 and 4(TIM3EN, TIM4EN)
  TIM3->ARR = 65535;
  TIM3->PSC = 35;
  TIM3->EGR |= 0x1;
  TIM3->CR1 |= 0x1;

    GPIOA->BRR |= (1 << 1);

    TIM3->CNT = 0;
    while(TIM3->CNT < 480);

    GPIOA->BSRR |= (1 << 1);
    
    TIM3->CNT = 0;
    while(TIM3->CNT < 60);
    
    if ((GPIOA->IDR & 0x2) == 0) {
      // initilization is right
    }
    while ((GPIOA->IDR & 0x2) == 0);

    write_byte(0xCC); // enter the system to write to scratchpad mode
    write_byte(0x1E); // set high temperature to 30 degree celsius
    write_byte(0x0A); // set low temperature to 10
    write_byte(0x7F); // 12-bit resolution/preciseness

  while(1) {
    
  }
}