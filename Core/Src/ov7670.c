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
	HAL_Delay(10);

	/* OV7670 Register Configuration */
	ov7670_write(0x3a, 0x04);
	ov7670_write(0x40, 0xd0);
	ov7670_write(0x12, 0x14);
	ov7670_write(0x32, 0x80);
	ov7670_write(0x17, 0x16);
	ov7670_write(0x18, 0x04);
	ov7670_write(0x19, 0x02);
	ov7670_write(0x1a, 0x7b);
	ov7670_write(0x03, 0x06);
	ov7670_write(0x0c, 0x00);
	// ov7670_write(0x3e, 0x00);
	ov7670_write(0x3e, 0x1a); // [NEW] COM14: enable manual scaling / PCLK division
	// ov7670_write(0x70, 0x3a);
	ov7670_write(0x71, 0x35); // If it is set to ov7670_write (0x71, 0x80); it will display eight colored vertical bars, for debugging purposes.
	ov7670_write(0x72, 0x11);
	// ov7670_write(0x73, 0x00);
	ov7670_write(0x73, 0xf2); // [NEW] QQVGA: PCLK divide by 4
	ov7670_write(0xa2, 0x02);
	ov7670_write(0x11, 0x81);

	ov7670_write(0x7a, 0x20);
	ov7670_write(0x7b, 0x1c);
	ov7670_write(0x7c, 0x28);
	ov7670_write(0x7d, 0x3c);
	ov7670_write(0x7e, 0x55);
	ov7670_write(0x7f, 0x68);
	ov7670_write(0x80, 0x76);
	ov7670_write(0x81, 0x80);
	ov7670_write(0x82, 0x88);
	ov7670_write(0x83, 0x8f);
	ov7670_write(0x84, 0x96);
	ov7670_write(0x85, 0xa3);
	ov7670_write(0x86, 0xaf);
	ov7670_write(0x87, 0xc4);
	ov7670_write(0x88, 0xd7);
	ov7670_write(0x89, 0xe8);

	ov7670_write(0x13, 0xe0);
	ov7670_write(0x00, 0x00);

	ov7670_write(0x10, 0x00);
	ov7670_write(0x0d, 0x00);
	ov7670_write(0x14, 0x28);
	ov7670_write(0xa5, 0x05);
	ov7670_write(0xab, 0x07);
	ov7670_write(0x24, 0x75);
	ov7670_write(0x25, 0x63);
	ov7670_write(0x26, 0xA5);
	ov7670_write(0x9f, 0x78);
	ov7670_write(0xa0, 0x68);
	ov7670_write(0xa1, 0x03);
	ov7670_write(0xa6, 0xdf);
	ov7670_write(0xa7, 0xdf);
	ov7670_write(0xa8, 0xf0);
	ov7670_write(0xa9, 0x90);
	ov7670_write(0xaa, 0x94);
	ov7670_write(0x13, 0xe5);

	ov7670_write(0x0e, 0x61);
	ov7670_write(0x0f, 0x4b);
	ov7670_write(0x16, 0x02);
	ov7670_write(0x1e, 0x37);
	ov7670_write(0x21, 0x02);
	ov7670_write(0x22, 0x91);
	ov7670_write(0x29, 0x07);
	ov7670_write(0x33, 0x0b);
	ov7670_write(0x35, 0x0b);
	ov7670_write(0x37, 0x1d);
	ov7670_write(0x38, 0x71);
	ov7670_write(0x39, 0x2a);
	ov7670_write(0x3c, 0x78);
	ov7670_write(0x4d, 0x40);
	ov7670_write(0x4e, 0x20);
	ov7670_write(0x69, 0x00);
	ov7670_write(0x6b, 0x60);
	ov7670_write(0x74, 0x19);
	ov7670_write(0x8d, 0x4f);
	ov7670_write(0x8e, 0x00);
	ov7670_write(0x8f, 0x00);
	ov7670_write(0x90, 0x00);
	ov7670_write(0x91, 0x00);
	ov7670_write(0x92, 0x00);
	ov7670_write(0x96, 0x00);
	ov7670_write(0x9a, 0x80);
	ov7670_write(0xb0, 0x84);
	ov7670_write(0xb1, 0x0c);
	ov7670_write(0xb2, 0x0e);
	ov7670_write(0xb3, 0x82);
	ov7670_write(0xb8, 0x0a);


	ov7670_write(0x43, 0x14);
	ov7670_write(0x44, 0xf0);
	ov7670_write(0x45, 0x34);
	ov7670_write(0x46, 0x58);
	ov7670_write(0x47, 0x28);
	ov7670_write(0x48, 0x3a);
	ov7670_write(0x59, 0x88);
	ov7670_write(0x5a, 0x88);
	ov7670_write(0x5b, 0x44);
	ov7670_write(0x5c, 0x67);
	ov7670_write(0x5d, 0x49);
	ov7670_write(0x5e, 0x0e);
	ov7670_write(0x64, 0x04);
	ov7670_write(0x65, 0x20);
	ov7670_write(0x66, 0x05);
	ov7670_write(0x94, 0x04);
	ov7670_write(0x95, 0x08);
	ov7670_write(0x6c, 0x0a);
	ov7670_write(0x6d, 0x55);
	ov7670_write(0x6e, 0x11);
	ov7670_write(0x6f, 0x9f);
	ov7670_write(0x6a, 0x40);
	ov7670_write(0x01, 0x40);
	ov7670_write(0x02, 0x40);
	ov7670_write(0x13, 0xe7);
	ov7670_write(0x15, 0x00);


	ov7670_write(0x4f, 0x80);
	ov7670_write(0x50, 0x80);
	ov7670_write(0x51, 0x00);
	ov7670_write(0x52, 0x22);
	ov7670_write(0x53, 0x5e);
	ov7670_write(0x54, 0x80);
	ov7670_write(0x58, 0x9e);

	ov7670_write(0x41, 0x08);
	ov7670_write(0x3f, 0x00);
	ov7670_write(0x75, 0x05);
	ov7670_write(0x76, 0xe1);
	ov7670_write(0x4c, 0x00);
	ov7670_write(0x77, 0x01);
	ov7670_write(0x3d, 0xc2);
	ov7670_write(0x4b, 0x09);
	ov7670_write(0xc9, 0x60);
	ov7670_write(0x41, 0x38);
	ov7670_write(0x56, 0x40);

	ov7670_write(0x34, 0x11);
	ov7670_write(0x3b, 0x02);

	ov7670_write(0xa4, 0x89);
	ov7670_write(0x96, 0x00);
	ov7670_write(0x97, 0x30);
	ov7670_write(0x98, 0x20);
	ov7670_write(0x99, 0x30);
	ov7670_write(0x9a, 0x84);
	ov7670_write(0x9b, 0x29);
	ov7670_write(0x9c, 0x03);
	ov7670_write(0x9d, 0x4c);
	ov7670_write(0x9e, 0x3f);
	ov7670_write(0x78, 0x04);

	ov7670_write(0x79, 0x01);
	ov7670_write(0xc8, 0xf0);
	ov7670_write(0x79, 0x0f);
	ov7670_write(0xc8, 0x00);
	ov7670_write(0x79, 0x10);
	ov7670_write(0xc8, 0x7e);
	ov7670_write(0x79, 0x0a);
	ov7670_write(0xc8, 0x80);
	ov7670_write(0x79, 0x0b);
	ov7670_write(0xc8, 0x01);
	ov7670_write(0x79, 0x0c);
	ov7670_write(0xc8, 0x0f);
	ov7670_write(0x79, 0x0d);
	ov7670_write(0xc8, 0x20);
	ov7670_write(0x79, 0x09);
	ov7670_write(0xc8, 0x80);
	ov7670_write(0x79, 0x02);
	ov7670_write(0xc8, 0xc0);
	ov7670_write(0x79, 0x03);
	ov7670_write(0xc8, 0x40);
	ov7670_write(0x79, 0x05);
	ov7670_write(0xc8, 0x30);
	ov7670_write(0x79, 0x26);
	ov7670_write(0x09, 0x00);

//	ov7670_write(REG_COM7, 0b10000000);		//all registers default
//	ov7670_write(REG_CLKRC, 0b10000000);	//double clock
//	ov7670_write(REG_COM11, 0b1000 | 0b10);	//enable auto 50/60Hz detect + exposure timing can be less...
//	ov7670_write(REG_TSLB, 0b100);	//sequence UYVY
//	ov7670_write(REG_COM7, 0b100);	//RGB
//	ov7670_write(REG_COM15,  0b11000000 | 0b010000);	//RGB565

//	frameControl(192, 48, 8, 488); //no clue why horizontal needs such strange values, vertical works ok

	ov7670_write(REG_COM10, 0x02);	//VSYNC negative
	ov7670_write(REG_COM3, 0x04);	//DCW enable (This is absolutely essential)
	ov7670_write(REG_MVFP, 0x2b);	//mirror flip

    //160x120 (1/4)
	ov7670_write(REG_COM14, 0x1a);	//pixel clock divided by 4, manual scaling enable, DCW and PCLK controlled by register
	ov7670_write(SCALING_DCWCTR, 0x22);	//downsample by 4
	ov7670_write(SCALING_PCLK_DIV, 0xf2); //pixel clock divided by 4

//	ov7670_write(0xb0, 0x84);	// no clue what this is but it's most important for colors
//	saturation(0);
//	ov7670_write(0x13, 0xe7);	//AWB on
//	ov7670_write(0x6f, 0x9f);	// Simple AWB


	/* Init Checking */
	/* Checking data of TLSB (0x3A) register */
	if (ov7670_read(0x3A) == 0x04) {
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
