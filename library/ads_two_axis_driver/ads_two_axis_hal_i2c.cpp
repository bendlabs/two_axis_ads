/**
 * ads_hal_i2c.c
 *
 * Created by cottley on 6/6/2018.
 */

#include "ads_two_axis_hal.h"

/* Hardware Specific Includes */
#include "Arduino.h"
#include <Wire.h>

static void (*ads_read_callback)(uint8_t *);


static uint8_t read_buffer[ADS_TRANSFER_SIZE];

#define ADS_DEFAULT_ADDR		(0x13)			// Default I2C address of the ADS

static uint32_t ADS_RESET_PIN = 0;
static uint32_t ADS_INTERRUPT_PIN = 0;

static uint8_t _address = ADS_DEFAULT_ADDR;

volatile bool _ads_int_enabled = false;

/* Device I2C address array. Use ads_hal_update_addr() to 
 * populate this array. */
static uint8_t ads_addrs[ADS_COUNT] = {
	ADS_DEFAULT_ADDR,
};


/************************************************************************/
/*                        HAL Stub Functions                            */
/************************************************************************/
static inline void ads_hal_gpio_pin_write(uint8_t pin, uint8_t val);
static void ads_hal_pin_int_init(void);


/**
 * @brief ADS data ready interrupt. Reads out packet from ADS and fires callback in
 *  		  ads.c
 *
 * @param buffer[in]	Write buffer
 * @param len			Length of buffer.
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
void ads_hal_interrupt(void)
{
	if(ads_hal_read_buffer(read_buffer, ADS_TRANSFER_SIZE) == ADS_OK)
	{
		ads_read_callback(read_buffer);
	}
}

static void ads_hal_pin_int_init(void)
{
	pinMode(ADS_INTERRUPT_PIN, INPUT_PULLUP);
	ads_hal_pin_int_enable(true);
}


static inline void ads_hal_gpio_pin_write(uint8_t pin, uint8_t val)
{
	digitalWrite(pin, val);
}

void ads_hal_delay(uint16_t delay_ms)
{
	delay(delay_ms);
}

void ads_hal_pin_int_enable(bool enable)
{
	_ads_int_enabled = enable;
	
	if(enable)
	{
		attachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN), ads_hal_interrupt, FALLING);
	}
	else
	{
		detachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN));
	}
}

/**
 * @brief Write buffer of data to the Angular Displacement Sensor
 *
 * @param buffer[in]	Write buffer
 * @param len			Length of buffer.
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_write_buffer(uint8_t * buffer, uint8_t len)
{
	// Disable the interrupt
	if(_ads_int_enabled)
		detachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN));
	
	Wire.beginTransmission(_address);
	uint8_t nb_written = Wire.write(buffer, len);
	Wire.endTransmission();
	
	// Enable the interrupt
	if(_ads_int_enabled)
	{
		attachInterrupt(digitalPinToInterrupt(ADS_INTERRUPT_PIN), ads_hal_interrupt, FALLING);
		
		// Read data packet if interrupt was missed
		if(digitalRead(ADS_INTERRUPT_PIN) == 0)
		{
			if(ads_hal_read_buffer(read_buffer, ADS_TRANSFER_SIZE) == ADS_OK)
			{
				ads_read_callback(read_buffer);
			}
		}
	}
	
	if(nb_written == len)
		return ADS_OK;
	else
		return ADS_ERR_IO;
}

/**
 * @brief Read buffer of data from the Angular Displacement Sensor
 *
 * @param buffer[out]	Read buffer
 * @param len			Length of buffer.
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_read_buffer(uint8_t * buffer, uint8_t len)
{
	Wire.requestFrom(_address, len);
	
	uint8_t i = 0; 
	
	while(Wire.available())
	{
		buffer[i] = Wire.read();
		i++;
	}
	
	if(i == len)
		return ADS_OK;
	else
		return ADS_ERR_IO;
}

/**
 * @brief Reset the Angular Displacement Sensor
 *
 * @param dfuMode	Resets ADS into firmware update mode if true
 */
void ads_hal_reset(void)
{
	// Configure reset line as an output
	pinMode(ADS_RESET_PIN, OUTPUT);
	
	ads_hal_gpio_pin_write(ADS_RESET_PIN, 0);
	ads_hal_delay(10);
	ads_hal_gpio_pin_write(ADS_RESET_PIN, 1);
	
	pinMode(ADS_RESET_PIN, INPUT_PULLUP);
}

/**
 * @brief Initializes the hardware abstraction layer 
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
int ads_hal_init(void (*callback)(uint8_t*), uint32_t reset_pin, uint32_t datardy_pin)
{
	ADS_RESET_PIN     = reset_pin;
	ADS_INTERRUPT_PIN = datardy_pin;
	
	// Set callback pointer
	ads_read_callback = callback;
	
	// Reset the ads
	ads_hal_reset();
	
	// Wait for ads to initialize
	ads_hal_delay(2000);
	
	// Configure and enable interrupt pin
	ads_hal_pin_int_init();
	
	// Configure I2C bus
	Wire.begin();
	Wire.setClock(400000);

	return ADS_OK;
}

/**
 * @brief Selects the current device address of the ADS driver is communicating with
 *
 * @param device select device 0 - ADS_COUNT
 * @return	ADS_OK if successful ADS_ERR_BAD_PARAM if invalid device number
 */
int ads_hal_select_device(uint8_t device)
{
	if(device < ADS_COUNT)
		_address = ads_addrs[device];
	else
		return ADS_ERR_BAD_PARAM;
		
	return ADS_OK;
}

/**
 * @brief Updates the I2C address in the ads_addrs[] array. Updates the current
 *		  selected address.
 *
 * @param	device	device number of the device that is being updated
 * @param	address	new address of the ADS
 * @return	ADS_OK if successful ADS_ERR_BAD_PARAM if failed
 */
int ads_hal_update_device_addr(uint8_t device, uint8_t address)
{
	if(device < ADS_COUNT)
		ads_addrs[device] = address;
	else
		return ADS_ERR_BAD_PARAM;
		
	_address = address;
		
	return ADS_OK;	
}

/**
 * @brief Gets the current i2c address that the hal layer is addressing. 	
 *				Used by device firmware update (dfu)
 * @return	uint8_t _address
 */
uint8_t ads_hal_get_address(void)
{
	return _address;
}

/**
 * @brief Sets the i2c address that the hal layer is addressing *	
 *				Used by device firmware update (dfu)
 */
void ads_hal_set_address(uint8_t address)
{
	_address = address;
}


