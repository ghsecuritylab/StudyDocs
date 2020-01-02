#ifndef _LIGHT_LIB_H_
#define _LIGHT_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_adapter_platform.h"

/*
version: 1.0.2 2018-11-29
1：  支持配置配网方式：上电低功耗/上电默认smartconfig
2：  支持配置上电复位次数
3：  支持1-5路灯
4：  支持配置默认亮度/色温百分比值
5：  支持配置最大/最小亮度
6：  支持V2版双路由产测
7：  支持自定义产测
8：  支持双路白光CW/CCT驱动方式
9：  支持新老面板
10：支持默认上电状态为纯彩色（RGB），亮度可选
11：支持断电记忆可选
12：支持配网方式选择（上电次数/按键长按）
*/

#define     MAX_TEST_SSID_LEN      16
#define PRODUCT_KEY_DATA_KEY "XYX_PRODUCT_KEY_DATA_KEY"

//wifi status
typedef enum
{
  CONF_STAT_SMARTCONFIG = 0,
  CONF_STAT_APCONFIG,
  CONF_STAT_UNCONNECT,
  CONF_STAT_CONNECT,
  CONF_STAT_LOWPOWER,
} CONF_STAT_E;

//light type
typedef enum
{
  LIGHT_COLOR_C = 1,
  LIGHT_COLOR_CW = 2,
  LIGHT_COLOR_RGB = 3,
  LIGHT_COLOR_RGBC = 4,
  LIGHT_COLOR_RGBCW = 5,
} LIGHT_COLOR_TYPE_E;

typedef enum
{
  LIGHT_DRV_MODE_PWM_IO = 0,    /* PWM driver */
  LIGHT_DRV_MODE_IIC_SMx726,    /* IIC SM16726/SM726 driver */
  LIGHT_DRV_MODE_IIC_SM2135,    /* IIC SM2135 driver */
  LIGHT_DRV_MODE_PWM_9231,    /* 9231 */
} LIGHT_DRIVER_MODE_E;

//
typedef enum
{
  INIT_COLOR_R = 0,
  INIT_COLOR_G,
  INIT_COLOR_B,
  INIT_COLOR_C,
  INIT_COLOR_W,
} INIT_LIGHT_COLOR_E;

//driver mode of warm and cold lights
typedef enum
{
  LIGHT_CW_PWM = 0,
  LIGHT_CW_CCT,
} LIGHT_CW_TYPE_E;

//Color control logic
typedef enum
{
  RGB_POWER_OFF = 0,
  RGB_POWER_ON,
} COLOR_POWER_CTL_E;

//Power on by default
typedef enum
{
  WIFI_CFG_OLD_PROD = 0,              // GWCM_OLD mode with product
  WIFI_CFG_LOW_POWER = 1,              // with low power mode
  WIFI_CFG_SPCL_MODE = 2,              // special with low power mode
  WIFI_CFG_OLD_CPT = 3,                // old mode scan
} WIFI_CFG_DEFAULT_E;

typedef enum
{
  PROD_TEST_TYPE_CUSTOM = 0,     //Custom, a callback must be passed in
  PROD_TEST_TYPE_V1,             //Version1 of single route production test
  PROD_TEST_TYPE_V2,             //Version2 of double route production test
} PROD_TEST_TYPE_E;

typedef enum
{
  LEVEL_LOW = FALSE,
  LEVEL_HIGH = TRUE,
} LEVEL_STA_E;

//Reset the wifi configuration mode
typedef enum
{
  LIGHT_RESET_BY_POWER_ON_ONLY = 0,
  LIGHT_RESET_BY_LONG_KEY_ONLY = 1,
  LIGHT_RESET_BY_ANYWAY = 2,
} LIGHT_RESET_NETWORK_STA_E;

//APP switch control mode
typedef enum
{
  POWER_ONOFF_BY_SHADE = 0,
  POWER_ONOFF_BY_DIRECT,
} LIGHT_POWER_ONOFF_TYPE_E;

//The final effect type of the data currently being sent
//(Some drivers do not support white light and color light output at the same time)
typedef enum
{
  WHITE_DATA_SEND = 0,
  COLOR_DATA_SEND,
  MIX_DATA_SEND,
  NULL_DATA_SEND,
} LIGHT_CURRENT_DATA_SEND_MODE_E;

typedef enum
{
  WHITE_NET_LIGHT = 0,
  COLOR_NET_LIGHT,
} NET_LIGHT_TYPE_E;

typedef struct
{
  WIFI_CFG_DEFAULT_E wf_cfg;
  UCHAR_T wf_rst_cnt;
  LIGHT_COLOR_TYPE_E color_type;
  LIGHT_DRIVER_MODE_E light_drv_mode;
  LIGHT_POWER_ONOFF_TYPE_E power_onoff_type;
  //
  NET_LIGHT_TYPE_E net_light_type;
  INIT_LIGHT_COLOR_E def_light_color;
  BYTE_T def_bright_precent;
  BYTE_T def_temp_precent;

  BYTE_T bright_max_precent;
  BYTE_T bright_min_precent;

  BYTE_T color_bright_max_precent;
  BYTE_T color_bright_min_precent;

  BOOL_T whihe_color_mutex;
}
LIGHT_DEFAULT_CONFIG_S;

//产测结构体定义
typedef struct
{
  UCHAR_T cc_time;
  UCHAR_T cw_time;
  UCHAR_T rgb_time;
  UCHAR_T suc_light_num;
} AGING_TIME_DEF;

typedef struct
{
  UCHAR_T fuc1_time;
} FUC_TIME_DEF;

//产测默认参数结构体定义
typedef struct
{
  UCHAR_T test1_ssid[MAX_TEST_SSID_LEN + 1];
  UCHAR_T test2_ssid[MAX_TEST_SSID_LEN + 1];

  AGING_TIME_DEF aging_time;

  BOOL_T retest;
} LIGHT_PROD_PAR;




OPERATE_RET tuya_light_init (VOID);
OPERATE_RET tuya_light_start (VOID);
VOID light_init_stat_set (VOID);
OPERATE_RET tuya_light_cw_type_set (LIGHT_CW_TYPE_E type);
OPERATE_RET tuya_light_reset_mode_set (LIGHT_RESET_NETWORK_STA_E mode);
OPERATE_RET tuya_light_cw_max_power_set (UCHAR_T max_power);
VOID tuya_light_memory_flag_set (BOOL_T status);
VOID light_key_fun_free_heap_check_start (VOID);
VOID light_key_fun_free_heap_check_stop (VOID);

VOID light_key_fun_power_onoff_ctrl (VOID);

VOID light_key_fun_wifi_reset (VOID);

TUYA_WF_CFG_MTHD_SEL light_get_wifi_cfg (VOID);

VOID light_dp_upload (VOID);

VOID prod_test (BOOL_T flag, SCHAR_T rssi);

VOID tuya_light_ctrl_precision_set (USHORT_T val);

USHORT_T tuya_light_ctrl_precision_get (VOID);

/*************************************************************************************
函数功能: 获取灯光最终输出模式
输入参数: 无
输出参数: 无
返 回 值: 灯光渐变最后的模式
备    注:   无
*************************************************************************************/
LIGHT_CURRENT_DATA_SEND_MODE_E light_send_final_data_mode_get (VOID);


#ifdef __cplusplus
}
#endif

#endif

