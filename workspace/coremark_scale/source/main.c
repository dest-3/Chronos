/* Include Files */

#include "sys_common.h"
#include "system.h"

/* USER CODE BEGIN (1) */
/* Include FreeRTOS scheduler files */
#include "FreeRTOS.h"
#include "os_task.h"

/* Include HET header file - types, definitions and function declarations for system driver */
#include "het.h"
#include "esm.h"
#include "gio.h"
#include "rti.h"
#include "edr.h"
#include "lwiplib.h"
#include <stdio.h>
#include "source/core_main.c"
#include "math.h"

EDR_DATA uint8   emacAddress[6U] =   {0x00U, 0x08U, 0xEEU, 0x03U, 0xA6U, 0x6CU};
EDR_DATA uint32  emacPhyAddress  =   1U;

void CoreMarkTask(void *pvParameters) {
    for(;;) {
        main();
        break;
    }

    vTaskDelete(NULL);

}

void os_main(void)
{
    _enable_IRQ();

    if (xTaskCreate(CoreMarkTask, "CMARK_1", 256 * 3, NULL, (EDR_TASK_PRIORITY - 1),  NULL) != pdTRUE) {
        while(1);
    }

    if (xTaskCreate(CoreMarkTask, "CMARK_2", 256 * 3, NULL, (EDR_TASK_PRIORITY - 1),  NULL) != pdTRUE) {
        while(1);
    }

    if (xTaskCreate(CoreMarkTask, "CMARK_3", 256 * 3, NULL, (EDR_TASK_PRIORITY - 1),  NULL) != pdTRUE) {
        while(1);
    }

    if (xTaskCreate(CoreMarkTask, "CMARK_4", 256 * 3, NULL, (EDR_TASK_PRIORITY - 1),  NULL) != pdTRUE) {
        while(1);
    }

    if (xTaskCreate(CoreMarkTask, "CMARK_5", 256 * 3, NULL, (EDR_TASK_PRIORITY - 1),  NULL) != pdTRUE) {
       while(1);
    }

    EMAC_LwIP_Main(emacAddress);

    edr_init();

    if (xTaskCreate(edr_run, "EDR_RUN", 256 * 2, NULL, (EDR_TASK_PRIORITY | portPRIVILEGE_BIT),  &edr_ptr_global->edr_t_handle) != pdTRUE) {
        while(1);
    }

    /* Start Scheduler */
    vTaskStartScheduler();


    /* Run forever */
    while(1);
}


void rtiNotification(uint32 not) {
    int z = 0;
    z += 1;
}
