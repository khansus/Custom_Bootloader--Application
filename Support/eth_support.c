/*
 * eth_support.c
 *
 *  Created on: Jun 30, 2026
 *      Author: Sajjad
 */


#include "ota_support.h"
#include "lan8742.h"
#include "main.h"   /* for heth, HAL_GetTick */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define ETH_RX_BUFFER_SIZE   1524
#define ETH_RX_BUFFER_CNT    4      /* match or exceed ETH_RX_DESC_CNT */



typedef struct
{
    uint8_t  buff[ETH_RX_BUFFER_SIZE];
} RxBuff_t;


static RxBuff_t rx_pool[ETH_RX_BUFFER_CNT] __attribute__((aligned(32)));
static uint8_t  rx_pool_idx = 0;

/* Assembled frame after ReadData completes */
uint8_t  *assembled_buf   = NULL;
uint16_t  assembled_len   = 0;


uint8_t recieved_hash[32] = {0};

uint8_t tx_frame[60] ={0};

static void ota_build_frame(void);


lan8742_Object_t LAN8742;

static int32_t ETH_PHY_IO_Init(void) { return 0; }
static int32_t ETH_PHY_IO_DeInit(void) { return 0; }
static int32_t ETH_PHY_IO_GetTick(void) { return HAL_GetTick(); }

static int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
    if( HAL_ETH_WritePHYRegister(&heth, DevAddr, RegAddr, RegVal) != HAL_OK )
        return -1;
    return 0;
}

static int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
    if( HAL_ETH_ReadPHYRegister(&heth, DevAddr, RegAddr, pRegVal) != HAL_OK )
        return -1;
    return 0;
}

lan8742_IOCtx_t LAN8742_IOCtx = {
    ETH_PHY_IO_Init,
    ETH_PHY_IO_DeInit,
    ETH_PHY_IO_WriteReg,
    ETH_PHY_IO_ReadReg,
    ETH_PHY_IO_GetTick
};

void LAN8742_init(void)
{
    LAN8742_RegisterBusIO(&LAN8742, &LAN8742_IOCtx);

    if( LAN8742_Init(&LAN8742) != LAN8742_STATUS_OK )
    {
       // printf("LAN8742 PHY init FAILED\r\n");
    }
    else
    {
        printf("LAN8742 PHY init OK\r\n");
    }
}


/* -----------------------------------------------------------------------
 * HAL_ETH_RxAllocateCallback
 * Called by HAL driver before DMA writes — supply a free buffer
 * ----------------------------------------------------------------------- */
void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    *buff = rx_pool[rx_pool_idx].buff;
    rx_pool_idx = (rx_pool_idx + 1) % ETH_RX_BUFFER_CNT;
}


/* -----------------------------------------------------------------------
 * HAL_ETH_RxLinkCallback
 * Called by HAL_ETH_ReadData internally for each buffer chunk.
 * This is where cache invalidation must happen — mirrors lwIP's impl exactly.
 * pStart/pEnd are used to chain multi-chunk frames (jumbo frames etc.)
 * For standard frames everything fits in one chunk so pStart==pEnd after call.
 * ----------------------------------------------------------------------- */
void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd,
                              uint8_t *buff, uint16_t Length)
{
    /* Invalidate DCache BEFORE reading — same as lwIP does it */
    SCB_InvalidateDCache_by_Addr((uint32_t*)buff,
                                  (Length + 31) & ~31);

    /* For non-lwIP: just store pointer and length of first chunk.
       For standard Ethernet frames (≤1500 bytes) there's only ever
       one chunk so this is sufficient. */
    if( *pStart == NULL )
    {
        *pStart = buff;
        assembled_buf = buff;
        assembled_len = Length;
    }
    else
    {
        /* Multi-chunk frame — accumulate length, data is contiguous
           in our static pool so pointer doesn't change */
        assembled_len += Length;
    }
    *pEnd = buff;
}


/* -----------------------------------------------------------------------
 * HAL_ETH_RxCpltCallback
 * Called after a complete frame is received and descriptors processed.
 * By the time this fires, RxLinkCallback has already run and cache
 * is already invalidated — safe to read assembled_buf directly.
 * ----------------------------------------------------------------------- */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
    void *p = NULL;

    /* Reset assembled frame state before ReadData populates it
       via RxLinkCallback */
    assembled_buf = NULL;
    assembled_len = 0;

    if( HAL_ETH_ReadData(heth, &p) != HAL_OK )
        return;

    /* assembled_buf and assembled_len are now populated by RxLinkCallback */
    if( assembled_buf == NULL || assembled_len < 14 )
        return;

    if(ota_intr_block)
    	return;

    uint8_t  *buf = assembled_buf;
    uint32_t  len = assembled_len;

    uint16_t ethertype = (buf[12] << 8) | buf[13];



    if( ethertype == OTA_ETHERTYPE && len >= 14 ){

    	ota_build_frame();
    	ota_requested_flag = true;


    	if(ota_window_started){
    		memcpy(&recieved_hash,&buf[14],32);
    		ota_process_response = true;
    		//HASH_print(recieved_hash,"Recieved HASH");
    	}

      }

}

/* -----------------------------------------------------------------------
 * Public: write_cfg_to_flash
 *
 * Erases config sector (sector 4) and writes the ETX_GNRL_CFG_ struct.
 * Non-static so it can be called from http_ota.c if needed.
 * ----------------------------------------------------------------------- */
HAL_StatusTypeDef write_cfg_to_flash( ETX_GNRL_CFG_ *cfg )
{
    HAL_StatusTypeDef ret;

    do
    {
        if( cfg == NULL )
        {
            ret = HAL_ERROR;
            break;
        }

        ret = HAL_FLASH_Unlock();
        if( ret != HAL_OK )
            break;

        FLASH_WaitForLastOperation( HAL_MAX_DELAY );

        FLASH_EraseInitTypeDef EraseInitStruct;
        uint32_t SectorError;

        EraseInitStruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
        EraseInitStruct.Sector       = FLASH_SECTOR_4;
        EraseInitStruct.NbSectors    = 1u;
        EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

        __HAL_FLASH_CLEAR_FLAG( FLASH_FLAG_EOP    | FLASH_FLAG_OPERR  |
                                 FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                                 FLASH_FLAG_PGPERR );

        ret = HAL_FLASHEx_Erase( &EraseInitStruct, &SectorError );
        if( ret != HAL_OK )
            break;

        uint8_t *data = (uint8_t*)cfg;
        for( uint32_t i = 0u; i < sizeof(ETX_GNRL_CFG_); i++ )
        {
            ret = HAL_FLASH_Program( FLASH_TYPEPROGRAM_BYTE,
                                     ETX_CONFIG_FLASH_ADDR + i,
                                     data[i] );
            if( ret != HAL_OK )
            {
                printf("OTA: config flash write error at byte %lu\r\n", i);
                break;
            }
        }

        FLASH_WaitForLastOperation( HAL_MAX_DELAY );

        if( ret != HAL_OK )
            break;

        ret = HAL_FLASH_Lock();

    } while( false );

    return ret;
}


HAL_StatusTypeDef eth_transmit_raw(uint8_t *data, uint16_t len)
{

    /* TX buffer descriptor */
    ETH_BufferTypeDef tx_buf;
    tx_buf.buffer = data;
    tx_buf.len    = len;
    tx_buf.next   = NULL;   /* single buffer, no chaining */

    /* Configure TX packet */
    memset(&TxConfig, 0, sizeof(ETH_TxPacketConfig));
    TxConfig.Attributes   = ETH_TX_PACKETS_FEATURES_CRCPAD;
    TxConfig.Length       = len;
    TxConfig.TxBuffer     = &tx_buf;
    TxConfig.CRCPadCtrl   = ETH_CRC_PAD_INSERT;  /* let hardware add CRC and padding */

    /* Flush DCache for TX buffer before DMA reads it */
    SCB_CleanDCache_by_Addr((uint32_t*)data, (len + 31) & ~31);

    return HAL_ETH_Transmit(&heth, &TxConfig, HAL_MAX_DELAY);
}

static void ota_build_frame(void){

	for(int i = 0;i<60;i++)tx_frame[i] = 0x00;
	for(int i = 0;i<6;i++)tx_frame[i] = 0xff;

	tx_frame[7] = 0x80;tx_frame[8] = 0xE1;

	tx_frame[12] = 0xBA;tx_frame[13] = 0xBE;
}


