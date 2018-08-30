/**
 * Created by cottley on 6/6/2018.
 */

#include "ads_two_axis.h"

static ads_callback ads_data_callback;

/**
 * @brief Parses sample buffer from two axis ADS. Scales to degrees and
 *				executes callback registered in ads_two_axis_init. 
 *				This function is called from ads_two_axis_hal. Application should never call this function.
 */	
void ads_two_axis_parse_read_buffer(uint8_t * buffer)
{
	if(buffer[0] == ADS_SAMPLE)
	{
		float sample[2];
				
		int16_t temp = ads_int16_decode(&buffer[1]);
		sample[0] = (float)temp/32.0f;
		
		temp = ads_int16_decode(&buffer[3]);
		sample[1] = (float)temp/32.0f;
		
		ads_data_callback(sample);
	}
}

/**
 * @brief Places ADS in free run or sleep mode
 *
 * @param	run	true if activating ADS, false is putting in suspend mode
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_two_axis_run(bool run)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
		
	buffer[0] = ADS_RUN;
	buffer[1] = run;
		
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Sets the sample rate of the ADS in free run mode
 *
 * @param	sps ADS_SPS_T sample rate
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_two_axis_set_sample_rate(ADS_SPS_T sps)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_SPS;
	ads_uint16_encode(sps, &buffer[1]);
	
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Enables the ADS data ready interrupt line
 *
 * @param	run	true if activating ADS, false is putting in suspend mode
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_two_axis_enable_interrupt(bool enable)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_INTERRUPT_ENABLE;
	buffer[1] = enable;
	
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Updates the I2C address of the selected ADS. The default address 
 *		  is 0x13. Use this function to program an ADS to allow multiple
 *		  devices on the same I2C bus. 
 *
 * @param	device	device number of the device that is being updated
 * @param	address	new address of the ADS
 * @return	ADS_OK if successful ADS_ERR_IO or ADS_ERR_BAD_PARAM if failed
 */
int ads_two_axis_update_device_address(uint8_t device, uint8_t address)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_SET_ADDRESS;
	buffer[1] = address;
	
	if(ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE) != ADS_OK)
		return ADS_ERR_IO;
	
	return ads_hal_update_device_addr(device, address);	
}

/**
 * @brief Updates selected device number the HAL is communicating with
 *
 * @param	device number 0-9 
 * @return	ADS_OK if successful ADS_ERR_BAD_PARAM if failed
 */
int ads_two_axis_select_device(uint8_t device)
{
	return ads_hal_select_device(device);
}

/**
 * @brief Initializes the hardware abstraction layer and sample rate of the ADS
 *
 * @param	ads_init_t	initialization structure of the ADS
 * @return	ADS_OK if successful ADS_ERR if failed
 */
int ads_two_axis_init(ads_init_t * ads_init)
{	
	ads_hal_init(&ads_two_axis_parse_read_buffer, ads_init->reset_pin, ads_init->datardy_pin);	
	
	ads_data_callback = ads_init->ads_sample_callback;
	
	// Check that the device id matched ADS_TWO_AXIS
	if(ads_get_dev_id() != ADS_OK)
		return ADS_ERR_DEV_ID;
	
	ads_hal_delay(2);
 	
#if ADS_DFU_CHECK
 	if(ads_two_axis_dfu_check((uint8_t)ADS_GET_FW_VER))
	{
		ads_two_axis_dfu_reset();
		ads_hal_delay(100);		// Give ADS time to reset
 		if(ads_two_axis_dfu_update() != ADS_OK)
			return ADS_ERR;
		ads_hal_delay(200);	// Let it reinitialize
	}
#endif

	if(ads_two_axis_set_sample_rate(ads_init->sps))
		return ADS_ERR;

	return ADS_OK;
}

/**
 * @brief Calibrates two axis ADS. ADS_CALIBRATE_FIRST must be at 0 degrees on both AXES.
 *				ADS_CALIBRATE_FLAT can be at 45 - 255 degrees, recommended 90 degrees.
 * 				When calibrating the flat axis the perpendicular axis should be at 0 degrees.
 *				ADS_CALIBRATE_PERP can be at 45 - 255 degrees, recommended 90 degrees.
 *				When calibrating the perpendicular axis the flat axis should be at 0 degrees
 *
 *				Note:	The flat axis is sample[0] (axis 0) perp axis is sample[1] (axis 1) 
 *				from ads_data_callback
 *
 * @param	ads_calibration_step 	ADS_CALIBRATE_STEP_T to perform
 * @param degrees uint8_t angle at which sensor is bent when performing 
 *				ADS_CALIBRATE_FLAT, and ADS_CALIBRATE_PERP
 * @return	ADS_OK if successful ADS_ERR_IO or ADS_BAD_PARAM if failed
 */
int ads_two_axis_calibrate(ADS_CALIBRATION_STEP_T ads_calibration_step, uint8_t degrees)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_CALIBRATE;
	buffer[1] = ads_calibration_step;
	buffer[2] = degrees;
	
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Enables/disables individual axes of the sensor. Both axes are
 *				enabled at reset. 
 *				ADS_AXIS_0_EN | ADS_AXIS_1_EN enables both.
 *				ADS_AXIS_0_EN enables axis zero and disables axis one
 *				ADS_AXIS_1_EN enables axis one and disables axis zero
 *
 * @param	axes_enabled	bit mask of which axes to enable
 * @return	ADS_OK if successful ADS_ERR_IO or ADS_BAD_PARAM if failed
 */
int ads_two_axis_enable_axis(uint8_t axes_enable)
{
	if(!(axes_enable & (ADS_AXIS_0_EN | ADS_AXIS_1_EN)))
			return ADS_ERR_BAD_PARAM;
	
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_AXES_ENALBED;
	buffer[1] = axes_enable;
	
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Shutdown ADS. Requires reset to wake up from Shutdown. ~50nA in shutdwon
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_two_axis_shutdown(void)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_SHUTDOWN;
	
	return ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
}

/**
 * @brief Wakes up ADS from shutdown. Delay is necessary for ADS to reinitialize 
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_two_axis_wake(void)
{
	// Reset ADS to wake from shutdown
	ads_hal_reset();
	
	// Allow time for ADS to reinitialize 
	ads_hal_delay(100);	
	
	return ADS_OK;
}

/**
 * @brief Checks that the device id is ADS_TWO_AXIS. ADS should not be in free run
 *				when this function is called.
 *
 * @return	ADS_OK if dev_id is ADS_TWO_AXIS, ADS_ERR_DEV_ID if not
 */
int ads_get_dev_id(void)
{
	uint8_t buffer[ADS_TRANSFER_SIZE];
	
	buffer[0] = ADS_GET_DEV_ID;
	
	// Disable interrupt to prevent callback from reading out device id
	ads_hal_pin_int_enable(false);
	
	ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
	ads_hal_delay(2);
	ads_hal_read_buffer(buffer, ADS_TRANSFER_SIZE);
	
	ads_hal_pin_int_enable(true);
	
	/* Check that packet read is a device id packet and that
	 * and that the device id is a two axis sensor */
	if(buffer[0] == ADS_DEV_ID && buffer[1] == ADS_TWO_AXIS)
	{
		return ADS_OK;
	}
	else
	{
		return ADS_ERR_DEV_ID;
	}
}
