#include "math.h"
#include "FreeRTOS.h"
#include "os_task.h"
#include "os_queue.h"
#include "os_timer.h"
#include "os_event_groups.h"
#include "os_stream_buffer.h"
#include <stdio.h>
#include "emac.h"
#include "sys_core.h"

void TC_PRIV_ROP_GADGET(void *pvParameters);
void TC_WDOG_INTERFER(); 
void TC_EMAC_INTERFER(); 
void TC_MPU_REGIONS(); 
void TC_TASK_CREATION();
void TC_NEVER_YIELD();
void TC_EDR_TASK_INTERFER();
void TC_MODIFY_EDR_STATE();
void TC_ROP_STUFF();
void TC_INJ_STACK_EXEC();
