/*
 * lsm6dsl.h
 *
 *  Created on: Feb 6, 2025
 *      Author: jingles
 */

#include "lsm6dsl.h"
#include "i2c.h"

void lsm6dsl_init(){

	// setting up accelerometer ODR (also turns on XL)
	uint8_t ctrl1_data[] = {CTRL1_XL, ODR_416_HZ};
	i2c_transaction(XL_ADDR, WRITE, ctrl1_data, 2);

	// Setting data ready flag for reading
	uint8_t int1_ctrl_data[] = {INT1_CTRL, INT1_DRDY_XL};
	i2c_transaction(XL_ADDR, WRITE, int1_ctrl_data, 2);
}

void lsm6dsl_read_xyz(int16_t* x, int16_t* y, int16_t* z) {

	/* Read XL data */
	uint8_t register_addr[] = {OUTX_L_XL, OUTX_H_XL, OUTY_L_XL, OUTY_H_XL, OUTZ_L_XL, OUTZ_H_XL};
	uint8_t xyz_data[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	for(int i = 0; i < 6; i++) {
		i2c_transaction(XL_ADDR, WRITE, &register_addr[i], 1);
		i2c_transaction(XL_ADDR, READ, &xyz_data[i], 1);
	}

	/* Update data (Maybe data process? I don't know what that entails) */
	*x = ((xyz_data[1]) << 8) | (xyz_data[0]);
	*y = ((xyz_data[3]) << 8) | (xyz_data[2]);
	*z = ((xyz_data[5]) << 8) | (xyz_data[4]);

}

	/* Init function*/
	// setting up accelerometer ODR (also turns on XL)
//	uint8_t ctrl1_addr = CTRL1_XL;
//	uint8_t ctrl1_data = ODR_416_HZ;
//
//	i2c_transaction(XL_ADDR, WRITE, &ctrl1_addr, 1);
//	i2c_transaction(XL_ADDR, WRITE, &ctrl1_data, 1);
//
//	// Setting data ready flag for reading
//	uint8_t int1_ctrl_addr = INT1_CTRL;
//	uint8_t int1_ctrl_data = INT1_DRDY_XL;
//
//	i2c_transaction(XL_ADDR, WRITE, &int1_ctrl_addr, 1);
//	i2c_transaction(XL_ADDR, WRITE, &int1_ctrl_data, 1);

	/* Reading function, readiung from XL*/

/* Read XDLA bit to see if XL data is ready*/
//	uint8_t status_reg_addr = STATUS_REG;
//	uint8_t status_reg_data = 0x00;

//	while(!(status_reg_data & STATUS_REG_XDLA_MSK)) {
//		i2c_transaction(XL_ADDR, WRITE, &status_reg_addr, 1);
//		i2c_transaction(XL_ADDR, READ, &status_reg_data, 1);
//	}

	/* Read XL data */
//	uint8_t xyz_data[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//	uint8_t register_addr = OUTX_L_XL;
//	for(int i = 0; i < 6; i++) {
//		register_addr = OUTX_L_XL + i;
//		i2c_transaction(XL_ADDR, WRITE, &register_addr, 1);
//		i2c_transaction(XL_ADDR, READ, &xyz_data[i], 1);
//	}
