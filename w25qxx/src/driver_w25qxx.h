#ifndef DRIVER_W25QXX_H
#define DRIVER_W25QXX_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <esp_err.h>


/**
 * @brief Initalize w25qxx flash chip
 */
esp_err_t w25qxx_init();

/**
 * @brief Deinitalize w25qxx flash chip
 */
esp_err_t w25qxx_deinit();

/**
 * @brief get the JEDEC ID of the flash chip
 * 
 * @param[in]  out point to buffer for storing results
 * @return     status code
 *             - 0 success
 *             - 1 failed
 * @note       out must be atleast 3 bytes
 */
uint8_t w25qxx_get_jedec_id(uint8_t *out);

/**
 * @brief Erase whole flash chip
 * 
 * @return     status code
 *             - 0 success
 *             - 1 failed
 */
uint8_t w25qxx_chip_erase();

/**
 * @brief Erase one sector of flash chip
 * 
 * @param[in]  addr address of sector to erase
 * @return     status code
 *             - 0 success
 *             - 1 failed
 */
uint8_t w25qxx_sector_erase(uint32_t addr);

/**
 * @brief Erase one 64KB block of flash chip
 *
 * @param[in]  addr address of block to erase (64KB aligned)
 * @return     status code
 *             - 0 success
 *             - 1 failed
 */
uint8_t w25qxx_block_erase_64k(uint32_t addr);

/**
 * @brief Write data to flash chip
 * 
 * @param[in]  addr address to write to
 * @param[in]  data pointer to data to write
 * @param[in]  len length of data to write
 * @return     status code
 *             - 0 success
 *             - 1 failed
 */
uint8_t w25qxx_write(uint32_t addr, uint8_t *data, uint32_t len);

/**
 * @brief Read data from flash chip
 * 
 * @param[in]  addr address to read from
 * @param[out]  data pointer to output data into
 * @param[in]  len length of data to read
 * @return     status code
 *             - 0 success
 *             - 1 failed
 */
uint8_t w25qxx_read(uint32_t addr, uint8_t *data, uint32_t len);

void debug();

#endif
