/*
 * ov7670.c
 *
 *  Created on: 15.05.2026
 *      Author: ADM
 */

#include "ov7670.h"
#include "string.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

volatile status_t ov7670_flag = OV7670_READY;
volatile state_t vsync_state = FRAME_IDLE;
uint8_t pixels[OV7670_DATA_SIZE] = {0};

static const int REG_CLKRC = 0x11;
static const int REG_COM7 = 0x12;
static const int REG_COM8 = 0x13;
static const int REG_COM10 = 0x15;
static const int REG_COM11 = 0x3B;
static const int REG_TSLB = 0x3A;
static const int REG_COM15 = 0x40;
static const int REG_COM3 = 0x0C;
static const int REG_MVFP = 0x1E;
static const int REG_HSTART = 0x17;
static const int REG_HSTOP = 0x18;
static const int REG_HREF = 0x32;
static const int REG_VSTART = 0x19;
static const int REG_VSTOP = 0x1A;
static const int REG_VREF = 0x03;
static const int REG_COM14 = 0x3E;
static const int SCALING_DCWCTR = 0x72;
static const int SCALING_PCLK_DIV = 0x73;



//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
//	/* Prevent unused argument(s) compilation warning */
//	UNUSED(GPIO_Pin);
//
//	if (GPIO_Pin == GPIO_PIN_9) {
//		if (ov7670_flag == OV7670_READY) {
//			if (vsync_state == FRAME_IDLE) {
//				ov7670_flag = OV7670_BUSY;
//				vsync_state = FRAME_WRITE;
//			}
//			else if (vsync_state == FRAME_READY) {
//				ov7670_flag = OV7670_BUSY;
//				vsync_state = FRAME_READ;
//			}
//		}
//	}
//}

//void ov7670_module(void) {
//
//    /* Enable FIFO output */
//    OV7670_FIFO_OE(LOW);
//
//	char str[100];
//	switch (vsync_state) {
//		case FRAME_WRITE:
//			strcpy(str, "First VSYNC Frame \n");
//			HAL_UART_Transmit(&huart2, (uint8_t *) str, strlen(str), 100);
//			ov7670_fifo_write();
//			vsync_state = FRAME_READY;
//			ov7670_flag = OV7670_READY;
//			break;
//		case FRAME_READ:
//			strcpy(str, "Second VSYNC Frame \n");
//			HAL_UART_Transmit(&huart2, (uint8_t *) str, strlen(str), 100);
//			ov7670_fifo_read();
//			ov7670_pixelDataToUart();
//			vsync_state = FRAME_IDLE;
//			ov7670_flag = OV7670_READY;
//			break;
//		case FRAME_IDLE:
//		case FRAME_READY:
//		default:
//			break;
//	}
//}

void ov7670_fifo_write(void) {
    /* Reset FIFO write pointer */
    OV7670_FIFO_WRST(LOW);

    for (volatile uint32_t i = 0; i < 100; i++);

    OV7670_FIFO_WRST(HIGH);

    /* Enable FIFO writing */
    OV7670_FIFO_WR(LOW);
}

void ov7670_fifo_read(void) {

    /* Disable FIFO writing */
    OV7670_FIFO_WR(HIGH);

    /* Reset FIFO read pointer */
    OV7670_FIFO_RRST(LOW);

    for (volatile uint32_t i = 0; i < 100; i++);

    OV7670_FIFO_RCK(LOW);

    for (volatile uint32_t i = 0; i < 100; i++);

    OV7670_FIFO_RCK(HIGH);

    for (volatile uint32_t i = 0; i < 100; i++);

    OV7670_FIFO_RRST(HIGH);


    /*
     * Read exactly one complete frame:
     *
     * 160 pixels × 120 lines × 2 bytes
     * = 38400 bytes
     */
    for (uint32_t i = 0; i < OV7670_DATA_SIZE; i++)
    {
        /* Advance FIFO read clock */
        OV7670_FIFO_RCK(LOW);
        OV7670_FIFO_RCK(HIGH);

        /* Read 8-bit data bus */
        pixels[i] = ov7670_getData();

    }

    /* Disable FIFO output */
    OV7670_FIFO_OE(HIGH);
}

void ov7670_pixelDataToUart(void) {
	char str[100];
	strcpy(str, "START\n");
	HAL_UART_Transmit(&huart2, (uint8_t *) str, strlen(str), 100);

	HAL_UART_Transmit(&huart2, pixels, OV7670_DATA_SIZE, HAL_MAX_DELAY);

	strcpy(str, "END\n");
	HAL_UART_Transmit(&huart2, (uint8_t *) str, strlen(str), 100);


}

void saturation(int s)  //-2 to 2
{
	//color matrix values
	ov7670_write(0x4f, 0x80 + 0x20 * s);
	ov7670_write(0x50, 0x80 + 0x20 * s);
	ov7670_write(0x51, 0x00);
	ov7670_write(0x52, 0x22 + (0x11 * s) / 2);
	ov7670_write(0x53, 0x5e + (0x2f * s) / 2);
	ov7670_write(0x54, 0x80 + 0x20 * s);
	ov7670_write(0x58, 0x9e); //matrix signs
}

void frameControl(int hStart, int hStop, int vStart, int vStop)
{
	ov7670_write(REG_HSTART, hStart >> 3);
	ov7670_write(REG_HSTOP, hStop >> 3);
	ov7670_write(REG_HREF, ((hStop & 0b111) << 3) | (hStart & 0b111));
	ov7670_write(REG_VSTART, vStart >> 2);
	ov7670_write(REG_VSTOP, vStop >> 2);
	ov7670_write(REG_VREF, 0x04);
}

uint8_t ov7670_init(void) {
	HAL_StatusTypeDef status = HAL_OK;
	/* Register Hard Reset */
	OV7670_RESET(LOW);
	HAL_Delay(20);
	OV7670_RESET(HIGH);
	HAL_Delay(20);

	/* Register Soft Reset */
	/* Set RegID COM7 (0x12) to 0x80 */
	status = ov7670_write(0x12, 0x80);
	if (status != HAL_OK) return status;
	HAL_Delay(100);

	ov7670_write(REG_CLKRC, 0x80);   		// CLKRC
	ov7670_write(REG_COM11, 0x0A);   		// COM11: auto 50/60Hz
	ov7670_write(REG_TSLB, 0x04);   		// TSLB
	ov7670_write(REG_COM7, 0x04);   		// COM7: RGB (VGA base)
	ov7670_write(REG_COM15, 0xD0);   		// COM15: RGB565

	frameControl(192, 48, 8, 488);

	ov7670_write(REG_COM10, 0x02);   		// COM10: VSYNC negative
	ov7670_write(REG_COM3, 0x04);   		// COM3: DCW enable
	ov7670_write(REG_MVFP, 0x2B);   		// MVFP: mirror/flip
		
	/* QQVGA */
	ov7670_write(REG_COM14, 0x1A);   		// COM14
	ov7670_write(SCALING_DCWCTR, 0x22); 	// DCWCTR: downsample by 4
	ov7670_write(SCALING_PCLK_DIV, 0xF2);	// PCLK divider	

	ov7670_write(0xB0, 0x84);				// no clue what this is but it's most important for colors
	saturation(0);
	ov7670_write(REG_COM8, 0xE7);   		// AEC, AGC, AWB on
	ov7670_write(0x6F, 0x9F);   			// simple AWB	

	/* Init Checking */
	/* Could also check REG_COM7 | REG_COM8 | SCALING_DCWCTR | SCALING_PCLK_DIV*/
	/* Checking data of TLSB (0x3A) register */
	if (ov7670_read(REG_TSLB) == 0x04) {
		char *str = "Success to initialize OV7670\n";
		HAL_UART_Transmit(&huart2, (uint8_t *) str, strlen(str), 100);
	}
	else {
		char *str = "Fail to initialize OV7670\n";
		HAL_UART_Transmit(&huart2, (uint8_t *) str, strlen(str), 100);
		return HAL_ERROR;
	}

	ov7670_flag = OV7670_READY;

	return status;
}

uint8_t ov7670_write(uint8_t memAddr, uint8_t data) {
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
								&hi2c1,
								OV7670_ADDW,
								memAddr,
								I2C_MEMADD_SIZE_8BIT,
								&data,
								1,
								100);
	return status;
}

uint8_t ov7670_read(uint8_t memAddr) {
	uint8_t retVal = 0x0;
	/* OV7670 Does not support "Repeated Start"
	 * -> Need to Stop after a Write Cmd to Mem-Address
	 * -> And Start again for a Read Cmd at that Mem-Address
	 */

//	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
//								&hi2c1,
//								OV7670_ADDR,
//								memAddr,
//								I2C_MEMADD_SIZE_8BIT,
//								&retVal,
//								1,
//								100);
	HAL_I2C_Master_Transmit(
				&hi2c1,
				OV7670_ADDW,
				&memAddr,
				1,
				100);

	HAL_Delay(20);

	HAL_I2C_Master_Receive(
				&hi2c1,
				OV7670_ADDR,
				&retVal,
				1,
				100);
	return retVal;
}

uint8_t ov7670_getData(void) {
    uint8_t rawData = 0;

    /* Put FIFO into data-output phase */
    OV7670_FIFO_RCK(HIGH);

    /* Read data while RCK is HIGH */
    rawData |= OV7670_DATA0() << 0;
    rawData |= OV7670_DATA1() << 1;
    rawData |= OV7670_DATA2() << 2;
    rawData |= OV7670_DATA3() << 3;
    rawData |= OV7670_DATA4() << 4;
    rawData |= OV7670_DATA5() << 5;
    rawData |= OV7670_DATA6() << 6;
    rawData |= OV7670_DATA7() << 7;

    /* Finish FIFO read cycle */
    OV7670_FIFO_RCK(LOW);

    return rawData;
}
