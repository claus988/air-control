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
#include "HT_PWM.h"


/** \brief TIMER instance for PWM output */
#define PWM_INSTANCE                     (1)

/** \brief PWM clock id for clock configuration */
#define PWM_CLOCK_ID                     (GPR_TIMER0FuncClk)

/** \brief PWM clock source select */
#define PWM_CLOCK_SOURCE                 (GPR_TIMER0ClkSel_26M )

/** \brief PWM initial duty cycle percent */
#define PWM_INIT_DUTY_CYCLE_PERCENT      (50)

/** \brief TIMER/PWM out pin */
#define PWM_OUT_PIN                      (13)

/** \brief TIMER/PWM Alt function select */
#define PWM_OUT_ALT_SEL                  (PAD_MuxAlt5)

/** \brief TIMER instance for PWM duty cycle control */
#define TIMER_INSTANCE                   (1)

/** \brief TIMER clock id for clock configuration */
#define TIMER_CLOCK_ID                   (GPR_TIMER1FuncClk)

/** \brief TIMER clock source select */
#define TIMER_CLOCK_SOURCE               (GPR_TIMER1ClkSel_32K)

/** \brief TIMER instance IRQ number */
#define TIMER_INSTANCE_IRQ               (PXIC_Timer1_IRQn)

volatile int update = 0;

#define SIGNAL_LEN 197

#define LED_INSTANCE             0                /**</ LED pin instance. */
#define LED_GPIO_PIN             2                  /**</ LED pin number. */
#define LED_PAD_ID               13                 /**</ LED Pad ID. */
#define LED_PAD_ALT_FUNC         PAD_MuxAlt0        /**</ LED pin alternate function. */

#define  LED_OFF 0

#define  CUSTOM_DELAY_US_CALIBRATION 1.2

void custom_delay_usec(uint32_t us)
{
  float countings = us * CUSTOM_DELAY_US_CALIBRATION;
  for(uint32_t fh = 0; fh < countings; fh++)
    __asm volatile("NOP");
}


void Timer_ISR()
{
    if (TIMER_GetInterruptFlags(TIMER_INSTANCE) & TIMER_Match0InterruptFlag)
    {
        update = 1;
        TIMER_ClearInterruptFlags(TIMER_INSTANCE, TIMER_Match0InterruptFlag);
    }
}

void pwm_config(void)
{
     //unsigned int dutyCycle = PWM_INIT_DUTY_CYCLE_PERCENT;

    //  printf("TIMER Example Start\r\n");

    //  PWM out pin config
    pad_config_t config;

    PAD_GetDefaultConfig(&config);
    config.mux = PWM_OUT_ALT_SEL;
    PAD_SetPinConfig(PWM_OUT_PIN, &config);

     // Config PWM clock, source from 26MHz and divide by 1
    CLOCK_SetClockSrc(PWM_CLOCK_ID, PWM_CLOCK_SOURCE);
    CLOCK_SetClockDiv(PWM_CLOCK_ID, 1);

    TIMER_DriverInit();

    timer_pwm_config_t pwmConfig;
    pwmConfig.pwmFreq_HZ = 38000;
    pwmConfig.srcClock_HZ = GPR_GetClockFreq(PWM_CLOCK_ID);  // 26MHz
    pwmConfig.dutyCyclePercent = PWM_INIT_DUTY_CYCLE_PERCENT;

    TIMER_SetupPwm(PWM_INSTANCE, &pwmConfig);
    
}

void start_pwm(uint16_t time)
{
//     pwm_config();
    //TIMER_Start(PWM_INSTANCE);
    HT_GPIO_WritePin(LED_GPIO_PIN, LED_INSTANCE, LED_ON);   
    // if(time > 0) custom_delay_usec(time);
    if(time > 0) custom_delay_usec(time);
  
}

void stop_pwm(uint16_t time)
{
    //TIMER_Stop(PWM_INSTANCE);
    HT_GPIO_WritePin(LED_GPIO_PIN, LED_INSTANCE, LED_OFF);   
    // if(time > 0) custom_delay_usec(time);
    if(time > 0) custom_delay_usec(time);

}

