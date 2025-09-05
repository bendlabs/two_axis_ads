/**
 * Created by nrudh on 1/15/2025.
 * 
 * This software is provided "as is", without any warranty of any kind, express or implied,
 * including but not limited to the warranties of merchantability, fitness for a particular purpose,
 * and noninfringement. In no event shall the authors or copyright holders be liable for any claim,
 * damages, or other liability, whether in an action of contract, tort, or otherwise, arising from,
 * out of, or in connection with the software or the use or other dealings in the software.
 */

#ifndef ADS_TWO_AXIS_ERR_
#define ADS_TWO_AXIS_ERR_

#include <stdint.h>


#define ADS_OK                 (0)  /**< Success */
#define ADS_ERR                (-1) /**< General Error */
#define ADS_ERR_BAD_PARAM      (-2) /**< Bad parameter to an API call */
#define ADS_ERR_OP_IN_PROGRESS (-3) /**< Operation in progress */
#define ADS_ERR_IO             (-4) /**< Error communicating with ads */
#define ADS_ERR_DEV_ID         (-5) /**< Device ID does not match expected ID */
#define ADS_ERR_TIMEOUT        (-6) /**< Operation timed out */


#endif /* ADS_TWO_AXIS_ERR_ */
