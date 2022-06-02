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

// ע��OS_Time1���붨��ȫ�ֱ�������ΪESP8266���ں˻�Ҫʹ��
os_timer_t OS_Timer1;  //���������ʱ��(os_timer_t���ͽṹ��)

// �����ʱ���ص�����(�����ʱ��������Ķ�ʱ�������������������еģ����Բ������жϣ����ǻص�����)
void ICACHE_FLASH_ATTR OS_Timer1_cb(void)
{
	/*************************
	 * struct ip_info�ṹ���Ա
	 * 		ip: IP��ַ
	 * netmask: ��������
	 * 	    gw: ����
	 *************************/
	struct ip_info ST_ESP8266_IP;	// IP��Ϣ�ṹ��
	u8  ESP8266_IP[4];		        // ���ʮ������ʽ����IP

	// ��ѯESP8266�Ĺ���ģʽ
	switch(wifi_get_opmode())
	{
		//STAģʽ
		case 0x01:	os_printf("\r\nESP8266_Mode = STAģʽ\r\n");		break;
		//APģʽ
		case 0x02:	os_printf("\r\nESP8266_Mode = APģʽ\r\n");		break;
		//STA��APģʽ
		case 0x03:	os_printf("\r\nESP8266_Mode = STA��APģʽ\r\n");	break;
	}

	// ��ȡESP8266_APģʽ�µ�IP��ַ
	//��APģʽ�£��������DHCP(Ĭ�Ͽ�����)������δ����IP��ز�����ESP8266��IP��ַ=192.168.4.1��
	// SOFTAP_IF 0x01 --- ��ȡ����APģʽ��IP��Ϣ
	wifi_get_ip_info(SOFTAP_IF,&ST_ESP8266_IP);	// ����2��IP��Ϣ�ṹ��ָ��

	// ��������IP��ַ��Ϊʮ����
	ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;			// ���ʮ����IP�ĵ�һ���� <==> addr�Ͱ�λ
	ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;		// ���ʮ����IP�ĵڶ����� <==> addr�εͰ�λ
	ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;		// ���ʮ����IP�ĵ������� <==> addr�θ߰�λ
	ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;		// ���ʮ����IP�ĵ��ĸ��� <==> addr�߰�λ

	// ��ӡIP��ַ
	os_printf("ESP8266_IP = %d.%d.%d.%d\r\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);
	// ��ӡ�����豸������
	os_printf("Number of devices connected to this WIFI = %d\r\n",wifi_softap_get_station_num());
}

// �����ʱ����ʼ��(ms)   ����1�����ö�ʱ��ʱ��     ����2�������Ƿ��ظ�����
void ICACHE_FLASH_ATTR OS_Timer1_Init(u32 time_ms, u8 time_repetitive)
{
	// �رն�ʱ��
	os_timer_disarm(&OS_Timer1);   //�ر������ʱ��

	// ���������ʱ��
	// ����1��Ҫ���ö�ʱ��   ����2����ʱ���Ļص�����   ����3�������ص������Ĳ���
	os_timer_setfn(&OS_Timer1, (os_timer_func_t *)OS_Timer1_cb, NULL);

	// ʹ�ܶ�ʱ��
	// ����1��Ҫʹ�����õĶ�ʱ��     ����2����ʱʱ��ms   ����3��1���ظ�����  0��ֻ����һ��
	os_timer_arm(&OS_Timer1, time_ms, time_repetitive);

	// ע�⣺
	//    system_timer_reinit �����ʱ�����³�ʼ��
	//    ���δ����system_timer_reinit, ��֧�ַ�Χ5ms ~ 6 870 947ms
	//    ����е���system_timer_reinit, ��֧�ַ�Χ100ms ~ 428 496ms
}

//��ʼ��ESP8266��APģʽ
void ICACHE_FLASH_ATTR ESP8266_AP_Init(void)
{
	/*****************************************
	 * struct softap_config�ṹ���Ա
	 * 		      ssid:  WiFi��SSID(���񼯱�ʶ)��ͨ�׵����WiFi��
	 *        password:  WiFi������
	 *        ssid_len:  WiFi��SSID����
	 *         channel:  ͨ�ź�1~13
	 *        authmode:  �Ƿ����ü���ģʽ
	 *     ssid_hidden:  �Ƿ�����SSID
	 *  max_connection:  ������ӵ��豸�����������4̨
	 * beacon_interval:  �ű���ʱ��100��60000 ms
	 ****************************************/
	struct softap_config AP_Config;     //����һ��AP�����ṹ��

	wifi_set_opmode(0x02);    //WiFi����ΪAPģʽ�������浽flash

	// ��ʼ��AP_Config�ṹ��(�������)
	os_memset(&AP_Config, 0, sizeof(struct softap_config));

	// ����WiFi��APģʽ����
	os_strcpy(AP_Config.ssid, "My_ESP8266");   //WiFi��ΪMy_ESP8266
	os_strcpy(AP_Config.password,"123456789"); //WiFi����Ϊ123456789
	AP_Config.ssid_len = 9;                    //���볤��Ϊ9
	AP_Config.channel=1;                       // ͨ����1
	AP_Config.authmode=AUTH_WPA2_PSK;          // ���ü���ģʽ
	AP_Config.ssid_hidden=0;                   // ������SSID
	AP_Config.max_connection=4;                // ���������4̨�豸
	AP_Config.beacon_interval=100;             // �ű���ʱ��100 ms

	// ��������������������ã����е�����������ֻ�п���ʱ�Ž�������
	wifi_softap_set_config(&AP_Config);				// ����soft-AP�������浽Flash
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

	//�����ʱ����ʼ��  5s
	OS_Timer1_Init(5000, 1);

	//����WiFiΪAPģʽ����������APģʽ����
	ESP8266_AP_Init();
}

