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

// �ɹ������������ݵĻص�����
// ����1 arg ----- ���紫��ṹ��espconnָ��
void ICACHE_FLASH_ATTR ESP8266_WIFI_Send(void *arg)
{
	os_printf("\r\nESP8266_WIFI_Send_OK\r\n");
}

// �ɹ������������ݵĻص�����
// ����1 arg ----- ���紫��ṹ��espconnָ��
// ����2 pdata ----- ���紫������ָ��
// ����3 len ----- ���ݳ���
void ICACHE_FLASH_ATTR ESP8266_WIFI_Recv(void * arg, char * pdata, unsigned short len)
{
	struct espconn * T_arg = arg;		// �����������ӽṹ��ָ��
	/******************************
	 *  remot_info��Ա
	 *  	      state : ����״̬
	 *  	remote_port : �˿ں�
	 *  	  remote_ip : IP��ַ
	 ******************************/
	remot_info * P_port_info = NULL;	//Զ��������Ϣ�ṹ��ָ��

	os_printf("\r\nESP8266_Receive_Data = %s\r\n",pdata);		//���ڴ�ӡ���յ�������

	// ��ȡԶ����Ϣ��UDPͨ���������ӵģ���Զ��������Ӧʱ���ȡ�Է���IP/�˿���Ϣ��
	if(espconn_get_connection_info(T_arg, &P_port_info, 0)==ESPCONN_OK)	//��ȡԶ����Ϣ
	{
		T_arg->proto.udp->remote_port  = P_port_info->remote_port;		//��ȡ�Է��˿ں�
		os_memcpy(T_arg->proto.udp->remote_ip,P_port_info->remote_ip, 4);	//��ȡ�Է�IP��ַ
	}

	os_printf("���ͷ�����Ϣ\r\n");
	os_printf("�˿ڣ�%d\r\n", P_port_info->remote_port);
	os_printf("IP��%d.%d.%d.%d\r\n",
			P_port_info->remote_ip[0],
			P_port_info->remote_ip[1],
			P_port_info->remote_ip[2],
			P_port_info->remote_ip[3]);

	//��Է�����Ӧ��
	espconn_send(T_arg,"ESP8266_WIFI_Recv_OK",os_strlen("ESP8266_WIFI_Recv_OK"));
}

//����espconn�ͽṹ��(�������ӽṹ��)
/***********************************
 * struct espconn�ṹ���Ա
 * 		type         : Э������(TCP/UDP)
 * 		state        : ��ǰ����״̬
 * 		proto        : Э��ռ�ָ��
 * 		recv_callback: ���ջص�����
 * 		sent_callback: ���ͻص�����
 * 		link_cnt     : ��������
 * 		reverse      :
 ***********************************/
struct espconn ST_NetCon;	// ע�����붨��Ϊȫ�ֱ������ں˽���ʹ�ô˱���(�ڴ�)

//��ʼ����������(UDPͨ��)
void ICACHE_FLASH_ATTR ESP8266_UDP_Init(void)
{
	// ESPCONN_UDP:0x20---UDPЭ��
	// ESPCONN_TCP:0x10---TCPЭ��
	ST_NetCon.type = ESPCONN_UDP;		// ͨ��Э�飺UDP
	ST_NetCon.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));	// UDPЭ��ָ�������ڴ�
	// �˴���������Ŀ��IP/�˿�(ESP8266��ΪServer������ҪԤ��֪��Client��IP/�˿�)
	ST_NetCon.proto.udp->local_port  = 8266;		// ���ñ��ض˿�

	//ע��ص�����
	espconn_regist_sentcb(&ST_NetCon, ESP8266_WIFI_Send);	// ע���������ݷ��ͳɹ��Ļص�����
	espconn_regist_recvcb(&ST_NetCon, ESP8266_WIFI_Recv);	// ע���������ݽ��ճɹ��Ļص�����

	//��ʽ��ʼ��UDPͨ��
	espconn_create(&ST_NetCon);	// ��ʼ��UDPͨ��
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
	u8 ESP8266_IP[4];		        // ���ʮ������ʽ����IP
	u8 S_WIFI_STA_Connect;			// WIFI����״̬��־

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

		ESP8266_UDP_Init();   //��ʼ����������(UDPͨ��)

		os_timer_disarm(&OS_Timer1);	// �رն�ʱ��
	}
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

	//�����ʱ����ʼ��  5s
	OS_Timer1_Init(5000, 1);
}

