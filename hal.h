#ifndef _HAL_H_
#define _HAL_H_

/*******************************************************************************
 * API
 ******************************************************************************/
/**
 * @brief Initialize for HAL
 *
 * @param [in] file_path is path to file
 * @return true if initialize success
 * @return false if initialize fail
 */
bool kmc_init(const uint8_t *const file_path);

/**
 * @brief Update size of sector
 *
 * @param [in] size is size to update
 */
void kmc_update_sector_size(const uint16_t size);

/**
 * @brief Read sector to buff
 *
 * @param [in] index is index-th sector
 * @param [inout] buff is where is sector stored
 * @return int32_t is number of bytes read
 */
int32_t kmc_read_sector(uint32_t index, uint8_t *p_buff);

/**
 * @brief Read multi sector to buff
 *
 * @param [in] index is index-th sector
 * @param [in] num is number of sector want to read
 * @param [inout] buff is where is sector stored
 * @return int32_t is number of bytes read
 */
int32_t kmc_read_multi_sector(uint32_t index, uint32_t num,
                              uint8_t *p_buff);

/**
 * @brief De-initialize HAL
 *
 */
void kmc_deinit(void);

#endif /* _HAL_H_ */

/*******************************************************************************
 * EOF
 ******************************************************************************/
