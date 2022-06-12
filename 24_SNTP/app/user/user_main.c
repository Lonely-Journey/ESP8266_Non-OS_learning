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
//�������ͷ�ļ�
#include "espconn.h"
#include "mem.h"
//SNTPͷ�ļ�
#include "sntp.h"


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
// ����SNTP��ʱ��ȡʱ���õ�
void ICACHE_FLASH_ATTR OS_Timer_SNTP_cb(void *arg)
{
	// �ַ������� ��ر���
	u8 C_Str = 0;				// �ַ����ֽڼ���
	char A_Str_Data[20] = {0};	// ��"����"���ַ�������
	char *T_A_Str_Data = A_Str_Data;	// ��������ָ��
	char A_Str_Clock[10] = {0};	// ��"ʱ��"���ַ�������
	char * Str_Head_Week;		// ��"����"���ַ����׵�ַ
	char * Str_Head_Month;		// ��"�·�"���ַ����׵�ַ
	char * Str_Head_Day;		// ��"����"���ַ����׵�ַ
	char * Str_Head_Clock;		// ��"ʱ��"���ַ����׵�ַ
	char * Str_Head_Year;		// ��"���"���ַ����׵�ַ
	uint32	TimeStamp;		// ʱ���
	char * Str_RealTime;	// ʵ��ʱ����ַ���

	// ��ѯ��ǰ�����׼ʱ��(1970.01.01 00:00:00 GMT+8)��ʱ���(��λ:��)
	TimeStamp = sntp_get_current_timestamp();

	if(TimeStamp)		// �ж��Ƿ��ȡ��ƫ��ʱ��
	{
		// ��ѯʵ��ʱ��(GMT+8):������(����ʱ��)
		Str_RealTime = sntp_get_real_time(TimeStamp);

		os_printf("\r\n---------------------------------------------\r\n");
		os_printf("SNTP_TimeStamp = %d\r\n",TimeStamp);		    // ʱ���
		os_printf("\r\nSNTP_InternetTime = %s",Str_RealTime);	// ʵ��ʱ��
		os_printf("\r\n---------------------------------------------\r\n");
	}
}

// �����ʱ����ʼ��(ms)   ����1�����ö�ʱ��ʱ��     ����2�������Ƿ��ظ�����
// ����SNTP��ʱ��ȡʱ���õ�
void ICACHE_FLASH_ATTR OS_Timer_SNTP_Init(u32 time_ms, u8 time_repetitive)
{
	// �رն�ʱ��
	os_timer_disarm(&OS_Timer1);   //�ر������ʱ��

	// ���������ʱ��
	// ����1��Ҫ���ö�ʱ��   ����2����ʱ���Ļص�����   ����3�������ص������Ĳ���
	os_timer_setfn(&OS_Timer1, (os_timer_func_t *)OS_Timer_SNTP_cb, NULL);

	// ʹ�ܶ�ʱ��
	// ����1��Ҫʹ�����õĶ�ʱ��     ����2����ʱʱ��ms   ����3��1���ظ�����  0��ֻ����һ��
	os_timer_arm(&OS_Timer1, time_ms, time_repetitive);

	// ע�⣺
	//    system_timer_reinit �����ʱ�����³�ʼ��
	//    ���δ����system_timer_reinit, ��֧�ַ�Χ5ms ~ 6 870 947ms
	//    ����е���system_timer_reinit, ��֧�ַ�Χ100ms ~ 428 496ms
}

//SNTP��������ʼ��
void ICACHE_FLASH_ATTR ESP8266_SNTP_Init(void)
{
	//����һ������IP��ַ�Ŀռ�
	ip_addr_t * addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));

	//ͨ����������SNTP������
	//us.pool.ntp.org��ntp.sjtu.edu.cn������IP��ַ��210.72.145.44���������ҵı���SNTP������
	//��������ȥʹ�þ���
	//ESP8266���֧��3��SNTP��������0��Ϊ����������1,2��Ϊ��ѡ����0�Ų�����ʱ�ᰴ˳��ʹ�ñ�ѡ������
	//����1������������0~3
	//����2��SNTP����
	sntp_setservername(0, "us.pool.ntp.org");	// ������0��
	sntp_setservername(1, "ntp.sjtu.edu.cn");	// ������1��

	//���ַ�����IP��ַת��Ϊu32Ϊ��IP��ַ
	ipaddr_aton("210.72.145.44", addr);			// ���ʮ���� => 32λ������
	//ͨ��IP��ַ����SNTP������
	//����1������������0~3
	//����2��IP��ַ
	sntp_setserver(2, addr);					// ������2��

	os_free(addr);								// �ͷ�addr�ڴ�ռ�

	sntp_init();	// SNTP��ʼ��API

	//�������������ʱ��
	OS_Timer_SNTP_Init(1000,1);
}

// �����ʱ���ص�����(�����ʱ��������Ķ�ʱ�������������������еģ����Բ������жϣ����ǻص�����)
// ��������WiFi
void ICACHE_FLASH_ATTR OS_Timer1_cb(void)
{
	/*************************
	* struct ip_info�ṹ���Ա
	* 		ip: IP��ַ
	* netmask: ��������
	* 	    gw: ����
	*************************/
	struct ip_info ST_ESP8266_IP;	// IP��Ϣ�ṹ��
	u8 ESP8266_IP[4];		        // ���ʮ������ʽ����IP
	u8 S_WIFI_STA_Connect;			// WIFI����״̬��־

	//��ѯSTA����WIFI״̬
	/*******************************************************
	*  Station����״̬��
	*        0 == STATION_IDLE -------------- STATION����
	*        1 == STATION_CONNECTING -------- ��������WIFI
	*        2 == STATION_WRONG_PASSWORD ---- WIFI�������
	*        3 == STATION_NO_AP_FOUND ------- δ����ָ��WIFI
	*        4 == STATION_CONNECT_FAIL ------ ����ʧ��
	*        5 == STATION_GOT_IP ------------ ���IP�����ӳɹ�
	*******************************************************/
	S_WIFI_STA_Connect = wifi_station_get_connect_status();

	switch(S_WIFI_STA_Connect)
	{
		case 0 : 	os_printf("\r\nSTATION_IDLE\r\n");				break;
		case 1 : 	os_printf("\r\nSTATION_CONNECTING\r\n");		break;
		case 2 : 	os_printf("\r\nSTATION_WRONG_PASSWORD\r\n");	break;
		case 3 : 	os_printf("\r\nSTATION_NO_AP_FOUND\r\n");		break;
		case 4 : 	os_printf("\r\nSTATION_CONNECT_FAIL\r\n");		break;
		case 5 : 	os_printf("\r\nSTATION_GOT_IP\r\n");			break;
	}

	// ���ӳɹ�
	if(S_WIFI_STA_Connect == STATION_GOT_IP)
	{
		// ��ȡESP8266_STAģʽ�µ�IP��ַ
		// STATION_IF 0x00 --- ��ȡ����STAģʽ��IP��Ϣ
		wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// ����2��IP��Ϣ�ṹ��ָ��

		// ��������IP��ַ��Ϊʮ����
		ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;			// ���ʮ����IP�ĵ�һ���� <==> addr�Ͱ�λ
		ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;		// ���ʮ����IP�ĵڶ����� <==> addr�εͰ�λ
		ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;		// ���ʮ����IP�ĵ������� <==> addr�θ߰�λ
		ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;		// ���ʮ����IP�ĵ��ĸ��� <==> addr�߰�λ

		// ��ӡIP��ַ
		os_printf("ESP8266_IP = %d.%d.%d.%d\r\n",ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);

		os_timer_disarm(&OS_Timer1);	// �رն�ʱ��

		ESP8266_SNTP_Init(); //��ʼ��SNTP
	}
}

// �����ʱ����ʼ��(ms)   ����1�����ö�ʱ��ʱ��     ����2�������Ƿ��ظ�����
// ��������WiFi�õ�
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

//��ʼ��ESP8266��STAģʽ
void ICACHE_FLASH_ATTR ESP8266_STA_Init(void)
{
	/*****************************************
	 * struct station_config�ṹ���Ա
	 * 		      ssid:  Ҫ�����WiFi��SSID(���񼯱�ʶ)��ͨ�׵����WiFi��
	 *        password:  Ҫ�����WiFi������
	 *       bssid_set:  ��һ�����������ж��ͬ��WiFiʱʹ�ã�����Ҫ���ӵ�WiFi�������ڸ����м���WiFi����һ����ʱ��
	 *                   0����У��MAC��ַ     1������WiFiʱҪУ��MAC��ַ
	 *           bssid:  Ҫ���ӵ�WiFi��MAC��ַ
	 *       threshold:  �ṹ�����ݰ���Ҫ���ӵ�WiFi�ļ��ܷ�ʽ���ź�ǿ�Ƚṹ��
	 ****************************************/
	struct station_config STA_Config;    //����һ��STA�����ṹ��
	/*************************
	 * struct ip_info�ṹ���Ա
	 * 		ip: IP��ַ
	 * netmask: ��������
	 * 	    gw: ����
	 *************************/
//	struct ip_info ST_ESP8266_IP;		 //STA��Ϣ�ṹ��

	wifi_set_opmode(0x01);    //WiFi����ΪSTAģʽ�������浽flash

	//�Զ���ESP8266��·�����е�IP
	//���û������Ĳ������ͨ��DHCP�Զ�����IP,һ������¶��ǲ��Լ���IP��
	/*
	wifi_station_dhcpc_stop();						// �ر� DHCP Client
	IP4_ADDR(&ST_ESP8266_IP.ip,192,168,8,88);		// ����IP��ַ
	IP4_ADDR(&ST_ESP8266_IP.netmask,255,255,255,0);	// ������������
	IP4_ADDR(&ST_ESP8266_IP.gw,192,168,8,1);		// �������ص�ַ
	wifi_set_ip_info(STATION_IF,&ST_ESP8266_IP);	// ����STAģʽ�µ�IP��ַ
	*/

	// ��ʼ��STA_Config�ṹ��(�������)
	os_memset(&STA_Config, 0, sizeof(struct station_config));
	// ����WiFi��STAģʽ����
	/*******************************************
	* ��ע�⡿��ע�⡿��ע�⡿����Ҫ������˵���顿
	* ssid������������ƣ������ı�����URL����
	* ssid������������ƣ������ı�����URL����
	* ssid������������ƣ������ı�����URL����
	*
	* ����������WiFi��ΪС��(URL����Ϊ0xe5b08f 0xe7b1b3)
	* \d����ֱ��\��8����     \x��16����
	* *****************************************/
	//WiFi����С��
//	os_strcpy(STA_Config.ssid, "\xe5\xb0\x8f\xe7\xb1\xb3");        //��Ҫ�����WiFi��
	//WiFi�����Ҽ�WiFi
	os_strcpy(STA_Config.ssid, "\xe6\x88\x91\xe5\xae\xb6WiFi");        //��Ҫ�����WiFi��
	os_strcpy(STA_Config.password,"13612746918"); //��Ҫ�����WiFi����

	// ��������������������ã����е�����������ֻ�п���ʱ�Ž�������
	wifi_station_set_config(&STA_Config);  // ����STAģʽ�����������浽Flash

	// wifi_station_connect()��API������user_init��ʼ���е���
	// ���user_init�е���wifi_station_set_config()�Ļ����ں˻��Զ���ESP8266����WIFI
	// wifi_station_connect();		// ESP8266����WIFI,���ﲻ��Ҫʹ��
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

	//����WiFiΪSTAģʽ����������STAģʽ����
	ESP8266_STA_Init();

	//�����ʱ����ʼ��  3s
	OS_Timer1_Init(3000, 1);
}

