/**
 * @file HT_Fsm.c
 * @brief Aplicação MQTT com arquitetura de múltiplas tarefas para processar mensagens de diferentes tópicos.
 */

#include "HT_Fsm.h"
#include "HT_PWM.h"

//GPIO3 - LED
// #define LED_INSTANCE             0                /**</ LED pin instance. */
// #define LED_GPIO_PIN             6//2                  /**</ LED pin number. */
// #define LED_PAD_ID               16//13                 /**</ LED Pad ID. */
// #define LED_PAD_ALT_FUNC         PAD_MuxAlt0        /**</ LED pin alternate function. */



void HT_GPIO_InitLed(void) {
  GPIO_InitType GPIO_InitStruct = {0};

  GPIO_InitStruct.af = PAD_MuxAlt0;
  GPIO_InitStruct.pad_id = LED_PAD_ID;
  GPIO_InitStruct.gpio_pin = LED_GPIO_PIN;
  GPIO_InitStruct.pin_direction = GPIO_DirectionOutput;
  GPIO_InitStruct.init_output = 1;
  GPIO_InitStruct.pull = PAD_AutoPull;
  GPIO_InitStruct.instance = LED_INSTANCE;
  GPIO_InitStruct.exti = GPIO_EXTI_DISABLED;

  HT_GPIO_Init(&GPIO_InitStruct);
}

/* --- Configurações da Aplicação --- */
static const char BROKER_ADDR[]      = "131.255.82.115";
static const char CLIENT_ID[]        = "HTNB32L/dcamSubscriber22";

// Tópicos que vamos ouvir
// static const char SUBSCRIBE_TOPIC_ESTADO[]      = "hana/externo/aircontrol/01/power";
static const char SUBSCRIBE_TOPIC_TEMPERATURA[] = "hana/externo/aircontrol/01/temperature";

/* --- Variáveis de Controle e Handles --- */
static MQTTClient mqttClient;
static Network mqttNetwork;
static uint8_t mqttSendbuf[HT_MQTT_BUFFER_SIZE] = {0};
static uint8_t mqttReadbuf[HT_MQTT_BUFFER_SIZE] = {0};

// Handles para as nossas "Caixas de Entrada" (Filas)
// static osMessageQueueId_t estadoQueueHandle;
static osMessageQueueId_t temperaturaQueueHandle;

/* --- Protótipos de Funções --- */
static void OnMessageReceived_Dispatcher(MessageData *msg);
static void YieldTask(void *arg);
// static void EstadoTask(void *arg);
static void TemperaturaTask(void *arg);
void vPwmTask(uint16_t buf[], unsigned int len);
#define SIGNAL_LEN 197


// variaveis do pwm
//ON 24
uint16_t LIGAR_24[197] = {6080,7420, 530,1670, 530,1620, 530,1670, 530,1670, 480,1720, 530,1620, 580,1620, 580,1620, 580,570, 530,570, 530,570, 580,520, 580,520, 580,520, 580,570, 530,570, 530,1670, 530,1670, 530,1670, 530,1670, 480,1670, 530,1620, 580,1620, 580,1620, 530,570, 530,570, 530,620, 530,570, 530,620, 530,570, 530,570, 480,670, 530,1670, 480,1720, 480,1720, 480,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,570, 580,520, 580,570, 530,620, 430,670, 530,620, 530,570, 580,570, 530,1670, 530,570, 580,570, 430,1770, 430,720, 480,620, 530,1670, 480,1720, 480,620, 530,1670, 530,1670, 480,570, 530,1670, 530,1670, 530,570, 530,570, 530,1670, 530,1670, 530,1670, 430,670, 430,1770, 430,620, 480,1720, 530,1620, 530,570, 530,620, 530,570, 530,1670, 530,570, 530,1670, 430,670, 480,570, 530,620, 530,1670, 530,570, 530,1670, 530,570, 530,1670, 530,570, 530,570, 530,1670, 530,520, 580,1620, 530,570, 530,1670, 530,620, 480,1670, 530,1720, 480,7420, 530};
//OFF 24A
uint16_t DESLIGAR_24[197] = {6030,7470, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 480,1720, 480,1720, 530,570, 480,670, 430,670, 480,620, 530,570, 580,570, 530,570, 530,570, 530,1670, 530,1670, 530,1670, 530,1620, 530,1620, 580,1620, 580,1620, 530,1670, 530,570, 580,570, 530,570, 480,670, 530,620, 530,570, 580,570, 530,570, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,570, 580,520, 580,520, 580,520, 530,620, 480,620, 480,670, 530,570, 530,1670, 530,1620, 530,570, 530,1620, 430,670, 430,670, 480,1670, 530,1670, 530,620, 430,670, 430,1720, 480,620, 480,1670, 480,1770, 430,670, 430,670, 430,1770, 480,1720, 480,1670, 530,620, 530,1620, 530,620, 530,1670, 530,1670, 530,570, 530,570, 530,570, 530,1670, 530,620, 480,1670, 480,670, 480,620, 480,620, 530,1670, 480,620, 530,1670, 480,620, 530,1670, 480,670, 480,670, 480,1720, 480,620, 480,1720, 480,620, 480,1720, 480,620, 480,1720, 480,1720, 480,7470, 480};  
//ON 16
uint16_t GELAR_16[197] = {6080,7420, 530,1670, 530,1670, 480,1720, 530,1670, 530,1620, 530,1670, 530,1670, 480,1670, 530,620, 530,570, 530,570, 530,570, 530,570, 530,570, 580,520, 530,620, 530,1670, 530,1670, 530,1620, 530,1670, 530,1670, 530,1620, 530,1670, 530,1670, 530,570, 530,570, 530,620, 530,570, 530,570, 480,670, 480,620, 480,670, 530,1670, 480,1720, 480,1670, 480,1720, 530,1620, 530,1670, 530,1620, 580,1620, 530,620, 530,570, 530,570, 580,570, 530,570, 580,520, 580,520, 530,620, 480,1720, 430,720, 480,620, 580,1620, 530,620, 530,570, 580,1620, 530,1670, 530,620, 530,1670, 530,1670, 530,570, 530,1670, 530,1670, 530,620, 480,620, 530,1670, 530,1670, 530,1670, 530,1670, 480,1720, 480,620, 480,1720, 480,1720, 480,620, 480,620, 480,620, 480,620, 480,620, 530,1720, 480,620, 480,670, 480,620, 480,1720, 480,620, 480,1720, 480,670, 430,1720, 430,720, 430,620, 480,1720, 480,620, 480,1720, 480,620, 480,1720, 480,620, 480,1720, 480,1720, 430,7520, 430};  
//ON 17
uint16_t GELAR_17[197] = {6130,7370, 530,1670, 530,1620, 530,1620, 530,1670, 530,1670, 480,1720, 480,1670, 530,1670, 530,570, 580,570, 530,570, 530,570, 530,570, 530,570, 580,520, 530,620, 530,1670, 530,1620, 580,1620, 530,1670, 480,1720, 430,1720, 530,1670, 530,1620, 530,620, 530,570, 530,570, 530,570, 530,620, 480,620, 530,570, 480,670, 530,1670, 430,1770, 430,1770, 530,1620, 530,1670, 530,1670, 530,1670, 530,1670, 530,570, 580,520, 580,570, 530,570, 530,570, 530,570, 530,620, 530,620, 530,1670, 530,570, 580,570, 530,1670, 530,570, 530,620, 480,1720, 480,1720, 480,670, 430,1770, 430,1770, 430,620, 480,1720, 480,1720, 480,670, 480,620, 480,620, 530,1670, 530,1670, 530,1670, 480,1720, 480,620, 480,1720, 480,1670, 480,1720, 480,620, 530,620, 480,620, 480,670, 480,1720, 480,620, 480,670, 480,670, 480,1720, 480,620, 480,1720, 480,620, 480,1720, 480,620, 480,670, 480,1720, 480,620, 480,1720, 480,620, 480,1670, 480,670, 480,1720, 480,1670, 480,7470, 480};  
//ON 18
uint16_t GELAR_18[197] = {5980,7520, 530,1670, 530,1670, 530,1670, 480,1720, 480,1670, 530,1670, 530,1670, 530,1670, 530,570, 580,570, 530,570, 530,570, 580,520, 580,520, 580,570, 530,570, 530,1670, 530,1670, 530,1670, 530,1670, 480,1670, 480,1720, 530,1620, 580,1620, 580,570, 530,570, 530,570, 580,520, 580,520, 530,620, 530,570, 530,620, 430,1720, 530,1670, 530,1670, 530,1670, 530,1620, 580,1620, 530,1670, 530,1620, 480,670, 530,570, 530,570, 580,570, 530,570, 580,520, 580,520, 580,570, 480,1720, 480,620, 480,670, 480,1720, 480,620, 530,620, 530,1670, 530,1670, 530,570, 530,1670, 530,1670, 530,620, 530,1620, 580,1620, 530,570, 530,570, 530,1670, 530,570, 530,1670, 530,1670, 480,1720, 480,620, 480,1720, 480,1670, 480,670, 480,1720, 480,620, 480,620, 480,620, 480,1720, 480,620, 480,620, 480,670, 480,1720, 480,620, 480,1720, 480,620, 480,1720, 480,620, 480,620, 480,1720, 480,620, 480,1720, 480,670, 430,1720, 480,670, 480,1720, 480,1720, 480,7470, 480};  
//ON 19
uint16_t GELAR_19[197] = {6080,7370, 580,1620, 580,1620, 580,1620, 480,1720, 480,1720, 530,1670, 530,1620, 580,1620, 580,570, 530,570, 530,620, 530,520, 580,570, 530,570, 530,620, 530,570, 480,1720, 530,1670, 530,1620, 580,1620, 580,1620, 580,1620, 580,1620, 580,1620, 580,570, 530,570, 580,520, 580,570, 530,620, 480,620, 530,620, 530,570, 580,1620, 580,1620, 480,1720, 430,1720, 530,1670, 530,1670, 530,1670, 530,1620, 530,620, 530,570, 530,520, 580,570, 480,670, 480,620, 580,520, 580,570, 530,1670, 530,570, 530,570, 530,1670, 530,570, 530,570, 530,1670, 530,1620, 530,570, 580,1620, 580,1620, 580,570, 530,1670, 530,1670, 530,570, 480,620, 480,670, 480,620, 480,1670, 430,1720, 530,1670, 530,570, 480,1720, 430,1770, 480,1720, 480,1720, 480,620, 480,620, 480,620, 480,1670, 480,670, 480,620, 480,620, 530,1670, 530,620, 530,1620, 530,620, 480,1720, 480,620, 480,620, 480,1720, 480,620, 480,1670, 480,670, 480,1670, 480,670, 480,1670, 480,1720, 480,7470, 480};  
//ON 20
uint16_t GELAR_20[197] = {6080,7420, 580,1620, 580,1620, 580,1620, 580,1620, 580,1620, 530,1670, 480,1720, 430,1770, 530,620, 430,620, 530,620, 430,670, 480,620, 480,670, 480,620, 530,570, 580,1620, 580,1620, 430,1720, 530,1670, 530,1620, 530,1670, 530,1670, 530,1670, 530,570, 580,570, 530,570, 530,570, 530,620, 480,620, 480,670, 530,570, 580,1620, 580,1620, 580,1620, 580,1620, 580,1620, 530,1670, 530,1670, 480,1720, 480,620, 530,570, 530,570, 530,570, 530,570, 530,570, 530,620, 530,570, 530,1670, 530,570, 530,570, 530,1670, 480,620, 530,620, 530,1670, 530,1620, 530,620, 530,1620, 480,1670, 530,570, 480,1720, 480,1720, 480,620, 480,620, 480,1720, 480,1720, 430,670, 430,1720, 480,1720, 480,620, 480,1670, 530,1670, 480,670, 480,620, 480,1670, 480,670, 480,620, 480,1720, 530,570, 480,620, 480,620, 530,1620, 530,620, 480,1670, 530,570, 530,1670, 530,570, 530,570, 530,1670, 480,620, 480,1720, 480,620, 480,1720, 480,620, 480,1670, 480,1720, 480,7470, 480};  
//ON 21
uint16_t GELAR_21[197] = {6080,7420, 530,1620, 580,1620, 580,1620, 580,1620, 580,1620, 480,1670, 530,1670, 530,1620, 580,570, 530,570, 580,520, 580,570, 480,620, 480,620, 480,620, 480,620, 480,1720, 530,1620, 580,1620, 530,1670, 530,1670, 530,1620, 530,1670, 430,1720, 530,620, 430,670, 430,670, 580,570, 580,570, 530,570, 530,570, 530,570, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,1620, 530,1670, 530,1670, 530,570, 580,570, 530,620, 430,670, 480,620, 480,620, 580,520, 530,570, 530,1670, 530,620, 530,570, 530,1670, 530,520, 580,520, 580,1620, 530,1670, 530,570, 530,1670, 530,1670, 530,570, 480,1670, 530,1670, 530,570, 480,670, 530,620, 530,1670, 530,570, 530,1620, 580,1620, 580,570, 530,1670, 480,1720, 430,1770, 480,620, 480,1720, 530,570, 530,570, 530,1620, 530,570, 530,570, 530,620, 530,1670, 530,570, 530,1620, 580,520, 530,1670, 530,570, 480,670, 480,1720, 480,570, 530,1670, 480,620, 480,1670, 480,620, 480,1670, 480,1720, 480,7470, 480};  
//ON 22
uint16_t GELAR_22[197] = {6130,7370, 480,1720, 530,1620, 580,1620, 530,1620, 480,1720, 480,1670, 530,1620, 530,1620, 530,620, 530,570, 580,570, 530,620, 530,570, 530,570, 480,670, 480,620, 580,1620, 530,1670, 430,1770, 430,1770, 430,1720, 530,1670, 580,1620, 530,1670, 530,570, 530,570, 530,570, 530,620, 430,670, 480,620, 580,570, 530,570, 530,1670, 530,1670, 530,1620, 530,1670, 480,1670, 480,1670, 530,1620, 580,1620, 530,570, 530,570, 530,570, 530,570, 530,570, 530,620, 480,620, 480,620, 480,1720, 530,570, 530,570, 530,1620, 530,620, 530,570, 530,1620, 530,1670, 530,570, 580,1620, 530,1670, 530,570, 530,1670, 530,1670, 530,620, 480,620, 480,1720, 480,620, 480,620, 530,1670, 530,1670, 530,570, 530,1620, 580,1620, 530,620, 480,1720, 480,1720, 480,620, 480,620, 480,1670, 480,620, 480,620, 480,670, 480,1720, 480,620, 480,1670, 530,570, 530,1620, 530,570, 530,620, 480,1720, 480,620, 480,1670, 480,620, 480,1720, 480,620, 480,1720, 480,1670, 480,7470, 480};  
//ON 23
uint16_t GELAR_23[197] = {6080,7420, 480,1720, 530,1670, 530,1670, 530,1620, 580,1620, 480,1670, 530,1620, 580,1620, 530,570, 530,620, 530,570, 530,570, 530,570, 530,570, 530,570, 530,570, 530,1670, 530,1670, 530,1670, 530,1620, 580,1620, 580,1620, 530,1670, 480,1720, 480,620, 580,520, 530,620, 530,570, 530,620, 480,670, 430,670, 430,670, 480,1720, 480,1670, 480,1720, 530,1620, 530,1670, 530,1620, 580,1620, 530,1620, 530,570, 580,570, 530,570, 530,570, 530,570, 530,570, 530,570, 480,670, 430,1770, 430,670, 530,570, 530,1670, 480,570, 580,570, 530,1670, 530,1670, 530,570, 580,1620, 580,1620, 530,570, 530,1670, 530,1620, 580,570, 480,620, 430,720, 530,570, 580,520, 580,1620, 530,1670, 530,620, 480,1720, 480,1720, 430,1770, 480,1670, 430,1770, 480,670, 530,520, 580,1620, 530,570, 530,570, 530,570, 580,1620, 530,570, 530,1670, 530,620, 480,1670, 480,620, 480,670, 530,1670, 530,570, 530,1620, 580,570, 530,1670, 480,670, 480,1670, 480,1720, 480,7420, 530};  
//ON 24
uint16_t GELAR_24[197] = {6080,7420, 430,1770, 480,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,570, 580,570, 530,570, 530,570, 530,570, 580,520, 580,520, 580,570, 530,1670, 530,1620, 580,1620, 580,1620, 580,1620, 580,1620, 530,1670, 480,1720, 480,620, 530,570, 530,570, 530,570, 530,570, 530,570, 580,520, 580,520, 580,1620, 580,1620, 530,1670, 530,1620, 530,1620, 530,1670, 430,1720, 530,1670, 480,670, 480,620, 480,620, 530,570, 530,570, 530,570, 530,570, 530,570, 530,1670, 530,570, 580,520, 580,1620, 530,570, 530,570, 480,1720, 480,1720, 480,670, 530,1670, 530,1670, 530,570, 530,1670, 530,1670, 530,620, 480,620, 480,1720, 480,1720, 480,1720, 480,620, 480,1720, 480,620, 480,1670, 530,1670, 480,670, 480,620, 480,620, 480,1720, 480,620, 480,1670, 480,620, 480,620, 480,620, 480,1770, 480,620, 480,1670, 480,620, 480,1670, 480,620, 480,670, 480,1720, 480,620, 480,1720, 430,670, 480,1720, 480,670, 480,1670, 480,1720, 480,7420, 480};  
//ON 25
uint16_t GELAR_25[197] = {6080,7420, 480,1720, 430,1770, 530,1620, 580,1620, 580,1620, 580,1620, 580,1620, 580,1620, 580,570, 530,570, 530,570, 580,570, 530,570, 530,620, 430,670, 530,620, 530,1670, 530,1620, 480,1720, 430,1770, 480,1670, 530,1670, 530,1670, 530,1670, 530,570, 580,570, 530,570, 430,720, 480,620, 530,570, 580,520, 580,520, 580,1620, 530,1670, 530,1670, 530,1620, 480,1670, 530,1670, 530,1620, 580,1620, 530,570, 530,570, 530,620, 480,620, 480,620, 530,570, 530,570, 530,570, 530,1670, 530,570, 580,520, 580,1620, 530,570, 530,570, 530,1670, 530,1670, 530,620, 480,1720, 430,1770, 430,670, 430,1720, 480,1720, 430,670, 530,570, 580,570, 530,1670, 530,1670, 530,570, 530,1670, 530,570, 480,1770, 430,1720, 480,1720, 530,570, 530,570, 530,1670, 480,620, 480,1670, 480,620, 480,620, 530,570, 580,1620, 530,570, 530,1620, 580,520, 580,1620, 580,570, 480,620, 480,1720, 480,620, 530,1670, 530,570, 530,1670, 530,570, 530,1670, 530,1670, 530,7420, 530};  
//ON 26
uint16_t GELAR_26[197] = {6080,7420, 530,1670, 480,1720, 430,1720, 530,1620, 580,1620, 530,1620, 480,1720, 480,1720, 530,620, 430,670, 530,620, 530,570, 530,570, 530,570, 580,570, 530,570, 530,1670, 530,1670, 530,1670, 530,1620, 580,1620, 580,1620, 580,1620, 580,1620, 580,570, 530,570, 530,570, 530,570, 580,520, 530,620, 480,620, 480,670, 530,1670, 530,1620, 480,1670, 530,1620, 580,1620, 530,1620, 530,1670, 530,1670, 530,620, 530,570, 530,520, 580,520, 580,520, 580,520, 580,520, 530,570, 530,1720, 430,670, 430,670, 430,1720, 530,620, 530,570, 530,1620, 530,1670, 530,620, 530,1670, 530,1670, 530,570, 530,1670, 530,1670, 480,670, 480,620, 480,1720, 480,620, 480,1670, 530,570, 530,1620, 530,620, 480,1720, 480,1720, 480,620, 480,1720, 480,670, 480,1670, 480,620, 480,1720, 480,620, 480,620, 480,670, 480,1720, 430,620, 480,1720, 480,620, 480,1670, 480,620, 480,670, 430,1720, 480,620, 480,1720, 480,670, 480,1720, 480,620, 430,1770, 430,1720, 480,7470, 480};  
//ON 27
uint16_t GELAR_27[197] = {6130,7370, 480,1720, 480,1720, 530,1620, 580,1620, 530,1670, 530,1670, 530,1620, 430,1770, 530,620, 430,670, 480,620, 430,670, 480,620, 480,620, 530,620, 530,570, 580,1620, 530,1670, 530,1670, 530,1670, 530,1620, 580,1620, 580,1620, 580,1620, 530,620, 530,570, 580,570, 530,570, 480,670, 430,670, 430,670, 530,570, 530,1670, 530,1670, 430,1720, 530,1620, 530,1620, 580,1620, 580,1620, 580,1620, 530,570, 530,570, 530,570, 530,570, 530,570, 530,570, 530,570, 480,620, 430,1770, 430,670, 480,620, 530,1670, 530,570, 580,520, 580,1620, 530,1670, 530,570, 580,1620, 530,1620, 580,570, 530,1670, 530,1670, 530,620, 430,670, 480,620, 530,570, 530,1670, 530,520, 580,1620, 530,570, 530,1620, 530,1670, 530,1670, 530,1670, 530,570, 580,1620, 530,570, 530,1620, 530,570, 530,570, 530,620, 430,1770, 480,620, 480,1670, 480,620, 480,1670, 480,620, 530,570, 530,1670, 530,570, 530,1670, 530,570, 530,1670, 480,670, 480,1720, 480,1720, 480,7470, 480};  
//ON 28
uint16_t GELAR_28[197] = {6030,7470, 530,1720, 480,1670, 530,1670, 530,1620, 530,1670, 480,1620, 580,1620, 580,1620, 580,570, 530,570, 580,520, 580,520, 580,520, 580,520, 580,570, 480,620, 430,1770, 480,1720, 480,1720, 480,1720, 530,1620, 530,1670, 530,1670, 530,1670, 530,620, 530,570, 530,570, 530,570, 530,620, 480,620, 480,620, 580,520, 580,1620, 580,1620, 480,1670, 430,1770, 430,1720, 530,1670, 530,1670, 530,1620, 580,570, 480,620, 430,670, 430,720, 480,620, 530,570, 530,520, 580,520, 580,1620, 580,520, 580,570, 530,1670, 530,570, 480,670, 430,1720, 480,1720, 430,670, 530,1670, 480,1720, 480,620, 530,1670, 530,1670, 530,620, 530,520, 580,1620, 580,1620, 530,570, 530,570, 530,1670, 480,670, 480,1670, 430,1770, 480,620, 580,570, 530,1620, 530,1670, 530,570, 530,1670, 530,620, 480,570, 480,670, 530,1670, 530,570, 530,1620, 580,520, 580,1620, 530,620, 530,570, 530,1720, 480,620, 480,1720, 480,570, 530,1670, 480,620, 480,1720, 480,1720, 480,7420, 480};  
//ON 29
uint16_t GELAR_29[197] = {6030,7420, 530,1670, 530,1670, 530,1670, 530,1620, 480,1720, 480,1670, 580,1620, 580,1620, 580,570, 530,570, 530,620, 430,670, 530,620, 530,570, 530,620, 530,570, 530,1670, 530,1670, 530,1670, 530,1620, 580,1620, 580,1620, 580,1620, 580,1620, 580,570, 530,570, 530,570, 580,570, 480,620, 430,670, 480,670, 530,570, 530,1670, 530,1620, 480,1670, 530,1620, 580,1620, 580,1620, 530,1670, 530,1670, 530,570, 530,570, 530,570, 480,620, 480,620, 480,620, 480,620, 480,620, 530,1670, 530,570, 530,620, 530,1620, 580,570, 530,570, 530,1620, 530,1670, 530,620, 480,1720, 480,1670, 480,620, 480,1720, 480,1720, 480,620, 530,570, 530,620, 480,1720, 480,620, 480,620, 480,1720, 480,620, 480,1720, 480,1670, 480,1720, 480,620, 480,1720, 480,1720, 480,670, 480,1670, 480,620, 480,620, 480,670, 480,1670, 480,620, 480,1770, 430,670, 430,1720, 430,670, 480,620, 480,1720, 480,670, 480,1720, 430,620, 480,1720, 480,620, 480,1720, 430,1770, 430,7520, 430};
//ON 30
uint16_t GELAR_30[197] = {6080,7420, 530,1670, 530,1670, 530,1620, 580,1620, 430,1720, 530,1670, 530,1670, 530,1670, 530,570, 530,570, 530,570, 530,620, 430,720, 480,620, 480,620, 480,620, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 480,1720, 480,1720, 480,1720, 480,620, 530,570, 530,570, 530,620, 530,570, 530,520, 580,520, 580,520, 580,1620, 580,1620, 530,1620, 580,1620, 480,1670, 430,1720, 530,1620, 580,1620, 530,570, 580,520, 530,570, 530,570, 530,570, 480,620, 480,620, 480,620, 480,1720, 530,570, 530,570, 530,1670, 480,620, 480,620, 480,1670, 480,1720, 480,620, 530,1670, 480,1720, 480,620, 530,1670, 480,1720, 480,620, 480,620, 480,1720, 480,620, 480,620, 480,620, 530,1670, 480,670, 480,1670, 480,1720, 480,670, 480,1720, 480,1670, 480,1720, 480,620, 480,1670, 480,620, 480,670, 480,670, 480,1670, 480,620, 480,1720, 480,620, 480,1720, 480,670, 480,670, 430,1770, 430,620, 480,1770, 430,670, 430,1770, 430,670, 430,1720, 430,1770, 430,7520, 430};  
//ON 31
uint16_t GELAR_31[197] = {6080,7370, 530,1670, 580,1620, 580,1620, 580,1620, 530,1670, 530,1670, 530,1620, 480,1670, 530,620, 480,670, 480,620, 530,570, 530,570, 530,620, 530,570, 530,620, 530,1670, 530,1620, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,1670, 530,570, 580,520, 580,570, 530,570, 530,570, 530,570, 530,620, 530,570, 530,1670, 530,1620, 480,1670, 580,1620, 530,1620, 530,1670, 530,1670, 530,1620, 580,570, 530,570, 530,570, 530,570, 530,570, 530,570, 530,570, 530,570, 530,1670, 480,620, 530,570, 530,1670, 480,620, 580,520, 580,1620, 530,1670, 530,570, 530,1620, 580,1620, 580,570, 530,1670, 530,1670, 530,570, 530,570, 530,620, 480,620, 530,570, 530,570, 580,1620, 530,570, 530,1670, 530,1670, 480,1720, 480,1720, 480,1720, 430,1770, 480,620, 480,1670, 480,620, 480,620, 530,570, 580,1620, 580,520, 530,1670, 530,570, 530,1620, 580,520, 580,520, 530,1670, 480,620, 480,1720, 480,620, 530,1670, 480,620, 480,1720, 530,1620, 530,7420, 480};  
//ON 32
uint16_t GELAR_32[197] = {6080,7420, 480,1720, 430,1770, 530,1620, 580,1620, 530,1620, 530,1670, 430,1720, 530,1620, 530,620, 530,570, 530,570, 530,570, 530,620, 480,620, 480,620, 480,620, 480,1720, 480,1720, 480,1720, 530,1620, 530,1670, 530,1670, 530,1670, 530,1670, 530,570, 580,570, 480,670, 430,670, 530,570, 530,620, 530,570, 530,570, 530,1670, 530,1670, 530,1670, 530,1620, 530,1620, 480,1670, 530,1670, 530,1620, 530,620, 480,670, 430,670, 480,570, 530,570, 530,570, 530,570, 530,570, 530,1670, 530,570, 530,570, 530,1670, 530,620, 480,620, 480,1670, 530,1670, 480,620, 530,1670, 480,1720, 480,620, 530,1720, 480,1670, 530,620, 530,570, 530,1670, 530,1620, 530,1670, 530,1670, 530,570, 530,570, 530,1620, 530,1670, 530,620, 530,570, 480,620, 480,620, 430,1770, 480,1720, 480,620, 480,620, 530,570, 530,1670, 530,570, 530,1620, 530,570, 530,1670, 530,620, 480,670, 480,1670, 580,520, 580,1620, 530,570, 530,1670, 530,570, 530,1620, 580,1620, 480,7470, 480};  




/**
 * @brief A "Recepcionista". Chamada para TODAS as mensagens.
 * Ela verifica o tópico e encaminha a mensagem para a fila correta.
 */
static void OnMessageReceived_Dispatcher(MessageData *msg) {
    if (msg == NULL || msg->message == NULL || msg->message->payload == NULL) return;

    // Verifica se a mensagem é do tópico de ESTADO
    // if (strncmp(msg->topicName->lenstring.data, SUBSCRIBE_TOPIC_ESTADO, msg->topicName->lenstring.len) == 0) {
    //     // Envia o payload para a fila da tarefa de estado
    //     uint8_t teste[100] = {0}; 
    //     memcpy(teste,msg->message->payload,msg->message->payloadlen);
    //     // printf("SUBSCRIBE_TOPIC_ESTADO: %s\n",teste);
    //     osMessageQueuePut(estadoQueueHandle, teste, 0, 0);
    //     memset(msg->message->payload,0,msg->message->payloadlen);
    // } 
    // Verifica se a mensagem é do tópico de TEMPERATURA
    else if (strncmp(msg->topicName->lenstring.data, SUBSCRIBE_TOPIC_TEMPERATURA, msg->topicName->lenstring.len) == 0) {
        // Envia o payload para a fila da tarefa de temperatura
        char teste[10] = {0};
        memcpy(teste,msg->message->payload,msg->message->payloadlen);

        int numero = 0;
        // printf("SUBSCRIBE_TOPIC_ESTADO: %s\n",teste);
        // printf("SUBSCRIBE_TOPIC_TEMPERATURA: %s\n",msg->message->payload);
        // osMessageQueuePut(temperaturaQueueHandle, msg->message->payload, 0, 0);

            if((strncmp(teste,"16", sizeof("16"))) == 0){
                numero = 16;
                printf("chegou dezesseis\n");
            }
            if((strncmp(teste,"17", sizeof("17"))) == 0){
                numero = 17;
                printf("chegou dezessete\n");
            }
            if((strncmp(teste,"18", sizeof("18"))) == 0){
                numero = 18;
                printf("chegou dezoito\n");
            }
            if((strncmp(teste,"19", sizeof("19"))) == 0){
                numero = 19;
                printf("chegou dezenove\n");
            }
            if((strncmp(teste,"20", sizeof("20"))) == 0){
                numero = 20;
                printf("chegou vinte\n");
            }
            if((strncmp(teste,"21", sizeof("21"))) == 0){
                numero = 21;
                printf("chegou vinte e um\n");
            }
            if((strncmp(teste,"22", sizeof("22"))) == 0){
                numero = 22;
                printf("chegou vinte e dois\n");
            } if((strncmp(teste,"23", sizeof("23"))) == 0){
                numero = 23;
                printf("chegou vinte e três\n");
            } if((strncmp(teste,"24", sizeof("24"))) == 0){
                numero = 24;
                printf("chegou vinte e quatro\n");
            } if((strncmp(teste,"25", sizeof("25"))) == 0){
                numero = 25;
                printf("chegou vinte e cinco\n");
            } if((strncmp(teste,"26", sizeof("26"))) == 0){
                numero = 26;
                printf("chegou vinte e seis\n");
            } if((strncmp(teste,"27", sizeof("27"))) == 0){
                numero = 27;
                printf("chegou vinte e sete\n");
            } if((strncmp(teste,"28", sizeof("28"))) == 0){
                numero = 28;
                printf("chegou vinte e oito\n");
            }
            if((strncmp(teste,"29", sizeof("29"))) == 0){
                numero = 29;
                printf("chegou vinte e nove\n");
            }
            if((strncmp(teste,"30", sizeof("30"))) == 0){
                numero = 30;
                printf("chegou tinta\n");
            }
            if((strncmp(teste,"31", sizeof("31"))) == 0){
                numero = 31;
                printf("chegou trinta e um\n");
            }
            if((strncmp(teste,"32", sizeof("32"))) == 0){
                numero = 32;
                printf("chegou trinta e dois\n");
            }
            
            switch (numero)
			{
				case 16:
                 for(int i = 0; i <= 5; i++){
                    vPwmTask(GELAR_16, sizeof(GELAR_16) / sizeof(GELAR_16[0]));	
                    printf("enviou %d\n", i);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                 }
                
                break;

				case 17:
                for(int i = 0; i <= 5; i++){
                vPwmTask(GELAR_17, sizeof(GELAR_17) / sizeof(GELAR_17[0]));
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(1000));
                
            }
                break;

				case 18:
                for(int i = 0; i <= 5; i++){
                vPwmTask(GELAR_18, sizeof(GELAR_18) / sizeof(GELAR_18[0]));	
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(10000));
                }
                break;

				case 19:
                for(int i = 0; i <= 5; i++){
                vPwmTask(GELAR_19, sizeof(GELAR_19) / sizeof(GELAR_19[0]));	
                 printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(10000));
                }
                break;

				case 20:
                for(int i = 0; i <= 5; i++){
                vPwmTask(GELAR_20, sizeof(GELAR_20) / sizeof(GELAR_20[0]));
                 printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(10000));
                }
                	break;

				case 21:
                for(int i = 0; i <= 5; i++){
                vPwmTask(GELAR_21, sizeof(GELAR_21) / sizeof(GELAR_21[0]));
                 printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(1000));
                }	
                break;

				case 22:
                for(int i = 0; i <= 3; i++){
                vPwmTask(GELAR_22, sizeof(GELAR_22) / sizeof(GELAR_22[0]));
                 printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(10));
                }
                	break;

				case 23:
                for(int i = 0; i <= 3; i++){
                vPwmTask(GELAR_23, sizeof(GELAR_23) / sizeof(GELAR_23[0]));
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(10));
                }	

                	break;

				case 24:
                for(int i = 0; i <= 3; i++){
                vPwmTask(GELAR_24, sizeof(GELAR_24) / sizeof(GELAR_24[0]));	
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(1000));
                }	

                break;

				case 25:
                for(int i = 0; i <= 3; i++){
                vPwmTask(GELAR_25, sizeof(GELAR_25) / sizeof(GELAR_25[0]));	
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(1000));
                }	
                break;

				case 26:
                for(int i = 0; i <= 3; i++){
                vPwmTask(GELAR_26, sizeof(GELAR_26) / sizeof(GELAR_26[0]));	
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(1000));
                }	

                break;

				case 27:
                for(int i = 0; i <= 10; i++){
                vPwmTask(GELAR_27, sizeof(GELAR_27) / sizeof(GELAR_27[0]));
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(50));
                }	

                	break;

				case 28:
                for(int i = 0; i <= 3; i++){
                vPwmTask(GELAR_28, sizeof(GELAR_28) / sizeof(GELAR_28[0]));
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(1000));
                }	

                break;

				case 29:
                for(int i = 0; i <= 3; i++){
                vPwmTask(GELAR_29, sizeof(GELAR_29) / sizeof(GELAR_29[0]));
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(1000));
                }	

                	break;

				case 30:
                for(int i = 0; i <= 5; i++){
                vPwmTask(GELAR_30, sizeof(GELAR_30) / sizeof(GELAR_30[0]));	
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(50));
                }	

                break;

                case 31:
                for(int i = 0; i <= 3; i++){
                vPwmTask(GELAR_31, sizeof(GELAR_31) / sizeof(GELAR_31[0]));
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(1000));
                }	

                break;

                case 32:
                for(int i = 0; i <= 3; i++){
                vPwmTask(GELAR_32, sizeof(GELAR_32) / sizeof(GELAR_32[0]));
                printf("enviou %d\n", i);
                vTaskDelay(pdMS_TO_TICKS(1000));
                }
                break;


				default: 
                    printf("sa porra\n");
                    vPwmTask(LIGAR_24, sizeof(LIGAR_24) / sizeof(LIGAR_24[0]));	
                break;
			}

        memset(msg->message->payload,0,msg->message->payloadlen);
    }
}


/**
 * @brief A "Telefonista". Mantém a conexão viva em segundo plano.
 */
static void YieldTask(void *arg) {
    int rc = 0;
    while (1) {
        MQTTYield(&mqttClient, 1000);
        // MQTTYield é o coração da manutenção da conexão.
        // Ela retorna 0 para sucesso e um código de erro em caso de falha.
        // Se MQTTYield retornar um erro, significa que a conexão foi perdida.
        if (MQTTYield(&mqttClient, 1000) == FAILURE) {
            printf("[YieldTask] ERRO: MQTTYield retornou o codigo %d. A conexão caiu.\n", rc);
            printf("[YieldTask] A reiniciar o sistema para restabelecer a ligacao...\n");
            
            osDelay(2000); // Espera 2 segundos para garantir que o log foi enviado.
            NVIC_SystemReset(); // Força uma reinicialização do sistema.
        }
    }
}

/**
 * @brief Tarefa Especialista 1: Processa apenas mensagens de ESTADO.
 */
// static void EstadoTask(void *arg) {
//     char buffer_estado[64];

//     while (1) {
//         // A tarefa "dorme" aqui, esperando uma mensagem na sua fila específica
//         if (osMessageQueueGet(estadoQueueHandle, buffer_estado, NULL, osWaitForever) == osOK) {
//             printf(">>> [TAREFA DE ESTADO] Processando mensagem: %s\n", buffer_estado);
//             // Aqui entraria a lógica para tratar o estado (ex: ligar/desligar algo)
//             osDelay(pdMS_TO_TICKS(100));
//         }
//     }
// }

/**
 * @brief Tarefa Especialista 2: Processa apenas mensagens de TEMPERATURA.
 */


void vPwmTask(uint16_t buf[], unsigned int len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        if ( i & 1 )
           stop_pwm(buf[i]);
           
        else
            start_pwm(buf[i]);
            
             
    }
    stop_pwm(0);

}

static void TemperaturaTask(void *arg) {
    char buffer_temperatura[64];
    while (1) {
        printf("entrou aqui\n");
        // A tarefa "dorme" aqui, esperando uma mensagem na sua fila específica
        if (osMessageQueueGet(temperaturaQueueHandle, buffer_temperatura, NULL, osWaitForever) == osOK) {
         printf(">>> [TAREFA DE TEMPERATURA] Processando mensagem: %s\n", buffer_temperatura);
            // Aqui entraria a lógica para tratar a temperatura (ex: ajustar um ar condicionado)
            uint8_t numero;
        
            if((strncmp(buffer_temperatura,"16", sizeof("16"))) == 0){
                numero = 16;
                printf("chegou dezesseis\n");
            }
            
            switch (numero)
			{
				case 16:vPwmTask(GELAR_16, sizeof(GELAR_16) / sizeof(GELAR_16[0]));	break;
				case 17:vPwmTask(GELAR_17, sizeof(GELAR_17) / sizeof(GELAR_17[0]));	break;
				case 18:vPwmTask(GELAR_18, sizeof(GELAR_18) / sizeof(GELAR_18[0]));	break;
				case 19:vPwmTask(GELAR_19, sizeof(GELAR_19) / sizeof(GELAR_19[0]));	break;
				case 20:vPwmTask(GELAR_20, sizeof(GELAR_20) / sizeof(GELAR_20[0]));	break;
				case 21:vPwmTask(GELAR_21, sizeof(GELAR_21) / sizeof(GELAR_21[0]));	break;
				case 22:vPwmTask(GELAR_22, sizeof(GELAR_22) / sizeof(GELAR_22[0]));	break;
				case 23:vPwmTask(GELAR_23, sizeof(GELAR_23) / sizeof(GELAR_23[0]));	break;
				case 24:vPwmTask(GELAR_24, sizeof(GELAR_24) / sizeof(GELAR_24[0]));	break;
				case 25:vPwmTask(GELAR_25, sizeof(GELAR_25) / sizeof(GELAR_25[0]));	break;
				case 26:vPwmTask(GELAR_26, sizeof(GELAR_26) / sizeof(GELAR_26[0]));	break;
				case 27:vPwmTask(GELAR_27, sizeof(GELAR_27) / sizeof(GELAR_27[0]));	break;
				case 28:vPwmTask(GELAR_28, sizeof(GELAR_28) / sizeof(GELAR_28[0]));	break;
				case 29:vPwmTask(GELAR_29, sizeof(GELAR_29) / sizeof(GELAR_29[0]));	break;
				case 30:vPwmTask(GELAR_30, sizeof(GELAR_30) / sizeof(GELAR_30[0]));	break;
                case 31:vPwmTask(GELAR_31, sizeof(GELAR_31) / sizeof(GELAR_31[0]));	break;
                case 32:vPwmTask(GELAR_32, sizeof(GELAR_32) / sizeof(GELAR_32[0]));	break;
				default: 
                    printf("sa porra\n");
                    vPwmTask(LIGAR_24, sizeof(LIGAR_24) / sizeof(LIGAR_24[0]));	
                break;
			}

            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
}



/**
 * @brief Função principal da lógica. Inicia a conexão e todas as tarefas.
 */
void HT_Fsm(void) {

    
    // Cria as "Caixas de Entrada" (Filas) para cada tarefa especialista
    // estadoQueueHandle = osMessageQueueNew(5, sizeof(char[64]), NULL);      // Fila para 5 mensagens de estado
    // temperaturaQueueHandle = osMessageQueueNew(5, sizeof(char[64]), NULL); // Fila para 5 mensagens de temperatura
    
   
    // 1. Conecta ao Broker MQTT
    while (HT_MQTT_Connect(&mqttClient, &mqttNetwork, (char *)BROKER_ADDR, HT_MQTT_PORT, HT_MQTT_TIMEOUT, HT_MQTT_TIMEOUT,
                        (char *)CLIENT_ID, NULL, NULL, HT_MQTT_VERSION, HT_MQTT_KEEP_ALIVE_INTERVAL,
                        mqttSendbuf, HT_MQTT_BUFFER_SIZE, mqttReadbuf, HT_MQTT_BUFFER_SIZE) == 0) {
        printf("ERRO FATAL: Nao foi possivel conectar ao Broker MQTT.\n");
         printf("Tentando novamente\n");
         osDelay(2000); // Espera 2 segundos para garantir que o log foi enviado.
        // NVIC_SystemReset(); // Força uma reinicialização do sistema.
    }
    printf("Conectado ao Broker MQTT com sucesso!\n");

    // 2. Inscreve-se nos DOIS tópicos, usando a mesma função "Recepcionista"
    printf("Inscrevendo-se nos topicos...\n");
    // MQTTSubscribe(&mqttClient, SUBSCRIBE_TOPIC_ESTADO, QOS0, OnMessageReceived_Dispatcher);
    MQTTSubscribe(&mqttClient, SUBSCRIBE_TOPIC_TEMPERATURA, QOS0, OnMessageReceived_Dispatcher);
    printf("Inscricoes realizadas.\n\n");

    // 3. Inicia todas as tarefas necessárias
    osThreadAttr_t task_attr;
    
    // Tarefa Telefonista (Yield)
    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "YieldTask";
    task_attr.stack_size = 2048;
    task_attr.priority = osPriorityNormal;
    osThreadNew(YieldTask, NULL, &task_attr);

    // // Tarefa Especialista de Estado
    // memset(&task_attr, 0, sizeof(task_attr));
    // task_attr.name = "EstadoTask";
    // task_attr.stack_size = 2048;
    // task_attr.priority = osPriorityNormal;
    // osThreadNew(EstadoTask, NULL, &task_attr);

    // Tarefa Especialista de Temperatura
    // memset(&task_attr, 0, sizeof(task_attr));
    // task_attr.name = "TemperaturaTask";
    // task_attr.stack_size = 1024 * 5 * 2;
    // task_attr.priority = osPriorityAboveNormal7;
    // osThreadNew(TemperaturaTask, NULL, &task_attr);

    printf("Sistema pronto. Todas as tarefas estao rodando em paralelo.\n");
    while (1)
    {
        vTaskDelay(1);
    }
    printf("Nõ deve chegar aqui!");
    // Deleta a tarefa atual para liberar recursos, pois ela já fez seu trabalho.
    osThreadTerminate(osThreadGetId());
}