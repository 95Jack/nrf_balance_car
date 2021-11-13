#include <stdbool.h>   
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "nrf_drv_clock.h"
#include "nrf_delay.h"
#include "sdk_errors.h"
#include "nrf_drv_twi.h"
#include "mpu6050.h"
#include "FreeRTOS.h"
#include "task.h"
#include "nrf_drv_clock.h"
#include "nrf_delay.h"
#include "sdk_errors.h"
#include "nrf_drv_twi.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
//Log��Ҫ���õ�ͷ�ļ�
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_uart.h"
#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif
#include "task_service.h"
#include "twi_master.h"

#define TASK_DELAY (2000)
ret_code_t err_code;

TaskHandle_t led_toggle_task_handle;
TaskHandle_t mpu6050_analysis_task_handle;

//static void led_toggle_task_function(void *pvParameter) {

//    while(1) {
//        nrf_gpio_pin_toggle(LED_1);
//        vTaskDelay(TASK_DELAY);
//    }
//}

//��־��ӡģ���ʼ��
static void log_init(void)
{
    //��ʼ��log����ģ��
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    //����log����նˣ�����sdk_config.h�е�������������ն�ΪUART����RTT��
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

//�����¼��ص��������ú������ж��¼����Ͳ����д���
void uart_error_handle(app_uart_evt_t * p_event)
{
    uint8_t cr;

    //ͨѶ�����¼�
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR) {
        
        APP_ERROR_HANDLER(p_event->data.error_communication);
    } else if (p_event->evt_type == APP_UART_FIFO_ERROR) {
            //FIFO�����¼�
            APP_ERROR_HANDLER(p_event->data.error_code);
    } else if (p_event->evt_type == APP_UART_DATA_READY) {
        //���ڽ����¼�
        //��תָʾ��D1״̬��ָʾ���ڽ����¼�
        nrf_gpio_pin_toggle(LED_1);
        //��FIFO�ж�ȡ����
        app_uart_get(&cr);
        //�����������
        printf("%c",cr);
    } else if (p_event->evt_type == APP_UART_TX_EMPTY) {
        //���ڷ�������¼�
        //��תָʾ��D2״̬��ָʾ���ڷ�������¼�
        nrf_gpio_pin_toggle(LED_2);
    }
}

//��������
void uart_config(void)
{
    uint32_t err_code;

    //���崮��ͨѶ�������ýṹ�岢��ʼ��
  const app_uart_comm_params_t comm_params =
  {
    RX_PIN_NUMBER,//����uart��������
    TX_PIN_NUMBER,//����uart��������
    RTS_PIN_NUMBER,//����uart RTS���ţ����عرպ���Ȼ������RTS��CTS���ţ����������������ԣ������������������ţ����������Կ���ΪIOʹ��
    CTS_PIN_NUMBER,//����uart CTS����
    APP_UART_FLOW_CONTROL_DISABLED,//�ر�uartӲ������
    false,//��ֹ��ż����
    NRF_UART_BAUDRATE_115200//uart����������Ϊ115200bps
  };
  //��ʼ�����ڣ�ע�ᴮ���¼��ص�����
  APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_error_handle,
                         APP_IRQ_PRIORITY_LOWEST,
                         err_code);

  APP_ERROR_CHECK(err_code);

}


/***********************************************************************************
 * ��  �� : ���ڷ������ݣ����ݸ�ʽΪ����������λ�����(V2.6�汾)���ݸ�ʽ
 * ��  �� : fun:������
 *        : dat:���ݻ�������ַ,���28�ֽ�
 *        : len:���ݳ��ȣ����28�ֽ�
 * ����ֵ : ��
 **********************************************************************************/ 
void Uart_SendDat_ToPC(uint8_t fun,uint8_t *dat,uint8_t len)
{
    uint8_t send_buf[32];
    uint8_t i;
    if(len>28)return;   //���28�ֽ����� 
    send_buf[len+3]=0;  //У��������
    send_buf[0]=0x88;   //֡ͷ
    send_buf[1]=fun;    //������
    send_buf[2]=len;    //���ݳ���
    for(i=0;i<len;i++)send_buf[3+i]=dat[i]; //��������
    for(i=0;i<len+3;i++)send_buf[len+3]+=send_buf[i];   //����У���
    for(i=0;i<len+4;i++)app_uart_put(send_buf[i]);  //�����������
}


/***********************************************************************************
 * ��  �� : ���ͼ��ٶȴ��������ݺ�����������
 * ��  �� : aacx,aacy,aacz:x,y,z������������ļ��ٶ�ֵ
 *          gyrox,gyroy,gyroz:x,y,z�������������������ֵ
 * ����ֵ : ��
 **********************************************************************************/
void mpu6050_send_dat(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz)
{
    uint8_t tx_buf[12]; 
    tx_buf[0]=(aacx>>8)&0xFF;
    tx_buf[1]=aacx&0xFF;
    tx_buf[2]=(aacy>>8)&0xFF;
    tx_buf[3]=aacy&0xFF;
    tx_buf[4]=(aacz>>8)&0xFF;
    tx_buf[5]=aacz&0xFF; 
    tx_buf[6]=(gyrox>>8)&0xFF;
    tx_buf[7]=gyrox&0xFF;
    tx_buf[8]=(gyroy>>8)&0xFF;
    tx_buf[9]=gyroy&0xFF;
    tx_buf[10]=(gyroz>>8)&0xFF;
    tx_buf[11]=gyroz&0xFF;
    Uart_SendDat_ToPC(0xA1,tx_buf,12);//�Զ���֡,0XA1
}


/***********************************************************************************
 * ��  �� : �����ϴ�MPU6050��̬����
 * ��  �� : aacx,aacy,aacz:x,y,z������������ļ��ٶ�ֵ
 *        : gyrox,gyroy,gyroz:x,y,z�������������������ֵ
 *        : roll:�����.��λ0.01�ȡ� -18000 -> 18000 ��Ӧ -180.00  ->  180.00��
 *        : pitch:������.��λ 0.01�ȡ�-9000 - 9000 ��Ӧ -90.00 -> 90.00 ��
 *        : yaw:�����.��λΪ0.1�� 0 -> 3600  ��Ӧ 0 -> 360.0��
 * ����ֵ : ��
 **********************************************************************************/ 
void Uart_ReportIMU(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz,short roll,short pitch,short yaw)
{
    uint8_t i,tx_buf[28]; 

    for(i=0;i<28;i++)tx_buf[i]=0;//��0
    tx_buf[0]=(aacx>>8)&0xFF;
    tx_buf[1]=aacx&0xFF;
    tx_buf[2]=(aacy>>8)&0xFF;
    tx_buf[3]=aacy&0xFF;
    tx_buf[4]=(aacz>>8)&0xFF;
    tx_buf[5]=aacz&0xFF; 
    tx_buf[6]=(gyrox>>8)&0xFF;
    tx_buf[7]=gyrox&0xFF;
    tx_buf[8]=(gyroy>>8)&0xFF;
    tx_buf[9]=gyroy&0xFF;
    tx_buf[10]=(gyroz>>8)&0xFF;
    tx_buf[11]=gyroz&0xFF;	
    tx_buf[18]=(roll>>8)&0xFF;
    tx_buf[19]=roll&0xFF;
    tx_buf[20]=(pitch>>8)&0xFF;
    tx_buf[21]=pitch&0xFF;
    tx_buf[22]=(yaw>>8)&0xFF;
    tx_buf[23]=yaw&0xFF;
    Uart_SendDat_ToPC(0xAF,tx_buf,28);//��������ɿ���ʾ֡,0xAF
}

static void mpu6050_analysis_task_function(void *pvParameter)
{
    double Temperature_centigrade;
    float pitch,roll,yaw;       //ŷ����
    int16_t AccValue[3];        //���ٶȴ�����ԭʼ����
    int16_t GyroValue[3];          //������ԭʼ����
    nrf_gpio_pin_set(LED_1);  
    while (mpu_dmp_init()) {
        NRF_LOG_INFO("mpu6050 init error %d",mpu_dmp_init());
        NRF_LOG_FLUSH();
        nrf_delay_ms(1000);
    }
    nrf_gpio_cfg_output(LED_1);
    while(true) {
        if(mpu_dmp_get_data(&pitch,&roll,&yaw) == 0)
        {
        
        //���ٶȴ�����ԭʼ����
        // Read acc value from mpu6050 internal registers and save them in the array
        if(MPU6050_ReadAcc(&AccValue[0], &AccValue[1], &AccValue[2]) == true) {
          // display the read values
          NRF_LOG_INFO("ACC Values:  x = %d  y = %d  z = %d",
            AccValue[0], AccValue[1], AccValue[2]); 
          NRF_LOG_FLUSH();
        } else {
          // if reading was unsuccessful then let the user know about it
          NRF_LOG_INFO("Reading ACC values Failed!!!");
          NRF_LOG_FLUSH();
        }
        
        //��ȡ����������
        // read the gyro values from mpu6050's internal registers and save them in another array
        if(MPU6050_ReadGyro(&GyroValue[0], &GyroValue[1], &GyroValue[2]) == true) 
        {
          // display then values
          NRF_LOG_INFO("GYRO Values: x = %d  y = %d  z = %d",
            GyroValue[0], GyroValue[1], GyroValue[2]);
          NRF_LOG_FLUSH();
        } else {
          NRF_LOG_INFO("Reading GYRO values Failed!!!");
          NRF_LOG_FLUSH();
        }

        mpu_6050_read_temp(&Temperature_centigrade);
        mpu6050_send_dat(AccValue[0],AccValue[1],AccValue[2],
        GyroValue[0],GyroValue[1],GyroValue[2]);//���Զ���֡���ͼ��ٶȺ�������ԭʼ����
        Uart_ReportIMU(
        AccValue[0],AccValue[1],AccValue[2],
        GyroValue[0],GyroValue[1],GyroValue[2],
        (int)(roll*100),(int)(pitch*100),(int)(yaw*10));
        nrf_delay_ms(50);
        }
    }
}

void mpu_run(void){

    uint8_t id;
    bsp_board_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS); // initialize the leds and buttons
    log_init();
    uart_config();
    nrf_drv_clock_init();
    twi_master_init(); // initialize the twi 
    nrf_delay_ms(2000); // give some delay
    while(mpu6050_init(0x68) == false) // wait until MPU6050 sensor is successfully initialized
    {
      NRF_LOG_INFO("MPU_6050 initialization failed!!!"); // if it failed to initialize then print a message
      NRF_LOG_FLUSH();
      nrf_delay_ms(800);
    }
    nrf_delay_ms(1000);
    NRF_LOG_INFO("mpu6050 init ok"); 
    mpu6050_register_read(0x75U, &id, 1);
    NRF_LOG_INFO("mpu6050 id is %d",id);
    NRF_LOG_FLUSH();

//    xTaskCreate(
//        led_toggle_task_function, "LED0", 
//        configMINIMAL_STACK_SIZE*2, NULL, 2, 
//        &led_toggle_task_handle
//    );

    xTaskCreate(
        mpu6050_analysis_task_function, "MPU6050", 
        configMINIMAL_STACK_SIZE*2 + 1, NULL, 3, 
        &mpu6050_analysis_task_handle
    );
    vTaskStartScheduler();
    
}



