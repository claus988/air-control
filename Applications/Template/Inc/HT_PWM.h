#include <stdio.h>
#include "timer_qcx212.h"
#include "bsp.h"
#include "stdint.h"
#include "stddef.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timer_qcx212.h"
#include "bsp.h"
#include <stdint.h>
#define USE_USART_DEBUG
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "slpman_qcx212.h"
#include "pad_qcx212.h"
#include "HT_gpio_qcx212.h"
#include "ic_qcx212.h"
#include "HT_ic_qcx212.h"
#include "HT_adc_qcx212.h"
#include "adc_qcx212.h"
#include "hal_adc.h"
#include <stdio.h>
#include "string.h"
#include "HT_bsp.h"
#include "stdint.h"
#include "qcx212.h"
#include "HT_Fsm.h"

void custom_delay_usec(uint32_t us);
void pwm_config(void);
void delay_us(uint32_t us);
void stop_pwm(uint16_t time);
void start_pwm(uint16_t time);
//void vPwm(void);
//void envPwm(void *arg);

//void envPwm(void *arg);

void Timer_ISR();



