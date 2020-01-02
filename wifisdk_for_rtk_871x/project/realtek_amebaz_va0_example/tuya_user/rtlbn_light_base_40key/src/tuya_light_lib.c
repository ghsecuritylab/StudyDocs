#include ".compile_usr_cfg.h"
#include "tuya_light_lib.h"
#include "tuya_light_hard.h"
#include "uf_file.h"
#include "irDecode.h"

#define TUYA_LIGH_LIB_VERSION   "1.0.2"

#define FASE_SW_CNT_KEY   "fsw_cnt_key"
#define LIGHT_DATA_KEY     "light_data_key"

#define LIGHT_PRECISION    500    //调光精度
#define LIGHT_PRECISION_MAX   10000   //调光精度最大限制
#define LIGHT_PRECISION_MIN   100    //调光精度最低限制
#define LIGHT_PRECISION_BASE_TIME 306900   //调光精度步进计算基准时间

#define HW_TIMER_CYCLE          100     //us
#define NORMAL_DELAY_TIME   614     //hw_arm = us（以1023级，每步300us为准）= (306900/LIGHT_PRECISION)
#define SCENE_DELAY_TIME   100     //情景渐变计算假定延时时间(ms)
#define STANDARD_DELAY_TIMES 300     //无极调光时间(ms)
#define SHADE_THREAD_TIMES  300     //shade线程耗时(us) 

#define NORMAL_DP_SAVE_TIME      2500    //DP数据保存定时器启动时间 单位 MS
#define NORMAL_DP_UPLOAD_TIME  100        //DP数据上传定时器启动时间 单位 MS
#define FAST_DP_UPLOAD_TIME   50        //DP数据上传定时器启动时间 单位 MS

#define LIGHT_RST_CNT_CYCLE   5000    //上电计数清除时间 单位 MS

#define LIGHT_LOW_POWER_ENTER_IMEW 3000    //APP关闭后低功耗进入延时（等待灯光处理完毕）单位 ms
#define LOWPOWER_MODE_DELAY_TIME 1200    //低功耗状态下线程轮询时间 单位 ms

#define BRIGHT_VAL_MAX          1000             //新面板下发最大亮度
#define BRIGHT_VAL_MIN          10              //新面板下发最小亮度

#define COLOR_DATA_LEN         12     //颜色数据长度
#define COLOR_DATA_DEFAULT  "000003e803e8"     //新版本默认彩光数据
#define FC_COLOR_DATA_DEFAULT   "000003e8000a"     //彩光&白光同时控制状态下，彩光初始数值
#define RGBCW_ORDER_DATA_LEN 6                   //RGBCW

//新版本默认情景数据
#define SCENE_DATA_DEFAULT_RGBC  "000e0d00002e03e8000000c803e8"//彩灯
#define SCENE_DATA_DEFAULT_RGB  "000e0d00002e03e802cc00000000"
#define SCENE_DATA_DEFAULT_CW  "000e0d00002e03e8000000c80000"//CW双色
#define SCENE_DATA_DEFAULT_DIM  "000e0d00002e03e8000000c803e8"//单色灯

//情景参数
#define SCENE_HEAD_LEN   2    //情景头长度
#define SCENE_UNIT_LEN      26          //新版灯情景单元长度
#define SCENE_UNIT_NUM_MAX  8    //最大情景单元数 

#define SCENE_DATA_LEN_MIN  (SCENE_HEAD_LEN + SCENE_UNIT_LEN)       //情景数据最小长度
#define SCENE_DATA_LEN_MAX  (SCENE_HEAD_LEN + SCENE_UNIT_LEN * SCENE_UNIT_NUM_MAX)  //情景数据最大长度


//音乐灯默认数据
#define MUSIC_INIT_VAL  COLOR_DATA_DEFAULT

#ifdef KEY_CHANGE_SCENE_FUN
  #define MAX_KEY_SCENE_BUF  (SCENE_DATA_LEN_MAX * 8) //按键情景需要的长度

  #define KEY_SCENE0_DEF_DATA "004f4f01016803e803e8000000004f4f01007803e803e8000000004f4f0100f003e803e800000000"
  #define KEY_SCENE1_DEF_DATA "013e3e01016803e803e8000000003e3e01007803e803e8000000003e3e0100f003e803e800000000" 
  #define KEY_SCENE2_DEF_DATA "024e4e01016803e803e8000000004e4e01007803e803e8000000004e4e0100f003e803e800000000" 
  #define KEY_SCENE3_DEF_DATA "034d4d01016803e803e8000000004d4d01007803e803e8000000004d4d0100f003e803e800000000" 
  #define KEY_SCENE4_DEF_DATA "044a4a01016803e803e8000000004a4a01007803e803e8000000004a4a0100f003e803e800000000" 
  #define KEY_SCENE5_DEF_DATA "05464601016803e803e800000000464601007803e803e80000000046460100f003e803e800000000"
  #define KEY_SCENE6_DEF_DATA "064c4c01016803e803e8000000004c4c01007803e803e8000000004c4c0100f003e803e800000000" 
  #define KEY_SCENE7_DEF_DATA "07464601016803e803e800000000464601007803e803e80000000046460100f003e803e800000000"
  
  //#define KEY_SCENE0_DEF_DATA "000d0d00002e03e802cc00000000" 
  //#define KEY_SCENE1_DEF_DATA "010d0d000084000003e800000000"
  //#define KEY_SCENE2_DEF_DATA "020d0d00001403e803e800000000"
  //#define KEY_SCENE3_DEF_DATA "030e0d0000e80383031c00000000"
  //#define KEY_SCENE4_DEF_DATA "04464602007803e803e800000000464602007803e8000a00000000"
  //#define KEY_SCENE5_DEF_DATA "05464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000"
  //#define KEY_SCENE6_DEF_DATA "06464601000003e803e800000000464601007803e803e800000000"
  //#define KEY_SCENE7_DEF_DATA "07464602000003e803e800000000464602007803e803e80000000046460200f003e803e800000000464602003d03e803e80000000046460200ae03e803e800000000464602011303e803e800000000"

#else
  #define MAX_KEY_SCENE_BUF  0
#endif

/**************************** 4 路默认情景数据 ***********/
#define KEY_SCENE0_DEFDATA0 "000e0d00002e03e8000000c803e8"
#define KEY_SCENE0_DEFDATA1 "000d0d0000000000000000c803e8"

#define KEY_SCENE1_DEFDATA0 "010e0d0000000000000003e803e8" 
#define KEY_SCENE1_DEFDATA1 "010e0d0000840000000003e803e8"

#define KEY_SCENE2_DEFDATA0 "020e0d0000000000000003e803e8"  
#define KEY_SCENE2_DEFDATA1 "020e0d0000e80383000003e803e8" 

#define KEY_SCENE3_DEFDATA0 "030e0d0000000000000001f403e8"  
#define KEY_SCENE3_DEFDATA1 "030e0d00001403e8000001f403e8"

#define KEY_SCENE4_DEFDATA "04464602007803e803e800000000464602007803e8000a00000000" 
#define KEY_SCENE5_DEFDATA "05464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000"
#define KEY_SCENE6_DEFDATA "06464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000" 
#define KEY_SCENE7_DEFDATA "07464602000003e803e800000000464602007803e803e80000000046460200f003e803e800000000464602003d03e803e80000000046460200ae03e803e8"



#define MAX_FLASH_BUF   (512 + MAX_KEY_SCENE_BUF)


typedef enum
{
  WHITE_MODE = 0,
  COLOR_MODE,
  SCENE_MODE,
  MUSIC_MODE,
} SCENE_MODE_E;

typedef struct
{
  BOOL_T power;
  BOOL_T rgb_power;  
  BOOL_T w_power;   
  SCENE_MODE_E mode;
  USHORT_T bright;
  USHORT_T col_temp;
  UCHAR_T color[COLOR_DATA_LEN + 1];
  UCHAR_T scene[SCENE_DATA_LEN_MAX + 1];
  UINT_T countdown;
#if USER_DEFINE_LIGHT_STRIP_LED_CONFIG
  UCHAR_T rgbcw_order[RGBCW_ORDER_DATA_LEN];
#endif
} DP_DATA_S;

typedef enum
{
  SCENE_TYPE_STATIC = 0,
  SCENE_TYPE_JUMP,
  SCENE_TYPE_SHADOW,
} SCENE_TYPE_E;

typedef enum
{
  CTRL_TYPE_JUMP = 0,
  CTRL_TYPE_SHADE,
} CRTL_TYPE_E;

//新版本最小的范围也超过了255，全部使用USHORT兼容
typedef struct
{
  USHORT_T red;
  USHORT_T green;
  USHORT_T blue;
  USHORT_T white;
  USHORT_T warm;
} BRIGHT_DATA_S;

typedef struct
{
  BRIGHT_DATA_S fin;                  //灯光目标值
  BRIGHT_DATA_S curr;                 //灯光当前值

  SCENE_TYPE_E scene_type;
  UCHAR scene_cnt;

  BOOL_T scene_enable;                  //SCENE使能标志
  BOOL_T scene_new;                     //计算新值标志

  BOOL_T shade_new;//新渐变标志
  uint8_t scene_typeno;
  BOOL_T flashflag;
  //BOOL_T memory;//断电记忆标志
} LIGHT_HANDLE_S;

typedef struct
{
  UINT_T total;
  UINT_T hw_cnt;

  BOOL_T enable;
} HW_TIMER_S;
STATIC HW_TIMER_S tuya_light_hw_timer;

STATIC DP_DATA_S dp_data;
STATIC LIGHT_HANDLE_S light_handle;

#ifdef KEY_CHANGE_SCENE_FUN
  STATIC CHAR_T key_scene_data[8][SCENE_DATA_LEN_MAX + 1];
#else
  STATIC CHAR_T* key_scene_data;
#endif

typedef struct
{
  TIMER_ID timer_init_dpdata;
  TIMER_ID timer_wf_stat;
  TIMER_ID timer_data_autosave;
  TIMER_ID timer_countdown;
  TIMER_ID timer_ram_checker;
  TIMER_ID timer_key;
#if USER_DEFINE_LOWER_POWER
  TIMER_ID lowpower;
#endif
  MUTEX_HANDLE mutex;
  MUTEX_HANDLE light_send_mutex;

  SEM_HANDLE sem_shade;
} SYS_HANDLE_S;

STATIC SYS_HANDLE_S sys_handle;

typedef struct
{
  SCENE_TYPE_E type;
  UCHAR_T speed;//需要精确数值保证计算
  UCHAR_T times;//0-100
} SCENE_HEAD_S;


/*******************************************************
内部函数、变量声明
********************************************************/
STATIC OPERATE_RET light_data_write_flash (VOID);
VOID light_dp_data_default_set (VOID);
STATIC VOID light_scene_start (VOID);
STATIC VOID light_scene_stop (VOID);
VOID light_dp_upload (VOID);
STATIC VOID light_shade_start (UINT time);
STATIC VOID light_shade_stop (VOID);
STATIC VOID light_bright_start (VOID);
STATIC VOID light_switch_set (BOOL stat);
STATIC VOID light_cct_col_temper_keep (UCHAR data);
STATIC USHORT light_default_bright_get (VOID);
STATIC USHORT light_default_temp_get (VOID);
STATIC VOID light_default_color_get (UCHAR* data);
STATIC UCHAR dp_data_change_for_old_type (USHORT val);
//按键
STATIC VOID light_key_scene_data_save (char* str, UCHAR len);
STATIC VOID light_key_scene_data_default_set (VOID);
//系统定时器回调
STATIC VOID lowpower_timer_cb (UINT_T timerID, PVOID_T pTimerArg);

/* 配置相关定义 */
STATIC LIGHT_DEFAULT_CONFIG_S _def_cfg = {0};  //通用配置项
STATIC LIGHT_CW_TYPE_E _cw_type = LIGHT_CW_PWM;  //白光输出模式
STATIC LIGHT_RESET_NETWORK_STA_E _reset_dev_mode = LIGHT_RESET_BY_ANYWAY; //模块状态重置方式
STATIC BOOL_T _light_memory; //断电记忆
STATIC UCHAR_T _white_light_max_power = 100; //白光最大功率（冷暖混色时）
#if USER_DEFINE_LOWER_POWER
  STATIC BYTE_T lowpower_flag = 0; //低功耗控制位
#endif
/* 功能相关定义 */
STATIC UCHAR_T key_trig_num = 0; //按键触发次数
STATIC USHORT_T key_trig_length = 0; 
STATIC USHORT_T light_precison = LIGHT_PRECISION;
STATIC UINT_T normal_delay_time = NORMAL_DELAY_TIME;
STATIC BOOL_T _light_threadFlag; //断电记忆

static uint8_t staticFlag,breathflag,raindropflag,colorfulflag;
static uint8_t marqueeflag,blinkingflag,snowcolorflag,streamerflag;

/*******************************************************
外部函数、变量声明
********************************************************/
extern OPERATE_RET light_prod_init (PROD_TEST_TYPE_E type, APP_PROD_CB callback);

uint8_t light_IrCrtl_IR_Mode;
uint16_t light_IrCrtl_IR_Runtimes;
uint8_t light_IrCrtl_IR_Cnts;
uint8_t light_IrCrtl_IR_Cnts;
uint8_t light_IrCrtl_IR_Flag;
uint8_t light_IrCrtl_Speedtimes;
uint8_t light_IrCrtl_ManulTimes;
uint8_t light_IrCrtl_ManulFlag;
uint8_t light_IrCrtl_IR_Nums;


/*** 自动运行模式处理 ***/
void IR_AutomMode(void)
{
  /**每种模式自动运行10s**/
  if(light_IrCrtl_IR_Mode)
  {
    if(light_IrCrtl_IR_Runtimes++>IRAUTO_RUNTIMES) 
    {
      light_IrCrtl_IR_Runtimes = 0;
      if(light_IrCrtl_IR_Cnts++>=IRAUTO_MODEMAXS)
      {
        light_IrCrtl_IR_Cnts = 0;
      }
      
      if(light_IrCrtl_IR_Cnts==1)
        cur_ircmd = KEY_JUMP3;
      else if(light_IrCrtl_IR_Cnts==2)
        cur_ircmd = KEY_FADE3;
      else if(light_IrCrtl_IR_Cnts==3)
        cur_ircmd = KEY_JUMP7;
      else if(light_IrCrtl_IR_Cnts==4)
        cur_ircmd = KEY_FADE7;
      else
        cur_ircmd = KEY_FLASH;
      
      cur_irType = IRCODEAUTOSTART; 
      PostSemaphore (irdeal.ir_cmddeal_sem);
      return;
    }
    //PR_NOTICE("----------------%d %d",light_IrCrtl_IR_Runtimes,light_IrCrtl_IR_Cnts);
  }
}

/*******************************************************
功能函数
********************************************************/
STATIC UCHAR_T __asc2hex (CHAR_T asccode)
{
  UCHAR_T ret;

  if ('0' <= asccode && asccode <= '9')
  {
    ret = asccode - '0';
  }
  else if ('a'<=asccode && asccode <= 'f')
  {
    ret = asccode - 'a' + 10;
  }
  else if ('A' <= asccode && asccode <= 'F')
  {
    ret = asccode - 'A' + 10;
  }
  else
  {
    ret = 0;
  }

  return ret;
}

STATIC CHAR_T __hex2asc (CHAR_T data)
{
  CHAR_T result;

  if ( (data>=0) && (data<=9) )       //变成ascii数字
  {
    result = data + 0x30;
  }
  else if ( (data >= 10) && (data <= 15) ) //变成ascii小写字母
  {
    result = data + 0x37 + 32;
  }
  else
  {
    result = 0xff;
  }

  return result;
}

//两字符合并为一字节
STATIC UCHAR_T __str2byte (UCHAR_T a, UCHAR_T b)
{
  return (a << 4) | (b & 0xf);
}

//四字符合并为一整型
STATIC UINT_T __str2short (u32_t a, u32_t b, u32_t c, u32_t d)
{
  return (a << 12) | (b << 8) | (c << 4) | (d & 0xf);
}

//绝对值
STATIC INT_T __abs (INT_T value)
{
  return value > 0 ? value : -value;
}

STATIC CHAR_T* dpid2str (CHAR_T* buf, BYTE_T dpid)
{
  memset (buf, 0, SIZEOF (buf) );
  snprintf (buf, SIZEOF (buf), "%d", dpid);
}

//获取最大值
STATIC UINT_T __max_value (UINT_T a, UINT_T b, UINT_T c, UINT_T d, UINT_T e)
{
  int x = a > b ? a : b;
  int y = c > d ? c : d;
  int z = x > y ? x : y;
  return z > e ? z : e;
}

//HSV转RGB
STATIC VOID hsv2rgb (FLOAT_T h, FLOAT_T s, FLOAT_T v, USHORT_T* color_r, USHORT_T* color_g, USHORT_T* color_b)
{
  FLOAT_T h60, f;
  UINT_T h60f, hi;
  h60 = h / 60.0;
  h60f = h / 60;
  hi = (int) h60f % 6;
  f = h60 - h60f;
  FLOAT_T p, q, t;
  p = v * (1 - s);
  q = v * (1 - f * s);
  t = v * (1 - (1 - f) * s);
  FLOAT_T r, g, b;
  r = g = b = 0;

  if (hi == 0)
  {
    r = v;          g = t;        b = p;
  }
  else if (hi == 1)
  {
    r = q;          g = v;        b = p;
  }
  else if (hi == 2)
  {
    r = p;          g = v;        b = t;
  }
  else if (hi == 3)
  {
    r = p;          g = q;        b = v;
  }
  else if (hi == 4)
  {
    r = t;          g = p;        b = v;
  }
  else if (hi == 5)
  {
    r = v;          g = p;        b = q;
  }

  r = (r * (FLOAT) light_precison);
  g = (g * (FLOAT) light_precison);
  b = (b * (FLOAT) light_precison);
  r *= 100;
  g *= 100;
  b *= 100;
  *color_r = (r + 50) / 100;
  *color_g = (g + 50) / 100;
  *color_b = (b + 50) / 100;
}

//彩色数据转化为字符
STATIC light_color_data_hex2asc (UCHAR_T* buf, USHORT_T data, BYTE_T len)
{
  if (len == 2)
  {
    buf[0] = __hex2asc ( (UCHAR) (data >> 4) & 0x0f);
    buf[1] = __hex2asc ( (UCHAR) data & 0x0f);
  }
  else if (len == 4)
  {
    buf[0] = __hex2asc ( (UCHAR) (data >> 12) & 0x0f);
    buf[1] = __hex2asc ( (UCHAR) (data >> 8) & 0x0f);
    buf[2] = __hex2asc ( (UCHAR) (data >> 4) & 0x0f);
    buf[3] = __hex2asc ( (UCHAR) data & 0x0f);
  }
}

void light_scene_SetDataPara(unsigned char times, unsigned char speed)
{
  uint16_t mydata;
  uint8_t index;  
  mydata = ((uint16_t)speed<<  8) | times;
  for(index = 0; index<light_IrCrtl_IR_Nums;index++)
  light_color_data_hex2asc(&(dp_data.scene[SCENE_HEAD_LEN+(index)*SCENE_UNIT_LEN]), mydata, 4);
  //PR_NOTICE("------------------------------------------------------------- %s %d", dp_data.scene,times);
  //light_bright_start();
  //light_dp_data_autoupload (FAST_DP_UPLOAD_TIME);
  //light_dp_data_autosave();
}


/*******************************************************
定时器回调函数
********************************************************/
STATIC VOID timer_key_cb (UINT_T timerID, PVOID_T pTimerArg)
{
  
}

#if USER_DEFINE_LOWER_POWER
STATIC VOID lowpower_timer_cb (UINT_T timerID, PVOID_T pTimerArg)
{
  if (dp_data.power == FALSE)
  {
    if (lowpower_flag == 0)
    {
      light_lowpower_enable();
      lowpower_flag = 1;
    }
  }
}
#endif

VOID tuya_light_wf_stat_cb (GW_WIFI_NW_STAT_E wf_stat)
{
  OPERATE_RET op_ret;
  STATIC TUYA_GW_WIFI_NW_STAT_E last_wf_stat = 0xff;

  if (last_wf_stat != wf_stat)
  {
    PR_DEBUG ("last_wf_stat:%d", last_wf_stat);
    PR_DEBUG ("wf_stat:%d", wf_stat);
    PR_DEBUG ("size:%d", tuya_light_get_free_heap_size());

    switch (wf_stat)
    {
      case TUYA_STAT_UNPROVISION:
      {
        //light_dp_data_default_set();
        //tuya_light_config_stat(CONF_STAT_SMARTCONFIG);
      }
      break;

      case TUYA_STAT_AP_STA_UNCFG:
      {
        //light_dp_data_default_set();
        //tuya_light_config_stat(CONF_STAT_APCONFIG);
      }
      break;

      case TUYA_STAT_STA_DISC:
      {
        tuya_light_config_stat (CONF_STAT_CONNECT);
        //tuya_light_config_stat(CONF_STAT_UNCONNECT);
      }
      break;

      case TUYA_STAT_LOW_POWER:
      {
        tuya_light_config_stat (CONF_STAT_LOWPOWER);
      }
      break;

      case TUYA_STAT_STA_CONN:
      case TUYA_STAT_AP_STA_CONN:
      {
        tuya_light_config_stat (CONF_STAT_CONNECT);
      }
      break;
    }

    last_wf_stat = wf_stat;
  }
}


/* 查询ram剩余大小 */
STATIC VOID ram_checker_cb (UINT_T timerID, PVOID_T pTimerArg)
{
  PR_NOTICE ("remain size:%d", tuya_light_get_free_heap_size() );
}

/* 网络状态定时查询 */
STATIC VOID timer_wf_stat_cb (UINT_T timerID, PVOID_T pTimerArg)
{
  OPERATE_RET op_ret;
  STATIC TUYA_GW_WIFI_NW_STAT_E last_wf_stat = 0xff;
  STATIC bool is_need_default = FALSE;
  TUYA_GW_WIFI_NW_STAT_E wf_stat = tuya_light_get_wf_gw_status();

  if (last_wf_stat != wf_stat)
  {
    PR_DEBUG ("last_wf_stat:%d", last_wf_stat);
    PR_DEBUG ("wf_stat:%d", wf_stat);
    PR_DEBUG ("size:%d", tuya_light_get_free_heap_size() );

    switch (wf_stat)
    {
      case TUYA_STAT_UNPROVISION:
      {
        is_need_default = TRUE;
        light_dp_data_default_set();
        light_scene_stop();
        light_shade_stop();
        //PR_DEBUG("light_scene_stop light_shade_stop ");
        tuya_light_config_stat (CONF_STAT_SMARTCONFIG, FALSE);
      }
      break;

      case TUYA_STAT_AP_STA_UNCFG:
      {
        is_need_default = TRUE;
        light_dp_data_default_set();
        light_scene_stop();
        light_shade_stop();
        //PR_DEBUG("light_scene_stop light_shade_stop ");
        tuya_light_config_stat (CONF_STAT_APCONFIG, FALSE);
      }
      break;

      case TUYA_STAT_STA_DISC:
      {
        tuya_light_config_stat (CONF_STAT_UNCONNECT, is_need_default);
      }
      break;

      case TUYA_STAT_LOW_POWER:
      {
        tuya_light_config_stat (CONF_STAT_LOWPOWER, FALSE);
      }
      break;

      case TUYA_STAT_STA_CONN:
      case TUYA_STAT_AP_STA_CONN:
      {
        tuya_light_config_stat (CONF_STAT_CONNECT, is_need_default);
      }
      break;
    }

    last_wf_stat = wf_stat;
  }
}
#if USER_DEFINE_LOWER_POWER
VOID light_lib_lowpower_disable (VOID)
{
  if (dp_data.power == TRUE)
  {
    light_lowpower_disable();
  }
}
#endif

/* MQTT连接后同步, key and ir sync*/
STATIC VOID idu_timer_cb (UINT_T timerID, PVOID_T pTimerArg)
{
  if (tuya_light_get_gw_mq_conn_stat() == TRUE)
  {
    light_dp_upload();
    sys_stop_timer (sys_handle.timer_init_dpdata);
#if USER_DEFINE_LOWER_POWER
    light_lib_lowpower_disable();
#endif
  }
}

/* 启动硬件定时器响应灯光渐变 */
STATIC VOID light_hw_timer_start (USHORT_T time)
{
  tuya_light_hw_timer.total = time;
  tuya_light_hw_timer.hw_cnt = 0;
  tuya_light_hw_timer.enable = TRUE;
}


/* 重启动硬件定时器开始渐变 */
STATIC VOID light_hw_timer_restart (VOID)
{
  tuya_light_hw_timer.enable = TRUE;
}

/* 硬件定时器停止响应灯光渐变 */
STATIC VOID light_hw_timer_stop (VOID)
{
  tuya_light_hw_timer.enable = FALSE;
  tuya_light_hw_timer.total = 0;
}

/* 硬件定时器回调函数 */
VOID tuya_light_hw_timer_cb (VOID)
{
  //PR_DEBUG("%d", tuya_light_hw_timer.hw_cnt);
  if (tuya_light_hw_timer.enable)
  {
    if (tuya_light_hw_timer.hw_cnt >= tuya_light_hw_timer.total)
    {
      PostSemaphore (sys_handle.sem_shade);
      tuya_light_hw_timer.hw_cnt = 0;
    }
    else
    {
      tuya_light_hw_timer.hw_cnt += HW_TIMER_CYCLE;
    }
  }
}

/* 本地倒计时部分 */
STATIC VOID countdown_timeout_cb (UINT_T timerID, PVOID_T pTimerArg)
{
  if (dp_data.countdown > 0)
  {
    dp_data.countdown -= 60;
    PR_DEBUG ("left times = %d", dp_data.countdown);
  }
  else
  {
    sys_stop_timer (sys_handle.timer_countdown);
    dp_data.countdown = 0;
  }

  if (dp_data.countdown == 0)
  {
    switch (dp_data.power)
    {
      case true:
        light_switch_set (FALSE);
        break;

      case false:
        light_switch_set (TRUE);
        break;

      default:
        break;
    }

    if (IsThisSysTimerRun (sys_handle.timer_countdown) == TRUE)
    {
      sys_stop_timer (sys_handle.timer_countdown);
    }
  }

  light_dp_upload();
}


/*****************************************
上电计数部分
*****************************************/
STATIC VOID light_rst_count_judge (VOID)
{
  OPERATE_RET op_ret;

  if (is_done_product_test() == 1)
  {
    if (_reset_dev_mode == LIGHT_RESET_BY_ANYWAY || _reset_dev_mode == LIGHT_RESET_BY_POWER_ON_ONLY)
    {
      light_dp_data_default_set();
      tuya_light_dev_reset();
    }
  }
}

STATIC VOID tuya_cnt_rst_inform_cb (VOID)
{
  PR_NOTICE ("tuya_cnt_rst_inform_cb cnt reset");

  if (_reset_dev_mode == LIGHT_RESET_BY_ANYWAY || _reset_dev_mode == LIGHT_RESET_BY_POWER_ON_ONLY)
  {
    //tuya_iot_wf_gw_fast_unactive will use kv component.
    tuya_iot_kv_flash_init (NULL);
    light_dp_data_default_set();
    //tuya_light_dev_reset();
    tuya_iot_wf_gw_fast_unactive (light_get_wifi_cfg(), WF_START_SMART_FIRST);
  }
}


/*****************************************
灯光数据处理部分
******************************************/
/* 白光亮度限制 */
STATIC USHORT_T light_val_lmt_get (USHORT_T dp_val)
{
  USHORT_T max = BRIGHT_VAL_MAX * _def_cfg.bright_max_precent / 100;
  USHORT_T min = BRIGHT_VAL_MAX * _def_cfg.bright_min_precent / 100;
  return ( (dp_val - BRIGHT_VAL_MIN) * (max - min) / (BRIGHT_VAL_MAX - BRIGHT_VAL_MIN) + min);
}

/* 彩光亮度限制 */
STATIC USHORT_T color_val_lmt_get (USHORT_T dp_val)
{
  USHORT_T max = BRIGHT_VAL_MAX * _def_cfg.color_bright_max_precent / 100;
  USHORT_T min = BRIGHT_VAL_MAX * _def_cfg.color_bright_min_precent / 100;
  return ( (dp_val - BRIGHT_VAL_MIN) * (max - min) / (BRIGHT_VAL_MAX - BRIGHT_VAL_MIN) + min);
}

/* 白光数据计算 */
STATIC VOID light_bright_cw_change (USHORT_T  bright, USHORT_T  temperture, BRIGHT_DATA_S* value)
{
  USHORT_T brt;

  if (bright >= BRIGHT_VAL_MIN)  //一般下发状态，白光正常输出时下发数值不会低于最低下发值
  {
    if (_def_cfg.whihe_color_mutex == FALSE &&
        bright == BRIGHT_VAL_MIN)  //分控功能下最低下发数值情况处于关闭状态
    {
      brt = 0;
    }
    else
    {
      brt = light_val_lmt_get (bright); //正常范围内计算实际输出范围
    }
  }
  else
  {
    brt = 0; //如果亮度出现下发数据小于最低限制情况（默认只有控制dp发送彩光数据时会清零白光数据），灯光默认关闭
  }

  if (_cw_type == LIGHT_CW_PWM)  //CW混光满足最大功率100% -- 200%
  {
    brt = (_white_light_max_power * light_precison / 100) * brt /
          BRIGHT_VAL_MAX; //灯光最大分级依照最大功率扩大
    value->white = brt * temperture / BRIGHT_VAL_MAX; //依据新的亮度总值进行冷暖输出分配
    value->warm  = brt - value->white;

    //如果上式计算中出现单路大于100%输出，此路输出限制在100%，并需要重新计算另外一路保持色温
    //低于最大值的一路根据 （大于最大值的另一路数据 / 最大值） 放大，100用于挺高计算精度
    if (value->white > light_precison)
    {
      value->warm  = value->warm * (value->white * 100 / light_precison) / 100;
      value->white = light_precison;
    }

    //同一时间内只会存在一路数据大于最大分级，处理思路同上
    if (value->warm > light_precison)
    {
      value->white  = value->white * (value->warm * 100 / light_precison) / 100;
      value->warm   = light_precison;
    }
  }
  else if (_cw_type == LIGHT_CW_CCT)
  {
    value->white = light_precison * brt / BRIGHT_VAL_MAX;
    value->warm = light_precison * temperture / BRIGHT_VAL_MAX;
  }
}

/* 白光数据格式转化（字符转数值） */
STATIC VOID light_cw_value_get (CHAR_T* buf, BRIGHT_DATA_S* value)
{
  if (buf == NULL)
  {
    PR_ERR ("invalid param...");
    return;
  }

  value->white = __str2short (__asc2hex (buf[0]), __asc2hex (buf[1]), __asc2hex (buf[2]), __asc2hex (buf[3]) );
  value->warm = __str2short (__asc2hex (buf[4]), __asc2hex (buf[5]), __asc2hex (buf[6]), __asc2hex (buf[7]) );

  if (value->white == 0 && _cw_type == LIGHT_CW_PWM)
  {
    value->warm = 0;
  }
  else
  {
    light_bright_cw_change (value->white, value->warm, value);
  }
}

/* 彩光数据格式转化（字符转数值）+ 处理（HSV转RGB） */
STATIC VOID light_rgb_value_get (CHAR_T* buf, BRIGHT_DATA_S* value)
{
  USHORT_T h, s, v;
  USHORT_T r, g, b;

  if (buf == NULL)
  {
    PR_ERR ("invalid param...");
    return;
  }

  if (_cw_type == LIGHT_CW_CCT)
  {
    if (dp_data.mode == COLOR_MODE || (dp_data.mode == SCENE_MODE && light_handle.fin.white == 0) )
    {
      light_handle.fin.warm = light_handle.curr.warm;
    }
  }

  h = __str2short (__asc2hex (buf[0]), __asc2hex (buf[1]), __asc2hex (buf[2]), __asc2hex (buf[3]) );
  s = __str2short (__asc2hex (buf[4]), __asc2hex (buf[5]), __asc2hex (buf[6]), __asc2hex (buf[7]) );
  v = __str2short (__asc2hex (buf[8]), __asc2hex (buf[9]), __asc2hex (buf[10]), __asc2hex (buf[11]) );

  if (_def_cfg.whihe_color_mutex == FALSE)
  {
    if (v <= 10)
    {
      v = 0;
    }
    else
    {
      v = color_val_lmt_get (v);
    }
  }
  else
  {
    if (v >= 10)
    {
      v = color_val_lmt_get (v);
    }
  }

  hsv2rgb ( (FLOAT) h, (FLOAT) s/1000.0, (FLOAT) v/1000.0, &r, &g, &b);
  tuya_light_gamma_adjust (r, g, b, &value->red, &value->green, &value->blue);
}

/* 获取当前灯光渐变最大差值 */
USHORT_T light_shade_max_value_get (BRIGHT_DATA_S* fin, BRIGHT_DATA_S* curr)
{
  USHORT_T max = __max_value (__abs (fin->red - curr->red), __abs (fin->green - curr->green),
                              __abs (fin->blue - curr->blue), \
                              __abs (fin->white - curr->white), __abs (fin->warm - curr->warm) );
  return max;
}

/* 根据定义渐变时间计算灯光渐变对应到硬件定时器时间 */
STATIC UINT light_shade_speed_get (USHORT_T time)
{
  USHORT_T step;
  UINT hw_set_cnt;
  step = light_shade_max_value_get (&light_handle.fin, &light_handle.curr);

  if (step == 0)
  {
    return HW_TIMER_CYCLE;
  }

  hw_set_cnt = 1000 * time / step - SHADE_THREAD_TIMES;
  return hw_set_cnt;                  //单位us
}


/*********************************************************
灯光功能处理函数
**********************************************************/
/* 目标点亮值获取 */
STATIC VOID light_bright_data_get (VOID)
{
  UCHAR_T i;

  if (_def_cfg.whihe_color_mutex == TRUE)
  {
      #if NEW_40IRKYE
        switch (dp_data.mode)
        {
          case WHITE_MODE:
            light_bright_cw_change (dp_data.bright, dp_data.col_temp, &light_handle.fin);
            light_rgb_value_get (dp_data.color, &light_handle.fin);
            // RGB 路关掉
            if(dp_data.rgb_power==0)
            {
              light_handle.fin.red = 0;
              light_handle.fin.green = 0;
              light_handle.fin.blue = 0;
            }
            PR_DEBUG ("white r=%d g=%d b=%d cw = %d   ww = %d",light_handle.fin.red, light_handle.fin.green,light_handle.fin.blue,light_handle.fin.white, light_handle.fin.warm);
            break;

          case COLOR_MODE:
            light_bright_cw_change (dp_data.bright, dp_data.col_temp, &light_handle.fin);
            light_rgb_value_get (dp_data.color, &light_handle.fin);
             // w 路关掉
            if(dp_data.w_power==0)
            {
              light_handle.fin.white = 0;
              light_handle.fin.warm = 0;
            }
            PR_DEBUG ("color r=%d g=%d b=%d cw = %d   ww = %d",light_handle.fin.red, light_handle.fin.green,light_handle.fin.blue,light_handle.fin.white, light_handle.fin.warm);
            break;

          default:
            break;
        }
      #else
        memset (&light_handle.fin, 0, SIZEOF (light_handle.fin) );

        switch (dp_data.mode)
        {
          case WHITE_MODE:
            light_bright_cw_change (dp_data.bright, dp_data.col_temp, &light_handle.fin);
            PR_DEBUG ("light_handle.fin ->color=%d cw = %d   ww = %d", dp_data.col_temp,light_handle.fin.white, light_handle.fin.warm);
            break;

          case COLOR_MODE:
            light_rgb_value_get (dp_data.color, &light_handle.fin);
            PR_DEBUG ("light_handle.fin -> color=%d r = %d   g = %d  b = %d", dp_data.col_temp ,light_handle.fin.red, light_handle.fin.green,light_handle.fin.blue);
            break;

          default:
            break;
        }
      #endif
  }
  else
  {
    light_bright_cw_change (dp_data.bright, dp_data.col_temp, &light_handle.fin);
    PR_DEBUG ("light_handle.fin -> cw = %d   ww = %d", light_handle.fin.white, light_handle.fin.warm);
    light_rgb_value_get (dp_data.color, &light_handle.fin);
    PR_DEBUG ("light_handle.fin -> r = %d   g = %d  b = %d", light_handle.fin.red, light_handle.fin.green,
              light_handle.fin.blue);
  }
}

/* 开始执行灯光功能 */
STATIC VOID light_bright_start (VOID)
{
  light_bright_data_get();
  
  switch (dp_data.mode)
  {
    case WHITE_MODE:
    case COLOR_MODE:
      light_scene_stop();
      light_shade_start (normal_delay_time);
      break;

    case SCENE_MODE:
      light_scene_start();
      break;

    case MUSIC_MODE:
      light_scene_stop();
      light_shade_stop();
      break;

    default:
      break;
  }
}

UINT light_work_mode_get()
{
  return dp_data.mode;
}

LIGHT_CURRENT_DATA_SEND_MODE_E light_send_final_data_mode_get (VOID)
{
  TUYA_GW_WIFI_NW_STAT_E wf_stat = tuya_light_get_wf_gw_status();

  if (wf_stat == TUYA_STAT_UNPROVISION || wf_stat == TUYA_STAT_AP_STA_UNCFG)
  {
    if (_def_cfg.net_light_type == WHITE_NET_LIGHT)
    {
      return WHITE_DATA_SEND;
    }
    else if (_def_cfg.net_light_type == COLOR_NET_LIGHT)
    {
      return COLOR_DATA_SEND;
    }
  }
  else
  {
    // PR_DEBUG("fin: r=%d g=%d b=%d c=%d w=%d", light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white, light_handle.fin.warm);
    if (light_handle.fin.red != 0 || light_handle.fin.green != 0 || light_handle.fin.blue != 0 || \
        light_handle.fin.white != 0 || light_handle.fin.warm != 0)
    {
      if (light_handle.fin.white == 0 && light_handle.fin.warm == 0)
      {
        return COLOR_DATA_SEND;
      }
      else if (light_handle.fin.red == 0 && light_handle.fin.green == 0 && light_handle.fin.blue == 0)
      {
        return WHITE_DATA_SEND;
      }
      else
      {
        return MIX_DATA_SEND;
      }
    }
    else
    {
      return NULL_DATA_SEND;
    }
  }
}

/* 发送灯光数据 */
STATIC VOID light_send_data (USHORT R_value, USHORT G_value, USHORT B_value, USHORT CW_value, USHORT WW_value)
{
  if (_def_cfg.light_drv_mode == LIGHT_DRV_MODE_IIC_SMx726)
  {
    if (R_value != 0 || G_value != 0 || B_value != 0)
    {
      tuya_light_color_power_ctl (RGB_POWER_ON);
    }
    else
    {
      tuya_light_color_power_ctl (RGB_POWER_OFF);
    }
  }

  if (_def_cfg.light_drv_mode == LIGHT_DRV_MODE_IIC_SM2135)
  {
    if (R_value != 0 || G_value != 0 || B_value != 0 || CW_value != 0 || WW_value != 0)
    {
      tuya_light_color_power_ctl(RGB_POWER_ON);
    }
    else
    {
      tuya_light_color_power_ctl(RGB_POWER_OFF);
    }
  }

  tuya_light_send_data (R_value, G_value, B_value, CW_value, WW_value);
}

STATIC VOID light_shade_start (UINT time)
{
  if (time < HW_TIMER_CYCLE)
  {
    time = HW_TIMER_CYCLE;
  }

  light_handle.shade_new = TRUE;
  PR_DEBUG ("light_shade_start :%d", time);
  light_hw_timer_start (time);
}

STATIC VOID light_shade_stop (VOID)
{
  light_hw_timer_stop();
  PR_DEBUG ("light_shade_stop");
}

/*
渐变处理线程
*/
STATIC VOID light_thread_shade (PVOID pArg)
{
  signed int delata_red   = 0;
  signed int delata_green = 0;
  signed int delata_blue  = 0;
  signed int delata_white = 0;
  signed int delata_warm  = 0;
  USHORT max_value;
  STATIC FLOAT_T r_scale;
  STATIC FLOAT_T g_scale;
  STATIC FLOAT_T b_scale;
  STATIC FLOAT_T w_scale;
  STATIC FLOAT_T ww_scale;
  UINT_T RED_GRA_STEP   = 1;
  UINT_T GREEN_GRA_STEP = 1;
  UINT_T BLUE_GRA_STEP  = 1;
  UINT_T WHITE_GRA_STEP = 1;
  UINT_T WARM_GRA_STEP  = 1;

  while (1)
  {
    WaitSemaphore (sys_handle.sem_shade);
    MutexLock (sys_handle.mutex);

    if (memcmp (&light_handle.fin, &light_handle.curr, SIZEOF (BRIGHT_DATA_S) ) != 0)
    {
      delata_red   = (light_handle.fin.red - light_handle.curr.red);
      delata_green = (light_handle.fin.green - light_handle.curr.green);
      delata_blue  = (light_handle.fin.blue - light_handle.curr.blue);
      delata_white = (light_handle.fin.white - light_handle.curr.white);
      delata_warm  = (light_handle.fin.warm - light_handle.curr.warm);
      //1、计算步进值（最大差值）
      max_value = light_shade_max_value_get (&light_handle.fin, &light_handle.curr);

      if (light_handle.shade_new == TRUE)
      {
        r_scale  = __abs (delata_red)   / 1.0 / max_value;
        g_scale  = __abs (delata_green) / 1.0 / max_value;
        b_scale  = __abs (delata_blue)  / 1.0 / max_value;
        w_scale  = __abs (delata_white) / 1.0 / max_value;
        ww_scale = __abs (delata_warm)  / 1.0 / max_value;
        light_handle.shade_new = FALSE;
      }//按差值对应MAX计算比例

      //2、各路按比例得到实际差值

      if (max_value == __abs (delata_red) )
      {
        RED_GRA_STEP = 1;
      }
      else
      {
        RED_GRA_STEP =  __abs (delata_red) - max_value*r_scale;
      }

      if (max_value == __abs (delata_green) )
      {
        GREEN_GRA_STEP = 1;
      }
      else
      {
        GREEN_GRA_STEP =  __abs (delata_green) - max_value*g_scale;
      }

      if (max_value == __abs (delata_blue) )
      {
        BLUE_GRA_STEP = 1;
      }
      else
      {
        BLUE_GRA_STEP =  __abs (delata_blue) - max_value*b_scale;
      }

      if (max_value == __abs (delata_white) )
      {
        WHITE_GRA_STEP = 1;
      }
      else
      {
        WHITE_GRA_STEP =  __abs (delata_white) - max_value*w_scale;
      }

      if (max_value == __abs (delata_warm) )
      {
        WARM_GRA_STEP = 1;
      }
      else
      {
        WARM_GRA_STEP =  __abs (delata_warm) - max_value*ww_scale;
      }

      //3、按计算出的差值调整各路light_bright数值
      if (delata_red != 0)
      {
        if (__abs (delata_red) < RED_GRA_STEP)
        {
          light_handle.curr.red += delata_red;
        }
        else
        {
          if (delata_red < 0)
          {
            light_handle.curr.red -= RED_GRA_STEP;
          }
          else
          {
            light_handle.curr.red += RED_GRA_STEP;
          }
        }
      }

      if (delata_green != 0)
      {
        if (__abs (delata_green) < GREEN_GRA_STEP)
        {
          light_handle.curr.green += delata_green;
        }
        else
        {
          if (delata_green < 0)
          {
            light_handle.curr.green -= GREEN_GRA_STEP;
          }
          else
          {
            light_handle.curr.green += GREEN_GRA_STEP;
          }
        }
      }

      if (delata_blue != 0)
      {
        if (__abs (delata_blue) < BLUE_GRA_STEP)
        {
          light_handle.curr.blue += delata_blue;
        }
        else
        {
          if (delata_blue < 0)
          {
            light_handle.curr.blue -= BLUE_GRA_STEP;
          }
          else
          {
            light_handle.curr.blue += BLUE_GRA_STEP;
          }
        }
      }

      if (delata_white != 0)
      {
        if (__abs (delata_white) < WHITE_GRA_STEP)
        {
          light_handle.curr.white += delata_white;
        }
        else
        {
          if (delata_white < 0)
          {
            light_handle.curr.white -= WHITE_GRA_STEP;
          }
          else
          {
            light_handle.curr.white += WHITE_GRA_STEP;
          }
        }
      }

      if (delata_warm != 0)
      {
        if (__abs (delata_warm) < WARM_GRA_STEP)
        {
          light_handle.curr.warm += delata_warm;
        }
        else
        {
          if (delata_warm < 0)
          {
            light_handle.curr.warm -= WARM_GRA_STEP;
          }
          else
          {
            light_handle.curr.warm += WARM_GRA_STEP;
          }
        }
      }

      //4、输出
      //PR_DEBUG("light send data %d %d %d %d %d", light_handle.curr.red, light_handle.curr.green, light_handle.curr.blue, light_handle.curr.white, light_handle.curr.warm);
      light_send_data (light_handle.curr.red, light_handle.curr.green, light_handle.curr.blue, light_handle.curr.white, light_handle.curr.warm);
    }
    else
    {
      //结束变化,显示一次目标值
      light_shade_stop();
      PR_DEBUG ("light send data %d %d %d %d %d", light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue,light_handle.fin.white, light_handle.fin.warm);

      if (dp_data.power == FALSE)  //CCT进入低功耗前需要将全部端口关闭
      {
        light_send_data (0, 0, 0, 0, 0);
      }
      else
      {
        light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,light_handle.fin.warm);
      }
    }
    MutexUnLock (sys_handle.mutex);
  }
}

/* 情景数据处理函数 */
STATIC VOID light_scene_data_get (UCHAR* buf, SCENE_HEAD_S* head)
{
  head->type = __str2byte (__asc2hex (buf[4]), __asc2hex (buf[5]) );

  if (head->type == SCENE_TYPE_SHADOW)
  {
    head->times = 110 - __str2byte (__asc2hex (buf[0]), __asc2hex (buf[1]) );
    head->speed = 105 - __str2byte (__asc2hex (buf[2]), __asc2hex (buf[3]) ); //新版本速度每个单元独立
  }
  else
  {
    head->times = 105 - __str2byte (__asc2hex (buf[0]), __asc2hex (buf[1]) );
    head->speed = 105 - __str2byte (__asc2hex (buf[2]), __asc2hex (buf[3]) ); //新版本速度每个单元独立
  }

  light_cw_value_get (buf + 18, &light_handle.fin);
  light_rgb_value_get (buf + 6, &light_handle.fin);
}

/* 开启情景功能 */
VOID light_scene_start (VOID)
{
  SCENE_HEAD_S scene;
  unsigned char scene_num;
  
  MutexLock (sys_handle.mutex);
  light_handle.scene_cnt = 0;
  light_handle.scene_new = FALSE;
  memset (&scene, 0, SIZEOF (SCENE_HEAD_S) );
  light_scene_data_get (&dp_data.scene[0 * SCENE_UNIT_LEN + 2], &scene);
  light_handle.scene_type = scene.type;
  
  scene_num = (key_trig_length-1)/26;
  scene_num = strlen (dp_data.scene + 2) / SCENE_UNIT_LEN;    
  light_IrCrtl_IR_Nums = scene_num;
  light_handle.scene_typeno = __str2byte (__asc2hex(dp_data.scene[0]), __asc2hex(dp_data.scene[1]));
  light_handle.flashflag = 0;
  //PR_DEBUG("------------- %d %d %d %d",light_IrCrtl_IR_Nums,light_handle.scene_typeno,scene.type,scene.times);
  /**场景模式**/
  if(light_handle.scene_typeno == SCENE_TYPE_FLASH)
  {
     light_handle.scene_enable = TRUE;
     light_handle.scene_new = 0;
     scene.times = scene.times/2;
     light_shade_stop();
     MutexUnLock (sys_handle.mutex);
  }
  else
  {
    switch (scene.type)
    {
      case SCENE_TYPE_SHADOW:
        //呼吸情景会直接进入第一个单元
        light_handle.scene_cnt ++;
        memcpy (&light_handle.curr, &light_handle.fin, SIZEOF (BRIGHT_DATA_S));
        light_handle.scene_enable = TRUE;
        light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue,light_handle.fin.white, light_handle.fin.warm);
        MutexUnLock (sys_handle.mutex);
        break;

      case SCENE_TYPE_JUMP:
        //直接显示
        light_shade_stop();
        light_handle.scene_enable = TRUE;
        MutexUnLock (sys_handle.mutex);
        break;

      case SCENE_TYPE_STATIC:
        MutexUnLock (sys_handle.mutex);
        //静态显示-----不进入线程处理
        light_scene_stop();
        //开启呼吸
        light_shade_start (normal_delay_time);
        break;

      default:
        MutexUnLock (sys_handle.mutex);
        break;
    }
  }
}

STATIC VOID light_scene_stop (VOID)
{
  MutexLock (sys_handle.mutex);
  light_handle.scene_enable = FALSE;
  MutexUnLock (sys_handle.mutex);
}

//情景线程只处理跳变和渐变模式
STATIC VOID light_thread_scene (PVOID pArg)
{
  SCENE_HEAD_S scene;
  BYTE_T scene_total;
  STATIC UINT_T Last_tick;
  Last_tick = GetSystemTickCount() * GetTickRateMs();

  while (1)
  {
    if ( (dp_data.power == FALSE) || (light_handle.scene_enable == FALSE) )
    {
      #if USER_DEFINE_LOWER_POWER

      if (lowpower_flag == 0)
      {
        #endif
          SystemSleep (SCENE_DELAY_TIME);
        #if USER_DEFINE_LOWER_POWER
      }
      else
      {
        SystemSleep (LOWPOWER_MODE_DELAY_TIME);
      }

      #endif
      
      continue;
    }
    
    if (light_handle.scene_new == FALSE)
    {
      MutexLock (sys_handle.mutex);
      light_handle.scene_new = TRUE;
      //scene_total = strlen (dp_data.scene + 2) /SCENE_UNIT_LEN;     //通过总数据长度计算情景个数
      scene_total = strlen (dp_data.scene + 2) / SCENE_UNIT_LEN;
      //PR_DEBUG("Num:%d", scene_total);
      
      if(scene_total == 1)
      {
          if(light_handle.scene_cnt == scene_total)
          {
            light_handle.scene_cnt = 0;
            memset (&scene, 0, SIZEOF (SCENE_HEAD_S) );
            light_scene_data_get(&dp_data.scene[light_handle.scene_cnt * SCENE_UNIT_LEN + 2], &scene);
            //目标值为全黑
            memset (&light_handle.fin, 0, sizeof (BRIGHT_DATA_S));

            if (_cw_type == LIGHT_CW_CCT)
            {
              light_handle.fin.warm = light_handle.curr.warm;
            }
          }
          else
          {
            memset(&scene, 0, SIZEOF (SCENE_HEAD_S));
            light_scene_data_get(&dp_data.scene[light_handle.scene_cnt * SCENE_UNIT_LEN + 2], &scene);
            light_handle.scene_cnt ++;
          }
      }
      else
      {
          memset (&scene, 0, SIZEOF (SCENE_HEAD_S) );
          light_scene_data_get(&dp_data.scene[light_handle.scene_cnt * SCENE_UNIT_LEN + 2], &scene);
          
          if(light_handle.scene_typeno == SCENE_TYPE_FLASH)
          {
            if(light_handle.flashflag)
               light_handle.scene_cnt ++;
          }
          else
          {
            light_handle.scene_cnt ++;
          }
          
          if (light_handle.scene_cnt >= scene_total)
          {
            light_handle.scene_cnt = 0;
          }
      }

        light_handle.scene_type = scene.type;
        if(light_handle.scene_typeno == SCENE_TYPE_FLASH)
        {
             /*******闪烁效果******/
             if(light_handle.flashflag==0)
             {
               light_handle.flashflag = 1;
               Last_tick = GetSystemTickCount() * GetTickRateMs();
               light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,light_handle.fin.warm);
             }
             else
             {
               light_handle.flashflag = 0;
               light_handle.fin.red = 0;
               light_handle.fin.green = 0;
               light_handle.fin.blue = 0;
               light_handle.fin.warm = 0;
               light_handle.fin.white = 0;
               light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,light_handle.fin.warm);
             }
             scene.times = scene.times/4;
        }
        else
        {
            if (scene.type == SCENE_TYPE_JUMP)
            {
              light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,light_handle.fin.warm);
            }
            else if (scene.type == SCENE_TYPE_SHADOW)
            {
              #if NEW_40IRKYE
              if(light_IrCrtl_ManulFlag)
              {
                scene.speed = light_IrCrtl_ManulTimes;
              }
              #endif
              light_shade_start(light_shade_speed_get(scene.speed * SCENE_DELAY_TIME));
            }
        }
        MutexUnLock (sys_handle.mutex);
    }

    #if NEW_40IRKYE
    if(light_IrCrtl_ManulFlag)
    {
      if(light_handle.scene_typeno == SCENE_TYPE_FLASH)
      scene.times = light_IrCrtl_ManulTimes/4;  
      else
      scene.times = light_IrCrtl_ManulTimes;
    }
    #endif
    
    //PR_DEBUG("++++++++++++ %d %d %d", scene_total,scene.times,light_IrCrtl_ManulFlag);
    if((GetSystemTickCount() * GetTickRateMs() - Last_tick) / 100 > scene.times)
    {
        light_handle.scene_new = FALSE;
        #if NEW_40IRKYE
        IR_AutomMode();
        #endif
        Last_tick = GetSystemTickCount() * GetTickRateMs();
    }
    
    SystemSleep (SCENE_DELAY_TIME);
  }
}

/* 开关 */
STATIC __light_power_ctl (BOOL ctl)
{
  MutexLock (sys_handle.mutex);

  if (ctl == TRUE)
  {
    dp_data.power = TRUE;
  }
  else
  {
    dp_data.power = FALSE;
  }

  MutexUnLock (sys_handle.mutex);
}
#if USER_DEFINE_LOWER_POWER
STATIC VOID dev_lowpower_mode_ctrl (BOOL_T stat)
{
  if (stat == TRUE)
  {
    if (lowpower_flag == 1)
    {
      light_lib_lowpower_disable();
      lowpower_flag = 0;
    }
  }
  else
  {
    sys_start_timer (sys_handle.lowpower, LIGHT_LOW_POWER_ENTER_IMEW, TIMER_ONCE);
  }
}
#endif

/* 冷光开始设置 */
STATIC VOID light_switch_set (BOOL stat)
{
#if USER_DEFINE_LOWER_POWER
  dev_lowpower_mode_ctrl (stat);
#endif

  if (stat == FALSE)
  {
    __light_power_ctl (FALSE);
    light_scene_stop();
    light_shade_stop();
    memset (&light_handle.fin, 0, sizeof (BRIGHT_DATA_S) );

    if (_cw_type == LIGHT_CW_CCT)
    {
      light_handle.fin.warm = light_handle.curr.warm; //CCT模式保持暖光脚不变
    }

    if (_def_cfg.power_onoff_type == POWER_ONOFF_BY_SHADE)
    {
      switch (dp_data.mode)
      {
        case WHITE_MODE:
        case COLOR_MODE:
          light_shade_start (normal_delay_time);
          break;

        case SCENE_MODE:
          if (light_handle.scene_type == SCENE_TYPE_JUMP)
          {
            light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,
                             light_handle.fin.warm);
          }
          else
          {
            light_shade_start (normal_delay_time);
          }

          break;

        case MUSIC_MODE:
          light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,
                           light_handle.fin.warm);
          break;

        default:
          break;
      }
    }
    else if (_def_cfg.power_onoff_type == POWER_ONOFF_BY_DIRECT)
    {
      if (dp_data.mode == SCENE_MODE && light_handle.scene_type == SCENE_TYPE_SHADOW)
      {
        // tuya_light_pwm_send_init();
        light_shade_start (normal_delay_time);
        // light_send_data(light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white, light_handle.fin.warm);
      }
      else
      {
        light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,
                         light_handle.fin.warm);
      }
    }
  }
  else
  {
    __light_power_ctl (TRUE);

    if (_def_cfg.power_onoff_type == POWER_ONOFF_BY_SHADE)
    {
      light_bright_start();
    }
    else if (_def_cfg.power_onoff_type == POWER_ONOFF_BY_DIRECT)
    {
      switch (dp_data.mode)
      {
        case WHITE_MODE:
        case COLOR_MODE:
          light_bright_data_get();
          light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,
                           light_handle.fin.warm);
          break;

        case SCENE_MODE:
          light_scene_start();
          break;

        default:
          break;
      }
    }
  }

  if (IsThisSysTimerRun (sys_handle.timer_countdown) == TRUE) //如果倒计时在跑，操控开关时会关闭
  {
    sys_stop_timer (sys_handle.timer_countdown);
    dp_data.countdown = 0;
  }
}

STATIC void __add_slash_if_needed (UCHAR* tmp_buf, UCHAR* str, UCHAR len)
{
  int i = 0;
  int j = 0;

  if (_def_cfg.color_type != LIGHT_COLOR_RGBC || len != 4)
  {
    memcpy (tmp_buf, str, len);
    tmp_buf[len] = '\0';
    return;
  }

  for (i = 0; i < len; i++)
  {
    if (str[i] == 'C')
    {
      tmp_buf[j++] = str[i];
      tmp_buf[j++] = '\/';//add /.
      tmp_buf[j++] = 'W';//add w.
      continue;
    }

    tmp_buf[j++] = str[i];
  }

  tmp_buf[6] = '\0';
}

/* dp数据上报同步功能函数 */
VOID light_dp_upload (VOID)
{
  OPERATE_RET op_ret;
  cJSON* root = NULL;
  CHAR_T* out = NULL;
  DEV_CNTL_N_S* dev_cntl = tuya_light_get_dev_cntl();
  UCHAR_T cmpflag;
  
  if (dev_cntl == NULL)
  {
    return;
  }

  DP_CNTL_S* dp_cntl =  NULL;
  dp_cntl = &dev_cntl->dp[1];
  root = cJSON_CreateObject();

  if (NULL == root)
  {
    PR_ERR ("cJSON_CreateObject error...");
    return;
  }
   
  //1-5路固定DP  开关 + 场景 + 模式
  CHAR_T tmp[4];
  dpid2str (tmp, DPID_SWITCH);
  cJSON_AddBoolToObject (root, tmp, dp_data.power);
  dpid2str (tmp, DPID_MODE);
  cJSON_AddStringToObject (root, tmp, tuya_light_ty_get_enum_str (dp_cntl, (UCHAR) dp_data.mode) );

  if (_def_cfg.color_type != LIGHT_COLOR_RGB)
  {
    dpid2str (tmp, DPID_BRIGHT);
    cJSON_AddNumberToObject (root, tmp, dp_data.bright);
  }

  if (_def_cfg.color_type == LIGHT_COLOR_RGBCW || _def_cfg.color_type == LIGHT_COLOR_CW)
  {
    dpid2str (tmp, DPID_TEMPR);
    cJSON_AddNumberToObject (root, tmp, dp_data.col_temp);
  }

  if (_def_cfg.color_type == LIGHT_COLOR_RGBCW || \
      _def_cfg.color_type == LIGHT_COLOR_RGBC || \
      _def_cfg.color_type == LIGHT_COLOR_RGB)
  {
    dpid2str (tmp, DPID_COLOR);
    cJSON_AddStringToObject (root, tmp, dp_data.color);
  }

  /*******************默认上报数据处理************/
  PR_DEBUG("------------------------------------------------------------- %s", dp_data.scene);
  if(dp_data.scene[1]==0x30)
  {
    if(strcmp(dp_data.scene,KEY_SCENE0_DEF_DATA)==0)
    {
        cmpflag = 0;
    }
    else
    {
        if(staticFlag)
        {
          if(strcmp(dp_data.scene,KEY_SCENE0_DEFDATA0)==0)
          {
             cmpflag = 0; 
          }
          else
          {
             if(strcmp(dp_data.scene,KEY_SCENE0_DEFDATA1)==0)
             {
                cmpflag = 0; 
             }
             else
             {
                cmpflag = 1;
             }
          }
        }
        else
        {
          staticFlag = 1;
          cmpflag = 0;
        }
    }
    /********* 初始状态发生改变 ********/
    if(cmpflag==0)
    {
      memcpy(dp_data.scene, KEY_SCENE0_DEF_DATA, strlen (KEY_SCENE0_DEF_DATA) + 1);
      memcpy(key_scene_data[0], KEY_SCENE0_DEF_DATA, strlen (KEY_SCENE0_DEF_DATA) + 1);
      key_trig_length = strlen (KEY_SCENE0_DEF_DATA);
    }
  }
  else if(dp_data.scene[1]==0x31)
  {
    if(strcmp(dp_data.scene,KEY_SCENE1_DEF_DATA)==0)
    {
        cmpflag = 0;
    }
    else
    {
        if(breathflag)
        {
          if(strcmp(dp_data.scene,KEY_SCENE1_DEFDATA0)==0)
          {
             cmpflag = 0; 
          }
          else
          {
             if(strcmp(dp_data.scene,KEY_SCENE1_DEFDATA1)==0)
             {
                 cmpflag = 0; 
             }
             else
             {
               cmpflag = 1;
             }
          }
        }
        else
        {
          breathflag = 1;
          cmpflag = 0;
        }
    }
    /********* 初始状态发生改变 ********/
    if(cmpflag==0)
    {
      memcpy(dp_data.scene, KEY_SCENE1_DEF_DATA, strlen (KEY_SCENE1_DEF_DATA) + 1);
      memcpy(key_scene_data[1], KEY_SCENE1_DEF_DATA, strlen (KEY_SCENE1_DEF_DATA) + 1);
      key_trig_length = strlen (KEY_SCENE1_DEF_DATA);
    }
  }
  else if(dp_data.scene[1]==0x32)
  {
    if(strcmp(dp_data.scene,KEY_SCENE2_DEF_DATA)==0)
    {
        cmpflag = 0;
    }
    else
    {
        if(raindropflag)
        {
          if(strcmp(dp_data.scene,KEY_SCENE2_DEFDATA0)==0)
          {
             cmpflag = 0; 
          }
          else
          {
             if(strcmp(dp_data.scene,KEY_SCENE2_DEFDATA1)==0)
             {
               cmpflag = 0; 
             }
             else
             {
                cmpflag = 1;
             }
          }
        }
        else
        {
          raindropflag = 1;
          cmpflag = 0;
        }
    }
    /********* 初始状态发生改变 ********/
    if(cmpflag==0)
    {
      memcpy(dp_data.scene, KEY_SCENE2_DEF_DATA, strlen (KEY_SCENE2_DEF_DATA) + 1);
      memcpy(key_scene_data[2], KEY_SCENE2_DEF_DATA, strlen (KEY_SCENE2_DEF_DATA) + 1);
      key_trig_length = strlen (KEY_SCENE2_DEF_DATA);
    }
  }
  else if(dp_data.scene[1]==0x33)
  {
    if(strcmp(dp_data.scene,KEY_SCENE3_DEF_DATA)==0)
    {
        cmpflag = 0;
    }
    else
    {
        if(colorfulflag)
        {
          if(strcmp(dp_data.scene,KEY_SCENE3_DEFDATA0)==0)
          {
             cmpflag = 0; 
          }
          else
          {
             if(strcmp(dp_data.scene,KEY_SCENE3_DEFDATA1)==0)
             {
                cmpflag = 0; 
             }
             else
             {
                cmpflag = 1;
             }
          }
        }
        else
        {
          colorfulflag = 1;
          cmpflag = 0;
        }
    }
    /********* 初始状态发生改变 ********/
    if(cmpflag==0)
    {
      memcpy(dp_data.scene, KEY_SCENE3_DEF_DATA, strlen (KEY_SCENE3_DEF_DATA) + 1);
      memcpy(key_scene_data[3], KEY_SCENE3_DEF_DATA, strlen (KEY_SCENE3_DEF_DATA) + 1);
      key_trig_length = strlen (KEY_SCENE3_DEF_DATA);
    }
  }
  else if(dp_data.scene[1]==0x34)
  {
    if(strcmp(dp_data.scene,KEY_SCENE4_DEF_DATA)==0)
    {
        cmpflag = 0;
    }
    else
    {
        if(marqueeflag)
        {
          if(strcmp(dp_data.scene,KEY_SCENE4_DEFDATA)==0)
          {
             cmpflag = 0; 
          }
          else
          {
             cmpflag = 1;
          }
        }
        else
        {
          marqueeflag = 1;
          cmpflag = 0;
        }
    }
    /********* 初始状态发生改变 ********/
    if(cmpflag==0)
    {
      memcpy(dp_data.scene, KEY_SCENE4_DEF_DATA, strlen (KEY_SCENE4_DEF_DATA) + 1);
      memcpy(key_scene_data[4], KEY_SCENE4_DEF_DATA, strlen (KEY_SCENE4_DEF_DATA) + 1);
      key_trig_length = strlen (KEY_SCENE4_DEF_DATA);
    }
  }
  else if(dp_data.scene[1]==0x35)
  {
    if(strcmp(dp_data.scene,KEY_SCENE5_DEF_DATA)==0)
    {
        cmpflag = 0;
    }
    else
    {
        if(blinkingflag)
        {
          if(strcmp(dp_data.scene,KEY_SCENE5_DEFDATA)==0)
          {
             cmpflag = 0; 
          }
          else
          {
             cmpflag = 1;
          }
        }
        else
        {
          blinkingflag = 1;
          cmpflag = 0;
        }
    }
    /********* 初始状态发生改变 ********/
    if(cmpflag==0)
    {
      memcpy(dp_data.scene, KEY_SCENE5_DEF_DATA, strlen (KEY_SCENE5_DEF_DATA) + 1);
      memcpy(key_scene_data[5], KEY_SCENE5_DEF_DATA, strlen (KEY_SCENE5_DEF_DATA) + 1);
      key_trig_length = strlen (KEY_SCENE5_DEF_DATA);
    }
  }
  else if(dp_data.scene[1]==0x36)
  {
    if(strcmp(dp_data.scene,KEY_SCENE6_DEF_DATA)==0)
    {
        cmpflag = 0;
    }
    else
    {
        if(snowcolorflag)
        {
          if(strcmp(dp_data.scene,KEY_SCENE6_DEFDATA)==0)
          {
             cmpflag = 0; 
          }
          else
          {
             cmpflag = 1;
          }
        }
        else
        {
          snowcolorflag = 1;
          cmpflag = 0;
        }
    }
    /********* 初始状态发生改变 ********/
    if(cmpflag==0)
    {
      memcpy(dp_data.scene, KEY_SCENE6_DEF_DATA, strlen (KEY_SCENE6_DEF_DATA) + 1);
      memcpy(key_scene_data[6], KEY_SCENE6_DEF_DATA, strlen (KEY_SCENE6_DEF_DATA) + 1);
      key_trig_length = strlen (KEY_SCENE6_DEF_DATA);
    }
  }
  else if(dp_data.scene[1]==0x37)
  {
    if(strcmp(dp_data.scene,KEY_SCENE7_DEF_DATA)==0)
    {
        cmpflag = 0;
    }
    else
    {
        if(streamerflag)
        {
          if(strcmp(dp_data.scene,KEY_SCENE7_DEFDATA)==0)
          {
             cmpflag = 0; 
          }
          else
          {
             cmpflag = 1;
          }
        }
        else
        {
          streamerflag = 1;
          cmpflag = 0;
        }
    }
    /********* 初始状态发生改变 ********/
    if(cmpflag==0)
    {
      memcpy(dp_data.scene, KEY_SCENE7_DEF_DATA, strlen (KEY_SCENE7_DEF_DATA) + 1);
      memcpy(key_scene_data[7], KEY_SCENE7_DEF_DATA, strlen (KEY_SCENE7_DEF_DATA) + 1);
      key_trig_length = strlen (KEY_SCENE7_DEF_DATA);
    }
  }
  
  dpid2str (tmp, DPID_SCENE);
  cJSON_AddStringToObject (root, tmp, dp_data.scene);
  dpid2str (tmp, DPID_COUNTDOWN);
  cJSON_AddNumberToObject (root, tmp, dp_data.countdown);
#if USER_DEFINE_LIGHT_STRIP_LED_CONFIG

  if (_def_cfg.color_type >= LIGHT_COLOR_RGB)
  {
    dpid2str (tmp, USER_DEFINE_RGBCW_ORDER_DPID);
    UCHAR tmp_buf[7];
    __add_slash_if_needed (tmp_buf, dp_data.rgbcw_order, strlen (dp_data.rgbcw_order) );
    PR_DEBUG ("upload rgbcw_order: %s, org:%s\n", tmp_buf, dp_data.rgbcw_order);
    cJSON_AddStringToObject (root, tmp, tmp_buf);
  }

#endif
  out = cJSON_PrintUnformatted (root);
  cJSON_Delete (root);

  if (NULL == out)
  {
    PR_ERR ("cJSON_PrintUnformatted err...");
    return;
  }

  PR_DEBUG ("upload: %s", out);
  op_ret = tuya_light_dp_report (out);
  Free (out);
  out = NULL;

  if (OPRT_OK != op_ret)
  {
    PR_ERR ("sf_obj_dp_report err:%d", op_ret);
    //PR_DEBUG_RAW("%s\r\n",buf);
    return;
  }

  return;
}

/* 控制dp处理函数 */
STATIC VOID light_ctrl_dp_proc (UCHAR* buf)
{
  CRTL_TYPE_E mode;
  mode = *buf - '0';
  // PR_DEBUG("mode = %d",mode);

  if (_def_cfg.whihe_color_mutex == FALSE && (dp_data.mode == WHITE_MODE || dp_data.mode == COLOR_MODE) )
  {
    if (dp_data.mode == WHITE_MODE)
    {
      light_cw_value_get (buf + 13, &light_handle.fin);
    }
    else if (dp_data.mode == COLOR_MODE)
    {
      light_rgb_value_get (buf + 1, &light_handle.fin);
    }
  }
  else
  {
    light_cw_value_get (buf + 13, &light_handle.fin);
    light_rgb_value_get (buf + 1, &light_handle.fin);
  }

  if (mode == CTRL_TYPE_JUMP)
  {
    memcpy (&light_handle.curr, &light_handle.fin, SIZEOF (BRIGHT_DATA_S) );
    light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,
                     light_handle.fin.warm);
  }
  else if (mode == CTRL_TYPE_SHADE)
  {
    if (dp_data.mode == SCENE_MODE)
    {
      light_scene_stop();
    }

    light_shade_start (normal_delay_time);
  }
}

STATIC void __str_bubble_sort (UCHAR* str, UCHAR len)
{
  int i, j;
  UCHAR t;

  for (i = 0; i < len - 1; i++)
  {
    for (j = 0; j < len - 1 - i; j++)
    {
      if (str[j] <= str[j + 1])
      {
        continue;
      }

      t = str[j + 1];
      str[j + 1] = str[j];
      str[j] = t;
    }
  }
}

STATIC void __str2lowwer (UCHAR* str, UCHAR len)
{
  int i = 0;

  for (i = 0; i < len; i++)
  {
    if (str[i] < 'b')
    {
      str[i] += 32;
    }
  }
}

#if USER_DEFINE_LIGHT_STRIP_LED_CONFIG
STATIC VOID __deal_rgbcw_order (UCHAR* rgbcw_order, UCHAR* str, UCHAR len)
{
  UCHAR tmp_str[6];
  PR_DEBUG ("new rgbcw_order:%s\n", str);
  memcpy (tmp_str, str, len);
  tmp_str[len] = '\0';
  __str2lowwer (tmp_str, len);
  PR_DEBUG ("lowwer rgbcw_order:%s\n", tmp_str);
  __str_bubble_sort (tmp_str, len);
  PR_DEBUG ("sort rgbcw_order:%s\n", tmp_str);

  if ( (strncmp (tmp_str, "bcgrw", len) == 0) || (strcmp (tmp_str, "bgr") == 0) )
  {
    PR_DEBUG ("valid rgbcw_order:%s\n", str);
    memcpy (rgbcw_order, str, len);
    rgbcw_order[len] = '\0';
  }

  tuya_light_set_rgbcw_order (rgbcw_order);
}

STATIC VOID __valid_rgbcw_order_set_deafult_if_needed (UCHAR* rgbcw_order, UCHAR* buf)
{
  if (_def_cfg.color_type < LIGHT_COLOR_RGB)
  {
    return;
  }

  switch (_def_cfg.color_type)
  {
    case LIGHT_COLOR_RGB:
      memcpy (rgbcw_order, USER_DEFINE_STRIP_RGB_ORDER, 4);

      if (strlen (buf) != 3 || buf == NULL)
      {
        break;
      }

      __deal_rgbcw_order (rgbcw_order, buf, 3);
      break;

    case LIGHT_COLOR_RGBC:
      memcpy (rgbcw_order, USER_DEFINE_STRIP_RGBC_ORDER, 5);

      if (strlen (buf) != 4 || buf == NULL)
      {
        break;
      }

      __deal_rgbcw_order (rgbcw_order, buf, 4);
      break;

    case LIGHT_COLOR_RGBCW:
      memcpy (rgbcw_order, USER_DEFINE_STRIP_RGBCW_ORDER, 6);

      if (strlen (buf) != 5 || buf == NULL)
      {
        break;
      }

      __deal_rgbcw_order (rgbcw_order, buf, 5);
      break;

    default:
      break;
  }
}

VOID valid_rgbcw_order_set_deafult_if_needed (UCHAR* buf)
{
  return __valid_rgbcw_order_set_deafult_if_needed (dp_data.rgbcw_order, buf);
}

STATIC void __remov_slash (UCHAR* str, UCHAR len)
{
  int i = 0;
  int j = 0;
  UCHAR tmp_buf[5];
  PR_DEBUG ("new rgbcw_order:%s\n", str);

  for (i = 0; i < len; i++)
  {
    if (str[i] == '\/')
    {
      ++i; //skip /w.
      continue;
    }

    tmp_buf[j++] = str[i];
  }

  memcpy (str, tmp_buf, 4);
  str[4] = '\0';
  PR_DEBUG ("new rgbcw_order:%s\n", str);
}

STATIC VOID light_rgbcw_order_dp_proc (UCHAR* rgbcw_order, UCHAR* buf)
{
  PR_DEBUG ("new rgbcw_order:%s\n", buf);

  if (_def_cfg.color_type < LIGHT_COLOR_RGB)
  {
    return;
  }

  switch (_def_cfg.color_type)
  {
    case LIGHT_COLOR_RGB:
      if (strlen (buf) != 3)
      {
        break;
      }

      __deal_rgbcw_order (rgbcw_order, buf, 3);
      break;

    case LIGHT_COLOR_RGBC:
      if (strlen (buf) != 6) //RGBC/W
      {
        break;
      }

      //remove slash
      __remov_slash (buf, 6);
      __deal_rgbcw_order (rgbcw_order, buf, 4);
      break;

    case LIGHT_COLOR_RGBCW:
      if (strlen (buf) != 5)
      {
        break;
      }

      __deal_rgbcw_order (rgbcw_order, buf, 5);
      break;

    default:
      break;
  }
}
#endif

/*************************************************
flash存取处理函数
**************************************************/
/* 定时5秒自动保存 */
STATIC VOID data_autosave_timeout_cb (UINT timerID, PVOID pTimerArg)
{
  PR_DEBUG ("%s", __FUNCTION__);
  light_data_write_flash();
}
VOID light_dp_data_autoupload (IN CONST TIME_MS timeCycle)
{
  if (!IsThisSysTimerRun (sys_handle.timer_init_dpdata) )
  {
    sys_start_timer (sys_handle.timer_init_dpdata, timeCycle, TIMER_ONCE);
  }
}

VOID light_dp_data_autosave (VOID)
{
  if (!IsThisSysTimerRun (sys_handle.timer_data_autosave) )
  {
    sys_start_timer (sys_handle.timer_data_autosave, NORMAL_DP_SAVE_TIME, TIMER_ONCE);
  }
}
#define DP_DATA_FILE_NAME "dp_data"

STATIC OPERATE_RET light_dp_data_read (DP_DATA_S* data)
{
  OPERATE_RET op_ret;
  UCHAR* buf = NULL;
  INT_T file_size = ufgetsize (DP_DATA_FILE_NAME);

  if (0 == file_size)
  {
    PR_ERR ("ufgetsize == 0");
    goto ERR_EXT;
  }

  //op_ret = tuya_light_common_flash_read(LIGHT_DATA_KEY, &buf);
  uFILE* file = ufopen (DP_DATA_FILE_NAME, "r");

  if (NULL == file)
  {
    PR_ERR ("ufopen failed");
    goto ERR_EXT;
  }

  buf = Malloc (file_size);

  if (NULL == buf)
  {
    PR_ERR ("Malloc=%d failed", file_size);
    ufclose (file);
    goto ERR_EXT;
  }

  INT_T read_len =  ufread (file, buf, file_size);
  ufclose (file);
  file = NULL;

  if (0 == read_len)
  {
    PR_ERR ("ufread failed read_len==0");
    Free (buf);
    goto ERR_EXT;
  }

  PR_ERR ("ufread read_len %d, file_size %d", read_len, file_size);
  PR_DEBUG ("read flash:[%s]", buf);
  cJSON* root = NULL;
  root = cJSON_Parse (buf);
  Free (buf);
  buf = NULL;

  if (NULL == root)
  {
    goto ERR_EXT;
  }

  cJSON* json = NULL;
  memset (data, 0, SIZEOF (DP_DATA_S) );
  json = cJSON_GetObjectItem (root, "switch");

  if (NULL == json)
  {
    data->power = TRUE;
  }
  else
  {
    data->power = json->valueint;
  }

  json = cJSON_GetObjectItem (root, "mode");

  if (NULL == json)
  {
    if (_def_cfg.color_type == LIGHT_COLOR_RGB)
    {
      data->mode = COLOR_MODE;
    }
    else
    {
      data->mode = WHITE_MODE;
    }
  }
  else
  {
    data->mode = json->valueint;
  }

  json = cJSON_GetObjectItem (root, "bright");

  if (NULL == json)
  {
    data->bright = light_default_bright_get();
  }
  else
  {
    data->bright = json->valueint;
  }

  json = cJSON_GetObjectItem (root, "temper");

  if (NULL == json)
  {
    data->col_temp = light_default_temp_get();
  }
  else
  {
    data->col_temp = json->valueint;
  }

  json = cJSON_GetObjectItem (root, "color");

  if (NULL == json)
  {
    if (_def_cfg.whihe_color_mutex == TRUE)
      // memcpy(data->color, COLOR_DATA_DEFAULT, COLOR_DATA_LEN);
    {
      light_default_color_get (data->color);
    }
    else
    {
      memcpy (data->color, FC_COLOR_DATA_DEFAULT, COLOR_DATA_LEN);
    }
  }
  else
  {
    memcpy (data->color, json->valuestring, SIZEOF (data->color) );
  }

  json = cJSON_GetObjectItem (root, "scene");

  if (NULL == json)
  {
    switch (_def_cfg.color_type)
    {
      case LIGHT_COLOR_C:
        memcpy (data->scene, SCENE_DATA_DEFAULT_DIM, SCENE_DATA_LEN_MAX);
        break;

      case LIGHT_COLOR_CW:
        memcpy (data->scene, SCENE_DATA_DEFAULT_CW, SCENE_DATA_LEN_MAX);
        break;

      default:
        memcpy (data->scene, SCENE_DATA_DEFAULT_RGB, SCENE_DATA_LEN_MAX);
        break;
    }
  }
  else
  {
    memcpy (data->scene, json->valuestring, SIZEOF (data->scene) );
    PR_DEBUG ("dev_get scene data len:%d", strlen (json->valuestring) );
  }

#if USER_DEFINE_LIGHT_STRIP_LED_CONFIG
  UCHAR* rgbcw_order="\0";
  json = cJSON_GetObjectItem (root, "rgbcw_order");

  if (NULL != json)
  {
    rgbcw_order = json->valuestring;
    PR_DEBUG ("read rgbcw_order %s\n", rgbcw_order);
    __valid_rgbcw_order_set_deafult_if_needed (data->rgbcw_order, rgbcw_order);
  }

#endif
#ifdef KEY_CHANGE_SCENE_FUN
  memset (key_scene_data, 0, SIZEOF (char) * (SCENE_DATA_LEN_MAX + 1) * 8);
  json = cJSON_GetObjectItem (root, "scene0");

  if (NULL == json)
  {
    memcpy (key_scene_data[0], KEY_SCENE0_DEF_DATA, strlen (KEY_SCENE0_DEF_DATA) + 1);
  }
  else
  {
    memcpy (key_scene_data[0], json->valuestring, strlen (json->valuestring)+1);
  }

  json = cJSON_GetObjectItem (root, "scene1");

  if (NULL == json)
  {
    memcpy (key_scene_data[1], KEY_SCENE1_DEF_DATA, strlen (KEY_SCENE1_DEF_DATA) + 1);
  }
  else
  {
    memcpy (key_scene_data[1], json->valuestring, strlen (json->valuestring)+1);
  }

  json = cJSON_GetObjectItem (root, "scene2");

  if (NULL == json)
  {
    memcpy (key_scene_data[2], KEY_SCENE2_DEF_DATA, strlen (KEY_SCENE2_DEF_DATA) + 1);
  }
  else
  {
    memcpy (key_scene_data[2], json->valuestring, strlen (json->valuestring)+1);
  }

  json = cJSON_GetObjectItem (root, "scene3");

  if (NULL == json)
  {
    memcpy (key_scene_data[3], KEY_SCENE3_DEF_DATA, strlen (KEY_SCENE3_DEF_DATA) + 1);
  }
  else
  {
    memcpy (key_scene_data[3], json->valuestring, strlen (json->valuestring)+1);
  }

  json = cJSON_GetObjectItem (root, "scene4");

  if (NULL == json)
  {
    memcpy (key_scene_data[4], KEY_SCENE4_DEF_DATA, strlen (KEY_SCENE4_DEF_DATA) + 1);
  }
  else
  {
    memcpy (key_scene_data[4], json->valuestring, strlen (json->valuestring)+1);
  }

  json = cJSON_GetObjectItem (root, "scene5");

  if (NULL == json)
  {
    memcpy (key_scene_data[5], KEY_SCENE5_DEF_DATA, strlen (KEY_SCENE5_DEF_DATA) + 1);
  }
  else
  {
    memcpy (key_scene_data[5], json->valuestring, strlen (json->valuestring)+1);
  }

  json = cJSON_GetObjectItem (root, "scene6");

  if (NULL == json)
  {
    memcpy (key_scene_data[6], KEY_SCENE6_DEF_DATA, strlen (KEY_SCENE6_DEF_DATA) + 1);
  }
  else
  {
    memcpy (key_scene_data[6], json->valuestring, strlen (json->valuestring)+1);
  }

  json = cJSON_GetObjectItem (root, "scene7");

  if (NULL == json)
  {
    memcpy (key_scene_data[7], KEY_SCENE7_DEF_DATA, strlen (KEY_SCENE7_DEF_DATA) + 1);
  }
  else
  {
    memcpy (key_scene_data[7], json->valuestring, strlen (json->valuestring)+1);
  }

#endif
  cJSON_Delete (root);
  return OPRT_OK;
ERR_EXT:
#if USER_DEFINE_LIGHT_STRIP_LED_CONFIG
  __valid_rgbcw_order_set_deafult_if_needed (data->rgbcw_order, NULL);
#endif
  light_dp_data_default_set();
  return OPRT_COM_ERROR;
}

STATIC OPERATE_RET light_data_write_flash (VOID)
{
  OPERATE_RET op_ret;
  INT_T i = 0;
  CHAR_T* out = NULL;
  uFILE* file = ufopen (DP_DATA_FILE_NAME, "w+");

  if (NULL == file)
  {
    PR_ERR ("ufopen failed");
    return OPRT_OPEN_FILE_FAILED;
  }

  PR_DEBUG ("ufopen %s w+ success\n", DP_DATA_FILE_NAME);
  cJSON* root = NULL;
  root = cJSON_CreateObject();

  if (NULL == root)
  {
    PR_ERR ("cJSON_CreateObject error...");
    return OPRT_CR_CJSON_ERR;
  }

  //dp数据
  if (_light_memory == TRUE)  //开启记忆
  {
    cJSON_AddBoolToObject (root, "switch", dp_data.power);
    cJSON_AddNumberToObject (root, "mode", dp_data.mode);
    cJSON_AddNumberToObject (root, "bright", dp_data.bright);
    cJSON_AddNumberToObject (root, "temper", dp_data.col_temp);
    cJSON_AddStringToObject (root, "color", dp_data.color);
  }
  else   //无记忆下默认写入默认值
  {
    CHAR color_data[COLOR_DATA_LEN + 1];
    cJSON_AddBoolToObject (root, "switch", TRUE);

    if (_def_cfg.color_type == LIGHT_COLOR_RGB || (_def_cfg.def_light_color < INIT_COLOR_C) )
    {
      cJSON_AddNumberToObject (root, "mode", COLOR_MODE);
    }
    else
    {
      cJSON_AddNumberToObject (root, "mode", WHITE_MODE);
    }

    cJSON_AddNumberToObject (root, "bright", light_default_bright_get() );
    cJSON_AddNumberToObject (root, "temper", light_default_temp_get() );

    if (dp_data.mode == COLOR_MODE)
    {
      light_default_color_get (color_data);
      cJSON_AddStringToObject (root, "color", color_data);
    }
    else
    {
      cJSON_AddStringToObject (root, "color", COLOR_DATA_DEFAULT);
    }
  }

  //情景设定任何模式下保持记忆
  cJSON_AddStringToObject (root, "scene", dp_data.scene);
#if USER_DEFINE_LIGHT_STRIP_LED_CONFIG
  cJSON_AddStringToObject (root, "rgbcw_order", dp_data.rgbcw_order);
#endif
#ifdef KEY_CHANGE_SCENE_FUN
  cJSON_AddStringToObject (root, "scene0", key_scene_data[0]);
  cJSON_AddStringToObject (root, "scene1", key_scene_data[1]);
  cJSON_AddStringToObject (root, "scene2", key_scene_data[2]);
  cJSON_AddStringToObject (root, "scene3", key_scene_data[3]);
  cJSON_AddStringToObject (root, "scene4", key_scene_data[4]);
  cJSON_AddStringToObject (root, "scene5", key_scene_data[5]);
  cJSON_AddStringToObject (root, "scene6", key_scene_data[6]);
  cJSON_AddStringToObject (root, "scene7", key_scene_data[7]);
#endif
  // cJSON_AddBoolToObject(root, "memory", _light_memory);//dp控制需要保存，json配置每次上电会更新
  out = cJSON_PrintUnformatted (root);
  cJSON_Delete (root);

  if (NULL == out)
  {
    PR_ERR ("cJSON_PrintUnformatted error...");
    return OPRT_MALLOC_FAILED;
  }

  PR_DEBUG ("light_dp_data:%s", out);
  //op_ret = tuya_light_common_flash_write(LIGHT_DATA_KEY, out, strlen(out));
  INT_T len = strlen (out);
  INT_T write_len = ufwrite (file, out, len);
  ufclose (file);
  file = NULL;
  Free (out);
  out = NULL;

  if (len != write_len)
  {
    PR_ERR ("ufwrite fail write_len=%d", write_len);
    return OPRT_WR_FLASH_ERROR;
  }

  return OPRT_OK;
}

/*****************************************
dp处理函数
*****************************************/
STATIC USHORT_T light_default_bright_get (VOID)
{
  USHORT_T bright;
  bright = (BRIGHT_VAL_MAX - BRIGHT_VAL_MIN) * _def_cfg.def_bright_precent / 100 + BRIGHT_VAL_MIN;
  bright = bright / 10 * 10; //Rounding down
  return bright;
}

STATIC USHORT_T light_default_temp_get (VOID)
{
  USHORT_T temp;
  temp = BRIGHT_VAL_MAX * _def_cfg.def_temp_precent / 100;
  temp = temp / 10 * 10; //Rounding down
  return temp;
}

STATIC VOID light_default_color_get (UCHAR* data)
{
  USHORT h, s, v;

  switch (_def_cfg.def_light_color)
  {
    case INIT_COLOR_R:
      h = 0;
      break;

    case INIT_COLOR_G:
      h = 120;
      break;

    case INIT_COLOR_B:
      h = 240;
      break;

    default:
      h = 0; //默认红色
      break;
  }

  s = 1000;
  v = 1000 / 100 * _def_cfg.def_bright_precent;
  light_color_data_hex2asc (&data[0], h, 4);
  light_color_data_hex2asc (&data[4], s, 4);
  light_color_data_hex2asc (&data[8], v, 4);
  PR_DEBUG ("set the def color data is [%s]", dp_data.color);
  return ;
}

/* 设置默认dp数值 */
VOID light_dp_data_default_set (VOID)
{
#if USER_DEFINE_LIGHT_STRIP_LED_CONFIG
  UCHAR_T rgbcw_order[RGBCW_ORDER_DATA_LEN];
  memcpy (rgbcw_order, dp_data.rgbcw_order, RGBCW_ORDER_DATA_LEN);
#endif
  memset (&dp_data, 0, SIZEOF (DP_DATA_S) );
#if USER_DEFINE_LIGHT_STRIP_LED_CONFIG
  memcpy (dp_data.rgbcw_order, rgbcw_order, RGBCW_ORDER_DATA_LEN);
#endif
  dp_data.power = TRUE;

  if (_def_cfg.color_type == LIGHT_COLOR_RGB || (_def_cfg.def_light_color < INIT_COLOR_C) )
  {
    dp_data.mode = COLOR_MODE;
  }
  else
  {
    dp_data.mode = WHITE_MODE;
  }

  dp_data.bright = light_default_bright_get();
  dp_data.col_temp = light_default_temp_get();

  if (dp_data.mode == COLOR_MODE)
  {
    light_default_color_get (dp_data.color);
  }
  else
  {
    if (_def_cfg.whihe_color_mutex == FALSE)
    {
      memcpy (dp_data.color, FC_COLOR_DATA_DEFAULT, COLOR_DATA_LEN);
    }
    else
    {
      memcpy (dp_data.color, COLOR_DATA_DEFAULT, COLOR_DATA_LEN);
    }
  }

  if (_def_cfg.color_type == LIGHT_COLOR_C)
  {
    memcpy (dp_data.scene, SCENE_DATA_DEFAULT_DIM, SCENE_DATA_LEN_MAX);
  }
  else if (_def_cfg.color_type == LIGHT_COLOR_CW)
  {
    memcpy (dp_data.scene, SCENE_DATA_DEFAULT_CW, SCENE_DATA_LEN_MAX);
  }
  else if (_def_cfg.color_type == LIGHT_COLOR_RGB)
  {
    memcpy (dp_data.scene, SCENE_DATA_DEFAULT_RGB, SCENE_DATA_LEN_MAX);
  }
  else
  {
    memcpy (dp_data.scene, SCENE_DATA_DEFAULT_RGBC, SCENE_DATA_LEN_MAX);
  }

  //rgbcw order will not be reset.
#if 0//USER_DEFINE_LIGHT_STRIP_LED_CONFIG    
  UCHAR tmp_buf[6];
  memcpy (tmp_buf, dp_data.rgbcw_order, 6);
  __valid_rgbcw_order_set_deafult_if_needed (dp_data.rgbcw_order, tmp_buf);
#endif
  light_key_scene_data_default_set();
  light_data_write_flash();
}

/* cct模式色温脚保持 */
STATIC VOID light_cct_col_temper_keep (UCHAR data)
{
  STATIC BOOL key = FALSE; //默认开锁，不会造成影响
  STATIC USHORT temp_value;
  STATIC UCHAR last_data = 0;

  if (data == DPID_CONTROL)
  {
    if (dp_data.mode == SCENE_MODE)  //在情景模式中，所有灯光的预览效果编辑都会通过调控DP点下发
    {
      if (key == FALSE)
      {
        temp_value = light_handle.curr.warm; //在进入调节之前，记录下色温数值
        key = TRUE; //第一次记录完毕之后自锁，
      }
    }
  }
  else
  {
    if (key == TRUE)
    {
      key = FALSE;

      if (last_data == DPID_CONTROL && data == DPID_SCENE)  //dpid：28->25，为情景提交
      {
        ;
      }
      else
      {
        light_handle.fin.warm = light_handle.curr.warm =
                                        temp_value; //如果不是控制DP，判断上锁情况，如果上锁说明有色温保存，释放色温
      }
    }
  }

  last_data = data;
}

BOOL_T light_light_dp_proc (TY_OBJ_DP_S* root)
{
  UCHAR_T dpid, type;
  WORD_T len, rawlen;
  UCHAR_T mode;
  BOOL_T flag = FALSE;
  dpid = root->dpid;
  PR_DEBUG ("light_light_dp_proc dpid=%d", dpid);

  if (_cw_type == LIGHT_CW_CCT)
  {
    light_cct_col_temper_keep (dpid);
  }

  switch (dpid)
  {
    case DPID_SWITCH:
      if (dp_data.power != root->value.dp_bool)
      {
        flag = TRUE;
        light_switch_set (root->value.dp_bool);
      }

      break;

    case DPID_MODE:
      if (root->type != PROP_ENUM)
      {
        break;
      }

      mode = root->value.dp_enum;

      if (dp_data.mode != mode || mode == SCENE_MODE)
      {
        flag = TRUE;
        dp_data.mode = mode;

        if (dp_data.power == TRUE)
        {
          light_bright_start();
        }
      }

      break;

    case DPID_BRIGHT://亮度
      if (root->type != PROP_VALUE)
      {
        break;
      }

      if (dpid == DPID_BRIGHT)
      {
        if (root->value.dp_value < BRIGHT_VAL_MIN || root->value.dp_value > BRIGHT_VAL_MAX)
        {
          PR_ERR ("the data length is wrong: %d", root->value.dp_value);
          break;
        }
      }

      if (dp_data.bright != root->value.dp_value)
      {
        flag = TRUE;
        dp_data.bright = root->value.dp_value;

        if (dp_data.power == TRUE && dp_data.mode == WHITE_MODE)
        {
          light_bright_data_get();
          light_shade_start (normal_delay_time);
        }
      }

      break;

    case DPID_TEMPR://色温
      if (root->type != PROP_VALUE)
      {
        break;
      }

      if (dpid == DPID_TEMPR)
      {
        if (root->value.dp_value < 0 || root->value.dp_value > BRIGHT_VAL_MAX)
        {
          PR_ERR ("the data length is wrong: %d", root->value.dp_value);
          break;
        }
      }

      if (dp_data.col_temp != root->value.dp_value)
      {
        flag = TRUE;
        dp_data.col_temp = root->value.dp_value;

        if (dp_data.power == TRUE && dp_data.mode == WHITE_MODE)
        {
          light_bright_data_get();
          light_shade_start (normal_delay_time);
        }
      }

      break;

    case DPID_COLOR://彩色
      if (root->type != PROP_STR)
      {
        break;
      }

      len = strlen (root->value.dp_str);

      if (len != COLOR_DATA_LEN)
      {
        PR_ERR ("the data length is wrong: %d", len);
        break;
      }

      if (strcmp (dp_data.color, root->value.dp_str) != 0)
      {
        flag = TRUE;
        memset (dp_data.color, 0, SIZEOF (dp_data.color) );
        memcpy (dp_data.color, root->value.dp_str, len);

        if (dp_data.power == TRUE)
        {
          light_bright_data_get();
          light_shade_start (normal_delay_time);
        }
      }

      break;

    case DPID_SCENE://新情景
      if (root->type != PROP_STR)
      {
        break;
      }

      BOOL scene_f = FALSE;
      len = strlen (root->value.dp_str);

      if (len < SCENE_DATA_LEN_MIN || len > SCENE_DATA_LEN_MAX)
      {
        PR_ERR ("the data length is wrong: %d", len);
      }
      else
      {
        scene_f = TRUE;
      }

      if (scene_f == TRUE)
      {
        flag = TRUE;
        memset (dp_data.scene, 0, SIZEOF (dp_data.scene) );
        memcpy (dp_data.scene, root->value.dp_str, len);
        light_key_scene_data_save (dp_data.scene, len);

        if (dp_data.power == TRUE)
        {
          light_scene_start();
        }
      }

      break;

    case DPID_COUNTDOWN:
      if (root->type != PROP_VALUE)
      {
        break;
      }

      if (root->value.dp_value < 0 || root->value.dp_value > 86400)
      {
        PR_ERR ("the data size is wrong: %d", root->value.dp_value);
        break;
      }

      if (root->value.dp_value == 0)
      {
        /* 取消倒计时 */
        if (IsThisSysTimerRun (sys_handle.timer_countdown) )
        {
          sys_stop_timer (sys_handle.timer_countdown);
          dp_data.countdown = 0;
          flag = TRUE; /* 重新上报 */
        }
      }

      if (dp_data.countdown != root->value.dp_value)
      {
        flag = TRUE;
        dp_data.countdown = root->value.dp_value;
        sys_start_timer (sys_handle.timer_countdown, 60000, TIMER_CYCLE);
      }

      break;

    case DPID_MUSIC:                            //音乐
    case DPID_CONTROL:                          //实时控制数据

      //实时下发，不保存&&不回复消息
      if (dp_data.power == FALSE)
      {
        break;
      }

      if (root->type != PROP_STR)
      {
        break;
      }

      if (dp_data.power == TRUE)
      {
        light_shade_stop();
        light_ctrl_dp_proc (root->value.dp_str);
      }

      break;
#if USER_DEFINE_LIGHT_STRIP_LED_CONFIG

    case USER_DEFINE_RGBCW_ORDER_DPID:
      if (root->type != PROP_STR)
      {
        break;
      }

      light_rgbcw_order_dp_proc (dp_data.rgbcw_order, root->value.dp_str);
      flag = TRUE;
      break;
#endif

    default:
      break;
  }

  PR_DEBUG ("light_light_dp_proc flag=%d", flag);
  return flag;
}

/**************************************************
按键功能函数
***************************************************/
/* 开启剩余内存监控 */
VOID light_key_fun_free_heap_check_start (VOID)
{
  sys_start_timer (sys_handle.timer_ram_checker, 300, TIMER_CYCLE);
}

/* 关闭剩余内存监控 */
VOID light_key_fun_free_heap_check_stop (VOID)
{
  sys_stop_timer (sys_handle.timer_ram_checker);
}

/* 按键重置配网函数 */
VOID light_key_fun_wifi_reset (VOID)
{
  if (_reset_dev_mode == LIGHT_RESET_BY_ANYWAY || _reset_dev_mode == LIGHT_RESET_BY_LONG_KEY_ONLY)
  {
    light_dp_data_default_set();
    tuya_light_dev_reset();
  }
}

/* 按键开关功能 */
VOID light_key_fun_power_onoff_ctrl (VOID)
{
#if USER_DEFINE_LOWER_POWER

  if (lowpower_flag == 1)
  {
    lowpower_flag = 0;
    light_lib_lowpower_disable();
  }

#endif

  if (dp_data.power == FALSE)
  {
    light_switch_set (TRUE);
  }
  else
  {
    light_switch_set (FALSE);
  }

  light_dp_data_autoupload (FAST_DP_UPLOAD_TIME);
}


STATIC VOID __light_ir_key_fun_color (USHORT_T hue, USHORT_T s)
{
  if (dp_data.power != TRUE)
  {
    return;
  }

  PR_DEBUG ("hue:%d \n", hue);
  light_color_data_hex2asc (&dp_data.color[0], hue, 4);
  light_color_data_hex2asc (&dp_data.color[4], s, 4);

  if (dp_data.mode != COLOR_MODE)
  {
    dp_data.mode = COLOR_MODE;
  }

  light_bright_start();
  light_dp_data_autoupload (NORMAL_DP_UPLOAD_TIME);
  light_dp_data_autosave();
}

VOID light_ir_fun_power_on_ctrl (VOID)
{
#if USER_DEFINE_LOWER_POWER

  if (lowpower_flag == 1)
  {
    lowpower_flag = 0;
    light_lib_lowpower_disable();
  }

#endif

  if (dp_data.power == FALSE)
  {
    light_switch_set (TRUE);
    light_dp_data_autoupload (FAST_DP_UPLOAD_TIME);
  }
}

VOID light_ir_fun_power_off_ctrl (VOID)
{
#if USER_DEFINE_LOWER_POWER

  if (lowpower_flag == 1)
  {
    lowpower_flag = 0;
    light_lib_lowpower_disable();
  }

#endif

  if (dp_data.power == TRUE)
  {
    light_switch_set (FALSE);
    light_dp_data_autoupload (FAST_DP_UPLOAD_TIME);
  }
}

#define BRIGHT_STEP 100 //10%
#if NEW_40IRKYE

VOID light_ir_fun_power_onff_RGB (uint8_t rgbpower)
{
  if(rgbpower==0)
  {
    dp_data.rgb_power=0;
    dp_data.mode = WHITE_MODE;
  }
  else
  {
    dp_data.rgb_power=1;
    dp_data.mode = COLOR_MODE;
  }

  if((dp_data.rgb_power==0)&&(dp_data.w_power==0))
  {
    light_switch_set (FALSE);
    light_dp_data_autoupload (FAST_DP_UPLOAD_TIME);
  }
  else
  {
    if (dp_data.power == FALSE)
    {
      light_switch_set (TRUE);
      light_dp_data_autoupload (FAST_DP_UPLOAD_TIME);
    }  
    else
    {
      light_bright_start();
      light_dp_data_autoupload (NORMAL_DP_UPLOAD_TIME);
    }
  }
}

VOID light_ir_fun_power_onoff_W (uint8_t wpower)
{
  if(wpower==0)
  {
    dp_data.w_power=0;
    dp_data.mode = COLOR_MODE;
  }
  else
  {
    dp_data.w_power=1;
    dp_data.mode = WHITE_MODE;
  }

  if((dp_data.rgb_power==0)&&(dp_data.w_power==0))
  {
    light_switch_set (FALSE);
    light_dp_data_autoupload (FAST_DP_UPLOAD_TIME);
  }
  else
  {
    if (dp_data.power == FALSE)
    {
      light_switch_set (TRUE);
      light_dp_data_autoupload (FAST_DP_UPLOAD_TIME);
    }  
    else
    {
      light_bright_start();
      light_dp_data_autoupload (NORMAL_DP_UPLOAD_TIME);
    }
  }
}

VOID light_ir_key_fun_colorRGBW (USHORT_T hue,USHORT_T s)
{
  __light_ir_key_fun_color (hue, s);
}

VOID light_ir_fun_bright_downW(uint8_t Setflag)
{
  if (dp_data.power != TRUE)
  {
    return;
  }

  if ( (dp_data.mode != COLOR_MODE) && (dp_data.mode != WHITE_MODE))
  {
    return;
  }

  if(Setflag==0)
  dp_data.bright = (dp_data.bright > BRIGHT_STEP) ? (dp_data.bright - BRIGHT_STEP) : BRIGHT_STEP/10;
  else if(Setflag==WHITE_PER25)
  dp_data.bright =  BRIGHT_VAL_MAX/4;
  else if(Setflag==WHITE_PER50)
  dp_data.bright =  BRIGHT_VAL_MAX/2;
  else if(Setflag==WHITE_PER75)
  dp_data.bright =  BRIGHT_VAL_MAX/2+BRIGHT_VAL_MAX/4;
  else  
  dp_data.bright =  BRIGHT_VAL_MAX;
  PR_DEBUG ("bright:%d \n", dp_data.bright);
  dp_data.mode = WHITE_MODE;
  light_bright_start();
  light_dp_data_autoupload (NORMAL_DP_UPLOAD_TIME);
  light_dp_data_autosave();
}

VOID light_ir_fun_bright_downRGB(void)
{
  if(dp_data.power != TRUE)
  {
    return;
  }

  if ((dp_data.mode != COLOR_MODE) && (dp_data.mode != WHITE_MODE))
  {
    return;
  }

  USHORT_T value = __str2short (__asc2hex (dp_data.color[8]), __asc2hex (dp_data.color[9]), __asc2hex (dp_data.color[10]),
                                __asc2hex (dp_data.color[11]));;

  value = (value > BRIGHT_STEP) ? (value - BRIGHT_STEP) : BRIGHT_STEP/10;
  PR_DEBUG ("value:%d \n", value);
  light_color_data_hex2asc (&dp_data.color[8], value, 4);
  dp_data.mode = COLOR_MODE;
  light_bright_start();
  light_dp_data_autoupload (NORMAL_DP_UPLOAD_TIME);
  light_dp_data_autosave();
}

VOID light_ir_fun_bright_upW(uint8_t Setflag)
{
  if (dp_data.power != TRUE)
  {
    return;
  }

  if ((dp_data.mode != COLOR_MODE) && (dp_data.mode != WHITE_MODE))
  {
    return;
  }

  PR_DEBUG ("bright:%d \n", dp_data.bright);
  
  if (dp_data.bright >= BRIGHT_VAL_MAX)
  {
    return;
  }
  
  if(Setflag==0)
  dp_data.bright = (dp_data.bright + BRIGHT_STEP > BRIGHT_VAL_MAX) ? BRIGHT_VAL_MAX: (dp_data.bright + BRIGHT_STEP);
  else if(Setflag==WHITE_PER25)
  dp_data.bright =  BRIGHT_VAL_MAX/4;
  else if(Setflag==WHITE_PER50)
  dp_data.bright =  BRIGHT_VAL_MAX/2;
  else if(Setflag==WHITE_PER75)
  dp_data.bright =  BRIGHT_VAL_MAX/2+BRIGHT_VAL_MAX/4;
  else  
  dp_data.bright =  BRIGHT_VAL_MAX;
  PR_DEBUG("bright:%d \n", dp_data.bright);

  dp_data.mode = WHITE_MODE;
  light_bright_start();
  light_dp_data_autoupload (NORMAL_DP_UPLOAD_TIME);
  light_dp_data_autosave();
}

VOID light_ir_fun_bright_upRGB( )
{
  if (dp_data.power != TRUE)
  {
    return;
  }

  if ( (dp_data.mode != COLOR_MODE) && (dp_data.mode != WHITE_MODE) )
  {
    return;
  }

  USHORT_T value = __str2short (__asc2hex (dp_data.color[8]), __asc2hex (dp_data.color[9]), __asc2hex (dp_data.color[10]), __asc2hex (dp_data.color[11]) );;
  PR_DEBUG ("value:%d \n", value);

  if (value >= BRIGHT_VAL_MAX)
  {
    return;
  }

  value = (value + BRIGHT_STEP > BRIGHT_VAL_MAX) ? BRIGHT_VAL_MAX: (value + BRIGHT_STEP);
  PR_DEBUG ("value:%d \n", value);
  light_color_data_hex2asc (&dp_data.color[8], value, 4);
  dp_data.mode = COLOR_MODE;
  light_bright_start();
  light_dp_data_autoupload (NORMAL_DP_UPLOAD_TIME);
  light_dp_data_autosave();
}

#else
VOID light_ir_key_fun_color (USHORT_T hue)
{
  __light_ir_key_fun_color (hue, 1000);
}

VOID light_ir_key_fun_white()
{
  if (dp_data.power != TRUE)
  {
    return;
  }
  
  PR_DEBUG("-color_type------%d",_def_cfg.color_type);
  if (_def_cfg.color_type == LIGHT_COLOR_RGB)
  {
    __light_ir_key_fun_color (0, 0);
    return;
  }

  dp_data.mode = WHITE_MODE;
  light_bright_start();
  light_dp_data_autoupload (NORMAL_DP_UPLOAD_TIME);
  light_dp_data_autosave();
}

VOID light_ir_fun_bright_down( )
{
  if (dp_data.power != TRUE)
  {
    return;
  }

  if ( (dp_data.mode != COLOR_MODE) && (dp_data.mode != WHITE_MODE))
  {
    return;
  }

  if (dp_data.mode == WHITE_MODE)
  {
    PR_DEBUG ("bright:%d \n", dp_data.bright);

    if (dp_data.bright < BRIGHT_STEP)
    {
      return;
    }

    dp_data.bright = (dp_data.bright > BRIGHT_STEP*2) ? (dp_data.bright - BRIGHT_STEP) : BRIGHT_STEP;
    PR_DEBUG ("bright:%d \n", dp_data.bright);
  }
  else
  {
    USHORT_T value = __str2short (__asc2hex (dp_data.color[8]), __asc2hex (dp_data.color[9]), __asc2hex (dp_data.color[10]),
                                  __asc2hex (dp_data.color[11]) );;
    PR_DEBUG ("value:%d \n", value);

    if (value < BRIGHT_STEP)
    {
      return;
    }

    value = (value > BRIGHT_STEP*2) ? (value - BRIGHT_STEP) : BRIGHT_STEP;
    PR_DEBUG ("value:%d \n", value);
    light_color_data_hex2asc (&dp_data.color[8], value, 4);
  }

  light_bright_start();
  light_dp_data_autoupload (NORMAL_DP_UPLOAD_TIME);
  light_dp_data_autosave();
}

VOID light_ir_fun_bright_up( )
{
  if (dp_data.power != TRUE)
  {
    return;
  }

  if ( (dp_data.mode != COLOR_MODE) && (dp_data.mode != WHITE_MODE) )
  {
    return;
  }

  if (dp_data.mode == WHITE_MODE)
  {
    PR_DEBUG ("bright:%d \n", dp_data.bright);

    if (dp_data.bright >= BRIGHT_VAL_MAX)
    {
      return;
    }

    dp_data.bright = (dp_data.bright + BRIGHT_STEP > BRIGHT_VAL_MAX) ? BRIGHT_VAL_MAX: (dp_data.bright + BRIGHT_STEP);
    PR_DEBUG ("bright:%d \n", dp_data.bright);
  }
  else
  {
    USHORT_T value = __str2short (__asc2hex (dp_data.color[8]), __asc2hex (dp_data.color[9]), __asc2hex (dp_data.color[10]),
                                  __asc2hex (dp_data.color[11]) );;
    PR_DEBUG ("value:%d \n", value);

    if (value >= BRIGHT_VAL_MAX)
    {
      return;
    }

    value = (value + BRIGHT_STEP > BRIGHT_VAL_MAX) ? BRIGHT_VAL_MAX: (value + BRIGHT_STEP);
    PR_DEBUG ("value:%d \n", value);
    light_color_data_hex2asc (&dp_data.color[8], value, 4);
  }

  light_bright_start();
  light_dp_data_autoupload (NORMAL_DP_UPLOAD_TIME);
  light_dp_data_autosave();
}
#endif

/* 按键七彩切换功能 */
VOID light_key_fun_7hue_cyclic (VOID) //七彩变化功能
{
  STATIC USHORT_T hue = 0;
  PR_DEBUG ("hue:%d \n", hue);
  light_color_data_hex2asc (&dp_data.color[0], hue, 4);

  if (hue < 60)
  {
    hue += 30;
  }
  else if (hue < 300)
  {
    hue += 60;
  }
  else
  {
    hue = 0;
  }

  dp_data.power = TRUE;

  if (dp_data.mode != COLOR_MODE)
  {
    dp_data.mode = COLOR_MODE;
  }

  light_bright_start();
  light_dp_data_autoupload (FAST_DP_UPLOAD_TIME);
  light_dp_data_autosave();
}

/* 按键切换情景功能 */
VOID light_key_fun_scene_change (VOID)
{
#ifdef KEY_CHANGE_SCENE_FUN

  if (dp_data.mode != SCENE_MODE)
  {
    dp_data.mode = SCENE_MODE;
    key_trig_num = 0;
  }

  PR_DEBUG ("key_trig_num:%d \n", key_trig_num);

  if (key_trig_num < SCENE_UNIT_NUM_MAX)
  {
    dp_data.power = TRUE;
    memset (dp_data.scene, 0, SIZEOF (dp_data.scene) );
    memcpy (dp_data.scene, key_scene_data[key_trig_num], strlen (key_scene_data[key_trig_num])+1);
    light_scene_start();
    light_dp_data_autoupload (FAST_DP_UPLOAD_TIME);
    light_dp_data_autosave();
    key_trig_num ++;
  }
  else
  {
    light_key_fun_power_onoff_ctrl();
    key_trig_num = 0;
  }

#else
  PR_ERR ("Not define the scene change of key's function!");
  return;
#endif
}

VOID light_key_fun_combo_change (VOID)
{
  STATIC UCHAR key_num = 0;

  if (dp_data.mode == SCENE_MODE)
  {
    key_num = key_trig_num + 7;
    PR_DEBUG ("key_num:%d \n", key_num);
  }

  if (key_num < 7)
  {
    light_key_fun_7hue_cyclic();
    PR_DEBUG ("key_num:%d \n", key_num);
  }
  else
  {
    light_key_fun_scene_change();
    PR_DEBUG ("key_num:%d \n", key_num);
  }

  if (++key_num > 15)
  {
    key_num = 0;
    dp_data.mode = COLOR_MODE;
    PR_DEBUG ("key_num:%d \n", key_num);
  }
}

VOID light_ir_fun_scene_change (UCHAR_T scene_index)
{
  key_trig_num = scene_index;
  dp_data.mode = SCENE_MODE;
  light_key_fun_scene_change();
}


/* 按键记忆情景功能函数 */
STATIC VOID light_key_scene_data_save (char* str, UCHAR_T len)
{
#ifdef KEY_CHANGE_SCENE_FUN
  UCHAR_T num;
  num = __str2byte (__asc2hex (str[0]), __asc2hex (str[1]) );

  if (num >= SCENE_UNIT_NUM_MAX) //max key scene num is 8.
  {
    num = SCENE_UNIT_NUM_MAX - 1;
  }

  memset (key_scene_data[num], 0, strlen (key_scene_data[num]) + 1);
  memcpy (key_scene_data[num], str, len);
  key_trig_num = num;
#else
  return;
#endif
}

STATIC VOID light_key_scene_data_default_set (VOID)
{
#ifdef KEY_CHANGE_SCENE_FUN
  memset (&key_scene_data, 0, SIZEOF (DP_DATA_S) );
  memcpy (key_scene_data[0], KEY_SCENE0_DEF_DATA, strlen (KEY_SCENE0_DEF_DATA) + 1);
  memcpy (key_scene_data[1], KEY_SCENE1_DEF_DATA, strlen (KEY_SCENE1_DEF_DATA) + 1);
  memcpy (key_scene_data[2], KEY_SCENE2_DEF_DATA, strlen (KEY_SCENE2_DEF_DATA) + 1);
  memcpy (key_scene_data[3], KEY_SCENE3_DEF_DATA, strlen (KEY_SCENE3_DEF_DATA) + 1);
  memcpy (key_scene_data[4], KEY_SCENE4_DEF_DATA, strlen (KEY_SCENE4_DEF_DATA) + 1);
  memcpy (key_scene_data[5], KEY_SCENE5_DEF_DATA, strlen (KEY_SCENE5_DEF_DATA) + 1);
  memcpy (key_scene_data[6], KEY_SCENE6_DEF_DATA, strlen (KEY_SCENE6_DEF_DATA) + 1);
  memcpy (key_scene_data[7], KEY_SCENE7_DEF_DATA, strlen (KEY_SCENE7_DEF_DATA) + 1);
#else
  return;
#endif
}

/*******************************************************
配置灯光参数
********************************************************/
/* 配置灯光控制精度 */
VOID tuya_light_ctrl_precision_set (USHORT_T val)
{
  if (val < LIGHT_PRECISION_MIN)
  {
    val = LIGHT_PRECISION_MIN;
  }
  else if (val > LIGHT_PRECISION_MAX)
  {
    val = LIGHT_PRECISION_MAX;
  }

  light_precison = val;
  normal_delay_time = LIGHT_PRECISION_BASE_TIME / light_precison;
}

USHORT_T tuya_light_ctrl_precision_get (VOID)
{
  return light_precison;
}


/**************************************************
初始化函数
***************************************************/
/* 灯光相关线程初始化 */
STATIC OPERATE_RET light_thread_init (VOID)
{
  OPERATE_RET op_ret;
  //
  TUYA_THREAD thread_scene;
  TUYA_THREAD thread_shade;
  op_ret = CreateMutexAndInit (&sys_handle.mutex);

  if (op_ret != OPRT_OK)
  {
    return op_ret;
  }

  op_ret = CreateMutexAndInit (&sys_handle.light_send_mutex);

  if (op_ret != OPRT_OK)
  {
    return op_ret;
  }

  op_ret = CreateAndInitSemaphore (&sys_handle.sem_shade, 0, 1);

  if (OPRT_OK != op_ret)
  {
    return ;
  }

  SystemSleep (10);
  op_ret = tuya_light_CreateAndStart (&thread_shade, light_thread_shade, NULL, 2048, TRD_PRIO_2, "gra_task");

  if (op_ret != OPRT_OK)
  {
    return op_ret;
  }

  SystemSleep (10);
  op_ret = tuya_light_CreateAndStart (&thread_scene, light_thread_scene, NULL, 2048, TRD_PRIO_2, "flash_scene_task");

  if (op_ret != OPRT_OK)
  {
    return op_ret;
  }

#if USER_DEFINE_LOWER_POWER
  op_ret = sys_add_timer (lowpower_timer_cb, NULL, &sys_handle.lowpower);

  if (OPRT_OK != op_ret)
  {
    return op_ret;
  }

#endif
  return OPRT_OK;
}

STATIC OPERATE_RET light_sys_timer_init (VOID)
{
  OPERATE_RET op_ret;
  /* MQTT连接后同步 */
  op_ret = sys_add_timer (idu_timer_cb, NULL, &sys_handle.timer_init_dpdata);

  if (OPRT_OK != op_ret)
  {
    PR_ERR ("idu_timer_cb add error:%d", op_ret);
    return op_ret;
  }
  else
  {
    sys_start_timer (sys_handle.timer_init_dpdata, 500, TIMER_CYCLE);
  }

  /* 数据保存 */
  op_ret = sys_add_timer (data_autosave_timeout_cb, NULL, &sys_handle.timer_data_autosave);

  if (OPRT_OK != op_ret)
  {
    PR_ERR ("data_autosave_timeout_cb add error:%d", op_ret);
    return op_ret;
  }

  //倒计时
  op_ret = sys_add_timer (countdown_timeout_cb, NULL, &sys_handle.timer_countdown);

  if (OPRT_OK != op_ret)
  {
    return op_ret;
  }

#if 0
  //内存检测
  op_ret = sys_add_timer (ram_checker_cb, NULL, &sys_handle.timer_ram_checker);

  if (OPRT_OK != op_ret)
  {
    return op_ret;
  }

  //按键定时器
  op_ret = sys_add_timer (timer_key_cb, NULL, &sys_handle.timer_key);

  if (OPRT_OK != op_ret)
  {
    return op_ret;
  }

#endif
  return OPRT_OK;
}

OPERATE_RET tuya_light_lib_app_init (VOID)
{
  OPERATE_RET op_ret;
  GW_WF_NWC_FAST_STAT_T wifi_type;
  op_ret = tuya_iot_wf_fast_get_nc_type (&wifi_type);

  if (OPRT_OK != op_ret)
  {
    PR_NOTICE ("Not Authorization !");
    return op_ret;
  }

  PR_NOTICE ("wifi statue %d", wifi_type);

  if (wifi_type == GWNS_FAST_LOWPOWER)
  {
    tuya_light_config_stat (CONF_STAT_LOWPOWER);
  }
  else if (wifi_type == GWNS_FAST_UNCFG_SMC)
  {
    tuya_light_config_stat (CONF_STAT_SMARTCONFIG);
  }
  else if (wifi_type == GWNS_FAST_UNCFG_AP)
  {
    tuya_light_config_stat (CONF_STAT_APCONFIG);
  }
  else
  {
    //nothing to do
  }

  return OPRT_OK;
}

OPERATE_RET tuya_light_start (VOID)
{
  PR_NOTICE ("==== %s ====", __FUNCTION__);
  OPERATE_RET op_ret;
  //tuya_light_common_flash_init();
  //do not enter in smart config mode.
  //light_rst_count_judge();
  op_ret = light_sys_timer_init();

  if (op_ret != OPRT_OK)
  {
    PR_ERR ("light_sys_timer_init err : %d", op_ret);
    return op_ret;
  }

  return OPRT_OK;
}

TUYA_WF_CFG_MTHD_SEL light_get_wifi_cfg (VOID)
{
  TUYA_WF_CFG_MTHD_SEL mthd = TUYA_GWCM_SPCL_MODE;

  if (_def_cfg.wf_cfg > WIFI_CFG_OLD_CPT)
  {
    return TUYA_GWCM_SPCL_MODE;
  }

  if (_def_cfg.wf_cfg == WIFI_CFG_OLD_CPT)
  {
    mthd = TUYA_GWCM_OLD;
  }
  else if (_def_cfg.wf_cfg == WIFI_CFG_SPCL_MODE)
  {
    mthd = TUYA_GWCM_SPCL_MODE;
  }
  else if (_def_cfg.wf_cfg == WIFI_CFG_LOW_POWER)
  {
    mthd = TUYA_GWCM_LOW_POWER;
  }
  else if (_def_cfg.wf_cfg == WIFI_CFG_OLD_PROD)
  {
    mthd = TUYA_GWCM_OLD_PROD;
  }

  return mthd;
}

/* 上电初始状态设置 */
VOID light_init_stat_set (VOID)
{
  light_bright_data_get();

  if (_cw_type == LIGHT_CW_CCT)
  {
    if (dp_data.mode != WHITE_MODE)
    {
      light_bright_cw_change (dp_data.bright, dp_data.col_temp, &light_handle.fin);
      light_handle.fin.white = 0;
    }
  }

  PR_DEBUG ("power = %d  mode = %d", dp_data.power, dp_data.mode);

  if (dp_data.power == TRUE)
  {
    //直接点亮
    light_handle.curr.red   = light_handle.fin.red;
    light_handle.curr.green = light_handle.fin.green;
    light_handle.curr.blue  = light_handle.fin.blue;
    light_handle.curr.white = light_handle.fin.white;
    light_handle.curr.warm  = light_handle.fin.warm;

    switch (dp_data.mode)
    {
      case WHITE_MODE:
      case COLOR_MODE:
        PR_NOTICE ("init send:R:%d G:%d B:%d C:%d W:%d", light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue,
                   light_handle.fin.white, light_handle.fin.warm);
        light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,
                         light_handle.fin.warm);
        break;

      case SCENE_MODE:
        //开启情景模式变化
        light_scene_start();
        break;

      case MUSIC_MODE:
        //默认值
        memset (&light_handle.fin, 0, SIZEOF (light_handle.fin) );
        light_rgb_value_get (MUSIC_INIT_VAL, &light_handle.fin);
        PR_NOTICE ("init send:R:%d G:%d B:%d C:%d W:%d", light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue,
                   light_handle.fin.white, light_handle.fin.warm);
        light_send_data (light_handle.fin.red, light_handle.fin.green, light_handle.fin.blue, light_handle.fin.white,
                         light_handle.fin.warm);
        break;

      default:
        break;
    }
  }
}

LIGHT_CW_TYPE_E tuya_light_cw_type_get (VOID)
{
  PR_DEBUG ("tuya_light_cw_type_get %d", _cw_type);
  return _cw_type;
}

OPERATE_RET tuya_light_cw_type_set (LIGHT_CW_TYPE_E type)
{
  if (type > LIGHT_CW_CCT || type < LIGHT_CW_PWM)
  {
    PR_ERR ("tuya_light_cw_type_set err:%d", type);
    return OPRT_INVALID_PARM;
  }

  _cw_type = type;
  return OPRT_OK;
}

OPERATE_RET tuya_light_reset_mode_set (LIGHT_RESET_NETWORK_STA_E mode)
{
  if (mode > LIGHT_RESET_BY_ANYWAY || mode < LIGHT_RESET_BY_POWER_ON_ONLY)
  {
    PR_ERR ("tuya_light_reset_mode_set err:%d", mode);
    return OPRT_INVALID_PARM;
  }

  _reset_dev_mode = mode;
  return OPRT_OK;
}

VOID tuya_light_memory_flag_set (BOOL_T status)
{
  _light_memory = status;
}

OPERATE_RET tuya_light_cw_max_power_set (UCHAR_T max_power)
{
  if (max_power > 200 || max_power < 100)
  {
    PR_ERR ("tuya_light_cw_type_set err:%d", max_power);
    return OPRT_INVALID_PARM;
  }

  _white_light_max_power = max_power;
  return OPRT_OK;
}

/* 获取灯光配置参数 */
LIGHT_DEFAULT_CONFIG_S* light_default_cfg_get (VOID)
{
  return &_def_cfg;
}

/* 灯光配置参数设置 */
STATIC OPERATE_RET light_cfg_param_set (VOID)
{
  tuya_light_config_param_set (&_def_cfg);

  if (_def_cfg.wf_cfg > WIFI_CFG_OLD_CPT)
  {
    PR_ERR ("config param wf_cfg err: %d", _def_cfg.wf_cfg);
    _def_cfg.wf_cfg = WIFI_CFG_SPCL_MODE;
  }

  if (_def_cfg.wf_rst_cnt < 3)
  {
    PR_ERR ("config param wf_rst_cnt err: %d", _def_cfg.wf_rst_cnt);
    _def_cfg.wf_rst_cnt = 3;
  }

  if ( (_def_cfg.color_type < LIGHT_COLOR_C) || (_def_cfg.color_type > LIGHT_COLOR_RGBCW) )
  {
    PR_ERR ("config param color_type err: %d", _def_cfg.color_type);
    _def_cfg.color_type = LIGHT_COLOR_RGBCW;
  }

  if (_def_cfg.power_onoff_type != POWER_ONOFF_BY_SHADE && _def_cfg.power_onoff_type != POWER_ONOFF_BY_DIRECT)
  {
    PR_ERR ("config param power_onoff_type err: %d", _def_cfg.power_onoff_type);
    _def_cfg.power_onoff_type = POWER_ONOFF_BY_SHADE;
  }

  if (_def_cfg.def_light_color < INIT_COLOR_R || _def_cfg.def_light_color > INIT_COLOR_W)
  {
    PR_ERR ("config param def_light_color err: %d", _def_cfg.power_onoff_type);
    _def_cfg.def_light_color = INIT_COLOR_C;
  }

  if (_def_cfg.def_bright_precent == 0 || _def_cfg.def_bright_precent > 100)
  {
    PR_ERR ("config param def_bright_precent err: %d", _def_cfg.power_onoff_type);
    _def_cfg.def_bright_precent = 50;
  }

  if (_def_cfg.def_temp_precent > 100)
  {
    PR_ERR ("config param def_temp_precent err: %d", _def_cfg.power_onoff_type);
    _def_cfg.def_temp_precent = 100;
  }

  if (_def_cfg.bright_max_precent <= _def_cfg.bright_min_precent)
  {
    return OPRT_INVALID_PARM;
  }

  if (_def_cfg.bright_max_precent > 100)
  {
    _def_cfg.bright_max_precent = 100;
    return OPRT_INVALID_PARM;
  }

  if (_def_cfg.bright_min_precent < 1)
  {
    _def_cfg.bright_min_precent = 1;
    return OPRT_INVALID_PARM;
  }

#if 0
  PR_NOTICE ("wf_cfg:%d", _def_cfg.wf_cfg);
  PR_NOTICE ("wf_rst_cnt:%d", _def_cfg.wf_rst_cnt);
  PR_NOTICE ("color_type:%d", _def_cfg.color_type);
  PR_NOTICE ("power_onoff_type:%d", _def_cfg.power_onoff_type);
  PR_NOTICE ("def_light_color:%d", _def_cfg.def_light_color);
  PR_NOTICE ("def_bright_precent:%d", _def_cfg.def_bright_precent);
  PR_NOTICE ("def_temp_precent:%d", _def_cfg.def_temp_precent);
  PR_NOTICE ("bright_max_precent:%d", _def_cfg.bright_max_precent);
  PR_NOTICE ("bright_min_precent:%d", _def_cfg.bright_min_precent);
  PR_NOTICE ("white_color_mutex:%d", _def_cfg.whihe_color_mutex);
#endif
  return OPRT_OK;
}

OPERATE_RET tuya_light_init (VOID)
{
  OPERATE_RET op_ret;
  PR_NOTICE ("==== tuya light lib version:%s ====", TUYA_LIGH_LIB_VERSION);
  // PR_DEBUG("DP_DATA_S = %d", sizeof(DP_DATA_S));
  op_ret = light_cfg_param_set();

  if (op_ret != OPRT_OK)
  {
    PR_ERR ("light_cfg_param_set err : %d", op_ret);
    return op_ret;
  }

  light_hw_timer_stop();
  tuya_light_device_hw_init();
  op_ret = ufinit();

  if (op_ret != OPRT_OK)
  {
    PR_ERR ("ufinit err : %d", op_ret);
    return op_ret;
  }

  light_dp_data_read (&dp_data);

  if ((FALSE == dp_data.power) && (tuya_light_dev_poweron() == TRUE))
  {
    //上电强制开灯
    __light_power_ctl (TRUE);
  }
  dp_data.rgb_power = TRUE;
  dp_data.w_power = TRUE;
  op_ret = light_thread_init();

  if (op_ret != OPRT_OK)
  {
    PR_ERR ("light_thread_init err : %d", op_ret);
    return op_ret;
  }

  light_init_stat_set();
  //light_rst_count_add();
  //set_light_signal_if_device_reset();
  tuya_set_cnt_rst_mode (_def_cfg.wf_rst_cnt, tuya_cnt_rst_inform_cb);
  return OPRT_OK;
}


