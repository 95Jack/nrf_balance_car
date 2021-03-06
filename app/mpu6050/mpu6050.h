/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#ifndef MPU6050_H
#define MPU6050_H

/*lint ++flb "Enter library region" */

#include <stdbool.h>
#include <stdint.h>
#define   mpu6050_device_address 0x68;          // !< Device address in bits [7:1]

#define MPU6050_TEMP_OUT        0x41
#define MPU6050_GYRO_OUT        0x43
#define MPU6050_ACC_OUT         0x3B

bool mpu6050_write(uint8_t device_address, uint8_t register_address, uint8_t len, uint8_t *data);
bool mpu6050_read(uint8_t device_address, uint8_t register_address, uint8_t number_of_bytes, uint8_t *destination);
/*lint --flb "Leave library region" */


/**
 * @brief Function for initializing MPU6050 and verifies it's on the bus.
 *
 * @param device_address Device TWI address in bits [6:0].
 * @return
 * @retval true MPU6050 found on the bus and ready for operation.
 * @retval false MPU6050 not found on the bus or communication failure.
 */
bool mpu6050_init(uint8_t device_address);

/**
  @brief Function for writing a MPU6050 register contents over TWI.
  @param[in]  register_address Register address to start writing to
  @param[in] value Value to write to register
  @retval true Register write succeeded
  @retval false Register write failed
*/
bool mpu6050_register_write(uint8_t register_address, const uint8_t value);

/**
  @brief Function for reading MPU6050 register contents over TWI.
  Reads one or more consecutive registers.
  @param[in]  register_address Register address to start reading from
  @param[in]  number_of_bytes Number of bytes to read
  @param[out] destination Pointer to a data buffer where read data will be stored
  @retval true Register read succeeded
  @retval false Register read failed
*/
bool mpu6050_register_read(uint8_t register_address, uint8_t *destination, uint8_t number_of_bytes);

/**
  @brief Function for reading and verifying MPU6050 product ID.
  @retval true Product ID is what was expected
  @retval false Product ID was not what was expected
*/
bool mpu6050_verify_product_id(void);

bool MPU6050_ReadAcc( int16_t *pACC_X , int16_t *pACC_Y , int16_t *pACC_Z );
bool MPU6050_ReadGyro(int16_t *pGYRO_X , int16_t *pGYRO_Y , int16_t *pGYRO_Z );
void mpu_6050_read_temp(double * p_temp);

#endif /* MPU6050_H */
