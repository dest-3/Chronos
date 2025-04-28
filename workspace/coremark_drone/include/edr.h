/* Auto-generated configuration header */
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

#ifndef EDR_H
#define EDR_H

#ifndef EDR_DATA
#define EDR_DATA __attribute__((section(".kernelExtension")))
#endif
#ifndef PRIVILEGED_FUNC
#define PRIVILEGED_FUNC __attribute__((section(".kernelTEXT")))
#endif
#ifndef EDR_RO_DATA
#define EDR_RO_DATA __attribute__((section(".kernelROExt")))
#endif

#define EDR_META_ARR_SIZE 4
#define EDR_SLEEP_MS 150
#define EDR_MIN_SLEEP 30
#define EDR_RW_MPU_SECT_SIZE ( 1UL << 10UL )
#define EDR_RO_MPU_SECT_SIZE ( 1UL << 9UL )
#define EDR_TIME_CHECK_TICK_FREQ 150
#define EDR_EXEC_TIMES_NUM 6
#define EDR_IDLE_MNTR_INTERVAL_TICKS 200
#define EDR_NUM_SAMPLES_AVG 8
#define EDR_HB_TICK_FREQ 500
#define EDR_TASK_PRIORITY 3
#define EMA_SCALE 256
#define EMA_SMOOTHING_FACTOR 56
#define IDLE_WINDOW_SIZE 10
#define TIMING_RESP LOG
#define DABORT_RESP DELETE
#define PABORT_RESP DELETE
#define WDOG_RESP LOG
#define USER_RESP LOG
#define KERNEL_RESP LOG
#define YIELD_RET_ADDR_CHECK 1
#define MAX_YIELD_ADDRS 4
#define FULL_EXEC_CHECK 0

typedef enum {
	TIMING,
	DABORT,
	PABORT,
	WDOG,
	USER,
	KERNEL
} sec_event_type;

typedef enum {
    LOG, /* Default */
    DELETE
} sec_event_resp;

typedef struct {
    uint32_t running_idle_t;
    uint32_t last_check_time;
    uint32_t idle_times[IDLE_WINDOW_SIZE];
    uint32_t idle_cnt;
    uint32_t curr_idle_avg;
    uint32_t last_idle_avg;
    uint32_t lowest_idle;
} adaptive_metadata;

typedef struct {
    uint32_t exec_times;
    uint32_t avg_exec_time;
    uint32_t exec_index;
    uint32_t exec_times_arr[EDR_EXEC_TIMES_NUM];
    uint8_t to_delete;
    uint8_t deleted;
    uint8_t voluntary_yield;
    uint32_t task_handle;
    uint32_t running_exec_t;
    uint32_t yield_ret_addr;
} edr_task_metadata;

typedef struct {
    edr_task_metadata meta_arr[EDR_META_ARR_SIZE];
    uint32_t edr_sleep_time;
    uint32_t edr_t_handle;
    uint32_t curr_tcb_num;
    uint32_t hb_tick_freq;
    uint32_t time_check_freq;
} edr_meta;

typedef struct {
    uint32_t static_avg_exec_time;
    uint32_t max_time_allowed;
    uint32_t max_avg_deviation;
    uint32_t lr_yield_addrs[4][MAX_YIELD_ADDRS];
} task_tm_conf;

EDR_DATA static edr_meta edr_meta_gbl = {
    .meta_arr = {
            {0, 0, 0, {0}, 0, 0, 0, 0, 0, 0}, // Task 0
        {0, 0, 0, {0}, 0, 0, 0, 0, 0, 0}, // Task 1
        {0, 0, 0, {0}, 0, 0, 0, 0, 0, 0}, // Task 2
        {0, 0, 0, {0}, 0, 0, 0, 0, 0, 0}, // Task 3
    },
    .edr_sleep_time = 0,
    .edr_t_handle = 0
};

extern EDR_RO_DATA task_tm_conf task_conf_array[];

typedef struct {
    int susp;
    char * metadata;
} edr_security_event;

extern void EMAC_LwIP_Main (uint8_t * emacAddress) PRIVILEGED_FUNC;

void init_task_meta(void * func_ptr, int task_num) PRIVILEGED_FUNC;
void edr_init() PRIVILEGED_FUNC;
void edr_run(void * pvParameters) PRIVILEGED_FUNC;
void reg_user_event(uint32_t api_id) PRIVILEGED_FUNC;
void reg_krnl_event() PRIVILEGED_FUNC;
void reg_dabort_event() PRIVILEGED_FUNC;
void reg_pabort_event() PRIVILEGED_FUNC;
void set_task_handle(uint32_t task_handle, int task_num) PRIVILEGED_FUNC;

extern adaptive_metadata * ad_meta_ptr;
extern edr_meta * edr_ptr_global;

#endif /* EDR_H */
