#include "edr.h"

 EDR_RO_DATA task_tm_conf task_conf_array[4] = {
            {
                .static_avg_exec_time = 143,
                .max_time_allowed = 168,
                .max_avg_deviation = 50,
                .lr_yield_addrs = {
                    { 150332 },        // vTaskDelay
                    { 0 },   // vTaskDelayUntil
                    { 0 },  // taskYIELD_FROM_TASK
                    { 0 }          // vTaskSuspend
                }
            },
    {
                .static_avg_exec_time = 293,
                .max_time_allowed = 399,
                .max_avg_deviation = 60,
                .lr_yield_addrs = {
                    { 150540 },        // vTaskDelay
                    { 0 },   // vTaskDelayUntil
                    { 0 },  // taskYIELD_FROM_TASK
                    { 0 }          // vTaskSuspend
                }
            },
    {
                .static_avg_exec_time = 415,
                .max_time_allowed = 583,
                .max_avg_deviation = 80,
                .lr_yield_addrs = {
                    { 150704 },        // vTaskDelay
                    { 0 },   // vTaskDelayUntil
                    { 0 },  // taskYIELD_FROM_TASK
                    { 0 }          // vTaskSuspend
                }
            },
    {
                .static_avg_exec_time = 171,
                .max_time_allowed = 198,
                .max_avg_deviation = 50,
                .lr_yield_addrs = {
                    { 150848 },        // vTaskDelay
                    { 0 },   // vTaskDelayUntil
                    { 150880 },  // taskYIELD_FROM_TASK
                    { 0 }          // vTaskSuspend
                }
            }
};
