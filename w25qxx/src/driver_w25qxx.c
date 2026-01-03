#include "driver_w25qxx.h"

#include "driver/spi_common.h"
#include "driver/spi_slave.h"
#include "driver/spi_master.h"
#include "ascent_r3_hardware_definition.h"

#include "esp_log.h"

#include <rom/ets_sys.h>
#include <assert.h>

static const char *TAG = "W25QXX";

/**
 * @brief chip command definition from: https://github.com/libdriver/w25qxx/
 */
#define W25QXX_COMMAND_WRITE_ENABLE                      0x06        /**< write enable */
#define W25QXX_COMMAND_VOLATILE_SR_WRITE_ENABLE          0x50        /**< sr write enable */
#define W25QXX_COMMAND_WRITE_DISABLE                     0x04        /**< write disable */
#define W25QXX_COMMAND_READ_STATUS_REG1                  0x05        /**< read status register-1 */
#define W25QXX_COMMAND_READ_STATUS_REG2                  0x35        /**< read status register-2 */
#define W25QXX_COMMAND_READ_STATUS_REG3                  0x15        /**< read status register-3 */
#define W25QXX_COMMAND_WRITE_STATUS_REG1                 0x01        /**< write status register-1 */
#define W25QXX_COMMAND_WRITE_STATUS_REG2                 0x31        /**< write status register-2 */
#define W25QXX_COMMAND_WRITE_STATUS_REG3                 0x11        /**< write status register-3 */
#define W25QXX_COMMAND_CHIP_ERASE                        0xC7        /**< chip erase */
#define W25QXX_COMMAND_ERASE_PROGRAM_SUSPEND             0x75        /**< erase suspend */
#define W25QXX_COMMAND_ERASE_PROGRAM_RESUME              0x7A        /**< erase resume */
#define W25QXX_COMMAND_POWER_DOWN                        0xB9        /**< power down */
#define W25QXX_COMMAND_RELEASE_POWER_DOWN                0xAB        /**< release power down */
#define W25QXX_COMMAND_READ_MANUFACTURER                 0x90        /**< manufacturer */
#define W25QXX_COMMAND_JEDEC_ID                          0x9F        /**< jedec id */
#define W25QXX_COMMAND_GLOBAL_BLOCK_SECTOR_LOCK          0x7E        /**< global block lock */
#define W25QXX_COMMAND_GLOBAL_BLOCK_SECTOR_UNLOCK        0x98        /**< global block unlock */
#define W25QXX_COMMAND_ENTER_QSPI_MODE                   0x38        /**< enter spi mode */
#define W25QXX_COMMAND_ENABLE_RESET                      0x66        /**< enable reset */
#define W25QXX_COMMAND_RESET_DEVICE                      0x99        /**< reset device */
#define W25QXX_COMMAND_READ_UNIQUE_ID                    0x4B        /**< read unique id */
#define W25QXX_COMMAND_PAGE_PROGRAM                      0x02        /**< page program */
#define W25QXX_COMMAND_QUAD_PAGE_PROGRAM                 0x32        /**< quad page program */
#define W25QXX_COMMAND_SECTOR_ERASE_4K                   0x20        /**< sector erase */
#define W25QXX_COMMAND_BLOCK_ERASE_32K                   0x52        /**< block erase */
#define W25QXX_COMMAND_BLOCK_ERASE_64K                   0xD8        /**< block erase */
#define W25QXX_COMMAND_READ_DATA                         0x03        /**< read data */
#define W25QXX_COMMAND_FAST_READ                         0x0B        /**< fast read */
#define W25QXX_COMMAND_FAST_READ_DUAL_OUTPUT             0x3B        /**< fast read dual output */
#define W25QXX_COMMAND_FAST_READ_QUAD_OUTPUT             0x6B        /**< fast read quad output */
#define W25QXX_COMMAND_READ_SFDP_REGISTER                0x5A        /**< read SFDP register */
#define W25QXX_COMMAND_ERASE_SECURITY_REGISTER           0x44        /**< erase security register */
#define W25QXX_COMMAND_PROGRAM_SECURITY_REGISTER         0x42        /**< program security register */
#define W25QXX_COMMAND_READ_SECURITY_REGISTER            0x48        /**< read security register */
#define W25QXX_COMMAND_INDIVIDUAL_BLOCK_LOCK             0x36        /**< individual block lock */
#define W25QXX_COMMAND_INDIVIDUAL_BLOCK_UNLOCK           0x39        /**< individual block unlock */
#define W25QXX_COMMAND_READ_BLOCK_LOCK                   0x3D        /**< read block lock */
#define W25QXX_COMMAND_FAST_READ_DUAL_IO                 0xBB        /**< fast read dual I/O */
#define W25QXX_COMMAND_DEVICE_ID_DUAL_IO                 0x92        /**< device id dual I/O */
#define W25QXX_COMMAND_SET_BURST_WITH_WRAP               0x77        /**< set burst with wrap */
#define W25QXX_COMMAND_FAST_READ_QUAD_IO                 0xEB        /**< fast read quad I/O */
#define W25QXX_COMMAND_WORD_READ_QUAD_IO                 0xE7        /**< word read quad I/O */
#define W25QXX_COMMAND_OCTAL_WORD_READ_QUAD_IO           0xE3        /**< octal word read quad I/O */
#define W25QXX_COMMAND_DEVICE_ID_QUAD_IO                 0x94        /**< device id quad I/O */

static spi_device_handle_t flash_handle;

#define MAX_TRANSACTION_SIZE 8192
// #define MAX_TRANSACTION_SIZE 4096
// #define MAX_TRANSACTION_SIZE 512
static uint8_t g_transaction_buf[MAX_TRANSACTION_SIZE];
static uint8_t g_staging_buf[512];

#define NOTHING 0xFF

// #define HELP

static uint8_t spi_write(uint8_t *in_buf, uint32_t in_len)
{
#ifdef HELP
    printf("SPI WRITE CALL %ld\n", in_len);
#endif

    if (in_len > MAX_TRANSACTION_SIZE)
    {
        printf("WARNING: MAX_TRANSACTION_SIZE too small in w25qxx interface, in_len=%ld\n", in_len);
        return 1;
    }

    memset(g_transaction_buf, NOTHING, MAX_TRANSACTION_SIZE);
    // memset(g_transaction_buf, 0, MAX_TRANSACTION_SIZE);
    
    memcpy(g_transaction_buf, in_buf, in_len);

    spi_transaction_t cmd = {
        .length = in_len*8,
        .tx_buffer = g_transaction_buf,
    };

    esp_err_t err = spi_device_polling_transmit(flash_handle, &cmd);
    if (err != ESP_OK) {
        printf("Flash chip SPI transaction failed\n");
        return 1;
    }

    return 0;
}

static uint8_t spi_write_read(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t out_len)
{
#ifdef HELP
    printf("SPI WRITE READ CALL %ld, %ld\n", in_len, out_len);
#endif

    if (in_len + out_len > MAX_TRANSACTION_SIZE)
    {
        printf("WARNING: MAX_TRANSACTION_SIZE too small in w25qxx interface, in_len=%ld, out_len=%ld\n", in_len, out_len);
        return 1;
    }

    memset(g_transaction_buf, NOTHING, MAX_TRANSACTION_SIZE);
    // memset(g_transaction_buf, 0, MAX_TRANSACTION_SIZE);
    
    memcpy(g_transaction_buf, in_buf, in_len);

    spi_transaction_t cmd = {
        .length = (in_len+out_len)*8,
        .tx_buffer = g_transaction_buf,
        .rx_buffer = g_transaction_buf,
    };

    esp_err_t err = spi_device_polling_transmit(flash_handle, &cmd);
    if (err != ESP_OK) {
        printf("Flash chip SPI transaction failed\n");
        return 1;
    }

    memcpy(out_buf, g_transaction_buf+in_len, out_len);

    return 0;
}

esp_err_t w25qxx_init()
{
#ifdef HELP
    printf("W25QXX INIT CALL\n");
#endif

    spi_device_interface_config_t flash_cfg = {
        .mode = 0,
        .clock_speed_hz = 60e6,
        .spics_io_num = FLASH_CS,
        .queue_size = 1,
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        // .flags = SPI_DEVICE_NO_DUMMY | SPI_DEVICE_HALFDUPLEX,  // Add HALFDUPLEX back
        .flags = SPI_DEVICE_NO_DUMMY,
    };

    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &flash_cfg, &flash_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Adding w25qxx to SPI bus failed");
        return 1;
    }

    // 4 byte address mode
    {
        uint8_t cmd[] = { 0xB7 };
        uint8_t res = spi_write(cmd, sizeof(cmd));
        if (res) return res;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);

    // {
    //     uint8_t cmd[] = { W25QXX_COMMAND_READ_STATUS_REG3 };
    //     uint8_t res2[1];
    //     uint8_t res = spi_write_read(cmd, sizeof(cmd), res2, sizeof(res2));
    //     if (res) return res;
    //     printf("Address mode (want: 1): %d\n", res2[0]&0x1);
    // }

    // {
    //     uint8_t cmd[] = { W25QXX_COMMAND_READ_STATUS_REG1 };
    //     uint8_t res2[1];
    //     uint8_t res = spi_write_read(cmd, sizeof(cmd), res2, sizeof(res2));
    //     if (res) return res;
    //     printf("Write enable (want: 1): %d\n", (res2[0]>>1)&0x1);
    // }

    return 0;
}

esp_err_t w25qxx_deinit()
{
    return spi_bus_remove_device(flash_handle);
}

static uint8_t w25qxx_spin_while_busy()
{
#ifdef HELP
    printf("W25QXX SPIN CALL\n");
#endif

    uint8_t statusReg1 = 0xFF;

    while (statusReg1 & 0x1) {
        uint8_t cmd2[] = { W25QXX_COMMAND_READ_STATUS_REG1 };
        uint8_t res2[] = { 0xFF };
        uint8_t res = spi_write_read(cmd2, sizeof(cmd2), res2, sizeof(res2));
        if (res) return res;
        statusReg1 = res2[0];
        ets_delay_us(10); // TODO: what should this value be
    }

    return 0;
}

static uint8_t w25qxx_write_enable()
{
    uint8_t cmd[] = { W25QXX_COMMAND_WRITE_ENABLE };
    return spi_write(cmd, sizeof(cmd));
}

uint8_t w25qxx_get_jedec_id(uint8_t *out)
{
#ifdef HELP
    printf("W25QXX GET JEDEC CALL\n");
#endif

    uint8_t cmd[] = { W25QXX_COMMAND_JEDEC_ID };
    uint8_t res2[3];
    uint8_t res = spi_write_read(cmd, sizeof(cmd), res2, sizeof(res2));
    if (res) return res;

    res = w25qxx_spin_while_busy();
    if (res) return res;

    memcpy(out, res2, sizeof(res2));

    return 0;
}

uint8_t w25qxx_chip_erase()
{
#ifdef HELP
    printf("W25QXX CHIP ERASE CALL\n");
#endif

    uint8_t res = w25qxx_spin_while_busy();
    if (res) return res;

    w25qxx_write_enable();

    uint8_t cmd[] = { W25QXX_COMMAND_CHIP_ERASE };
    res = spi_write(cmd, sizeof(cmd));
    if (res) return res;

    res = w25qxx_spin_while_busy();
    if (res) return res;

    return 0;
}

uint8_t w25qxx_sector_erase(uint32_t addr)
{
#ifdef HELP
    printf("W25QXX SECTOR ERASE CALL\n");
#endif

    uint8_t res = w25qxx_spin_while_busy();
    if (res) return res;

    w25qxx_write_enable();

    uint8_t cmd[] = { 0x21, (addr >> 24) & 0xFF, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, (addr >> 0) & 0xFF };
    res = spi_write(cmd, sizeof(cmd));
    if (res) return res;

    res = w25qxx_spin_while_busy();
    if (res) return res;

    return 0;
}

static uint8_t w25qxx_page_program(uint32_t addr, uint8_t *data, uint32_t len)
{
#ifdef HELP
    printf("W25QXX PAGE PROGRAM CALL\n");
#endif

    assert(len <= 256);

    uint8_t res = w25qxx_spin_while_busy();
    if (res) return res;

    w25qxx_write_enable();

    uint32_t n = 0;
    g_staging_buf[0] = 0x12;
    g_staging_buf[1] = (addr >> 24) & 0xFF;
    g_staging_buf[2] = (addr >> 16) & 0xFF;
    g_staging_buf[3] = (addr >> 8) & 0xFF;
    g_staging_buf[4] = (addr >> 0) & 0xFF;
    n += 5;

    for (int i = 0; i < len; i++)
        g_staging_buf[n+i] = data[i];
    n += len;

    res = spi_write(g_staging_buf, n);
    if (res) return res;

    return 0;
}

uint8_t w25qxx_write(uint32_t addr, uint8_t *data, uint32_t len)
{
#ifdef HELP
    printf("W25QXX WRITE CALL\n");
#endif

    uint8_t res;
    uint16_t n;
    uint16_t left;

    do {
        left = 256 - addr % 256;
        n = len < left ? len : left;
        res = w25qxx_page_program(addr, data, n);
        if (res) return res;

        len -= n;
        addr += n;
        data += n;
    } while (len > 0);

    return 0;
}

uint8_t w25qxx_read(uint32_t addr, uint8_t *data, uint32_t len)
{
#ifdef HELP
    printf("W25QXX READ CALL\n");
#endif

    uint8_t res = w25qxx_spin_while_busy();
    if (res) return res;

    uint32_t n = 0;
    g_staging_buf[0] = 0x13;
    g_staging_buf[1] = (addr >> 24) & 0xFF;
    g_staging_buf[2] = (addr >> 16) & 0xFF;
    g_staging_buf[3] = (addr >> 8) & 0xFF;
    g_staging_buf[4] = (addr >> 0) & 0xFF;
    n += 5;

    res = spi_write_read(g_staging_buf, n, data, len);
    if (res) return res;

    return 0;
}

void debug()
{
    w25qxx_spin_while_busy();
}