#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(zbus_test, LOG_LEVEL_DBG);

#define STACK_SIZE                (2048)
#define SENSOR_SAMPLE_INTERVAL_MS (100)
#define QUEUE_DEPTH               (10)

struct sensor_data 
{
    uint32_t value;
    uint32_t timestamp;
};

K_MSGQ_DEFINE(sensor_queue, sizeof(struct sensor_data), QUEUE_DEPTH, 4);

/* ================================================================== */
/*  Publisher                                                         */
/* ================================================================== */

static void sensor_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    LOG_INF("Sensor Publisher: Starting");

    while (1) 
    {
        struct sensor_data data = 
        {
            .value = sys_rand32_get() % 100,
            .timestamp = k_uptime_get_32()
        };

        LOG_INF("Sensor Publisher: value = %u, timestamp = %u",
            data.value, data.timestamp);

        int ret = k_msgq_put(&sensor_queue, &data, K_NO_WAIT);
        if (ret != 0) 
        {
            LOG_WRN("Sensor Publisher: publish failed (%d)", ret);
        }

        k_msleep(SENSOR_SAMPLE_INTERVAL_MS);
    }
}

/* ================================================================== */
/*  Message subscriber - logger                                       */
/* ================================================================== */


void task_wdt_callback(int channel_id, void *user_data)
{
    LOG_ERR("Task Watchdog: Fired!");
}

static void logger_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    struct sensor_data msg = {0};
    int ret = 0;
    int wdt = task_wdt_add(1000, task_wdt_callback, (void *)k_current_get());

    while (1) 
    {   
        msg = (struct sensor_data){0};

        do 
        {
            ret = k_msgq_get(&sensor_queue, &msg, K_NO_WAIT);
            if (ret != 0)
            {
                if (ret != -ENOMSG)
                {
                    LOG_WRN("Sensor Logger: Failed to get message (%d)", ret);
                }
                   
                break;    
            }

            LOG_INF("Sensor Logger: Value = %u, Timestamp = %u",
                    msg.value, msg.timestamp);

        } while (!ret);

        task_wdt_feed(wdt);

        int sleep_ms = 100 * (1 + sys_rand32_get() % 10);
        LOG_INF("Sensor Logger: Sleep for %d ms", sleep_ms);
        k_msleep(sleep_ms);
    }
}

/* ================================================================== */
/*  Health checker                                                    */
/* ================================================================== */

static void health_checker_thread_fn(void *p1, void *p2, void *p3)
{
    bool reported = false;

    while (1)
    {
        if (k_msgq_num_used_get(&sensor_queue) >= ((QUEUE_DEPTH * 75) / 100))
        {
            if (!reported)
            {
                LOG_WRN("Health Checker: 75%% of sensor queue filled!");
                reported = true;
            }
        }
        else
        {
            reported = false;
        }

        k_sleep(K_MSEC(5));
    }
}

/* ================================================================== */
/*  Threads                                                           */
/* ================================================================== */

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_thread_fn,
                NULL, NULL, NULL, 5, 0, 0);

K_THREAD_DEFINE(logger_thread, STACK_SIZE, logger_thread_fn,
                NULL, NULL, NULL, 6, 0, 0);

K_THREAD_DEFINE(health_checker_thread, STACK_SIZE, health_checker_thread_fn,
                NULL, NULL, NULL, 7, 0, 0);

/* ================================================================== */
/*  Main                                                              */
/* ================================================================== */

int main(void)
{
    task_wdt_init(NULL);
    return 0;
}
