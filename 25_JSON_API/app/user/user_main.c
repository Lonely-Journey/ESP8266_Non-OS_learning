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
//JSON���ͷ�ļ�
#include "user_json.h"
#include "c_types.h"


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

//��������
LOCAL int ICACHE_FLASH_ATTR JSON_AssignValue(struct jsontree_context *js_ctx);
int ICACHE_FLASH_ATTR JSON_Tree_Parse(struct jsontree_context *js_ctx, struct jsonparse_state *parser);

// ���á�����JSON������������JSON�����Ļص�����
//��������÷���һ����"ֵ"�������鷳�����Զ�����JSON���ƺ�����
struct jsontree_callback JSON_Tree_Set = JSONTREE_CALLBACK(JSON_AssignValue, JSON_Tree_Parse);	// �ṹ�嶨��
char A_JSON_Tree[256] = {0};	// ���JSON��

/************************************************************************************/
/*                                  JSON��������ش���                                                                                                       */
/************************************************************************************/
//��jsontree_path_name(...)��		��ȡJSON����Ӧ��ȵġ�"��"����ָ�롿
//��jsontree_write_string(...)��	��ֵJSON����Ӧ��ȵġ�"��"��(���ַ�����ʽ��ֵ)��
//��jsontree_write_atom(...)��	��ֵJSON����Ӧ��ȵġ�"��"��(����ֵ����ʽ��ֵ)��
/*����JSON���Ļص�������JSON����ֵ*/
LOCAL int ICACHE_FLASH_ATTR JSON_AssignValue(struct jsontree_context *js_ctx)
{
	//��ȡJSON����Ӧ��ȵġ�"��"����ָ�룬����Ҫ��ȡ��ǰ��ļ�ֵ�Եļ���
	const char *P_Key_current = jsontree_path_name(js_ctx,js_ctx->depth-1);	// ��ȡָ��ǰ��"��"����ָ��

	//�ж�Ŀ�������������Ҫ��Ŀ���������Ҫ�ü�ֵ
	if(os_strncmp(P_Key_current, "XXX", os_strlen("XXX")) == 0 )			// �жϵ�ǰ��"��"�� ?= "result"
	{
		//��ֵJSON����Ӧ��ȵġ�"��"��(���ַ�����ʽ��ֵ)
		jsontree_write_string(js_ctx,"Hello World!");	// ����"ֵ"="Shenzhen is too hot!"��д���Ӧλ��
	}
	else
	{
		//��ȡJSON����Ӧ��ȵġ�"��"����ָ�룬����Ҫ��ȡ��ǰ��ļ�ֵ�Ե���һ�����
		const char *P_Key_upper = jsontree_path_name(js_ctx,js_ctx->depth-2);

		//�жϵ���һ��ļ����Ƿ�Ϊ"Shanghai"
		if(os_strncmp(P_Key_upper, "Shanghai", os_strlen("Shanghai")) == 0)
		{
			//�жϵ�ǰ��ļ����Ƿ���Ŀ�����
			if(os_strncmp(P_Key_current, "temp", os_strlen("temp")) == 0)
			{
				//��ֵJSON����Ӧ��ȵġ�"��"��(���ַ�����ʽ��ֵ)
				jsontree_write_string(js_ctx,"30��");
			}
			if(os_strncmp(P_Key_current, "humid", os_strlen("humid")) == 0)
			{
				//��ֵJSON����Ӧ��ȵġ�"��"��(����ֵ����ʽ��ֵ)
				jsontree_write_atom(js_ctx,"30");
			}

		}
		//�жϵ���һ��ļ����Ƿ�Ϊ"Shenzhen"
		else if(os_strncmp(P_Key_upper, "Shenzhen", os_strlen("Shenzhen")) == 0 )
		{
			if(os_strncmp(P_Key_current, "temp", os_strlen("temp")) == 0)
			{
				jsontree_write_string(js_ctx,"35��");
			}

			if(os_strncmp(P_Key_current, "humid", os_strlen("humid")) == 0)
			{
				jsontree_write_atom(js_ctx,"50");
			}
		}
	}

	return 0;
}

/*******************************************
* ����������һ��������ΪV_Key_1��
* ��������������ֵ�Էֱ���
* ����Ϊtemp����ֵΪJSON_Tree_Set
* ����Ϊhumid����ֵΪJSON_Tree_Set
******************************************/
JSONTREE_OBJECT(V_Key_1,
				JSONTREE_PAIR("temp", &JSON_Tree_Set),
				JSONTREE_PAIR("humid", &JSON_Tree_Set));	// ���á�"��":"ֵ"��
JSONTREE_OBJECT(V_Key_2,
				JSONTREE_PAIR("temp", &JSON_Tree_Set),
				JSONTREE_PAIR("humid", &JSON_Tree_Set));	// ���á�"��":"ֵ"��

/*******************************************
* �����ڶ���һ��������ΪV_JSON��
* �ڶ�����������ֵ�Էֱ���
* ����ΪShanghai����ֵΪV_Key_1
* ����ΪShenzhen����ֵΪV_Key_2
* ����Ϊguangzhou����ֵΪV_Key_3
******************************************/
JSONTREE_OBJECT(V_JSON,
				JSONTREE_PAIR("Shanghai", &V_Key_1),
				JSONTREE_PAIR("Shenzhen", &V_Key_2),
				JSONTREE_PAIR("XXX", &JSON_Tree_Set));		// ���á�"��":"ֵ"��


//JSONTREE_OBJECT( ����1, ����2, ����3, ... )
//����1�����ɵ�JSON�����������
//����2~n����ֵ��
//��ע�⡿��������JSON��������"��"������ʾ��ֻ��ʾ��{ &V_JSON }
/*���������(��һ��)һ��������ΪObject_JOSN(����ʾ)�������(��һ��)����ΪV_JOSN(����ʾ)�������(��һ��)��ֵΪV_JSON*/
JSONTREE_OBJECT(Object_JOSN, JSONTREE_PAIR("K_JOSN", &V_JSON));		// ����JOSN����

/*����JSON��*/
void ICACHE_FLASH_ATTR Setup_JSON_Tree(void)
{
	//����JSON��
	//����1---��JSON����ָ��
	//����2---��JSON����ļ�
	//����3---JSON������ָ��
	json_ws_send((struct jsontree_value *)&Object_JOSN, "K_JOSN", A_JSON_Tree);

	os_printf("\r\n-------------------- ����JSON�� -------------------\r\n");

	os_printf("%s",A_JSON_Tree);	// ���ڴ�ӡJSON��

	os_printf("\r\n-------------------- ����JSON�� -------------------\r\n");
}
/************************************************************************************/



/************************************************************************************/
/*                                  JSON��������ش���                                                                                                       */
/************************************************************************************/
/*����JSON���Ļص�����*/
int ICACHE_FLASH_ATTR JSON_Tree_Parse(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
{
	int type;						// �ַ�����
	char buffer[64] = {0};			// ���桾ֵ��������

	os_printf("\r\n-------------------- ����JSON�� -------------------\r\n");

	//jsonparse_next()---���ã�����JSON��ʽ�µ�һ��Ԫ��
	//��ע�⡿��ÿ����һ��jsonparse_next(),parser����ı�
	type = jsonparse_next(parser);	// ��{����JSON����Ŀ�����ַ�
	os_printf("%c\n", type);

	//��ע�⡿����������һ���������򷵻�'N'
	type = jsonparse_next(parser);	// ����N������һ����������Shanghai��
	if(type != 0)
	{
		// �жϽ������ǲ��Ǽ�ֵ
		if(type == JSON_TYPE_PAIR_NAME)
		{
			//�жϼ����ǲ���"Shanghai"
			if(jsonparse_strcmp_value(parser, "Shanghai") == 0)
			{
				os_printf("\t Shanghai{2} \n");
				type = jsonparse_next(parser);		// ��:��
				type = jsonparse_next(parser);		// ��{��
				type = jsonparse_next(parser);  	// ����N��

				if (jsonparse_strcmp_value(parser, "temp") == 0)	// ��temp��
				{
					type = jsonparse_next(parser);		// ��:��
					type = jsonparse_next(parser);  	// ��"��
					if (type == JSON_TYPE_STRING)		// �ж��Ƿ��ǡ�"��
					{
						jsonparse_copy_value(parser, buffer, sizeof(buffer));	// ��ȡ��������Ӧ��ֵ
						os_printf("\t\t temp: %s\n", buffer);		// ��30�桿
					}

					type = jsonparse_next(parser);  	// ��,��
					type = jsonparse_next(parser);  	// ����N��

					if (jsonparse_strcmp_value(parser, "humid") == 0)	// ��humid��
					{
						type = jsonparse_next(parser);  	// ��:��
						type = jsonparse_next(parser);  	// ��0��("ֵ" = ��ֵ)
						if (type == JSON_TYPE_NUMBER)		// �ж��Ƿ��ǡ�0��(��ֵ == ASSIC����ʽ)
						{
							jsonparse_copy_value(parser, buffer, sizeof(buffer));	// ��ȡ��������Ӧ��ֵ
							os_printf("\t\t humid: %s\n", buffer);	// ��30��

							type = jsonparse_next(parser);  	// ��}��
							type = jsonparse_next(parser);  	// ��,��

							type = jsonparse_next(parser);  	//����N�����ڶ�����������Shenzhen��
							if(jsonparse_strcmp_value(parser, "Shenzhen") == 0)	// ��Shenzhen��
							{
								os_printf("\t Shenzhen{2} \n");

								jsonparse_next(parser);		// ��:��
								jsonparse_next(parser);		// ��{��

								jsonparse_next(parser);  	// ����N��
								if(jsonparse_strcmp_value(parser, "temp") == 0)	// ��temp��
								{
									type = jsonparse_next(parser);		// ��:��
									type = jsonparse_next(parser);  	// ��"��
									if(type == JSON_TYPE_STRING)		// �ж��Ƿ��ǡ�"��
									{
										jsonparse_copy_value(parser, buffer, sizeof(buffer));	// ��ȡ��������Ӧ��ֵ
										os_printf("\t\t temp: %s\n", buffer);	// ��35�桿

										type = jsonparse_next(parser);  	// ��,��
										type = jsonparse_next(parser);  	// ����N��
										if(jsonparse_strcmp_value(parser, "humid") == 0)	// ��humid��
										{
											type = jsonparse_next(parser);  	// ��:��
											type = jsonparse_next(parser);  	// ��0��("ֵ" = ��ֵ)

											if (type == JSON_TYPE_NUMBER)		// �ж��Ƿ��ǡ�0��(��ֵ == ASSIC����ʽ)
											{
												jsonparse_copy_value(parser, buffer, sizeof(buffer));	// ��ȡ��������Ӧ��ֵ
												os_printf("\t\t humid: %s\n", buffer);	// ��50%RH��

												type = jsonparse_next(parser);  	// ��}��
												type = jsonparse_next(parser);  	// ��,��

												type = jsonparse_next(parser);  	//����N������������������result��
												if(jsonparse_strcmp_value(parser, "XXX") == 0)
												{
													type = jsonparse_next(parser);  // ��:��
													type = jsonparse_next(parser);  // ��"��
													if (type == JSON_TYPE_STRING)	// �ж��Ƿ��ǡ�"��
													{
														jsonparse_copy_value(parser, buffer, sizeof(buffer));	// ��ȡ��������Ӧ��ֵ
														os_printf("\t XXX: %s\n", buffer);	//��Shenzhen is too hot!��

														type = jsonparse_next(parser);	// ��}����JSON����Ŀ��β�ַ�
														os_printf("%c\n", type);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	os_printf("\r\n-------------------- ����JSON�� -------------------\r\n");
	return 0;
}


/*����JSON��*/
void ICACHE_FLASH_ATTR Parse_JSON_Tree(void)
{
	struct jsontree_context js;		// JSON�������ṹ��

	// ��Ҫ������JSON��������ϵ
	//������1��JSON�������ṹ��ָ��		����2��JSON���ĵڶ���������ָ��		����3��json_putchar������
	jsontree_setup(&js, (struct jsontree_value *)&V_JSON, json_putchar);

	// ����JSON��
	//������1��JSON�������ṹ��ָ��(������JSON��)		����2����Ž���JSON����Ļ���ָ��
	json_parse(&js, A_JSON_Tree);	// ִ��������䣬������ö�Ӧ��JSON�������ص�����
}
/************************************************************************************/



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

	Setup_JSON_Tree();		// ����JSON��
	Parse_JSON_Tree();		// ����JSON��
}

