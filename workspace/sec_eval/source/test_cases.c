#include "test_cases.h"
#include "edr.h"

// =============================
//   TEST_CASE_CODE_INJ_ROP_STOP_TIMER
// =============================

// Rctrl_task - BOF - ROP - vTaskSwitchContext inlined macro code to zero out counter
// Triggers prefetch entry error as this function is in kernelTEXT
void TC_CODE_INJ_ROP_STOP_TIMER() { 
    char incoming_command[32];  // Larger than rc_buffer
    memcpy(incoming_command, "AAAAAAAAAAAAAAAABBBBCCCCDDDDDDDD", 32);
    incoming_command[16] = 0x38;
    incoming_command[17] = 0x2c;
    incoming_command[18] = 0x01;
    incoming_command[19] = 0x00;  // Overwrite LR

    process_RC_input(incoming_command);
}

// =============================
//   TEST_CASE_RESTR_TASK_ACCESS_STACK
// =============================

void TC_RESTR_TASK_ACCESS_STACK(void *pvParameters) {
    int packet_counter = 0;
    float signal_strength = -50.0f;  // RSSI in dBm

    for (;;) {
        // Simulate sending telemetry data
        packet_counter++;
        signal_strength += (packet_counter % 10 == 0) ? -0.1f : 0.05f;

        // Simulate receiving a control packet
        if (packet_counter % 20 == 0) {
            float received_value = 1.0f / (float)packet_counter;
        }

        // RCctrl_Stack[0] = 120;

        // Enforce 100ms communication interval
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// =============================
//   TEST_CASE_CODE_INJ_ROP_LR_CHECK
// =============================

void process_RC_input(const char *input) {
    char rc_buffer[16];  // Small buffer for processing RC commands

    // *** Buffer Overflow Vulnerability ***
    strncpy(rc_buffer, input, 24);  // No bounds checking!
}

// Rctrl_task - BOF - ROP - Flight_ctrl task - Triggers LR USER event
void TC_INJ_ROP_LR_CHECK() { 
    char incoming_command[32];  // Larger than rc_buffer
    memcpy(incoming_command, "AAAAAAAAAAAAAAAABBBBCCCCDDDDDDDD", 32);
    incoming_command[16] = 0x08;
    incoming_command[17] = 0x35;
    incoming_command[18] = 0x02;
    incoming_command[19] = 0x00;  // Overwrite LR

    process_RC_input(incoming_command);
}

// =============================
//   TEST_CASE_WDOG_INTERFER
// =============================

#define portRTI_CNT1_CPUC1_REG  ( * ( ( volatile uint32_t * ) 0xFFFFFC38 ) )
#define portRTI_CNT1_COMP1_REG  ( * ( ( volatile uint32_t * ) 0xFFFFFC58 ) )
#define portRTI_CNT1_UDCP1_REG  ( * ( ( volatile uint32_t * ) 0xFFFFFC5C ) )
#define portRTI_CNT1_UC1_REG    ( * ( ( volatile uint32_t * ) 0xFFFFFC34 ) )
#define portRTI_CNT1_FRC1_REG   ( * ( ( volatile uint32_t * ) 0xFFFFFC30 ) )
#define portRTI_SETINTENA_REG   ( * ( ( volatile uint32_t * ) 0xFFFFFC80 ) )

void TC_WDOG_INTERFER() {
    /* CUSTOM CODE: Setup Timer 1 (e.g., for 4ms intervals). */
    portRTI_CNT1_UC1_REG  = 0x00000000U;
    portRTI_CNT1_FRC1_REG = 0x00000000U;

    /* Set prescaler and comparison values for Timer 1. */
    portRTI_CNT1_CPUC1_REG = 0x00000001U;
    portRTI_CNT1_COMP1_REG = ((configCPU_CLOCK_HZ / 2) / configTICK_RATE_HZ) * 500; // 40ms intervals
    portRTI_CNT1_UDCP1_REG = ((configCPU_CLOCK_HZ / 2) / configTICK_RATE_HZ) * 500;
    portRTI_SETINTENA_REG &= 0x00000001U;
}

// =============================
//   TEST_CASE_INJ_STACK_EXEC
// =============================

// RCTRL task - Redirect execution to stack buffer
// 0x08017DADC buffer address (task stack address)
void TC_INJ_STACK_EXEC() { 
    char incoming_command[32];
    memcpy(incoming_command,
        "\x01\x10\xa0\xe3"  // MOV R1, #1
        "\x02\x20\xa0\xe3"  // MOV R2, #2
        "\x03\x30\xa0\xe3"  // MOV R3, #3
        "\xBB\xBB\xBB\xBB" 
        "\xCC\xCC\xCC\xCC"  
        "\xDD\xDD\xDD\xDD"  
        "\xDD\xDD\xDD\xDD", 
        32);

//    printf("Addr of buffer: %u\n", rc_buffer);

    incoming_command[16] = 0xAC;
    incoming_command[17] = 0x7D;
    incoming_command[18] = 0x01;
    incoming_command[19] = 0x08;

    process_RC_input(incoming_command);
}

// =============================
//   TEST_CASE_PRIV_ROP_GADGET
// =============================

void TC_PRIV_ROP_GADGET(void *pvParameters) {
    int packet_counter = 0;
    float signal_strength = -50.0f;  // RSSI in dBm

    for (;;) {
        // Simulate sending telemetry data
        packet_counter++;
        signal_strength += (packet_counter % 10 == 0) ? -0.1f : 0.05f;

        // Simulate receiving a control packet
        if (packet_counter % 20 == 0) {
            float received_value = 1.0f / (float)packet_counter;
        }

        // Simulated malicious RC command
        char incoming_command[32];  // Larger than rc_buffer
        memcpy(incoming_command, "AAAAAAAAAAAAAAAABBBBCCCCDDDDDDDD", 32);
        // 00011128:   E59F0F4C            ldr        r0, [pc, #0xf4c]
        // portRTI_GCTRL_REG &= 0xFFFFFFFDUL; vTaskDelay
        incoming_command[16] = 0x28;
        incoming_command[17] = 0x11;
        incoming_command[18] = 0x01;
        incoming_command[19] = 0x00;  // Overwrite LR

        // Call vulnerable function
        process_RC_input(incoming_command);

        // Enforce 100ms communication interval
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// =============================
//   TEST_CASE_MODIFY_MPU_CONF
// =============================

/* MPU access routines defined in portASM.asm */
extern void prvMpuEnable( void );
extern void prvMpuDisable( void );
extern void prvMpuSetRegion( unsigned region, unsigned base, unsigned size, unsigned access );

void TC_MODIFY_MPU_CONF() {

    prvMpuDisable();
    
    prvMpuSetRegion(portPRIVILEGED_EDR_REGION,  0x0803F800 + 0x80, EDR_RW_MPU_SECT_SIZE | portMPU_REGION_ENABLE, portMPU_PRIV_RW_USER_RW_EXEC | portMPU_DEVICE_NONSHAREABLE);

    prvMpuSetRegion(portPRIVILEGED_RO_EDR_REGION, 0x0803F800, EDR_RO_MPU_SECT_SIZE | portMPU_REGION_ENABLE, portMPU_PRIV_RW_USER_RW_EXEC | portMPU_DEVICE_NONSHAREABLE);

    prvMpuEnable();

}

// =============================
//   TEST_CASE_EMAC_INTERFER
// =============================

#define EMAC_BASE_ADDRESS        0xFCF78800
#define TX_INT_ENABLE_REG_OFFSET 0x18

// triggers DABORT
void TC_EMAC_INTERFER() {
    // Calculate the address of the Transmit Interrupt Enable Register
    volatile uint32_t *tx_int_enable_reg = (volatile uint32_t *)(EMAC_BASE_ADDRESS + TX_INT_ENABLE_REG_OFFSET);

    // Zero out the entire register by writing 0x00000000
    *tx_int_enable_reg = 0x00000000;
}

// =============================
//   TEST_CASE_VERIFY_MPU_REGIONS
// =============================

void TC_VERIFY_MPU_REGIONS() {
    
    /* Test Unprivileged Peripheral Write - Should Trigger MPU Fault */
    volatile uint32_t *peripheralAddr = (uint32_t *)0xF0000000;
    *peripheralAddr = 0xA5A5A5A5;

    /* Test Unprivileged System Region Access - Should Trigger MPU Fault */
    volatile uint32_t value;
    value = *(volatile uint32_t *)0xFFF80000;

    volatile uint32_t *tx_int_enable_reg = (volatile uint32_t *)(EMAC_BASE_ADDRESS + TX_INT_ENABLE_REG_OFFSET);
    *tx_int_enable_reg = 0x00000000;

    /* Test Unprivileged RAM Write - Should Trigger MPU Fault */
    volatile uint32_t *privRamAddr = (uint32_t *)0x08000000;
    *privRamAddr = 0xDEADBEEF;

    /* Test Unprivileged Flash Access - Should Trigger MPU Fault */
    volatile uint32_t value2;
    value = *(volatile uint32_t *)0x00000000; /* Attempt to read privileged flash */

    return;
}


// =============================
//   TEST_CASE_TASK_CREATION
// =============================

void fun(int z) {
    int x = z;
    for(;;) {
        x += 1;
        vTaskDelay(600);
    }
}

void TC_TASK_CREATION() {
    xTaskCreate(fun, "Bad", 256, (void *)1, (2|portPRIVILEGE_BIT), NULL);
    return;
}

// =============================
//   TEST_CASE_NEVER_YIELD
// =============================

void TC_NEVER_YIELD() {
    int i = 0;
    /* WDOG EVENT */
    taskDISABLE_INTERRUPTS();
    while(i < 1000000000) {i += 1;};
}

// =============================
//   TEST_CASE_EDR_TASK_INTERFER
// =============================

void TC_EDR_TASK_INTERFER() {
    /* USER EVENT */
    // This will return NULL as recon on EDR task is detected in the MPU wrapper
    TaskHandle_t edr_h = xTaskGetHandle("EDR_RUN");

    // Hardcoding as an assumption that the adversary has obtained the EDR task handle via an unknown vector
    edr_h = 0x08013050;

    /* USER EVENT */
    vTaskPrioritySet(edr_h, 0);

    /* USER EVENT */
    vTaskDelete(edr_h);

    /* USER EVENT */
    vTaskSuspend(edr_h);

    /* USER EVENT */
    TaskStatus_t xTaskDetails;
    vTaskGetInfo(edr_h, &xTaskDetails, pdTRUE, eInvalid);

    return;
}

// =============================
//   TEST_CASE_MODIFY_EDR_STATE
// =============================

void TC_RW_EDR_STATE() {
    /* DABORT EVENT - Write to EDR data structure */
    edr_ptr_global->edr_sleep_time = 20000;

    /* DABORT EVENT - Read from EDR data structure */
    uint32_t sleep_time = edr_ptr_global->edr_sleep_time; 
}

// =============================
//   TEST_CASE_CODE_INJ_ROP_EXCEED_WCET
// =============================

// Comms Task - BOF - ROP - Calls Flight_ctrl task
// WCET 287 - AVG 273
// Normal: 179 171
void TC_CODE_INJ_ROP_EXCEED_WCET(void *pvParameters) {
    int packet_counter = 0;
    float signal_strength = -50.0f;  // RSSI in dBm

    for (;;) {
        // Simulate sending telemetry data
        packet_counter++;
        signal_strength += (packet_counter % 10 == 0) ? -0.1f : 0.05f;

        // Simulate receiving a control packet
        if (packet_counter % 20 == 0) {
            float received_value = 1.0f / (float)packet_counter;
        }

        // Simulated malicious RC command
        char incoming_command[32];  // Larger than rc_buffer
        memcpy(incoming_command, "AAAAAAAAAAAAAAAABBBBCCCCDDDDDDDD", 32);
        incoming_command[16] = 0x08;
        incoming_command[17] = 0x35;
        incoming_command[18] = 0x02;
        incoming_command[19] = 0x00;  // Overwrite LR

        // Call vulnerable function
        process_RC_input(incoming_command);

        // Enforce 100ms communication interval
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// =============================
//   TEST_CASE_CODE_INJ_ROP_BELOW_WCET
// =============================


// Rctrl_task - BOF - ROP - Flight_ctrl task
// WCET 578 - AVG 480
void TC_CODE_INJ_ROP_BELOW_WCET(void *pvParameters) {
    float throttle = 0.0f, roll = 0.0f, pitch = 0.0f, yaw = 0.0f;

    for (;;) {
        // Simulate receiving commands from the remote controller
        throttle = (throttle >= 100.0f) ? 0.0f : throttle + 0.5f;
        roll = sinf(throttle) * 30.0f;
        pitch = cosf(throttle) * 30.0f;
        yaw = atan2f(roll, pitch);
        
        char incoming_command[32];  // Larger than rc_buffer
        memcpy(incoming_command, "AAAAAAAAAAAAAAAABBBBCCCCDDDDDDDD", 32);
        incoming_command[16] = 0x38;
        incoming_command[17] = 0x34;
        incoming_command[18] = 0x02;
        incoming_command[19] = 0x00;  // Overwrite LR

        // Call vulnerable function
        process_RC_input(incoming_command);

        // Enforce 40ms update rate
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}
