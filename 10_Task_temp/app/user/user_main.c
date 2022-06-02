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
//�����ڴ�malloc������ļ�
#include "mem.h"


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

// �Լ�д����ʱms����
void ICACHE_FLASH_ATTR delay_ms(u32 C_time)
{
	for(;C_time>0;C_time--)
		os_delay_us(1000);
}

//������Ϣ������Ⱥ�
#define   MESSAGE_QUEUE_LEN   2   //��Ϣ�������(����ͬһ������ϵͳ�����ܵĵ���������)
//��ע�⡿ESP8266�޲���ϵͳNon-OS���ֻ����3������

//��������ָ��ṹ��
os_event_t *Pointer_Task1;

//����ִ�к���(��ע�⡿�β����ͱ���Ϊos_event_t *)
void Func_Task1(os_event_t *Task_message)
{
	// os_event_t�ṹ��ֻ������������sig=��Ϣ����   par=��Ϣ����
	os_printf("\r\n��Ϣ����=%d����Ϣ����=%c\r\n", Task_message->sig, Task_message->par);
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
	u8 C_Task = 0;           //���ڼ�����������Ϣ��������
	u8 Message_Type = 1;     //��Ϣ����
	u8 Message_Para = 'A';   //��Ϣ����

	//��ʼ������
	uart_init(9600, 9600);    //��һ�������Ǵ���0�Ĳ����ʣ� �ڶ����Ǵ���1�Ĳ�����

	os_delay_us(1000);   //��ʱ�ȴ������ȶ�

	os_printf("\r\n-----------------------------------------------------\r\n");
	os_printf("\r\nSDK version: %s \r\n", system_get_sdk_version());   //���ڴ�ӡSDK�汾
	uart0_sendStr("\r\nHello World ; Hello WiFi\r\n");    //����0�����ַ���
	os_printf("\r\n-----------------------------------------------------\r\n");

	// ������1�����ڴ�ռ�(����1�ռ� = 1�����пռ�*������)
	Pointer_Task1 = (os_event_t *)os_malloc((sizeof(os_event_t) * MESSAGE_QUEUE_LEN));
	// ��������
	// ����1-����ִ�к���      ����2-�������ȵȼ�    ����3-����ռ�ָ��   ����4-��Ϣ�������
	// ��ע�⡿�������ȵȼ�  2>1>0
	system_os_task(Func_Task1, USER_TASK_PRIO_0, Pointer_Task1, MESSAGE_QUEUE_LEN);

	for(; C_Task<4; C_Task++)
	{
		system_soft_wdt_feed();  //ι������
		delay_ms(1000);
		os_printf("\r\n��������Task==%d\r\n", C_Task);

		// ��ϵͳ��������
		// ����1-�������ȵȼ�(��������������)  ����2-��Ϣ����     ����3-��Ϣ����
		// �޲���ϵͳNon-os����3������
		system_os_post(USER_TASK_PRIO_0, Message_Type++, Message_Para++);
	}

	os_printf("\r\n------���ź�������------\r\n");
}

