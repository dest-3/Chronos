import json
import math
import os

def next_power_of_2(n):
    if n < 0x80:
        return 0x80
    if n == 0:
        return 1
    return 2 ** math.ceil(math.log2(n))

def create_edr_mem_section(sect_name, base_addr, file_path, new_length):

    edr_line_template = "    {}\t\t(RW) : origin={} length=0x{:X}\n"
    edr_line = edr_line_template.format(sect_name, base_addr, new_length)
    memory_block_start = "MEMORY"
    memory_block_end = "}"
    modified = False
    in_memory_block = False

    with open(file_path, "r") as file:
        lines = file.readlines()

    updated_lines = []
    for line in lines:
        if memory_block_start in line and not in_memory_block:
            in_memory_block = True
            updated_lines.append(line)
            continue

        if in_memory_block and memory_block_end in line:
            if not modified: 
                updated_lines.append(edr_line)
                modified = True
            in_memory_block = False

        if in_memory_block and line.strip().startswith(sect_name):
            updated_lines.append(edr_line)
            modified = True
        else:
            updated_lines.append(line)

    if not any(memory_block_start in line for line in lines):
        raise ValueError("[ERROR] No MEMORY block found in the linker script.")

    with open(file_path, "w") as file:
        file.writelines(updated_lines)
        print("[+] Set EDR memory section length in linker script.")


def generate_c_header(json_data, header_filename, source_filename):
    # Parse the JSON data
    config = json.loads(json_data)

    # Extract fields
    total_tasks = config.get("totalTasks", 0)
    initial_edr_sleep_time = config.get("INIT_EDR_SLEEP_TIME", 0)
    min_edr_sleep = config.get("MIN_EDR_SLEEP_TIME", 0)
    max_edr_mem = config.get("MAX_EDR_MEM", 0)
    time_check_freq = config.get("TIME_CHECK_TICK_FREQ", 0)
    exec_times_len = config.get("CONCRETE_EXEC_TIMES_ARRAY_LEN", 0)
    hb_timer_freq = config.get("HBEAT_TICK_FREQ", 0)
    ema_scale = config.get("EMA_SCALE_FACTOR", 0)
    n_samples_avg = config.get("NUM_SAMPLES_FOR_AVERAGE", 0)
    ret_addr_check = config.get("ENABLE_YIELD_RET_ADDR_CHECK", 0)
    full_check = config.get("FULL_EXEC_CHECK", 0)
    edr_task_prio = config.get("EDR_TASK_PRIORITY", 0)

    max_yield_addresses = max(
        len(task["vTaskDelay_LR"]) +
        len(task["vTaskDelayUntil_LR"]) +
        len(task["taskYIELD_FROM_TASK"]) +
        len(task["vTaskSuspend"]) for task in config["tasks"]
    )

    timing_resp = config.get("TIMING_RESPONSE")
    wdog_resp = config.get("WDOG_RESPONSE")
    dabort_resp = config.get("DABORT_RESPONSE")
    pabort_resp = config.get("PABORT_RESPONSE")
    user_resp = config.get("USER_RESPONSE")
    kernel_resp = config.get("KERNEL_RESPONSE")
    ema_smooth_factor = int(2 /(n_samples_avg + 1) * ema_scale)
    if (ema_smooth_factor < 46): # min 10 samples for average
       raise ValueError("[ERROR] Smoothing factor is less than the min for 10 samples")

    print("[+] Calculated EMA Smoothing Factor for {} samples: {}".format(n_samples_avg, ema_smooth_factor))

    enum_values = ["\tTIMING,\n", "\tDABORT,\n", "\tPABORT,\n", "\tWDOG,\n", "\tUSER,\n", "\tKERNEL"]

    # Calculate memory required by EDR data structures
    # EDR_META_ARR_SIZE * EDR_META_ARR_STRUCT_SIZE + ((EDR_SECURITY_EVENT_STRUCT_SIZE * 3) + 3 * META_BUF_SIZE) + EDR_SEC_EVENT_PTR_ARR_SIZE + EDR_META SIZE + HB_DATA_SIZE(20) + UDP_PCB_PTR_SIZE + IP_ADDR_STRUCT_SIZE + PBUF_PTR_SIZE

    # 7 UINTS + 3 single byte flags + 2 bytes padding + 4 * array_len + 
    if (ret_addr_check):
        edr_task_meta_struct_size = (7 * 4) + (2 + 2) + (4 * exec_times_len)
    else:
        edr_task_meta_struct_size = (5 * 4) + (2 + 2) + (4 * exec_times_len)

    edr_sec_event_structs_size = 8 * 5 # * 5 since we have 5 sec event types
    meta_buf_size = 45
    global_ptrs_size = 2 * 4
    edr_meta_size = 6 * 3
    hb_arr_size = 20
    lwip_struct_ptrs_size = 4 * 2
    ip_addr_struct_size = 4 + 16 + 1
    sec_event_arr_ptr_size = 5 * 4
    emac_addr_size = 6 + 4
    adaptive_struct_size = (6 * 4) + (4 * 10)
    # sec_event_enum_size = 5
    last_idle_check_size = 4


    total_rw_memory = last_idle_check_size + edr_meta_size + (total_tasks * edr_task_meta_struct_size) + adaptive_struct_size + (edr_sec_event_structs_size + (5 * meta_buf_size)) + (global_ptrs_size) + hb_arr_size + lwip_struct_ptrs_size + ip_addr_struct_size + sec_event_arr_ptr_size + emac_addr_size #+ sec_event_enum_size
    
    if (ret_addr_check):
        fixed_ro_struct_size = 4 * 3 # 3 ints
        yield_arr_size = 4 * max_yield_addresses * 4 # 2d arr, 4 rows , max_yield_addresses columns
        total_ro_memory = (fixed_ro_struct_size + yield_arr_size) * total_tasks
    else: 
        total_ro_memory = (4 * 3) * total_tasks

    total_memory = total_rw_memory + total_ro_memory

    edr_rw_mpu_sect_size = next_power_of_2(total_rw_memory)
    edr_ro_mpu_sect_size = next_power_of_2(total_ro_memory)

    base_addr = 0x0803F800

    create_edr_mem_section("RO_EDR", str(hex(base_addr)), "source/sys_link.cmd", edr_ro_mpu_sect_size)

    base_addr += edr_ro_mpu_sect_size
    create_edr_mem_section("RW_EDR", str(hex(base_addr)), "source/sys_link.cmd", edr_rw_mpu_sect_size)

    print("[+] Total memory in EDR RO mem region \nACTUAL: [INT = {}] [HEX = {}] \nROUNDED: [INT = {}] [HEX = {}]".format(total_ro_memory, hex(total_ro_memory), edr_ro_mpu_sect_size, hex(edr_ro_mpu_sect_size)))
    print("[+] Total memory in EDR RW mem region \nACTUAL: [INT = {}] [HEX = {}] \nROUNDED: [INT = {}] [HEX = {}]".format(total_rw_memory, hex(total_rw_memory), edr_rw_mpu_sect_size, hex(edr_rw_mpu_sect_size)))

    bit_shift = math.log2(edr_rw_mpu_sect_size)
    macro_rw_val = f"( 1UL << {int(bit_shift)}UL )"

    bit_shift = math.log2(edr_ro_mpu_sect_size)
    macro_ro_val = f"( 1UL << {int(bit_shift)}UL )"

    if total_memory > max_edr_mem:
        print("[WARNING] TOTAL MEMORY REQUIRED BY EDR DATA STRUCTURES EXCEEDS MAX EDR MEMORY SET")

    # Generate the C header content
    header_content = f"""/* Auto-generated configuration header */
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

#define EDR_META_ARR_SIZE {total_tasks}
#define EDR_SLEEP_MS {initial_edr_sleep_time}
#define EDR_MIN_SLEEP {min_edr_sleep}
#define EDR_RW_MPU_SECT_SIZE {macro_rw_val}
#define EDR_RO_MPU_SECT_SIZE {macro_ro_val}
#define EDR_TIME_CHECK_TICK_FREQ {time_check_freq}
#define EDR_EXEC_TIMES_NUM {exec_times_len}
#define EDR_IDLE_MNTR_INTERVAL_TICKS 200
#define EDR_NUM_SAMPLES_AVG {n_samples_avg}
#define EDR_HB_TICK_FREQ {hb_timer_freq}
#define EDR_TASK_PRIORITY {edr_task_prio}
#define EMA_SCALE {ema_scale}
#define EMA_SMOOTHING_FACTOR {ema_smooth_factor}
#define IDLE_WINDOW_SIZE 10
#define TIMING_RESP {timing_resp}
#define DABORT_RESP {dabort_resp}
#define PABORT_RESP {pabort_resp}
#define WDOG_RESP {wdog_resp}
#define USER_RESP {user_resp}
#define KERNEL_RESP {kernel_resp}
#define YIELD_RET_ADDR_CHECK {ret_addr_check}
#define MAX_YIELD_ADDRS {max_yield_addresses}
#define FULL_EXEC_CHECK {full_check}

typedef enum {{
{''.join(enum_values)}
}} sec_event_type;

typedef enum {{
    LOG, /* Default */
    DELETE
}} sec_event_resp;

typedef struct {{
    uint32_t running_idle_t;
    uint32_t last_check_time;
    uint32_t idle_times[IDLE_WINDOW_SIZE];
    uint32_t idle_cnt;
    uint32_t curr_idle_avg;
    uint32_t last_idle_avg;
    uint32_t lowest_idle;
}} adaptive_metadata;

typedef struct {{
    uint32_t exec_times;
    uint32_t avg_exec_time;
    uint32_t exec_index;
    uint32_t exec_times_arr[EDR_EXEC_TIMES_NUM];
    uint8_t to_delete;
    uint8_t deleted;
    uint8_t voluntary_yield;
    uint32_t task_handle;
    uint32_t running_exec_t;"""

    if (ret_addr_check):
        header_content += """
    uint32_t yield_ret_addr;
} edr_task_metadata;"""
    else:
        header_content += """
} edr_task_metadata;"""
    header_content += """

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
    uint32_t max_avg_deviation;"""
    
    if (ret_addr_check):
        header_content += """
    uint32_t lr_yield_addrs[4][MAX_YIELD_ADDRS];
} task_tm_conf;
"""
    else:
        header_content += """
} task_tm_conf;

"""

    # Initialize the struct
    header_content += """\nEDR_DATA static edr_meta edr_meta_gbl = {
    .meta_arr = {
    """

    # Add task configurations
    for task in config["tasks"]:
        if(ret_addr_check):
            header_content += f"        {{0, 0, 0, {{0}}, 0, 0, 0, 0, 0, 0}}, // Task {task['id']}\n"
        else: 
            header_content += f"        {{0, 0, 0, {{0}}, 0, 0, 0, 0, 0}}, // Task {task['id']}\n"

    # Close the meta_arr without a trailing comma for the last element
    header_content = header_content.rstrip(",\n") + "\n    },\n"

    # Close the struct initialization
    header_content += """\
    .edr_sleep_time = 0,
    .edr_t_handle = 0
};

"""

    if (ret_addr_check):
        header_content += """extern EDR_RO_DATA task_tm_conf task_conf_array[];
"""

    if (ret_addr_check):
        source_content = """#include "edr.h"\n\n """
        source_content += f"""EDR_RO_DATA task_tm_conf task_conf_array[{len(config["tasks"])}] = {{
        """

        for i, task in enumerate(config["tasks"]):
            # Extract addresses for each yield function, ensuring they're correctly formatted as lists
            vTaskDelay_LR = ", ".join(map(str, task["vTaskDelay_LR"]))
            vTaskDelayUntil_LR = ", ".join(map(str, task["vTaskDelayUntil_LR"]))
            taskYIELD_FROM_TASK = ", ".join(map(str, task["taskYIELD_FROM_TASK"]))
            vTaskSuspend = ", ".join(map(str, task["vTaskSuspend"]))

            source_content += f"""    {{
                .static_avg_exec_time = {task['averageExecutionTime']},
                .max_time_allowed = {task['worstCaseExecutionTime']},
                .max_avg_deviation = {task['maximumAverageDeviation']},
                .lr_yield_addrs = {{
                    {{ {vTaskDelay_LR} }},        // vTaskDelay
                    {{ {vTaskDelayUntil_LR} }},   // vTaskDelayUntil
                    {{ {taskYIELD_FROM_TASK} }},  // taskYIELD_FROM_TASK
                    {{ {vTaskSuspend} }}          // vTaskSuspend
                }}
            }}"""
            
            # Add a comma if it's not the last task
            if i < len(config["tasks"]) - 1:
                source_content += ",\n"
            else:
                source_content += "\n"

        source_content += "};\n"

        with open(f"source/{source_filename}", "w+") as source_file:
            source_file.write(source_content)
    else:
        header_content += f"""EDR_RO_DATA static task_tm_conf task_conf_array[{len(config["tasks"])}] = {{
    """
        for i, task in enumerate(config["tasks"]):
            header_content += f"""    {{
                .static_avg_exec_time = {task['averageExecutionTime']},
                .max_time_allowed = {task['worstCaseExecutionTime']},
                .max_avg_deviation = {task['maximumAverageDeviation']}
            }}"""
            # Add a comma if it's not the last task
            if i < len(config["tasks"]) - 1:
                header_content += ",\n"
            else:
                header_content += "\n"

        header_content += "};\n"

        # delete edr_ro_dat if it exists
        os.system("rm source/edr_ro_dat.c")

    header_content += """
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
"""

    # Write to the output file
    with open("include/{}".format(header_filename), "w") as header_file:
        header_file.write(header_content)

def main():
    json_input = '''{
        "totalTasks": 5,
        "MAX_EDR_MEM": 1024,
        "EDR_TASK_PRIORITY": "3",
        "INIT_EDR_SLEEP_TIME": 3000,
        "MIN_EDR_SLEEP_TIME": 100,
        "TIME_CHECK_TICK_FREQ": 1000,
        "HBEAT_TICK_FREQ": 2000,
        "CONCRETE_EXEC_TIMES_ARRAY_LEN": 10,
        "EMA_SCALE_FACTOR": 256,
        "NUM_SAMPLES_FOR_AVERAGE": 8,
        "TIMING_RESPONSE": "LOG",
        "WDOG_RESPONSE": "LOG",
        "DABORT_RESPONSE": "DELETE",
        "PABORT_RESPONSE": "DELETE",
        "USER_RESPONSE" : "LOG",
        "KERNEL_RESPONSE": "LOG",
        "ENABLE_YIELD_RET_ADDR_CHECK": 0,
        "FULL_EXEC_CHECK": 0,
        "tasks": [
            {"id": 0, "averageExecutionTime": 143, "worstCaseExecutionTime": 168, "maximumAverageDeviation": 50, "vTaskDelay_LR": [150332], "vTaskDelayUntil_LR": [0], "taskYIELD_FROM_TASK": [0], "vTaskSuspend":[0]},
            {"id": 1, "averageExecutionTime": 293, "worstCaseExecutionTime": 399, "maximumAverageDeviation": 60, "vTaskDelay_LR": [150540], "vTaskDelayUntil_LR": [0], "taskYIELD_FROM_TASK": [0], "vTaskSuspend":[0]},
            {"id": 2, "averageExecutionTime": 415, "worstCaseExecutionTime": 583, "maximumAverageDeviation": 80, "vTaskDelay_LR": [150704], "vTaskDelayUntil_LR": [0], "taskYIELD_FROM_TASK": [0], "vTaskSuspend":[0]},
            {"id": 3, "averageExecutionTime": 171, "worstCaseExecutionTime": 198, "maximumAverageDeviation": 50, "vTaskDelay_LR": [150848], "vTaskDelayUntil_LR": [0], "taskYIELD_FROM_TASK": [150880], "vTaskSuspend":[0]},
            {"id": 4, "averageExecutionTime": 171, "worstCaseExecutionTime": 198, "maximumAverageDeviation": 50, "vTaskDelay_LR": [150848], "vTaskDelayUntil_LR": [0], "taskYIELD_FROM_TASK": [150880], "vTaskSuspend":[0]}
        ]
    }'''

    # Generate the header file
    generate_c_header(json_input, "edr.h", "edr_ro_dat.c")
    print("[+] Generated EDR header file")

main()
