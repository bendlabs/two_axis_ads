/**
 * Created by cottley on 6/6/2018.
 */

#ifndef ADS_TWO_AXIS_H_
#define ADS_TWO_AXIS_H_

#include <stdint.h>
#include <stdbool.h>
#include "ads_two_axis_hal.h"
#include "ads_two_axis_err.h"
#include "ads_two_axis_dfu.h"
#include "ads_two_axis_util.h"

#define ADS_DFU_CHECK				(1)		// Set this to 1 to check if the newest firmware is on the ADS

typedef void (*ads_callback)(float*);


typedef enum {
	ADS_CALIBRATE_FIRST = 0,
	ADS_CALIBRATE_FLAT,
	ADS_CALIBRATE_PERP,
	ADS_CALIBRATE_CLEAR
} ADS_CALIBRATION_STEP_T;


typedef enum {
	ADS_1_HZ   = 16384,
	ADS_10_HZ  = 1638,
	ADS_20_HZ  = 819,
	ADS_50_HZ  = 327,
	ADS_100_HZ = 163,
	ADS_200_HZ = 81,
	ADS_333_HZ = 49,
	ADS_500_HZ = 32,
} ADS_SPS_T;

typedef struct {
	ADS_SPS_T sps;
	ads_callback ads_sample_callback;
	uint32_t reset_pin;
	uint32_t datardy_pin;
} ads_init_t;


/**
 * @brief Places ADS in free run or sleep mode
 *
 * @param	run	true if activating ADS, false is putting in suspend mode
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_two_axis_run(bool run);

/**
 * @brief Sets the sample rate of the ADS in free run mode
 *
 * @param	sps ADS_SPS_T sample rate
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_two_axis_set_sample_rate(ADS_SPS_T sps);

/**
 * @brief Enables the ADS data ready interrupt line
 *
 * @param	run	true if activating ADS, false is putting in suspend mode
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_two_axis_enable_interrupt(bool enable);

/**
 * @brief Updates the I2C address of the selected ADS. The default address 
 *		  is 0x12. Use this function to program an ADS to allow multiple
 *		  devices on the same I2C bus.
 *
 * @param	device	device number of the device that is being updated
 * @param	address	new address of the ADS
 * @return	ADS_OK if successful ADS_ERR_IO or ADS_ERR_BAD_PARAM if failed
 */
int ads_two_axis_update_device_address(uint8_t device, uint8_t address);

/**
 * @brief Initializes the hardware abstraction layer and sample rate of the ADS
 *
 * @param	ads_init_t	initialization structure of the ADS
 * @return	ADS_OK if successful ADS_ERR if failed
 */
int ads_two_axis_init(ads_init_t * ads_init);

/**
 * @brief Calibrates two axis ADS. ADS_CALIBRATE_FIRST must be at 0 degrees on both AXES.
 *				ADS_CALIBRATE_FLAT can be at 45 - 255 degrees, recommended 90 degrees.
 * 				When calibrating the flat axis the perpendicular axis should be at 0 degrees.
 *				ADS_CALIBRATE_PERP can be at 45 - 255 degrees, recommended 90 degrees.
 *				When calibrating the perpendicular axis the flat axis should be at 0 degrees
 *
 *				Note:	The flat axis is sample[0] perp axis is sample[1] from ads_data_callback
 *
 * @param	ads_calibration_step 	ADS_CALIBRATE_STEP_T to perform
 * @param degrees uint8_t angle at which sensor is bent when performing ADS_CALIBRATE_FLAT, and ADS_CALIBRATE_PERP
 * @return	ADS_OK if successful ADS_ERR_IO or ADS_BAD_PARAM if failed
 */
int ads_two_axis_calibrate(ADS_CALIBRATION_STEP_T ads_calibration_step, uint8_t degrees);

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
int ads_two_axis_enable_axis(uint8_t axes_enable);

/**
 * @brief Shutdown ADS. Requires reset to wake up from Shutdown. ~50nA in shutdwon
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_two_axis_shutdown(void);

/**
 * @brief Wakes up ADS from shutdown. Delay is necessary for ADS to reinitialize 
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_two_axis_wake(void);

/**
 * @brief Checks that the device id is ADS_TWO_AXIS. ADS should not be in free run
					when this function is called.
 *
 * @return	ADS_OK if dev_id is ADS_TWO_AXIS, ADS_ERR_DEV_ID if not
 */
int ads_get_dev_id(void);

#endif /* ADS_TWO_AXIS_H_ */
