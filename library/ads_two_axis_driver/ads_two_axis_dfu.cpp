/**
 * Created by cottley on 6/6/2018.
 */

#include "ads_two_axis_dfu.h"
#include "ads_two_axis_fw.h"
#include <string.h>

#define ADS_BOOTLOADER_ADDRESS (0x12)

static inline int ads_two_axis_dfu_get_ack(void);

/**
 * @brief Checks if the firmware image in the driver is newer than 
 *			the firmware on the device.
 *
 * @param ads_get_fw_ver	Get fw version command
 * @return	TRUE if update needed. FALSE if no updated needed
 */
bool ads_two_axis_dfu_check(uint8_t ads_get_fw_ver)
{
	uint8_t buffer[] = {ads_get_fw_ver, 0, 0};
	uint16_t fw_ver;
	
	ads_hal_pin_int_enable(false);
	
	ads_hal_write_buffer(buffer, ADS_TRANSFER_SIZE);
	ads_hal_delay(2);
	ads_hal_read_buffer(buffer, ADS_TRANSFER_SIZE);
	
	ads_hal_pin_int_enable(true);
	
	if(buffer[0] == ADS_FW_VER)
	{
		fw_ver = ads_uint16_decode(&buffer[1]);
		fw_ver = ads_uint16_decode(&buffer[1]);
	}
	else
	{
		return false;
	}
		
	if(fw_ver < ads_fw_rev)
		return true;
	return false;
}

/**
 * @brief Reads acknowledgment byte from the ADS bootloader
 *
 * @return	ADS_OK if successful ADS_ERR_TIMEOUT if failed
 */
static inline int ads_two_axis_dfu_get_ack(void)
{
	uint8_t timeout = 250;
	uint8_t ack = 0;
	
	do
	{
		ads_hal_read_buffer(&ack, 1);
	} while(--timeout && ack != 's');
	
	if(timeout)
		return ADS_OK;
	else
		return ADS_ERR_TIMEOUT;
}

/**
 * @brief Resets the ADS into bootloader mode
 *
 * @return	ADS_OK if successful ADS_ERR_IO if failed
 */
 int ads_two_axis_dfu_reset(void)
 {
	 uint8_t packet[ADS_TRANSFER_SIZE];
	 
	 packet[0] = ADS_DFU;
	 packet[1] = packet[2] = 0;
	 
	 
	 return ads_hal_write_buffer(packet, ADS_TRANSFER_SIZE);
 }

/**
 * @brief Writes firmware image to ADS contained in ads_fw.h
 *
 * @return	ADS_OK if successful ADS_ERR_TIMEOUT if failed
 */
int ads_two_axis_dfu_update(void)
{
	uint32_t len = sizeof(ads_fw);
	
	uint8_t page_size   = 64;
	uint8_t block_len   = page_size;
	
	uint8_t packet[page_size];
	
	// Store a local copy of the current i2c address
	uint8_t address = ads_hal_get_address();
	
	// Set the i2c address to the booloader address
	ads_hal_set_address(ADS_BOOTLOADER_ADDRESS);
	
	// Transmit the length of the new firmware to the bootloader
	packet[0] = (uint8_t)(len & 0xff);
	packet[1] = (uint8_t)((len >> 8) & 0xff);
	packet[2] = (uint8_t)((len>>16) & 0xff);
	packet[3] = (uint8_t)((len>>24) & 0xff); 
	
	ads_hal_write_buffer(packet, 4);
	
	// Get acknowledgement of the fw length
	if(ads_two_axis_dfu_get_ack() != ADS_OK)
	{
		// restore i2c address
		ads_hal_set_address(address);
		return ADS_ERR_TIMEOUT;
	}
	
	// Transfer the new firmware image
	uint32_t nb_blocks = len/block_len;
	uint32_t rem_data  = len % block_len;
	
	for(uint32_t i = 0; i < nb_blocks; i++)
	{
		// Copy the next page
		memcpy(packet, &ads_fw[i*page_size], page_size);
		
		// Send the page
		ads_hal_write_buffer(packet, page_size/2);
		
		ads_hal_write_buffer(&packet[page_size/2], page_size/2);
		
		// Get acknowledgement of the recieved page
		if(ads_two_axis_dfu_get_ack() != ADS_OK)
		{
			// restore i2c address
			ads_hal_set_address(address);
			return ADS_ERR_TIMEOUT;
		}
	}
	
	// Transfer the remainder and get acknowledgement
	memcpy(packet, &ads_fw[nb_blocks*block_len], rem_data);
	
	if(rem_data > page_size/2)
	{
		ads_hal_write_buffer(packet, page_size/2);
		ads_hal_write_buffer(&packet[page_size/2], rem_data - page_size/2);
	}
	else
		ads_hal_write_buffer(packet, rem_data);
	
	if(ads_two_axis_dfu_get_ack() != ADS_OK)
	{
		// restore i2c address
		ads_hal_set_address(address);
		return ADS_ERR_TIMEOUT;
	}
	
	// restore i2c address
	ads_hal_set_address(address);
	
	return ADS_OK;
}

