#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include "shiftReg.h"
#include "lcd.h"
#include "spi.h"
#include "delay.h"
#include <stdio.h>

#define MAX_COUNT 60

typedef struct {
  uint8_t cmd;
  uint8_t data1;
  uint8_t data2;
} packet_t;

uint8_t count = 0;
uint8_t rawData[MAX_COUNT];
uint8_t crc = 0;
uint8_t i;

uint8_t calcCrc(uint8_t* buf, uint8_t len);

int main(void) {
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM6, ENABLE );
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC, ENABLE );

  GPIO_StructInit( &GPIO_InitStructure );
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init( GPIOC, &GPIO_InitStructure );

  lcd_init();
  spi_init();
  lcd_goto( 8, 2 );
  lcd_string( "ARVP " );

  while(1) {
    static int ledVal = 0;
    GPIO_WriteBit(GPIOC, GPIO_Pin_9, (ledVal) ? Bit_SET : Bit_RESET);
    ledVal = 1 - ledVal;
    //delay_ms(200);
  }
}


void EXTI4_IRQHandler( void ) {
  if( EXTI_GetITStatus( EXTI_Line4 ) != RESET ) {
    EXTI_ClearFlag( EXTI_Line4 );
    // Get count
    SPI_I2S_SendData( SPI1, 0x00 );
    while( SPI_I2S_GetFlagStatus( SPI1, SPI_I2S_FLAG_RXNE ) == RESET );
    count = SPI_I2S_ReceiveData( SPI1 );

    // Get data
    for( i=0; i<count*3; i++ ) {
      while( SPI_I2S_GetFlagStatus( SPI1, SPI_I2S_FLAG_RXNE ) == RESET );
      rawData[i] = SPI_I2S_ReceiveData( SPI1 );
    }
    
    // Process CRC
    while( SPI_I2S_GetFlagStatus( SPI1, SPI_I2S_FLAG_RXNE ) == RESET );
    crc = SPI_I2S_ReceiveData( SPI1 );
    if( crc == calcCrc( rawData, count ) )
      SPI_I2S_SendData( SPI1, 0x00 );
    else
      SPI_I2S_SendData( SPI1, 0x01 );
    while( SPI_I2S_GetFlagStatus( SPI1, SPI_I2S_FLAG_RXNE ) == RESET );
  }
}

uint8_t calcCrc(uint8_t* buf, uint8_t len) {
	uint8_t cksum = 0;
	for (i = 0; i < len; i++)
		cksum ^= buf[i];
	return cksum;
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
  /* Infinite loop */
  while(1);
}
#endif
