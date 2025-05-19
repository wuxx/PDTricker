#include "stdio.h"
#include "CH552.H"

#include "Public\TIMER.H"
#include "Public\ADC.H"
#include "Public\GPIO.H"
#include "Public\DEBUG.H"


void led_all_off();    
void led_off(UINT8 value);
void led_set(UINT8 value, UINT8 blink);
UINT16 adc_read();
int pd_verify(UINT8 value);

enum {
    PD_5V = 0,
    PD_9V,
    PD_12V,
    PD_15V,
    PD_20V,
    PD_MAX,
};

unsigned int x=0;

sbit  KEY = P1^0;

sbit LED_5V  = P3^2;
sbit LED_9V  = P1^4;
sbit LED_12V = P1^5;
sbit LED_15V = P1^6;
sbit LED_20V = P1^7;

sbit CFG_1 = P3^3;
sbit CFG_2 = P3^4;
sbit CFG_3 = P3^5;


/*******************************************************************************/
/*******************************************************************************/

/*******************************************************************************
* Function Name  : WriteDataFlash(UINT8 Addr,PUINT8 buf,UINT8 len)
* Description    : DataFlash写
* Input          : UINT8 Addr，PUINT16 buf,UINT8 len
* Output         : None
* Return         : UINT8 i 返回写入长度
*******************************************************************************/
UINT8 WriteDataFlash(UINT8 Addr,PUINT8 buf,UINT8 len)
{
    UINT8 i;
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;                                                           //进入安全模式
    GLOBAL_CFG |= bDATA_WE;                                                    //使能DataFlash写
    SAFE_MOD = 0;                                                              //退出安全模式	
		ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    Addr <<= 1;
    for(i=0;i<len;i++)
	  {
        ROM_ADDR_L = Addr + i*2;
        ROM_DATA_L = *(buf+i);			
        if ( ROM_STATUS & bROM_ADDR_OK ) {                                     // 操作地址有效
           ROM_CTRL = ROM_CMD_WRITE;                                           // 写入
        }
        if((ROM_STATUS ^ bROM_ADDR_OK) > 0) return i;                          // 返回状态,0x00=success,  0x02=unknown command(bROM_CMD_ERR)
	  }
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;                                                           //进入安全模式
    GLOBAL_CFG &= ~bDATA_WE;                                                   //开启DataFlash写保护
    SAFE_MOD = 0;                                                              //退出安全模式	
    return i;		
}

/*******************************************************************************
* Function Name  : ReadDataFlash(UINT8 Addr,UINT8 len,PUINT8 buf)
* Description    : 读DataFlash
* Input          : UINT8 Addr UINT8 len PUINT8 buf
* Output         : None
* Return         : UINT8 i 返回写入长度
*******************************************************************************/
UINT8 ReadDataFlash(UINT8 Addr,UINT8 len,PUINT8 buf)
{
    UINT8 i;
    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    Addr <<= 1;
    for(i=0;i<len;i++){
	  ROM_ADDR_L = Addr + i*2;                                                   //Addr必须为偶地址
	  ROM_CTRL = ROM_CMD_READ;
//     if ( ROM_STATUS & bROM_CMD_ERR ) return( 0xFF );                        // unknown command
	  *(buf+i) = ROM_DATA_L;
		}
    return i;
}

UINT8 pd_config_read()
{
    UINT8 pc;
    
    ReadDataFlash(0x0, 1, &pc);
    return pc;
}

void pd_config_update(UINT8 pc)
{
    WriteDataFlash(0x0, &pc, 1);
}

/*******************************************************************************/
/*******************************************************************************/

/* TX:  P3.1/PWM2/TXD */
/* RX:  P3.0/PWM1/RXD */
int pd_set(UINT8 value)
{
    switch (value) {
        case (PD_5V):
            CFG_1 = 1;
            CFG_2 = 0;
            CFG_3 = 0;
            break;
        case (PD_9V):
            CFG_1 = 0;
            CFG_2 = 0;
            CFG_3 = 0;
            break;
        case (PD_12V):
            CFG_1 = 0;
            CFG_2 = 0;
            CFG_3 = 1;
            break;
        case (PD_15V):
            CFG_1 = 0;
            CFG_2 = 1;
            CFG_3 = 1;
            break;
        case (PD_20V):
            CFG_1 = 0;
            CFG_2 = 1;
            CFG_3 = 0;
            break;
        default:
            break;
    }

    pd_verify(value);
    
    return 0;    
}


    
 /* compare with adc value */
int pd_verify(UINT8 value)
{
    UINT8 r = 0;

    /* 100K 510K */
    UINT16 adc_value;
    adc_value = adc_read();

    printf("ADC_VALUE: %d (%ld mV)\n",(UINT16)adc_value, (((UINT32)(adc_value) * 20130)) / 256);

    switch (value) {
        case (PD_5V):
            if (adc_value >= 57 && adc_value <= 69) {
                r = 0;
            } else {
                r = 1;
            }
            break;
        case (PD_9V):
            if (adc_value >= 108 && adc_value <= 120) {
                r = 0;
            } else {
                r = 1;
            }
            break;
        case (PD_12V):
            if (adc_value >= 146 && adc_value <= 158) {
                r = 0;
            } else {
                r = 1;
            }
            break;
        case (PD_15V):
            if (adc_value >= 184 && adc_value <= 197) {
                r = 0;
            } else {
                r = 1;
            }
            break;
        case (PD_20V):
            if (adc_value >= 247 && adc_value <= 260) {
                r = 0;
            } else {
                r = 1;
            }
            break;
        default:            
            break;
    }
    
    led_set(value, r);
    return r;
}

UINT16 count = 0;
UINT8 blink = 0;
UINT8 blink_value = PD_5V;

void led_all_off()
{
    LED_5V  = 0;
    LED_9V  = 0;
    LED_12V = 0;
    LED_15V = 0;
    LED_20V = 0;
}

void led_off(UINT8 value)
{
    switch (value) {
        case (PD_5V):
            LED_5V = 0;
            break;
        case (PD_9V):
            LED_9V = 0;
            break;
        case (PD_12V):
            LED_12V = 0;
            break;
        case (PD_15V):
            LED_15V = 0;
            break;
        case (PD_20V):
            LED_20V = 0;
            break;
        default:
            break;
    }    
}

void led_set(UINT8 value, UINT8 mode)
{

    
    if (mode == 0) {
#if 0            
        LED_5V  = 0;
        LED_9V  = 0;
        LED_12V = 0;
        LED_15V = 0;
        LED_20V = 0;
#endif
        
        blink = 0;

        switch (value) {
            case (PD_5V):
                LED_5V = 1;
                break;
            case (PD_9V):
                LED_9V = 1;
                break;
            case (PD_12V):
                LED_12V = 1;
                break;
            case (PD_15V):
                LED_15V = 1;
                break;
            case (PD_20V):
                LED_20V = 1;
                break;
            default:
                break;
        }
    } else {
        blink = 1;
        blink_value = value;
    }
}

UINT16 adc_read()
{
    UINT16 v;

    //printf("AIN%02x ",(UINT16)i);		
    ADC_ChannelSelect( 0 );
    ADC_START = 1;
    while(ADC_START);
    //printf("DATA: %d\n",(UINT16)ADC_DATA);
    v = (UINT16)ADC_DATA;
    return v;
}



void blink_handler()
{

    switch (blink_value) {
        case (PD_5V):
            LED_5V = !LED_5V;
            break;
        case (PD_9V):
            LED_9V = !LED_9V;
            break;
        case (PD_12V):
            LED_12V = !LED_12V;
            break;
        case (PD_15V):
            LED_15V = !LED_15V;
            break;
        case (PD_20V):
            LED_20V = !LED_20V;
            break;
        default:
            break;
    }
}

void timer0_int_handler()
{
    if (blink) {
        blink_handler();
    }    
}

void timer1_int_handler()
{
    if (blink) {
        blink_handler();
    }
}

UINT16 key_read()
{
    UINT16 k;
    
    /* 1: release; 0: pressed */
    k = (UINT16)(KEY == 0 ? 0 : 1);

    //printf("KEY: %02x\r\n", k);
    return k;    
}

/* uart-0 baudrate: 57600 */
main()
{
    UINT16 i, curr_key, prev_key;
    UINT16 curr_pd = pd_config_read();
    UINT32 loop = 0;

	CfgFsys();	
	mInitSTDIO();
    printf("\r\nPDTricker system buildtime [" __TIME__ " " __DATE__ "] " "rev     1.0\r\n");
    printf("curr_pd: %d\n", curr_pd);
    
    if (curr_pd == 255) {
        pd_config_update(PD_5V);
        curr_pd = pd_config_read();
        printf("factory init: curr_pd: %d\n", curr_pd);
    }
        
#if 0
    printf("T0 Test ...\n"); 
    mTimer0Clk12DivFsys();	                                                   // 12MHz / 12 = 1MHz
    mTimer_x_ModInit(0,2);                                                     // timer-0, mode-2
    mTimer_x_SetData(0,0xc8c8);	                                               // timer-0 data timer interrupt freq: 1MHz / 0xc8 = 5KHz, every 0.2ms
    mTimer0RunCTL(1);                                                          // timer-0 start
    ET0 = 1;                                                                   // 
    EA = 1;
#endif

	Port1Cfg(0, 0);
    ADCInit( 0 );

    prev_key = curr_key = 1;
    led_all_off();
    pd_set(curr_pd);

    i = 0;
    
#if 0
    while(1){
      for(i=0;i<128;i++){	                                                     //????128??		
        len = WriteDataFlash(i,&i,1);                                          //?DataFlash??????i??i
        if(len != 1){
          printf("Write Err ? = %02x,m = %02x\n",j,(UINT16)m);                //?????				
        }
      }
      for(i=0;i<128;i++){                                                      //?DataFlash??????i???
        len = ReadDataFlash(i,1,&m);
        if((len != 1) ||(m != i)){
          printf("Read Err ? = %02x, = %02x,addr =%02x ,?= %02x\n",j,(UINT16)(i*2),(UINT16)ROM_DATA_L,(UINT16)m);				
        }                                                                      //???????
      }  
      printf("$$OK %02x \n",j);			
      j++;
      mDelaymS(100);			
    }    
#endif
    
	while ( 1 ) {

        prev_key = curr_key;
        curr_key = key_read();

        if (prev_key == 1 && curr_key == 0) {
            led_off(curr_pd);
            curr_pd++;
            if (curr_pd == PD_MAX) {
                curr_pd = PD_5V;
            }

            printf("pd_set %d\n", curr_pd);
            pd_set(curr_pd);
            pd_config_update(curr_pd);

        }

        pd_verify(curr_pd);

        /* every 200ms */
        if (loop % 20 == 0) {
            if (blink) {
                blink_handler();
            }
        }

        mDelaymS(10);
        loop++;
        
	}
}