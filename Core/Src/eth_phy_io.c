/*
 * eth_phy_io.c
 *
 *  Created on: Jun 30, 2026
 *      Author: Hp
 */


#include "lan8742.h"
#include "main.h"   /* for heth, HAL_GetTick */
#include <stdio.h>

#define ETH_RX_BUFFER_SIZE   1524
#define ETH_RX_BUFFER_CNT    4      /* match or exceed ETH_RX_DESC_CNT */

extern ETH_HandleTypeDef heth;
static uint8_t rx_buffers[ETH_RX_BUFFER_CNT][ETH_RX_BUFFER_SIZE] __attribute__((aligned(32)));
static uint8_t rx_buf_index = 0;

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

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    *buff = rx_buffers[rx_buf_index];
    rx_buf_index = (rx_buf_index + 1) % ETH_RX_BUFFER_CNT;
}
