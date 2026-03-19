#include "debug.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/init.h"
#include "lwip/tcpip.h"

#include "lwip/dhcp.h"
#include "lwip/apps/httpd.h"

#include "ethernetif.h"

#define LED1_PIN GPIO_Pin_15 // PA15
#define LED2_PIN GPIO_Pin_4  // PB4

#define PHY_ADDRESS 0x01 // : Quase sempre é 0x01 para o PHY interno.

#define PHY_BSR 0x01 // (Endereço do registrador de status).

#define PHY_Linked_Status 0x0004 // (Máscara do bit de link).

// Contador de persistência do LED ACT (em milissegundos)
extern volatile uint32_t act_led_timer;

extern void Ethernet_LED_Configuration(void);
extern void Ethernet_LED_LINKSET(uint8_t mode);
extern void Ethernet_LED_DATASET(uint8_t mode);
extern uint16_t ETH_ReadPHYRegister(uint16_t PHYAddress, uint16_t PHYReg);
extern void RNG_Cmd(FunctionalState NewState);

void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    // 1. Habilita o clock para os Portais A, B e a fun??o de Remapeamento (AFIO)
    // RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    // RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    // 2. Libera PA15 e PB4 da fun??o JTAG (Essencial para esses LEDs funcionarem)
    // GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

    // 3. Configura PA15 (LED1)
    GPIO_InitStructure.GPIO_Pin = LED1_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // Saикda Push-Pull
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 4. Configura PB4 (LED2)
    GPIO_InitStructure.GPIO_Pin = LED2_PIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    // GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void GPIO_Toggle_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

struct netif lwip_netif;

void LwIP_Init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_RNG, ENABLE);
    RNG_Cmd(ENABLE);
    // printf("enable rng ok \r\n");

    tcpip_init(NULL, NULL);
    netif_add(&lwip_netif, NULL, NULL, NULL, NULL, &ethernetif_init, &tcpip_input);
    netif_set_default(&lwip_netif);

    /*When the netif is fully configured this function must be called*/
    netif_set_up(&lwip_netif);

    dhcp_start(&lwip_netif);

    httpd_init();

    TaskHandle_t lwip_handle = xTaskGetHandle("tcpip_thread");
    if (lwip_handle != NULL)
    {
        printf("Folga Stack LwIP: %d\r\n", (int)uxTaskGetStackHighWaterMark(lwip_handle));
    }

    // printf("Initialized LwIP \r\n");
}

void led_link_task(void *pvParameters)
{
    // Garante que o Port C esteja pronto
    Ethernet_LED_Configuration();

    const TickType_t xDelay10ms = pdMS_TO_TICKS(10);
    uint16_t phy_status = 0;

    while (1)
    {
        // --- GERENCIAMENTO DO LINK (Verde) ---
        // Lendo o registrador 1 do PHY (Basic Status Register)
        // O bit 2 (0x0004) indica se o link físico está estabelecido
        phy_status = ETH_ReadPHYRegister(PHY_ADDRESS, PHY_BSR);

        if (phy_status & PHY_Linked_Status)
        {
            Ethernet_LED_LINKSET(0); // ON
            // Opcional: Avisa o LwIP que o cabo voltou
            netif_set_link_up(&lwip_netif);
        }
        else
        {
            Ethernet_LED_LINKSET(1); // OFF
            // Opcional: Avisa o LwIP que o cabo saiu
            netif_set_link_down(&lwip_netif);
        }

        // --- Lógica do ACT (Amarelo) com Flag ---
        if (act_led_timer > 0)
        {
            Ethernet_LED_DATASET(0); // ON

            // Subtrai o tempo que passou (50ms)
            if (act_led_timer >= 10)
            {
                act_led_timer -= 10;
            }
            else
            {
                act_led_timer = 0;
            }
        }
        else
        {
            Ethernet_LED_DATASET(1); // OFF
        }

        vTaskDelay(xDelay10ms);
    }
}

void t1_task(void *pvParameters)
{
    uint8_t ip_exibido = 0;

    uint8_t alert_heap = 0;
    uint8_t alert_stack = 0;

    // Definimos 10% como margem de segurança (ajuste conforme preferir)
    const size_t HEAP_THRESHOLD = 1500;      // Alerta se o Heap cair abaixo de 1.5KB
    const UBaseType_t STACK_THRESHOLD = 100; // Alerta se a Stack cair abaixo de 100 words

    while (1)
    {
        size_t free_heap = xPortGetFreeHeapSize();
        UBaseType_t stack_folga = uxTaskGetStackHighWaterMark(NULL);

        // --- MONITOR DE HEAP ---
        if (free_heap < HEAP_THRESHOLD)
        {
            if (!alert_heap)
            {
                printf("\r\n[ALERTA] Heap Crítico: %d bytes!\r\n", (int)free_heap);
                alert_heap = 1; // Trava o alerta para não repetir toda hora
            }
        }
        else
        {
            alert_heap = 0; // Reseta o alerta se a memória voltar a subir
        }

        // --- MONITOR DE STACK ---
        if (stack_folga < STACK_THRESHOLD)
        {
            if (!alert_stack)
            {
                printf("\r\n[ALERTA] Stack T1 no limite: %d words!\r\n", (int)stack_folga);
                alert_stack = 1;
            }
        }
        else
        {
            alert_stack = 0;
        }

        size_t min_heap = xPortGetMinimumEverFreeHeapSize();
        if (min_heap < HEAP_THRESHOLD)
        {
            printf("CUIDADO: O Heap já chegou a cair para %d em algum momento!\r\n", (int)min_heap);
        }

        if (netif_is_up(&lwip_netif) && (lwip_netif.ip_addr.addr != 0))
        {
            if (!ip_exibido)
            {
                printf("IP Obtido: %s\r\n", ip4addr_ntoa(netif_ip4_addr(&lwip_netif)));
                ip_exibido = 1;
            }
        }
        else
        {
            ip_exibido = 0; // Se cair a rede, permite imprimir de novo ao voltar
        }

        // Liga LEDs (Se for lиоgica inversa, use Reset para ligar)
        GPIO_WriteBit(GPIOA, LED1_PIN, Bit_SET);
        GPIO_WriteBit(GPIOB, LED2_PIN, Bit_RESET);
        // Delay_Ms(500);
        vTaskDelay(500 / portTICK_PERIOD_MS);

        // Desliga LEDs
        GPIO_WriteBit(GPIOA, LED1_PIN, Bit_RESET);
        GPIO_WriteBit(GPIOB, LED2_PIN, Bit_SET);
        // Delay_Ms(500);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void vAssertCalled(const char *pcFile, unsigned long ulLine)
{
    printf("ASSERT FAILED: File %s, Line %d\r\n", pcFile, (int)ulLine);
    while (1)
        ; // Trava aqui para você ler no Putty
}

void vApplicationMallocFailedHook(void)
{
    printf("FreeRTOS: Malloc Failed! Sem memória no Heap.\r\n");
    while (1)
        ;
}

/* Global Variable */
TaskHandle_t Task1Task_Handler;

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    Delay_Init();
    USART_Printf_Init(115200);

    GPIO_Config();

    printf("SystemClk:%d\r\n", SystemCoreClock);

    printf("FreeRTOS Kernel Version:%s\r\n", tskKERNEL_VERSION_NUMBER);
    printf("LwIP Version: %s \r\n", LWIP_VERSION_STRING);

    LwIP_Init();

    xTaskCreate((TaskFunction_t)t1_task,
                (const char *)"t1_task",
                (uint16_t)(1024 * 8) / 4,
                (void *)NULL,
                (UBaseType_t)5,
                (TaskHandle_t *)&Task1Task_Handler);

    xTaskCreate((TaskFunction_t)led_link_task,
                (const char *)"led_link_task",
                (uint16_t)256, // Stack pequena, só controla GPIO
                (void *)NULL,
                (UBaseType_t)3, // Prioridade menor que a t1_task
                NULL);

    vTaskStartScheduler();
}
