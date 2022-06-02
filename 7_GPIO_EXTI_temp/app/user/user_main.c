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

u8 T_LED = 1;   //led��״̬
// �Լ����ⲿ�жϺ���
void GPIO_INTERRUPT(void)
{
	u32 temp;         //����IO���ж�״̬
	u32 GPIO0_temp;   //GPIO0�ж�״̬

	//��ȡGPIO�ж�״̬
	temp = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	GPIO0_temp = temp & (0x01 << 0);   //��ȡGPIO0�ⲿ�ж�״̬

	//�ж��Ƿ���GPIO0�ⲿ�ж�
	if(GPIO0_temp)
	{
		T_LED = !T_LED;
		GPIO_OUTPUT_SET(GPIO_ID_PIN(4), T_LED);
	}

	// ����жϱ�׼
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, 1);
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{

	//��ʼ������
	uart_init(9600, 9600);    //��һ�������Ǵ���0�Ĳ����ʣ� �ڶ����Ǵ���1�Ĳ�����

	os_delay_us(1000);   //��ʱ�ȴ������ȶ�

	os_printf("\r\n-----------------------------------------------------\r\n");
	os_printf("\r\nSDK version: %s \r\n", system_get_sdk_version());   //���ڴ�ӡSDK�汾
	uart0_sendStr("\r\nHello World ; Hello WiFi\r\n");    //����0�����ַ���
	os_printf("\r\n-----------------------------------------------------\r\n");

	// ����LED��--GPIO4   ��ʼ״̬�����
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);   //��GPIO4����Ϊ��ͨIO
	GPIO_OUTPUT_SET(GPIO_ID_PIN(4), 1);   //GPIO4���ģʽ������ߵ�ƽ

	// ���ð���--GPIO0  ����ģʽ
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);   //��GPIO4����Ϊ��ͨIO
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(0));                       //GPIO0ʧ�����

	// ����GPIO0���ⲿ�ж�
	ETS_GPIO_INTR_DISABLE();                                         //�ر�GPIO0�жϹ���
	ETS_GPIO_INTR_ATTACH((ets_isr_t)GPIO_INTERRUPT, NULL);           //ע���жϻص�����
	gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_NEGEDGE);  //GPIO0�½��ش����ж�
	ETS_GPIO_INTR_ENABLE();                                          //ʹ��GPIO�жϹ���

	// GPIO_PIN_INTR_DISABLE = 0     �������ⲿ�ж�
	// GPIO_PIN_INTR_POSEDGE = 1     �������ж�
	// GPIO_PIN_INTR_NEGEDGE = 2     �½����ж�
	// GPIO_PIN_INTR_ANYEDGE = 3     ˫��(�������½�)�ж�
	// GPIO_PIN_INTR_LOLEVEL = 4    �͵�ƽ�ж�
	// GPIO_PIN_INTR_HILEVEL = 5    �ߵ�ƽ�ж�

	while(1)
	{
		system_soft_wdt_feed();  //ι������
	}
}

