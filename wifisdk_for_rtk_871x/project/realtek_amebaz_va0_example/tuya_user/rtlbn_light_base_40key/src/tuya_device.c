/***********************************************************
*  File: device.c
*  Author: nzy
*  Date: 20150605
***********************************************************/
#define __DEVICE_GLOBALS
#include ".compile_usr_cfg.h"
#include "tuya_device.h"
#include "tuya_light_lib.h"
#include "tuya_common_light.h"
#include "tuya_hw_table.h"
#include "irDecode.h"
#include "uf_file.h"


/***********************************************************
*************************micro define***********************
***********************************************************/

#define REG_WRITE(_r,_v)    (*(volatile uint32 *)(_r)) = (_v)
#define REG_READ(_r)        (*(volatile uint32 *)(_r))
#define WDEV_NOW()          REG_READ(0x3ff20c00)

/***********************************************************
*************************function define********************
***********************************************************/
extern  VOID start_test (VOID);
STATIC VOID __userIrCmdDeal (IRCMD cmd, IRCODE irType);

OPERATE_RET set_reset_cnt (INT_T val);

LED_HANDLE wf_light = NULL;

STATIC BOOL recv_debug_flag = FALSE;

VOID tuya_light_pre_device_init (VOID)
{
#if USER_DEBUG_APP
  SetLogManageAttr (LOG_LEVEL_DEBUG);
  PR_DEBUG ("USER SET LOG_LEVEL_DEBUG \n");
#else
  SetLogManageAttr (LOG_LEVEL_NOTICE);
  PR_NOTICE ("USER SET LOG_LEVEL_NOTICE \n");
#endif
#if USER_DEFINE_LOWER_POWER
  tuya_set_lp_mode (TRUE);
#endif
  tuya_light_config_param_get();
  sys_log_uart_off();
  tuya_light_init();  //light硬件相关初始
}

#if _IS_OEM
static CHAR_T __product_key[PRODUCT_KEY_LEN+4];
STATIC OPERATE_RET __read_product_key_fast( )
{
  OPERATE_RET op_ret;
  //op_ret = tuya_light_common_flash_read(LIGHT_DATA_KEY, &buf);
  uFILE* file = ufopen (PRODUCT_KEY_DATA_KEY, "r");

  if (NULL == file)
  {
    __product_key[0]='\0';
    PR_ERR ("ufopen failed");
    return OPRT_COM_ERROR;
  }

  INT_T read_len =  ufread (file, __product_key, PRODUCT_KEY_LEN);
  ufclose (file);
  file = NULL;

  if (PRODUCT_KEY_LEN != read_len)
  {
    PR_ERR ("ufread failed read_len%d!=PRODUCT_KEY_LEN", read_len);
    __product_key[0]='\0';
    return OPRT_COM_ERROR;
  }

  __product_key[PRODUCT_KEY_LEN]='\0';
  PR_NOTICE ("read flash: product_key[%s]", __product_key);
  return OPRT_OK;
}
#endif

static void __product_key_init()
{
#if _IS_OEM
  memset (__product_key, 0x0, PRODUCT_KEY_LEN+4);

  if (__read_product_key_fast( ) != OPRT_OK)
  {
    __product_key[0]='\0';
  }

  if (__product_key[0] != '\0')
  {
    PR_NOTICE ("====Got PID from flash[%s]====\r\n", __product_key);
    return;
  }

  memcpy (__product_key, PRODECT_KEY, PRODUCT_KEY_LEN);
  __product_key[PRODUCT_KEY_LEN] = '\0';
  PR_NOTICE ("====doesn't got PID from flash, use pre define Key[%s]====\r\n", __product_key);
  tuya_light_oem_set (TRUE);
#endif
}


VOID tuya_light_app_init (VOID)
{
  OPERATE_RET op_ret;
  tuya_light_lib_app_init( );
  sys_log_uart_on();
  PR_NOTICE ("%s:%s:%s:%s", APP_BIN_NAME, USER_SW_VER, __DATE__, __TIME__);
  PR_NOTICE ("%s", tuya_iot_get_sdk_info() );
#if USER_DEBUG_APP
  hw_config_def_printf();
#endif
  __product_key_init();
  light_prod_init (PROD_TEST_TYPE_V2, prod_test);
}

STATIC OPERATE_RET device_differ_init (VOID)
{
  OPERATE_RET op_ret;
  op_ret = tuya_light_key_init (tuya_light_key_process);

  if (op_ret != OPRT_OK)
  {
    PR_ERR ("tuya_light_key_init ERR:%d", op_ret);
  }

  light_strip_infrared_init (__userIrCmdDeal);
  return OPRT_OK;
}

VOID tuya_light_device_init (VOID)
{
  tuya_light_print_port_init();
#if _IS_OEM
  tuya_light_smart_frame_init (__product_key, USER_SW_VER);
#else
  tuya_light_smart_frame_init (USER_SW_VER);
#endif
  tuya_light_start();                          /* 必须在云服务启动后调�?*/
  device_differ_init();
  INT_T size = SysGetHeapSize();
  PR_DEBUG ("init ok  free_mem_size:%d", size);
  return ;
}

VOID mf_user_callback (VOID)
{
	
}

/*****************************************
    按键控制部分
*****************************************/
VOID normal_key_function_cb (VOID)
{
  KEY_FUNCTION_E fun = tuya_light_normal_key_fun_get();
#if (USER_DEFINE_LIGHT_TYPE == USER_LIGTH_MODE_C || USER_DEFINE_LIGHT_TYPE == USER_LIGTH_MODE_CW)
  fun = KEY_FUN_POWER_CTRL;
#endif
  TUYA_GW_WIFI_NW_STAT_E wf_stat = tuya_light_get_wf_gw_status(); //处于配网状态下，功能暂时屏�?
  if (wf_stat == STAT_UNPROVISION || wf_stat == STAT_AP_STA_UNCFG)
  {
    return;
  }

  PR_DEBUG ("normal key fun = [%x]", fun);
  light_IrCrtl_IR_Mode = 0;
  switch (fun)
  {
    case KEY_FUN_POWER_CTRL :
      light_key_fun_power_onoff_ctrl();
      break;

    case KEY_FUN_RAINBOW_CHANGE :
      light_key_fun_7hue_cyclic();
      break;

    case KEY_FUN_SCENE_CHANGE :
      light_key_fun_scene_change();
      break;

    case KEY_FUN_POWER_RAINBOW_SCENE :
      light_key_fun_combo_change();
      break;

    default:
      break;
  }
}

VOID long_key_function_cb (VOID)
{
  KEY_FUNCTION_E fun = tuya_light_long_key_fun_get();

  if (KEY_FUN_RESET_WIFI != fun)
  {
    TUYA_GW_WIFI_NW_STAT_E wf_stat = tuya_light_get_wf_gw_status();

    if (wf_stat == STAT_UNPROVISION || wf_stat == STAT_AP_STA_UNCFG)
    {
      return;
    }
  }

  PR_DEBUG ("long key fun = [%x]", fun);

  switch (fun)
  {
    case KEY_FUN_RESET_WIFI :
      light_key_fun_wifi_reset();
      break;

    default:
      break;
  }
}


VOID seq_key_function_cb (UCHAR cnt)
{
  STATIC UCHAR key_flag = 0;

  if (cnt == 15)
  {
    recv_debug_flag = 1 - recv_debug_flag;
  }

  if (cnt == 10)
  {
    key_flag = 1 - key_flag;

    if (key_flag)
    {
      light_key_fun_free_heap_check_start();
    }
    else
    {
      light_key_fun_free_heap_check_stop();
    }
  }
}


VOID tuya_light_key_process (INT_T gpio_no, PUSH_KEY_TYPE_E type, INT_T cnt)
{
  PR_DEBUG ("gpio_no: %d", gpio_no);
  PR_DEBUG ("type: %d", type);
  PR_DEBUG ("cnt: %d", cnt);
  UCHAR_T temp[16];

  if ( (INT) light_key_id_get() == gpio_no)
  {
    if (LONG_KEY == type)
    {
      long_key_function_cb();
    }
    else if (NORMAL_KEY == type)
    {
      normal_key_function_cb();
    }
    else if (SEQ_KEY == type)
    {
      //  seq_key_function_cb(cnt);
    }
  }
}


STATIC VOID __userIrCmdDeal (IRCMD cmd, IRCODE irType)
{
  STATIC IRCMD_new last_cmd = -1;
  static uint8_t I = 0;

  // uint8_t index;
  if (irType == IRCODESTART || irType == IRCODEREPEAT)
  {
    if (irType == IRCODESTART)
    {
      last_cmd = cmd;
	  light_IrCrtl_IR_Flag = 0;
      PR_NOTICE ("NEW CMD:%x\r\n", last_cmd);
      I = 0;
    }
    else
    {
      I++;
      //PR_DEBUG ("REPEAT%d CMD:%x\r\n", I, last_cmd);
      switch (last_cmd)
      {
        case KEY_POWER_RGBON:
          if (I >= 48)
          {
            PR_DEBUG ("===>ir remote KEY_POWER_ON long press\r\n");
            long_key_function_cb();
            //wifi reset.
          }
          return;
          
        case -1:
          I = 0;
          return;
        
        #if NEW_40IRKYE 
          // 亮度等级支持长按
          case KEY_BRIGHTNESS_RGBUP:
          case KEY_BRIGHTNESS_RGBDOWN:  
          case KEY_BRIGHTNESS_WUP:
          case KEY_BRIGHTNESS_WDOWN:
          case KEY_QUICK:  
          case KEY_SLOW:
            if (I >= 5)
            {
               I = 0;
              break;
            }
        #endif    
      
        default:
          return;
      }
    }
  }
  else if(irType == IRCODEAUTOSTART)
  {
	 if(light_IrCrtl_IR_Mode)
	 light_IrCrtl_IR_Flag = 1;
  }
  
  TUYA_GW_WIFI_NW_STAT_E wf_stat = tuya_light_get_wf_gw_status();

  if ( (wf_stat == STAT_UNPROVISION) || (wf_stat == STAT_AP_STA_UNCFG) )
  {
    return;
  }


  /*速度加和减不处理*/
  if((last_cmd==KEY_QUICK)||(last_cmd==KEY_SLOW))
  {
      light_IrCrtl_ManulFlag = 1;
  }
  else
  {
	 if(last_cmd!=KEY_AUTO)
	 {
		 light_IrCrtl_ManulFlag = 0;
		 // 退出手动模式
		  if(light_IrCrtl_IR_Mode)
		  {
		    if(light_IrCrtl_IR_Flag==0)  
		    {
		       light_IrCrtl_IR_Mode = 0;
		    }
		  }
	 }
  }

  #if NEW_40IRKYE
    switch (last_cmd)
    {
      case KEY_BRIGHTNESS_RGBUP:
        light_ir_fun_bright_upRGB();
        break;

      case KEY_BRIGHTNESS_RGBDOWN:
        light_ir_fun_bright_downRGB();
        break;

      case KEY_POWER_RGBOFF: 
        light_ir_fun_power_onff_RGB(FALSE);
        break;

      case KEY_POWER_RGBON: 
        light_ir_fun_power_onff_RGB(TRUE);
        break;

      case KEY_R_LEVEL_5:
        light_ir_key_fun_colorRGBW (0,1000);
        break;

      case KEY_R_LEVEL_4:
        light_ir_key_fun_colorRGBW (15,1000);
        break;

      case KEY_R_LEVEL_3:
        light_ir_key_fun_colorRGBW (30,1000);
        break;

      case KEY_R_LEVEL_2:
        light_ir_key_fun_colorRGBW (45,1000);
        break;

      case KEY_R_LEVEL_1:
        light_ir_key_fun_colorRGBW (60,1000);
        break;

      case KEY_G_LEVEL_5:
        light_ir_key_fun_colorRGBW (120,1000);
        break;

      case KEY_G_LEVEL_4:
        light_ir_key_fun_colorRGBW (135,1000);
        break;

      case KEY_G_LEVEL_3:
        light_ir_key_fun_colorRGBW (150,1000);
        break;

      case KEY_G_LEVEL_2:
        light_ir_key_fun_colorRGBW (160,1000);
        break;

      case KEY_G_LEVEL_1:
        light_ir_key_fun_colorRGBW (180,1000);
        break;

      case KEY_B_LEVEL_5:
        light_ir_key_fun_colorRGBW (240,1000);
        break;

      case KEY_B_LEVEL_4:
        light_ir_key_fun_colorRGBW (255,1000);
        break;

      case KEY_B_LEVEL_3:
        light_ir_key_fun_colorRGBW (270,1000);
        break;

      case KEY_B_LEVEL_2:
         light_ir_key_fun_colorRGBW(285,1000);
        break;

      case KEY_B_LEVEL_1:
        light_ir_key_fun_colorRGBW(300,1000);
        break;

      case KEY_W_LEVEL_5:
        light_ir_key_fun_colorRGBW(0,0);
        break;

      case KEY_W_LEVEL_4:
        light_ir_key_fun_colorRGBW(0,100);
        break;

      case KEY_W_LEVEL_3:
        light_ir_key_fun_colorRGBW(0,200);
        break;
      
      case KEY_W_LEVEL_2:
        light_ir_key_fun_colorRGBW(120,100);
        break;

      case KEY_W_LEVEL_1:
        light_ir_key_fun_colorRGBW(120,200);
        break;

      
      case KEY_BRIGHTNESS_WUP:
        light_ir_fun_bright_upW(0);
        break;

      case KEY_BRIGHTNESS_WDOWN:
        light_ir_fun_bright_downW(0);
        break;

      case KEY_POWER_WOFF:
        light_ir_fun_power_onoff_W(FALSE);
        break;

      case KEY_POWER_WON:
        light_ir_fun_power_onoff_W(TRUE);
        break;

      case KEY_W_25:
        light_ir_fun_bright_upW(WHITE_PER25);
        break;

      case KEY_W_50:
        light_ir_fun_bright_upW(WHITE_PER50);
        break;

      case KEY_W_75:
        light_ir_fun_bright_upW(WHITE_PER75);
        break;

      case KEY_W_100:
        light_ir_fun_bright_upW(WHITE_PER100);
        break;

      case KEY_JUMP3:
        light_ir_fun_scene_change (4);
        break;

      case KEY_FADE3:
        light_ir_fun_scene_change (5);
        break;

      case KEY_JUMP7:
        light_ir_fun_scene_change (6);
        break;

      case KEY_FADE7:
        light_ir_fun_scene_change (7);
        break;

      case KEY_FLASH:
        light_ir_fun_scene_change (3);
        break;

      case KEY_AUTO:
        if(light_IrCrtl_IR_Mode==0)
        {
          light_IrCrtl_IR_Mode = 1;
          light_IrCrtl_IR_Cnts = 0;
          light_IrCrtl_IR_Runtimes = 0;
          light_ir_fun_scene_change(3);
        }
        break;
        
      case KEY_QUICK:
        //if(key_trig_num<4)
        {
          if(light_IrCrtl_Speedtimes<96)
          light_IrCrtl_Speedtimes = light_IrCrtl_Speedtimes+5;
          else
            light_IrCrtl_Speedtimes = 100;
          /********全局速度控制*******/
          light_IrCrtl_ManulTimes = 105-light_IrCrtl_Speedtimes;
          light_scene_SetDataPara(light_IrCrtl_Speedtimes,light_IrCrtl_Speedtimes);
        }
        break;

      case KEY_SLOW:
        //if(key_trig_num<4)
        {
          if(light_IrCrtl_Speedtimes>44)
          light_IrCrtl_Speedtimes = light_IrCrtl_Speedtimes-5;
          else
            light_IrCrtl_Speedtimes = 40;
          /********全局速度控制*******/
          light_IrCrtl_ManulTimes = 105- light_IrCrtl_Speedtimes;
          light_scene_SetDataPara(light_IrCrtl_Speedtimes,light_IrCrtl_Speedtimes);
        }
        break;
        
      default:
        
        break;
    }
  #else
    switch (last_cmd)
    {
      case KEY_POWER_ON:
        light_ir_fun_power_on_ctrl();
        break;

      case KEY_POWER_OFF:
        light_ir_fun_power_off_ctrl();
        break;

      case KEY_BRIGHTNESS_UP://�?        light_ir_fun_bright_up();
        break;

      case KEY_BRIGHTNESS_DOWN://�?        light_ir_fun_bright_down();
        break;

      case KEY_R_LEVEL_5:
        //0
        light_ir_key_fun_color (0);
        break;

      case KEY_R_LEVEL_4:
        //15
        light_ir_key_fun_color (15);
        break;

      case KEY_R_LEVEL_3:
        //30
        light_ir_key_fun_color (30);
        break;

      case KEY_R_LEVEL_2:
        //45
        light_ir_key_fun_color (45);
        break;

      case KEY_R_LEVEL_1:
        //60
        light_ir_key_fun_color (60);
        break;

      case KEY_G_LEVEL_5:
        //120
        light_ir_key_fun_color (120);
        break;

      case KEY_G_LEVEL_4:
        //135
        light_ir_key_fun_color (135);
        break;

      case KEY_G_LEVEL_2:
        //150
        light_ir_key_fun_color (150);
        break;

      case KEY_G_LEVEL_3:
        //165
        light_ir_key_fun_color (160);
        break;

      case KEY_G_LEVEL_1:
        //180
        light_ir_key_fun_color (180);
        break;

      case KEY_B_LEVEL_5:
        //240
        light_ir_key_fun_color (240);
        break;

      case KEY_B_LEVEL_4:
        //255
        light_ir_key_fun_color (255);
        break;

      case KEY_B_LEVEL_3:
        //270
        light_ir_key_fun_color (270);
        break;

      case KEY_B_LEVEL_2:
        //285
        light_ir_key_fun_color (285);
        break;

      case KEY_B_LEVEL_1:
        //300
        light_ir_key_fun_color (300);
        break;

      case KEY_W_LEVEL_5:
        light_ir_key_fun_white();
        break;

      case KEY_MODE_SMOOTH:
        light_ir_fun_scene_change (7);
        break;

      case KEY_MODE_FLASH:
        light_ir_fun_scene_change (6);
        break;

      case KEY_MODE_STROBE:
        light_ir_fun_scene_change (5);
        break;

      case KEY_MODE_FADE:
        light_ir_fun_scene_change (4);
        break;

      default:
        break;
    }
  #endif
}


