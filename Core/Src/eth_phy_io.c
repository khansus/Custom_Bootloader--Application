/*
 * eth_phy_io.c
 *
 *  Created on: Jun 30, 2026
 *      Author: Hp
 */


#include "lan8742.h"
#include "main.h"   /* for heth, HAL_GetTick */
#include <stdio.h>
#include <stdbool.h>

#define ETH_RX_BUFFER_SIZE   1524
#define ETH_RX_BUFFER_CNT    4      /* match or exceed ETH_RX_DESC_CNT */

typedef struct
{
    uint8_t  buff[ETH_RX_BUFFER_SIZE];
} RxBuff_t;

extern ETH_HandleTypeDef heth;
static RxBuff_t rx_pool[ETH_RX_BUFFER_CNT] __attribute__((aligned(32)));
static uint8_t  rx_pool_idx = 0;

/* Assembled frame after ReadData completes */
uint8_t  *assembled_buf   = NULL;
uint16_t  assembled_len   = 0;

volatile bool ota_requested_flag = false;

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

void eth_phy_init(void)
{
    LAN8742_RegisterBusIO(&LAN8742, &LAN8742_IOCtx);

    if( LAN8742_Init(&LAN8742) != LAN8742_STATUS_OK )
    {
        printf("LAN8742 PHY init FAILED\r\n");
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
