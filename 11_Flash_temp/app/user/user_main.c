/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"

//���ô�������ļ�
#include "driver/uart.h"
#include "driver/uart_register.h"
//���Ź�����ļ�
#include "os_type.h"
#include "osapi.h"


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
            rf_cal_sec = 512 - 5;
            break;
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR
user_rf_pre_init(void)
{
}

//�Լ�д��ms��ʱ����
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{
	for(;C_time>0;C_time--)
		os_delay_us(1000);
}

//��ע�⡿������� != ������ַ          ������ַ = ������� * 4096
u16 FLASH_SEC = 0x77;     //�洢���ݵ��������
u32 W_Data[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};   //Ҫд��Flash������
u32 R_Data[16] = {0};   //��������Flash������

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
	u8 i;

	//��ʼ������
	uart_init(9600, 9600);    //��һ�������Ǵ���0�Ĳ����ʣ� �ڶ����Ǵ���1�Ĳ�����

	os_delay_us(1000);   //��ʱ�ȴ������ȶ�

	os_printf("\r\n-----------------------------------------------------\r\n");
	os_printf("\r\nSDK version: %s \r\n", system_get_sdk_version());   //���ڴ�ӡSDK�汾
	uart0_sendStr("\r\nHello World ; Hello WiFi\r\n");    //����0�����ַ���
	os_printf("\r\n-----------------------------------------------------\r\n");

	/*******************************************************************************
	 * ESP-12Fģ����ⲿFlash�����˴洢ϵͳ����ϵͳ�����⣬�����������洢�û����ݣ���λ/����Ҳ���ᶪʧ�û�����
	 *
	 * ESP-12Fģ����ⲿFlash = 32Mbit = 4MB
	 * ��ַ��ΧΪ 0x000 000 ~ 0x3ff fff
	 * �������Ϊ 0x000 000 ~ 0x3ff
	 * ������ַ = ������� *4096
	 * һ��������4KB��С
	 *
	 * ע�⣺
	 * 	��дFlash�ĵ�ַ��������ϵͳ������������ͻ�����Է���0x70 000��ַ֮��
	 * 	Flash��д������4�ֽڶ���
	 * 	Flashд����ʱ�������Ȳ�����������
	 *****************************************************************************/

	// ����0x77 000 ��ַ����
	// ����---�������      ������ַ = ������� * 4096
	spi_flash_erase_sector(0x77);
	// ��0x77 000 ��ַ����д������
	// ����1-Ҫд���������ַ     ����2-Ҫд������ݿ�ʼ��ַ    ����3-Ҫд������ݸ���
	spi_flash_write(FLASH_SEC*4096, (u32 *)W_Data, sizeof(W_Data));

	os_printf("\r\n------����д��flash���------\r\n");

	// �� 0x77 000 ������ַ�У���ȡ16������
	// ����1-Ҫ��ȡ��������ַ     ����2-Ҫ�������ݵĻ��濪ʼ��ַ      ����3-��ȡ���ݵĸ���
	spi_flash_read(FLASH_SEC*4096, (u32 *)R_Data, sizeof(R_Data));

	for(i=0; i<16; i++)
	{
		os_printf("Read Data%d = %d\r\n", i, R_Data[i]);
		delay_ms(100);
	}
}

