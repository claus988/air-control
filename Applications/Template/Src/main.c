#include "main.h"
/**
 * @file main.c
 * @brief Ponto de entrada da aplicação.
 * Foca em inicializar o sistema e a rede, e então passa o controle para a lógica principal.
 */

#include "HT_Fsm.h" 
#include "HT_PWM.h"

static StaticTask_t initTask;
static uint8_t appTaskStack[INIT_TASK_STACK_SIZE];
static QueueHandle_t psEventQueueHandle;
static uint32_t gCellID = 0;
static uint8_t gImsi[16] = {0};
static volatile uint32_t Event;
static NmAtiSyncRet gNetworkInfo;
// static uint8_t mqttEpSlpHandler = 0xff;
static volatile uint8_t simReady = 0;

static uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE | 
                              ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

extern USART_HandleTypeDef huart1;

/* --- Funções de Rede e Inicialização --- */

static void sendQueueMsg(uint32_t msgId, uint32_t xTickstoWait) {
    eventCallbackMessage_t *queueMsg = NULL;
    queueMsg = malloc(sizeof(eventCallbackMessage_t));
    queueMsg->messageId = msgId;
    if (psEventQueueHandle) {
        if (pdTRUE != xQueueSend(psEventQueueHandle, &queueMsg, xTickstoWait)) {
            printf("xQueueSend error\n");
        }
    }
}

static INT32 registerPSUrcCallback(urcID_t eventID, void *param, uint32_t paramLen) {
    CmiSimImsiStr *imsi = NULL;
    CmiPsCeregInd *cereg = NULL;
    UINT8 rssi = 0;
    NmAtiNetifInfo *netif = NULL;

    switch(eventID)
    {
        case NB_URC_ID_SIM_READY:
        {
            imsi = (CmiSimImsiStr *)param;
            memcpy(gImsi, imsi->contents, imsi->length);
            simReady = 1;
            break;
        }
        case NB_URC_ID_MM_SIGQ:
        {
            rssi = *(UINT8 *)param;
            HT_TRACE(UNILOG_MQTT, mqttAppTask81, P_INFO, 1, "RSSI signal=%d", rssi);
            break;
        }
        case NB_URC_ID_PS_BEARER_ACTED:
        {
            HT_TRACE(UNILOG_MQTT, mqttAppTask82, P_INFO, 0, "Default bearer activated");
            break;
        }
        case NB_URC_ID_PS_BEARER_DEACTED:
        {
            HT_TRACE(UNILOG_MQTT, mqttAppTask83, P_INFO, 0, "Default bearer Deactivated");
            break;
        }
        case NB_URC_ID_PS_CEREG_CHANGED:
        {
            cereg = (CmiPsCeregInd *)param;
            gCellID = cereg->celId;
            HT_TRACE(UNILOG_MQTT, mqttAppTask84, P_INFO, 4, "CEREG changed act:%d celId:%d locPresent:%d tac:%d", cereg->act, cereg->celId, cereg->locPresent, cereg->tac);
            break;
        }
        case NB_URC_ID_PS_NETINFO:
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
                sendQueueMsg(QMSG_ID_NW_IPV4_READY, 0);
            break;
        }

        default:
            break;
    }
    return 0;
}

static void HT_MQTTExampleTask1(void *arg){
    eventCallbackMessage_t *queueItem = NULL;

        int32_t ret;
    uint8_t psmMode = 0, actType = 0;
    uint16_t tac = 0;
    uint32_t tauTime = 0, activeTime = 0, cellID = 0, nwEdrxValueMs = 0, nwPtwMs = 0;


    registerPSEventCallback(NB_GROUP_ALL_MASK, registerPSUrcCallback);
    psEventQueueHandle = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(eventCallbackMessage_t*));
    if (psEventQueueHandle == NULL) {
        printf("psEventQueue create error!\n");
        return;
    }
    HT_TRACE(UNILOG_MQTT, mqttAppTask1, P_INFO, 0, "first time run mqtt example");
    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
    printf("HTNB32L-XXX Minimal Subscriber. Aguardando SIM card...\n");

    
    while(!simReady) osDelay(500);

    printf("SIM card pronto. Configurando APN...\n");
    
    PsAPNSetting apnSetting;
    apnSetting.cid = 0;
    apnSetting.apnLength = strlen("iot.datatem.com.br");
    strcpy((char *)apnSetting.apnStr, "iot.datatem.com.br");
    apnSetting.pdnType = CMI_PS_PDN_TYPE_IP_V4V6;
    appSetAPNSettingSync(&apnSetting, &apnSetting.cid);

    printf("Aguardando conexao com a rede celular...\n");

    if (xQueueReceive(psEventQueueHandle, &queueItem, portMAX_DELAY)) {
        if (queueItem->messageId == QMSG_ID_NW_IPV4_READY)
         {
            printf("Rede Celular Conectada! Iniciando aplicacao MQTT...\n\n");

            appGetImsiNumSync((CHAR *)gImsi);
                    HT_STRING(UNILOG_MQTT, mqttAppTask2, P_SIG, "IMSI = %s", gImsi);
                
                    appGetNetInfoSync(gCellID, &gNetworkInfo);
                    if ( NM_NET_TYPE_IPV4 == gNetworkInfo.body.netInfoRet.netifInfo.ipType)
                        HT_TRACE(UNILOG_MQTT, mqttAppTask3, P_INFO, 4,"IP:\"%u.%u.%u.%u\"", ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[0],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[1],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[2],
                                                                      ((UINT8 *)&gNetworkInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr)[3]);
                    ret = appGetLocationInfoSync(&tac, &cellID);
                    HT_TRACE(UNILOG_MQTT, mqttAppTask4, P_INFO, 3, "tac=%d, cellID=%d ret=%d", tac, cellID, ret);
                    //edrxModeValue = CMI_MM_ENABLE_EDRX_AND_ENABLE_IND;
                    actType = CMI_MM_EDRX_NB_IOT;
                    //reqEdrxValueMs = 20480;
                    // appSetEDRXSettingSync(edrxModeValue, actType, reqEdrxValueMs);
                    ret = appGetEDRXSettingSync(&actType, &nwEdrxValueMs, &nwPtwMs);
                    HT_TRACE(UNILOG_MQTT, mqttAppTask5, P_INFO, 4, "actType=%d, nwEdrxValueMs=%d nwPtwMs=%d ret=%d", actType, nwEdrxValueMs, nwPtwMs, ret);

                    psmMode = 1;
                    tauTime = 4000;
                    activeTime = 30;

                    {
                        appGetPSMSettingSync(&psmMode, &tauTime, &activeTime);
                        HT_TRACE(UNILOG_MQTT, mqttAppTask6, P_INFO, 3, "Get PSM info mode=%d, TAU=%d, ActiveTime=%d", psmMode, tauTime, activeTime);
                    }

            
            //Chama a lógica principal do arquivo HT_Fsm.c 
            HT_Fsm(); 
        }
        free(queueItem);
    }
    osDelay(pdMS_TO_TICKS(100));
}

static void appInit(void *arg) {    
    osThreadAttr_t task_attr;      // cria uma instancia

    memset(&task_attr,0,sizeof(task_attr));  // zera a instancia
    memset(appTaskStack, 0xA5,INIT_TASK_STACK_SIZE);
    task_attr.name = "HT_MQTTExample";
    task_attr.stack_mem = appTaskStack;
    task_attr.stack_size = INIT_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &initTask;
    task_attr.cb_size = sizeof(StaticTask_t);

    osThreadNew(HT_MQTTExampleTask1, NULL, &task_attr);
}

/**
 * \fn          int main_entry(void)
 * \brief       main entry function.
 * \return
 */
void main_entry(void) {
    // printf("INICIAR");

    BSP_CommonInit();   // init das libs da board
    //HT_GPIO_InitLed();  // config gpi para o led
    pwm_config();       // config do Pwm 
    
    osKernelInitialize();   // init dom kernel do freeRTOS
    // setvbuf(stdout, NULL, _IONBF, 0); //
    
    registerAppEntry(appInit, NULL);   //  inicializa as tasks
    if (osKernelGetState() == osKernelReady) {  
        osKernelStart();  // Start the RTOS Kernel scheduler
    }
    while(1);
}
