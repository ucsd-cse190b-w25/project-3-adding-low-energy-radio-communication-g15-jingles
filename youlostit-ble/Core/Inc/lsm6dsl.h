/*
 * lsm6dsl.h
 *
 *  Created on: Feb 6, 2025
 *      Author: mremi
 */

#ifndef LSM6DSL_C_
#define LSM6DSL_C_

#include <stdint.h>

#define XL_ADDR              0b1101010

/* Read/Write Constants*/
#define WRITE                0x00
#define READ                 0x01

/* Accelerometer Register Addresses*/
#define CTRL1_XL             0x10
#define INT1_CTRL            0x0D
#define STATUS_REG           0x1E

#define OUTX_L_XL            0x28
#define OUTX_H_XL            0x29
#define OUTY_L_XL            0x2A
#define OUTY_H_XL            0x2B
#define OUTZ_L_XL            0x2C
#define OUTZ_H_XL            0x2D

/* Data rate and output mode selection */
#define ODR_416_HZ           0x60

/* INT1_CTRL Flags */
#define INT1_DRDY_XL         0x01 // data ready interrupt flag

/* STATUS_REG Flags */
#define STATUS_REG_XDLA_MSK  0x01

void lsm6dsl_init();
void lsm6dsl_read_xyz(int16_t* x, int16_t* y, int16_t* z);

#endif /* LSM6DSL_C_ */
