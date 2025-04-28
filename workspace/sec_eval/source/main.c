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
#include "test_cases.h"

#define TC_CODE_INJ_ROP_STOP_TIMER_ENABLE     0
#define TC_INJ_ROP_LR_CHECK_ENABLE            0
#define TC_INJ_STACK_EXEC_ENABLE              0
#define TC_TASK_CREATION_ENABLE               0
#define TC_PRIV_ROP_GADGET_ENABLE             0
#define TC_EDR_TASK_INTERFER_ENABLE           0
#define TC_RW_EDR_STATE_ENABLE                0
#define TC_CODE_INJ_ROP_BELOW_WCET_ENABLE     0
#define TC_WDOG_INTERFER_ENABLE               0
#define TC_NEVER_YIELD_ENABLE                 0
#define TC_VERIFY_MPU_REGIONS_ENABLE          0
#define TC_EMAC_INTERFER_ENABLE               0
#define TC_MODIFY_MPU_CONF_ENABLE             0
#define TC_CODE_INJ_ROP_EXCEED_WCET_ENABLE    0
#define TC_RESTR_TASK_ACCESS_STACK_ENABLE     0

EDR_DATA uint8   emacAddress[6U] =   {0x00U, 0x08U, 0xEEU, 0x03U, 0xA6U, 0x6CU};
EDR_DATA uint32  emacPhyAddress  =   1U;

static TaskHandle_t xFlightCtrl_Handle = NULL;
static TaskHandle_t xSensor1_Handle = NULL;
static TaskHandle_t xRCctrl_Handle = NULL;
static TaskHandle_t xComms_Handle = NULL;

#pragma DATA_ALIGN(FlightCtrl_Stack, 256 * sizeof(portSTACK_TYPE))
#pragma DATA_ALIGN(Sensor1_Stack, 256 * sizeof(portSTACK_TYPE))
#pragma DATA_ALIGN(RCctrl_Stack, 256 * sizeof(portSTACK_TYPE))
#pragma DATA_ALIGN(Comms_Stack, 256 * sizeof(portSTACK_TYPE))

static portSTACK_TYPE FlightCtrl_Stack[256] __attribute__ ((aligned (256 * sizeof(portSTACK_TYPE))));
static portSTACK_TYPE Sensor1_Stack[256] __attribute__ ((aligned (256 * sizeof(portSTACK_TYPE))));
static portSTACK_TYPE RCctrl_Stack[256] __attribute__ ((aligned (256 * sizeof(portSTACK_TYPE))));
static portSTACK_TYPE Comms_Stack[256] __attribute__ ((aligned (256 * sizeof(portSTACK_TYPE))));
char cReadyOnlyArray[ 32 ] __attribute__((aligned(32)));

void CoreMarkTask(void *pvParameters) {
    for(;;) {
        main();
        break;
    }

    vTaskDelete(NULL);

}

void busy_wait_ms(uint32_t time_ms) {
    volatile int i = 0;
    uint32_t cycles_per_ms = 220;  // Rough estimate: 220 MHz / 1000 = 220,000 cycles per ms
    uint32_t iterations = time_ms * cycles_per_ms;

    for (i = 0; i < iterations; i++) {
        asm volatile(" nop"); // Prevents optimization
    }
}

void FlightCtrl_Task(void *pvParameters) {
    static float prev_error = 0.0f;
    static float integral = 0.0f;
    const float dt = 0.01f;  // 10ms loop
    const float Kp = 1.2f, Ki = 0.1f, Kd = 0.5f;

    static float prev_angle = 0.0f;
    const float alpha = 0.98f;

    for (;;) {
        // Simulate sensor input
        float error = 0.5f;  // Example error
        float gyro_rate = 0.02f;  // Example gyro data (rad/s)
        float accel_angle = 1.5f;  // Example accelerometer angle (degrees)

        // PID Control
        float derivative = (error - prev_error) / dt;
        integral += error * dt;
        float output = Kp * error + Ki * integral + Kd * derivative;
        prev_error = error;

        // Angle Estimation with Sensor Fusion
        float gyro_angle = prev_angle + gyro_rate * dt;
        prev_angle = alpha * gyro_angle + (1 - alpha) * accel_angle;

        // Enforce 10ms loop period using a hardware timer or RTOS delay
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void Sensor_Task(void *pvParameters) {
    float accel_x = 0.0f, accel_y = 0.0f, accel_z = 9.81f;
    float gyro_x = 0.0f, gyro_y = 0.0f, gyro_z = 0.02f;

    for (;;) {
        // Simulate accelerometer & gyroscope readings
        accel_x = sinf(gyro_x) * 9.81f;
        accel_y = cosf(gyro_y) * 9.81f;
        accel_z = 9.81f + 0.01f * gyro_z;

        gyro_x += 0.001f;
        gyro_y += 0.002f;
        gyro_z += 0.0015f;

        // Enforce 300ms sampling interval
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void process_RC_input2(const char *input) {
    char rc_buffer[16];  // Small buffer for processing RC commands

    // *** Buffer Overflow Vulnerability ***
    strncpy(rc_buffer, input, 32);  // No bounds checking!
}

void RCctrl_Task(void *pvParameters) {
    float throttle = 0.0f, roll = 0.0f, pitch = 0.0f, yaw = 0.0f;

    for (;;) {
        throttle = (throttle >= 100.0f) ? 0.0f : throttle + 0.5f;
        roll = sinf(throttle) * 30.0f;
        pitch = cosf(throttle) * 30.0f;
        yaw = atan2f(roll, pitch);

#if TC_INJ_STACK_EXEC_ENABLE
        char incoming_command[32];
        memcpy(incoming_command,
            "\x01\x10\xa0\xe3\x02\x20\xa0\xe3\x03\x30\xa0\xe3\xBB\xBB\xBB\xBB\xCC\xCC\xCC\xCC\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD",
            32);
        incoming_command[16] = 0xAC;
        incoming_command[17] = 0x7D;
        incoming_command[18] = 0x01;
        incoming_command[19] = 0x08;
        process_RC_input2(incoming_command);
#endif

#if TC_EDR_TASK_INTERFER_ENABLE
        TC_EDR_TASK_INTERFER();
#endif

#if TC_RW_EDR_STATE
        TC_RW_EDR_STATE();
#endif

#if TC_CODE_INJ_ROP_BELOW_WCET_ENABLE
        char incoming_command[32];
        memcpy(incoming_command, "AAAAAAAAAAAAAAAABBBBCCCCDDDDDDDD", 32);
        incoming_command[16] = 0x38;
        incoming_command[17] = 0x34;
        incoming_command[18] = 0x02;
        incoming_command[19] = 0x00;
        process_RC_input2(incoming_command);
#endif

#if TC_CODE_INJ_ROP_STOP_TIMER_ENABLE
        char incoming_command[32];
        memcpy(incoming_command, "AAAAAAAAAAAAAAAABBBBCCCCDDDDDDDD", 32);
        incoming_command[16] = 0x38;
        incoming_command[17] = 0x2c;
        incoming_command[18] = 0x01;
        incoming_command[19] = 0x00;
        process_RC_input2(incoming_command);
#endif

#if TC_WDOG_INTERFER_ENABLE
        TC_WDOG_INTERFER();
#endif

        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void Comms_Task(void *pvParameters) {
#if TC_NEVER_YIELD_ENABLE
        TC_NEVER_YIELD();
#endif

#if TC_TASK_CREATION_ENABLE
        TC_TASK_CREATION();
#endif

#if TC_VERIFY_MPU_REGIONS_ENABLE
        TC_VERIFY_MPU_REGIONS();
#endif

#if TC_EMAC_INTERFER_ENABLE
        TC_EMAC_INTERFER();
#endif

#if TC_MODIFY_MPU_CONF_ENABLE
        TC_MODIFY_MPU_CONF();
#endif

#if TC_CODE_INJ_ROP_EXCEED_WCET_ENABLE
    char incoming_command[32];
    memcpy(incoming_command, "AAAAAAAAAAAAAAAABBBBCCCCDDDDDDDD", 32);
    incoming_command[16] = 0x08;
    incoming_command[17] = 0x35;
    incoming_command[18] = 0x02;
    incoming_command[19] = 0x00;
    process_RC_input2(incoming_command);
#endif

#if TC_RESTR_TASK_ACCESS_STACK_ENABLE
    RCctrl_Stack[0] = 120;
#endif

    int packet_counter = 0;
    float signal_strength = -50.0f;

    for (;;) {
        packet_counter++;
        signal_strength += (packet_counter % 10 == 0) ? -0.1f : 0.05f;
        if (packet_counter % 20 == 0) {
            float received_value = 1.0f / (float)packet_counter;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void Log_Task(void *pvParameters) {
    static volatile int log_index = 0;
    static volatile float battery_voltage = 12.6f;
    static volatile float flight_time = 0.0f;

    for (;;) {
        // Simulate logging flight parameters
        log_index++;
        battery_voltage -= 0.01f;  // Simulate battery drain
        flight_time += 0.16f;  // Log every 160ms

        // Log system state (could be stored in memory or sent via telemetry)
        vTaskDelay(pdMS_TO_TICKS(1000));  // Logging every second
    }
}

static const xTaskParameters xFlightCtrl_Params = {
    FlightCtrl_Task, "FlightCtrl", 256, NULL, (6 | portPRIVILEGE_BIT), FlightCtrl_Stack,
    {{cReadyOnlyArray,32,portMPU_REGION_READ_ONLY}, {0,0,0}, {0,0,0}}
};

static const xTaskParameters xSensor1_Params = {
    Sensor_Task, "Sensor1", 256, NULL, 5, Sensor1_Stack,
    {{cReadyOnlyArray,32,portMPU_REGION_READ_ONLY}, {0,0,0}, {0,0,0}}
};

static const xTaskParameters xRCctrl_Params = {
    RCctrl_Task, "RCctrl", 256, NULL, 5, RCctrl_Stack,
    {{cReadyOnlyArray,32,portMPU_REGION_READ_ONLY}, {0,0,0}, {0,0,0}}
};

static const xTaskParameters xComms_Params = {
    Comms_Task, "Comms", 256, NULL, 3, Comms_Stack,
    {{cReadyOnlyArray,32,portMPU_REGION_READ_ONLY}, {0,0,0}, {0,0,0}}
};

void os_main(void)
{
    _enable_IRQ();

    if (xTaskCreateRestricted(&xFlightCtrl_Params, &xFlightCtrl_Handle) != pdTRUE) while(1);
    if (xTaskCreateRestricted(&xSensor1_Params, &xSensor1_Handle) != pdTRUE) while(1);
    if (xTaskCreateRestricted(&xRCctrl_Params, &xRCctrl_Handle) != pdTRUE) while(1);
    if (xTaskCreateRestricted(&xComms_Params, &xComms_Handle) != pdTRUE) while(1);

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
