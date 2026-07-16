/*
 * ota_support.h
 *
 * Minimal support file created to verify and trigger OTA request.
 * Firmware is received as raw binary over HTTP POST from a browser.
 */

#ifndef INC_OTA_SUPPORT_H_
#define INC_OTA_SUPPORT_H_

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

/* -----------------------------------------------------------------------
 * Flash address map  (STM32F767ZI — do not change without updating linker)
 * ----------------------------------------------------------------------- */
#define ETX_APP_FLASH_ADDR        0x08040000U   /* Application run address   */
#define ETX_CONFIG_FLASH_ADDR     0x08020000U   /* Bootloader config sector  */

#define ETX_NO_OF_SLOTS           2U

/* -----------------------------------------------------------------------
 * Reboot reason codes (stored in config flash)
 * ----------------------------------------------------------------------- */
#define ETX_FIRST_TIME_BOOT  0xFFFFFFFFU
#define ETX_NORMAL_BOOT      0xBEEFFEEDU
#define ETX_OTA_REQUEST      0xDEADBEEFU
#define ETX_LOAD_PREV_APP    0xFACEFADEU

#define OTA_ETHERTYPE       0xBABE


extern volatile bool ota_window_started;
extern volatile bool ota_intr_block;
extern volatile bool ota_process_response ;
extern volatile bool ota_requested_flag;

extern ETH_HandleTypeDef heth;
extern RNG_HandleTypeDef hrng;
extern CRC_HandleTypeDef hcrc;
extern ETH_TxPacketConfig TxConfig;

extern uint8_t tx_frame[60];

/*----------------------------------------------------------------------
 * Slot table entry (packed — written directly to flash)
 * ----------------------------------------------------------------------- */
typedef struct
{
    uint8_t  is_this_slot_not_valid;  /* 0 = valid firmware present         */
    uint8_t  is_this_slot_active;     /* 1 = currently running from this slot */
    uint8_t  should_we_run_this_fw;   /* 1 = bootloader should boot this    */
    uint32_t fw_size;                 /* Firmware size in bytes             */
    uint32_t fw_crc;                  /* CRC32 of firmware in slot          */
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
} __attribute__((packed)) ETX_SLOT_;

/* -----------------------------------------------------------------------
 * General bootloader configuration (packed — written directly to flash)
 * ----------------------------------------------------------------------- */
typedef struct
{
    uint32_t  reboot_cause;
    ETX_SLOT_ slot_table[ETX_NO_OF_SLOTS];
} __attribute__((packed)) ETX_GNRL_CFG_;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/*
 * @brief Triggerd once OTA Rquest is Authenticated, Starts the BOOTLOADER
 */
void OTA_RESET(void);

/*
 * @brief Initialises LAN8742 peripheral.
 */
void LAN8742_init(void);

/*
 * @brief Generates nonce and verifies recieved HASH
 */
void HASH_compute_verify(void);

/*
 * @brief prints HASH
 * @param hash_buf is the pointer to the HASH buffer
 * @param hash_name is the same to be printed: name:hash
 */
void HASH_print(uint8_t *hash_buf, char *hash_name);

/*
 * @brief transmit buffers via ethernet
 * @param data is pointer to the buffer
 * @param is the length of the buffer to be transmitted
 * @retval HAL_OK on success
 */
HAL_StatusTypeDef eth_transmit_raw(uint8_t *data, uint16_t len);


/*
 * @brief  Erase config sector and write cfg struct to ETX_CONFIG_FLASH_ADDR.
 * @retval HAL_OK on success
 */
HAL_StatusTypeDef write_cfg_to_flash( ETX_GNRL_CFG_ *cfg );

#endif /* INC_ETX_OTA_UPDATE_H_ */
