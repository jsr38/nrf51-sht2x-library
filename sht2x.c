/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Copyright 2015 Argusat Limited 

*  Licensed under the Apache License, Version 2.0 (the "License"); 
*  you may not use this file except in compliance with the License. 
*  You may obtain a copy of the License at 
*
*    http://www.apache.org/licenses/LICENSE-2.0 
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/

/** @file
 *
 * @defgroup nrf51_sht2x_sht21 main.c
 * @{
 * @ingroup nrf51_sht2x_sht21
 * @brief SHT2x I2C library for nRF51 devices
 *
 * This file contains library code for communicating with
 * 
 */
 
#include "sht2x.h"
#include "twi_master.h"
#include "nrf_delay.h"

/*lint ++flb "Enter library region" */

/**
 * @brief Function for reading the current configuration of the sensor.
 *
 * @return uint8_t Zero if communication with the sensor failed. Contents (always non-zero) of configuration register (@ref SHT2X_ONESHOT_MODE and @ref SHT2X_CONVERSION_DONE) if communication succeeded.
 */
static uint8_t sht2x_check_crc(uint8_t data[], uint8_t nbrOfBytes, uint8_t checksum)
{
  uint8_t crc = 0;	
  uint8_t byteCtr;
  //calculates 8-Bit checksum with given polynomial
  for (byteCtr = 0; byteCtr < nbrOfBytes; ++byteCtr)
  { crc ^= (data[byteCtr]);
    for (uint8_t bit = 8; bit > 0; --bit)
    { if (crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
      else crc = (crc << 1);
    }
  }
  if (crc != checksum) return CHECKSUM_ERROR;
  else return 0;
}


/**
 * @brief Function for reading the current configuration of the sensor.
 *
 * @return uint8_t Zero if communication with the sensor failed. Contents (always non-zero) of configuration register (@ref SHT2X_ONESHOT_MODE and @ref SHT2X_CONVERSION_DONE) if communication succeeded.
 */
static uint8_t sht2x_config_read(void)
{

  u8t checksum;   //variable for checksum byte
  u8t error=0;    //variable for error code

  I2c_StartCondition();
  error |= I2c_WriteByte (I2C_ADR_W);
  error |= I2c_WriteByte (USER_REG_R);
  I2c_StartCondition();
  error |= I2c_WriteByte (I2C_ADR_R);
  *pRegisterValue = I2c_ReadByte(ACK);
  checksum=I2c_ReadByte(NO_ACK);
  error |= SHT2x_CheckCrc (pRegisterValue,1,checksum);
  I2c_StopCondition();
  return error;

  // ***************
  uint8_t config = 0;
  uint8_t checksum = 0;
    
  // Write: command protocol
  if (twi_master_transfer(SHT2X_I2C_ADDRESS, (uint8_t*)USER_REG_R, 1, TWI_DONT_ISSUE_STOP))
  {
    if (twi_master_transfer(SHT2X_I2C_ADDRESS | TWI_READ_BIT, &config, 1, TWI_ISSUE_STOP)) // Read: current configuration
    {
      // validate checksum
      if (twi_master_transfer(SHT2X_I2C_ADDRESS | TWI_READ_BIT, &checksum, 1, TWI_ISSUE_STOP)) // Read: checksum
      {
        
      }
      // Read succeeded, configuration stored to variable "config"
    }
    else
    {
      // Read failed
      config = 0;
    }
  } 

  return config;
}

/**
 * @brief Function for reading the current configuration of the sensor.
 *
 * @return uint8_t Zero if communication with the sensor failed. Contents (always non-zero) of configuration register (@ref SHT2X_ONESHOT_MODE and @ref SHT2X_CONVERSION_DONE) if communication succeeded.
 */
bool sht2x_init(uint8_t device_address)
{
  nrf_delay(15); // delay at least 15ms for the sensor to power up
  bool transfer_succeeded = true;



  uint8_t config = sht2x_config_read();

  if (config != 0)
  {
    // Configure SHT2X for 1SHOT mode if not done so already.
    if (!(config & SHT2X_ONESHOT_MODE))
    {
      uint8_t data_buffer[2];
  
      data_buffer[0] = command_access_config;
      data_buffer[1] = SHT2X_ONESHOT_MODE;
  
      transfer_succeeded &= twi_master_transfer(m_device_address, data_buffer, 2, TWI_ISSUE_STOP);
    }
  }
  else
  {
    transfer_succeeded = false;
  }

  return transfer_succeeded;
}

bool sht2x_start_temp_conversion(void)
{
  return twi_master_transfer(m_device_address, (uint8_t*)&command_start_convert_temp, 1, TWI_ISSUE_STOP);
}

bool sht2x_is_temp_conversion_done(void)
{
  uint8_t config = sht2x_config_read();

  if (config & SHT2X_CONVERSION_DONE)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool sht2x_temp_read(int8_t *temperature_in_celcius, int8_t *temperature_fraction)
{
  bool transfer_succeeded = false;

  // Write: Begin read temperature command
  if (twi_master_transfer(m_device_address, (uint8_t*)&command_read_temp, 1, TWI_DONT_ISSUE_STOP))
  {
    uint8_t data_buffer[2];

    // Read: 2 temperature bytes to data_buffer
    if (twi_master_transfer(m_device_address | TWI_READ_BIT, data_buffer, 2, TWI_ISSUE_STOP)) 
    {
      *temperature_in_celcius = (int8_t)data_buffer[0];
      *temperature_fraction = (int8_t)data_buffer[1];
      
      transfer_succeeded = true;
    }
  }

  return transfer_succeeded;
}

/*lint --flb "Leave library region" */ 