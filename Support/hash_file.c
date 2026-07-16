#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ota_support.h"


#define PASSWORD "WASSUP_SEXXY"
#define OTA_WINDOW 100

volatile bool ota_window_started = false;
volatile bool ota_intr_block = false;
volatile bool ota_process_response = false;
volatile bool ota_requested_flag = false;


static uint32_t randomNumber;
static uint8_t computed_hash[32];

extern uint8_t recieved_hash[32];

void hmac_sha256(const uint8_t* key, size_t key_len, const uint8_t* message,
                 size_t message_len, uint8_t* hmac_result);

void HASH_compute_verify(void){

	uint32_t ota_tick;
	bool hash_mismatch = false;
	ota_intr_block = true;
	HAL_RNG_GenerateRandomNumber(&hrng, &randomNumber);
	memcpy(&tx_frame[14],&randomNumber,4);
	hmac_sha256((const uint8_t*)PASSWORD, 12, (const uint8_t*)&randomNumber,4, computed_hash);
	eth_transmit_raw(tx_frame, 60);
    ota_window_started = true;
    ota_tick = HAL_GetTick();
    ota_intr_block = false;
    while(HAL_GetTick() - ota_tick < OTA_WINDOW){
    	if(hash_mismatch)
    		continue;
    	if(ota_process_response){
    		ota_process_response = false;
    		if(!memcmp(&recieved_hash,&computed_hash,32)){
    			OTA_RESET();
    		}

  		    else{
  		    	//HASH_print(recieved_hash,"Recieved HASH");
  		    	hash_mismatch = true;

  		    }
    	}
    }
    printf("OTA REQUEST WINDOW TIMEOUT\r\n");
    if(hash_mismatch){
    	hash_mismatch = false;
    	printf("FUCK OFF BITCH\r\n");
    }
    ota_window_started = false;
    ota_process_response = false;
    ota_requested_flag = false;
    return;

}
void HASH_print(uint8_t *hash_buf, char *hash_name){
	printf("%s:",hash_name);
	for (int i = 0; i < 32; i++)printf("%02X", hash_buf[i]);
	printf("\r\n");
}

void OTA_RESET(void){

	printf("OTA REQUEST AUTHENTICATED, REBOOTING...\r\n");
    ETX_GNRL_CFG_ cfg;
    memcpy( &cfg, (ETX_GNRL_CFG_*)(ETX_CONFIG_FLASH_ADDR), sizeof(ETX_GNRL_CFG_) );
    cfg.reboot_cause = ETX_OTA_REQUEST;
    if( write_cfg_to_flash( &cfg ) != HAL_OK )
    {
        printf("OTA: failed to update reboot cause.\r\n");
        return;
    }
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1000);
    HAL_NVIC_SystemReset();
}
/*
 * ALL THE FUNCTIONS NECESSARY FOR SHA-256 HASING ARE IMPLEMENTED BELOW
 */

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned long long uint64;

// SHA-256 ������ǰ 64 ����������������С�����֣�
static const uint32 K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// ѭ����λ����
#define ROTL(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32-(n))))
#define SHR(x,n) ((x)>>(n))
// SHA-256 �߼�����
#define Ch(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define Sigma0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define Sigma1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define sigma0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x,3))
#define sigma1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x,10))



typedef struct{
    uint32 state[8];
    uint64 bitcount;
    uint8 buffer[64];
    int buffer_len;
}sha256_CTX;




void sha256_Init(sha256_CTX* ctx) {
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;

    ctx->bitcount = 0;
    ctx->buffer_len = 0;
}



void sha256_Transform(sha256_CTX* ctx, const uint8 data[64]) {
    uint32 a, b, c, d, e, f, g, h, T1, T2;
    uint32 m[64];
    int i;

        // ����Ϣ��ת��Ϊ 32 λ��
        for (i = 0; i < 16; i++) {
            m[i] = (data[i * 4] << 24) | (data[i * 4 + 1] << 16) |
                (data[i * 4 + 2] << 8) | data[i * 4 + 3];
        }

        // ��չ��Ϣ��Ϊ 64 ����
        for (i = 16; i < 64; i++) {
            m[i] = sigma1(m[i - 2]) + m[i - 7] + sigma0(m[i - 15]) + m[i - 16];
        }

        // ��ʼ����������
        a = ctx->state[0];
        b = ctx->state[1];
        c = ctx->state[2];
        d = ctx->state[3];
        e = ctx->state[4];
        f = ctx->state[5];
        g = ctx->state[6];
        h = ctx->state[7];

        // 64 ��ѹ������
        for (i = 0; i < 64; i++) {
            T1 = h + Sigma1(e) + Ch(e, f, g) + K[i] + m[i];
            T2 = Sigma0(a) + Maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }

        // ��������ӵ���ǰ��ϣֵ
        ctx->state[0] += a;
        ctx->state[1] += b;
        ctx->state[2] += c;
        ctx->state[3] += d;
        ctx->state[4] += e;
        ctx->state[5] += f;
        ctx->state[6] += g;
        ctx->state[7] += h;

}



void sha256_Update(sha256_CTX* ctx, const uint8* data, size_t len) {

    size_t i, rem;

    ctx->bitcount += len * 8;

    rem = ctx->buffer_len;
    if (rem) {
        size_t copy_len = (len < (64 - rem)) ? len : (64 - rem);
        memcpy(ctx->buffer+rem, data, copy_len);
        ctx->buffer_len += copy_len;

        if (ctx->buffer_len == 64) {
            sha256_Transform(ctx, ctx->buffer);
            ctx->buffer_len = 0;
        }
        data += copy_len;
        len -= copy_len;
    }

    //����ʣ�����ݿ�
    for (i = 0;i + 64 <= len;i += 64) {
        sha256_Transform(ctx, data + i);
    }

    //��ʣ�����ݿ���뻺����
    rem = len - i;
    if (rem) {
        memcpy(ctx->buffer, data+i, rem);
        ctx->buffer_len = rem;
    }


}

void sha256_Final(sha256_CTX* ctx,  uint8 digest[32]) {
    int i;
    int pad_len;
    uint8 final_block[64] = {0};
    uint64 bit_len = ctx->bitcount;

    //������䳤��
    pad_len = 64 - ((ctx->buffer_len + 8) % 64);
    if (pad_len == 0) pad_len = 64;

    //���ƻ���������
    memcpy(final_block, ctx->buffer, ctx->buffer_len);

    //ĩβ��1
    final_block[ctx->buffer_len] = 0x80;

    //������Ϣ����
    for (i = 0; i < 8; i++) {
        final_block[56 + i] = (bit_len >> ((7 - i) * 8)) & 0xFF;
    }

    sha256_Transform(ctx, final_block);

    for (i = 0; i < 8; i++) {
        digest[i * 4] = (ctx->state[i] >> 24) & 0xFF;
        digest[i * 4 + 1] = (ctx->state[i] >> 16) & 0xFF;
        digest[i * 4 + 2] = (ctx->state[i] >> 8) & 0xFF;
        digest[i * 4 + 3] = ctx->state[i] & 0xFF;
    }
}

void hmac_to_hex_string(const uint8 digest[], char hex_string[]) {
    static const char hex_chars[16] = "0123456789abcdef";
    int i;

    for (i = 0; i < 32; i++) {
        hex_string[i * 2] = hex_chars[(digest[i] >> 4) & 0xF];
        hex_string[i * 2 + 1] = hex_chars[digest[i] & 0xF];
    }
    hex_string[64] = '\0';
}

void hmac_sha256(const uint8* key, size_t key_len, const uint8* message,
    size_t message_len, uint8* hmac_result) {
    sha256_CTX ctx;
    int i;
    uint8 i_key_pad[64];
    uint8 o_key_pad[64];
    uint8 key_hash[32];


    if (key_len > 64) {
        sha256_Init(&ctx);
        sha256_Update(&ctx, key, key_len);
        sha256_Final(&ctx, key_hash);
        key = key_hash;
        key_len = 32;
    }
    //λģʽ����󣬺�������������������ɢ�Ժͻ�����
    for (i = 0;i <64;i++){
        if (i < key_len) {
            i_key_pad[i] = key[i] ^ 0x36;
            o_key_pad[i] = key[i] ^ 0x5C;
        }
        else {
            i_key_pad[i] = 0x36;
            o_key_pad[i] = 0x5C;
        }
    }


    sha256_Init(&ctx);
    sha256_Update(&ctx, i_key_pad, 64);
    sha256_Update(&ctx, message, message_len);
    sha256_Final(&ctx, hmac_result);


    sha256_Init(&ctx);
    sha256_Update(&ctx, o_key_pad, 64);
    sha256_Update(&ctx, hmac_result, 32);
    sha256_Final(&ctx, hmac_result);

}
