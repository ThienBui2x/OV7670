/*
 * ov7670.h
 *
 *  Created on: 15.05.2026
 *      Author: ADM
 */

#ifndef INC_OV7670_H_
#define INC_OV7670_H_

#include "stm32f4xx_hal.h"

#define OV7670_ADDW			0x42
#define OV7670_ADDR			0x43

#define OV7670_WIDTH		120
#define OV7670_LENGTH		160
#define BYTES_PER_PIXEL		2
#define OV7670_DATA_SIZE	OV7670_LENGTH*OV7670_WIDTH*BYTES_PER_PIXEL

#define HIGH				GPIO_PIN_SET
#define LOW					GPIO_PIN_RESET

/* ------------ PIN MAPPING ------------
 * PB8	SIO_C - I2C2 SDL
 * PB9	SIO_D - I2C2 SDA
 * PC9	VSNC  - EXT9
 * X	HREF
 * PA5	D7	  - INPUT
 * PA6	D6	  - INPUT
 * PA7	D5	  - INPUT
 * PC5	D4	  - INPUT
 * PB12	D3	  - INPUT
 * PB15	D2	  - INPUT
 * PB14	D1	  - INPUT
 * PB13	D0	  - INPUT
 * PC8	RESET - OUTPUT
 * X	PWDN
 * X	STROBE
 * PC13	FIFO-RCK	// Read Control
 * PC14	FIFO-WR		// Write Control
 * PC15	FIFO-OE		// Off Control
 * PC10	FIFO-WRST	// Write RST
 * PC11 FIFO_RRST	// Read RST
 * ------------------------------------
 */
#define OV7670_RESET(x) \
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, x)

#define OV7670_DATA7() \
	HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5)
#define OV7670_DATA6() \
	HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6)
#define OV7670_DATA5() \
	HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7)
#define OV7670_DATA4() \
	HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5)
#define OV7670_DATA3() \
	HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12)
#define OV7670_DATA2() \
	HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15)
#define OV7670_DATA1() \
	HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14)
#define OV7670_DATA0() \
	HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13)

#define OV7670_FIFO_WRST(x)	\
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, x)

#define OV7670_FIFO_RRST(x)	\
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, x)

#define OV7670_FIFO_WR(x) \
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, x)

#define OV7670_FIFO_RCK(x) \
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, x)

#define OV7670_FIFO_OE(x) \
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, x)

typedef enum {
	FRAME_IDLE,
	FRAME_WRITE,
	FRAME_READY,
	FRAME_READ
} state_t;

typedef enum {
	OV7670_IDLE,
	OV7670_READY,
	OV7670_BUSY,
	OV7670_ERROR
} status_t;

void ov7670_module(void);
void ov7670_fifo_write(void);
void ov7670_fifo_read(void);
void ov7670_pixelDataToUart(void);
uint8_t ov7670_init(void);
uint8_t ov7670_write(uint8_t memAddr, uint8_t data);
uint8_t ov7670_read(uint8_t memAddr);
uint8_t ov7670_getData(void);

#endif /* INC_OV7670_H_ */
