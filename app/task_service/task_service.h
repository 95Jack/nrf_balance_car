#ifndef TASK_SERVICE_H__
#define TASK_SERVICE_H__
#include "nrf_delay.h"
#include "boards.h"

#define UART_TX_BUF_SIZE 256       //���ڷ��ͻ����С���ֽ�����
#define UART_RX_BUF_SIZE 256       //���ڽ��ջ����С���ֽ�����

void uart_config(void);
void mpu_run(void);

#endif

