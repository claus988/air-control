/**
 * @file HT_Fsm.h
 * @brief Cabeçalho para a aplicação MQTT Subscriber.
 */

#ifndef __HT_FSM_H__
#define __HT_FSM_H__

#include "stdint.h"
#include "main.h"
#include "HT_MQTT_Api.h"
#include "MQTTFreeRTOS.h"
#include "bsp.h"
#include "cmsis_os2.h"
#include "MQTTClient.h"

/* Defines Essenciais ----------------------------------------------------------*/

#define HT_MQTT_KEEP_ALIVE_INTERVAL 240         /**</ Intervalo de Keep Alive em segundos. */
#define HT_MQTT_VERSION             4           /**</ Versão do protocolo MQTT. */

#if MQTT_TLS_ENABLE == 1
#define HT_MQTT_PORT                8883        /**</ Porta MQTT para conexão segura (TLS). */
#else
#define HT_MQTT_PORT                1883        /**</ Porta MQTT para conexão padrão. */
#endif

#define HT_MQTT_TIMEOUT             60000       /**</ Timeout para envio e recebimento MQTT em ms. */
#define HT_MQTT_BUFFER_SIZE         1024        /**</ Tamanho do buffer para MQTT. */

#define LED_ON  1                                   /**</ LED on. */
#define LED_OFF 0                                   /**</ LED off. */

/* Funções Públicas -----------------------------------------------------------*/

/*!******************************************************************
 * \fn void HT_Fsm(void)
 * \brief Função principal que inicia e gerencia a lógica do subscriber.
 * Conecta, se inscreve e processa mensagens recebidas.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
void HT_Fsm(void);

void HT_FSM_SetSubscribeBuff(uint8_t *buff, uint8_t payload_len);
void HT_GPIO_InitLed(void);

#endif /* __HT_FSM_H__ */

