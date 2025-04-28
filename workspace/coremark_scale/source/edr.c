#include "edr.h"
#include "FreeRTOS.h"
#include "os_task.h"
#include "os_timer.h"
#include "os_semphr.h"

#include "lwip\udp.h"
#include "lwip\pbuf.h"

EDR_DATA edr_meta * edr_ptr_global = &edr_meta_gbl;

EDR_DATA static edr_security_event sec_timing_event;
EDR_DATA static edr_security_event sec_dabort_event;
EDR_DATA static edr_security_event sec_wd_event;
EDR_DATA static edr_security_event sec_user_event;
EDR_DATA static edr_security_event sec_krnl_event;

EDR_DATA adaptive_metadata ad_meta_gbl;
EDR_DATA adaptive_metadata * ad_meta_ptr = &ad_meta_gbl;

EDR_DATA struct udp_pcb *upcb;
EDR_DATA struct ip_addr DestIPaddr;
EDR_DATA struct pbuf *p;
EDR_DATA u8_t data[20];

EDR_DATA char timing_metadata[45];
EDR_DATA char dabort_metadata[45];
EDR_DATA char wd_metadata[45];
EDR_DATA char user_metadata[45];
EDR_DATA char kernel_metadata[45];
EDR_DATA edr_security_event * sec_event_arr[5];

EDR_DATA uint32_t last_idle_check_t;

extern uint32_t get_DFAR();
extern uint32_t get_IFAR();

PRIVILEGED_FUNC void vApplicationIdleHook() {
    if ((xTaskGetTickCount() - last_idle_check_t) >= EDR_IDLE_MNTR_INTERVAL_TICKS) {
        uint32_t curr_idle_time = ad_meta_gbl.running_idle_t;
//        printf("[+] Curr idle_time: %u\n", curr_idle_time);
        ad_meta_gbl.idle_times[ad_meta_gbl.idle_cnt % IDLE_WINDOW_SIZE] = curr_idle_time;
        ad_meta_gbl.idle_cnt += 1;
//        printf("[+] Current idle count: %d\n", idle_c);
        last_idle_check_t = xTaskGetTickCount();

        // Reset idle time for current MNTR TICK INTERVAL
        ad_meta_gbl.running_idle_t = 0;

        if (ad_meta_gbl.idle_cnt >= IDLE_WINDOW_SIZE) {
            uint32_t sum = 0;
            uint32_t lowest_idle = ad_meta_gbl.idle_times[0];
            uint32_t curr_val = 0;
            uint32_t last_sum = 0;

            int i;
            for (i = 0; i < IDLE_WINDOW_SIZE; i++) {
                curr_val = ad_meta_gbl.idle_times[i];
                if (curr_val < lowest_idle) {
                    lowest_idle = curr_val;
                    ad_meta_gbl.lowest_idle = lowest_idle;
                }
                last_sum = sum;
                sum += curr_val;
                if (last_sum > sum) {
                    sum = last_sum;
                    printf("[!] Handled INT Overflow in IDLE TIME sum\n");
                    break;
                }
            }

//            printf("[+] Sum of idle time: %u\n", sum);

            uint32_t curr_idle_avg = sum / IDLE_WINDOW_SIZE;
            uint32_t last_idle_avg = ad_meta_gbl.last_idle_avg;
            uint32_t edr_sleep_time = edr_ptr_global->edr_sleep_time;
            ad_meta_gbl.curr_idle_avg = curr_idle_avg;

//            printf("[+] Curr idle avg: %u - last idle avg: %u\n", curr_idle_avg, last_idle_avg);

            if (curr_idle_avg > 100 && edr_sleep_time > EDR_MIN_SLEEP) {
                edr_ptr_global->edr_sleep_time = edr_sleep_time - (edr_sleep_time * 0.25);
                edr_ptr_global->hb_tick_freq = edr_ptr_global->hb_tick_freq - (edr_ptr_global->hb_tick_freq * 0.25);
                edr_ptr_global->time_check_freq = edr_ptr_global->time_check_freq - (edr_ptr_global->time_check_freq * 0.25);
//                printf("[+] Idle avg > 100 and Sleep time greater than 100, reducing by 0.25\n");
            } else if (curr_idle_avg <= 100) {
                if (last_idle_avg < curr_idle_avg && last_idle_avg != 0) {
                    uint32_t incr_factor = curr_idle_avg / last_idle_avg;
//                    printf("[+] Increase factor: %u\n", incr_factor);
                    edr_ptr_global->edr_sleep_time = (edr_sleep_time / incr_factor);
                    edr_ptr_global->hb_tick_freq = (edr_ptr_global->hb_tick_freq / incr_factor);
                    edr_ptr_global->time_check_freq = (edr_ptr_global->time_check_freq / incr_factor);
//                    printf("[+] Last idle avg is lower, system has less load now\n");
                } else if (last_idle_avg > curr_idle_avg && last_idle_avg != 0) {
                    uint32_t decr_factor = last_idle_avg / curr_idle_avg;
//                    printf("[+] Decrease factor:  %u\n", decr_factor);
                    edr_ptr_global->edr_sleep_time = (edr_sleep_time * decr_factor);
                    edr_ptr_global->hb_tick_freq = (edr_ptr_global->hb_tick_freq * decr_factor);
                    edr_ptr_global->time_check_freq = (edr_ptr_global->time_check_freq * decr_factor);
//                    printf("[+] Last idle avg is higher, system has more load now\n");
                }
            }

            ad_meta_gbl.last_idle_avg = ad_meta_gbl.curr_idle_avg;
            ad_meta_gbl.idle_cnt = 0;

            //printf("\nNew sleep time: %u\n", edr_ptr_global->edr_sleep_time);
        }
    }

    return;
}

PRIVILEGED_FUNC void comms_init() {
    err_t err;
    upcb = udp_new();

    if (upcb != NULL) {
      IP4_ADDR( &DestIPaddr, 192, 168, 4, 100 );
      err = udp_connect(upcb, &DestIPaddr, 5000);
      if (err == ERR_OK) {
          printf("Connected to UDP server\n");
      }
    } else {
        printf("Failed to init PCB\n");
        return;
    }

}

PRIVILEGED_FUNC send_hb() {
    p = pbuf_alloc(PBUF_TRANSPORT,strlen((char*)data), PBUF_POOL);

    if (p != NULL)
    {
      /* copy data to pbuf */
      pbuf_take(p, (char*)data, strlen((char*)data));

      err_t err;
      /* send udp data */
      err = udp_send(upcb, p);

      //printf("---------------> Sent HeartBeat\n");

      if (err != ERR_OK) {
          printf("ERROR IN UDP HB SEND\n");
      }


      /* free pbuf */
      pbuf_free(p);
    }
}

PRIVILEGED_FUNC send_susp(char * metadata) {
    p = pbuf_alloc(PBUF_TRANSPORT,strlen((char*)metadata), PBUF_POOL);

    if (p != NULL)
    {
      /* copy data to pbuf */
      pbuf_take(p, (char*)metadata, strlen((char*)metadata));

      err_t err;

      /* send udp data */
      err = udp_send(upcb, p);

      if (err != ERR_OK) {
          printf("ERROR IN UDP META SEND\n");
      }

      //printf("---------------> Sent sus metadata\n");

      /* free pbuf */
      pbuf_free(p);
    }
}

PRIVILEGED_FUNC void edr_init() {
    sec_event_arr[0] = &sec_timing_event;
    sec_event_arr[0]->susp = 0;
    sec_event_arr[0]->metadata = timing_metadata;

    sec_event_arr[1] = &sec_dabort_event;
    sec_event_arr[1]->susp = 0;
    sec_event_arr[1]->metadata = dabort_metadata;

    sec_event_arr[2] = &sec_wd_event;
    sec_event_arr[2]->susp = 0;
    sec_event_arr[2]->metadata = wd_metadata;

    sec_event_arr[3] = &sec_user_event;
    sec_event_arr[3]->susp = 0;
    sec_event_arr[3]->metadata = user_metadata;

    sec_event_arr[4] = &sec_krnl_event;
    sec_event_arr[4]->susp = 0;
    sec_event_arr[4]->metadata = kernel_metadata;

    ad_meta_gbl.curr_idle_avg = 0;
    ad_meta_gbl.running_idle_t = 0;
    ad_meta_gbl.idle_cnt = 0;
    ad_meta_gbl.curr_idle_avg = 0;
    ad_meta_gbl.last_idle_avg = 0;
    ad_meta_gbl.lowest_idle = 0;

    int i = 0;
    for (i = 0; i < IDLE_WINDOW_SIZE; i++) {
        ad_meta_gbl.idle_times[i] = 0;
    }

    edr_meta_gbl.edr_t_handle = 0;
    edr_meta_gbl.edr_sleep_time = EDR_SLEEP_MS;
    edr_meta_gbl.hb_tick_freq = EDR_HB_TICK_FREQ;
    edr_meta_gbl.time_check_freq =  EDR_TIME_CHECK_TICK_FREQ;
    edr_meta_gbl.curr_tcb_num = 0;
    comms_init();
}

PRIVILEGED_FUNC void set_task_handle(uint32_t task_handle, int task_num) {
    edr_meta_gbl.meta_arr[task_num].task_handle = task_handle;
}

PRIVILEGED_FUNC void sec_poll_task() {

    //printf("[+] Polling for sus\n");
    int i = 0;

    for (i = 0; i < 5; i++) {
        if (sec_event_arr[i]->susp) {
            sec_event_arr[i]->susp = 0;
            send_susp(sec_event_arr[i]->metadata);
        }
    }
}

PRIVILEGED_FUNC void heartbeat() {
    memset(data, 0, 20);
    snprintf((char*)data, 19, "EDR Heartbeat %d", 1);
    send_hb();

    return;
}

PRIVILEGED_FUNC void reg_sec_event(int task_num, char * detection, uint32_t val, sec_event_type event_type) {
    edr_security_event * sec_ev = sec_event_arr[event_type];
    memset(sec_ev->metadata, 0, 45);
    snprintf((char*)sec_ev->metadata, 44, "[%u][%s] - Task: %u, Val: %u",  xTaskGetTickCount(), detection, task_num, val);
    sec_ev->susp = 1;

    return;
}

PRIVILEGED_FUNC void reg_wd_event() {
    reg_sec_event(edr_ptr_global->curr_tcb_num, "WDOG", 0, WDOG);
    //printf("[!] Registered suspicious WD event\n");

    if (WDOG_RESP == DELETE) {
        taskENTER_CRITICAL();
        edr_task_metadata * task_meta = &(edr_ptr_global->meta_arr[edr_ptr_global->curr_tcb_num]);
        if (task_meta->to_delete != 1) {
            vTaskDelete(*((TaskHandle_t *)edr_ptr_global->meta_arr[edr_ptr_global->curr_tcb_num].task_handle));
        }
        taskEXIT_CRITICAL();
    }

    return;
}

PRIVILEGED_FUNC void custom_dabort() {
    reg_sec_event(edr_ptr_global->curr_tcb_num, "DABORT", get_DFAR(), DABORT);
    //printf("[!] Registered suspicious DABORT event\n");

    if (DABORT_RESP == DELETE) {
        taskENTER_CRITICAL();
        edr_task_metadata * task_meta = &(edr_ptr_global->meta_arr[edr_ptr_global->curr_tcb_num]);
        if (task_meta->to_delete != 1) {
            vTaskDelete(*((TaskHandle_t *)edr_ptr_global->meta_arr[edr_ptr_global->curr_tcb_num].task_handle));
        }
        taskEXIT_CRITICAL();
    }

    return;
}

PRIVILEGED_FUNC void prefetch_handler() {
    reg_sec_event(edr_ptr_global->curr_tcb_num, "PABORT", get_IFAR(), PABORT);
    //printf("[!] Registered suspicious DABORT event\n");

    if (PABORT_RESP == DELETE) {
        taskENTER_CRITICAL();
        edr_task_metadata * task_meta = &(edr_ptr_global->meta_arr[edr_ptr_global->curr_tcb_num]);
        if (task_meta->to_delete != 1) {
            vTaskDelete(*((TaskHandle_t *)edr_ptr_global->meta_arr[edr_ptr_global->curr_tcb_num].task_handle));
        }
        taskEXIT_CRITICAL();
    }

    return;
}

PRIVILEGED_FUNC void reg_user_event(uint32_t api_id) {
    reg_sec_event(edr_ptr_global->curr_tcb_num, "USER", api_id, USER);
    //printf("[!] Registered suspicious USER event\n");

    if (USER_RESP == DELETE) {
        taskENTER_CRITICAL();
        edr_task_metadata * task_meta = &(edr_ptr_global->meta_arr[edr_ptr_global->curr_tcb_num]);
        if (task_meta->to_delete != 1) {
            vTaskDelete(*((TaskHandle_t *)edr_ptr_global->meta_arr[edr_ptr_global->curr_tcb_num].task_handle));
        }
        taskEXIT_CRITICAL();
    }

    return;
}

PRIVILEGED_FUNC void reg_krnl_event() {
    reg_sec_event(edr_ptr_global->curr_tcb_num, "KERNEL", 0, KERNEL);
    //printf("[!] Registered suspicious KERNEL event\n");

    if (KERNEL_RESP == DELETE) {
        taskENTER_CRITICAL();
        edr_task_metadata * task_meta = &(edr_ptr_global->meta_arr[edr_ptr_global->curr_tcb_num]);
        if (task_meta->to_delete != 1) {
            vTaskDelete(*((TaskHandle_t *)edr_ptr_global->meta_arr[edr_ptr_global->curr_tcb_num].task_handle));
        }
        taskEXIT_CRITICAL();
    }

    return;
}

PRIVILEGED_FUNC void time_analysis() {
    uint32_t max_avg_dev = 0;
    uint32_t max_time_allowed = 0;
    uint32_t last_exec_time = 0;
    uint32_t avg_exec_time = 0;
    uint8_t to_delete = 0;
    uint8_t deleted = 0;
    uint32_t static_avg_exec_time = 0;
    uint32_t exec_times = 0;
    uint32_t running_exec_time = 0;

    int i;

    for (i = 0; i < EDR_META_ARR_SIZE; i++) {
        // printf("Task num: %u\n", i);
        deleted = edr_ptr_global->meta_arr[i].deleted;
        if (!deleted && (edr_ptr_global != NULL)) {

            // TIME DEVIATION CHECK
            max_avg_dev = task_conf_array[i].max_avg_deviation;
            avg_exec_time = edr_ptr_global->meta_arr[i].avg_exec_time;
            max_time_allowed = task_conf_array[i].max_time_allowed;
            to_delete = edr_ptr_global->meta_arr[i].to_delete;
            static_avg_exec_time = task_conf_array[i].static_avg_exec_time;
            exec_times = edr_ptr_global->meta_arr[i].exec_times;
            running_exec_time = edr_ptr_global->meta_arr[i].running_exec_t;

//            printf("[+] Avg exec t: %u - check %u - %u\n", avg_exec_time, (static_avg_exec_time + max_avg_dev), (static_avg_exec_time - max_avg_dev));
            if (((avg_exec_time > (static_avg_exec_time + max_avg_dev)) || (avg_exec_time < (static_avg_exec_time - max_avg_dev))) && (exec_times >= EDR_NUM_SAMPLES_AVG) ) {
                // i + 1 becuase uxTCBNum starts at 1
                reg_sec_event(i + 1, "AVG", avg_exec_time, TIMING);
                if (TIMING_RESP == DELETE)  {
                    taskENTER_CRITICAL();
                    if (edr_ptr_global->meta_arr[i].to_delete != 1) {
                        vTaskDelete(*((TaskHandle_t *)edr_ptr_global->meta_arr[i].task_handle));
                    }
                    taskEXIT_CRITICAL();
                }
                //printf("[!] Registered suspicious TIMING event\n");
            }

            #if (FULL_EXEC_CHECK == 0)
            int z;

            for (z = 0; z < EDR_EXEC_TIMES_NUM; z++) {
                last_exec_time = edr_ptr_global->meta_arr[i].exec_times_arr[z];

                // printf("Last exec t: %u - check %u\n", last_exec_time, max_time_allowed);
                if (last_exec_time == 0) {
                    break;
                } else if (last_exec_time > max_time_allowed) {
                    if (TIMING_RESP == DELETE)  {
                        taskENTER_CRITICAL();
                        if (edr_ptr_global->meta_arr[i].to_delete != 1) {
                            vTaskDelete(*((TaskHandle_t *)edr_ptr_global->meta_arr[i].task_handle));
                        }
                        taskEXIT_CRITICAL();
                    }
                    reg_sec_event(i + 1, "CONC", last_exec_time, TIMING);
                    //printf("[!] Registered suspicious TIMING event\n");
                }

            }
            #endif

            configASSERT(edr_ptr_global != NULL);
            if (running_exec_time > max_time_allowed) {
                if (TIMING_RESP == DELETE)  {
                    taskENTER_CRITICAL();
                    if (edr_ptr_global->meta_arr[i].to_delete != 1) {
                        vTaskDelete(*((TaskHandle_t *)edr_ptr_global->meta_arr[i].task_handle));
                    }
                    taskEXIT_CRITICAL();
                }
                reg_sec_event(i + 1, "CONC", running_exec_time, TIMING);
            }

            // If task was marked as deleted
            if (to_delete) {
                edr_ptr_global->meta_arr[i].deleted = 1;
            }
        }
    }

    return;
}

PRIVILEGED_FUNC void edr_run(void *pvParameters) {
    uint32_t last_time_check = xTaskGetTickCount();
    uint32_t last_hb = last_time_check;
    uint32_t curr_ticks = 0;

    for( ; ; )
    {
        sec_poll_task();
        curr_ticks = xTaskGetTickCount();
        if ((curr_ticks - last_time_check) >= EDR_TIME_CHECK_TICK_FREQ) {
            //printf("======> Running time check\n");
            last_time_check = curr_ticks;
            time_analysis();
        }

        if ((curr_ticks - last_hb) >= EDR_HB_TICK_FREQ) {
            last_hb = curr_ticks;
            heartbeat();
        }

        TickType_t xBlockTime = pdMS_TO_TICKS(edr_ptr_global->edr_sleep_time);
        vTaskDelay(xBlockTime);

    }
}


