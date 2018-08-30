/**
 * Created by cottley on 6/6/2018.
 */

#ifndef ADS_TWO_AXIS_UTIL_H_
#define ADS_TWO_AXIS_UTIL_H_

#include <stdint.h>

#define ADS_AXIS_0_EN		(0x01)
#define ADS_AXIS_1_EN		(0x02)

/* Command set for ADS */
typedef enum {
	ADS_RUN = 0,
	ADS_SPS,
	ADS_RESET,
	ADS_DFU,
	ADS_SET_ADDRESS,
	ADS_INTERRUPT_ENABLE,
	ADS_GET_FW_VER,
	ADS_CALIBRATE,
	ADS_AXES_ENALBED,
	ADS_SHUTDOWN,
	ADS_GET_DEV_ID
} ADS_COMMAND_T;

/* Identifier for packet received from ADS */
typedef enum {
	ADS_SAMPLE = 0,
	ADS_FW_VER,
	ADS_DEV_ID
} ADS_PACKET_T;

/* Device IDS */
typedef enum {
	ADS_ONE_AXIS = 1,
	ADS_TWO_AXIS = 2,
} ADS_DEV_IDS_T;

/**@brief Function for decoding a int16 value.
 *
 * @param[in]   p_encoded_data   Buffer where the encoded data is stored.
 * @return      Decoded value.
 */
inline int16_t ads_int16_decode(const uint8_t * p_encoded_data)
{
        return ( (((uint16_t)(p_encoded_data)[0])) |
                 (((int16_t)(p_encoded_data)[1]) << 8 ));
}

/**@brief Function for decoding a uint16 value.
 *
 * @param[in]   p_encoded_data   Buffer where the encoded data is stored.
 * @return      Decoded value.
 */
inline uint16_t ads_uint16_decode(const uint8_t * p_encoded_data)
{
        return ( (((uint16_t)(p_encoded_data)[0])) |
                 (((uint16_t)(p_encoded_data)[1]) << 8 ));
}

/**@brief Function for encoding a uint16 value.
 *
 * @param[in]   value            Value to be encoded.
 * @param[out]  p_encoded_data   Buffer where the encoded data is to be written.
 *
 * @return      Number of bytes written.
 */
inline uint8_t ads_uint16_encode(uint16_t value, uint8_t * p_encoded_data)
{
    p_encoded_data[0] = (uint8_t) ((value & 0x00FF) >> 0);
    p_encoded_data[1] = (uint8_t) ((value & 0xFF00) >> 8);
    return sizeof(uint16_t);
}


#endif /* ADS_TWO_AXIS_UTIL_H_ */
