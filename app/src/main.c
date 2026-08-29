#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(zbus_test, LOG_LEVEL_DBG);

#define STACK_SIZE                (2048)
#define SENSOR_SAMPLE_INTERVAL_MS (100)

struct sensor_data 
{
    uint32_t value;
    uint32_t timestamp;
};

static void sensor_lis_cb(const struct zbus_channel *chan);

/* ================================================================== */
/*  Observers                                                         */
/* ================================================================== */

ZBUS_LISTENER_DEFINE(sensor_lis, sensor_lis_cb);

ZBUS_MSG_SUBSCRIBER_DEFINE(sensor_sub);

/* ================================================================== */
/*  Channel                                                           */
/* ================================================================== */

ZBUS_CHAN_DEFINE(sensor_chan, struct sensor_data,
                 NULL, NULL,
                 ZBUS_OBSERVERS(sensor_lis, sensor_sub),
                 ZBUS_MSG_INIT(
                    .value = 0,
                    .timestamp = 0));

/* ================================================================== */
/*  Listener - synchronous observer                                   */
/* ================================================================== */

static void sensor_lis_cb(const struct zbus_channel *chan)
{
    const struct sensor_data *msg =
        (const struct sensor_data *)zbus_chan_const_msg(chan);

    LOG_INF("Sensor Listener: value = %u, timestamp = %u",
            msg->value, msg->timestamp);
}

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

        int ret = zbus_chan_pub(&sensor_chan, &data, K_MSEC(100));
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

static void logger_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    const struct zbus_channel *chan;
    struct sensor_data temp_msg = {0};
    struct sensor_data last_msg = {0};
    int messages_dropped = 0;
    int ret = 0;

    while (1) 
    {
        temp_msg = (struct sensor_data){0};
        last_msg = (struct sensor_data){0};
        messages_dropped = 0;

        do
        {
            ret = zbus_sub_wait_msg(&sensor_sub, &chan, &temp_msg, K_NO_WAIT);
            if (!ret)
            {
                if (last_msg.timestamp)
                {
                    messages_dropped++;
                }

                last_msg = temp_msg;    
            }
          
        } while (!ret);
        
        if (ret == -ETIMEDOUT) 
        {
            LOG_WRN("Sensor Logger: timeout (%d)", ret);
            break;
        }

        LOG_INF("Sensor Logger: value = %u, timestamp = %u (latency = %d ms, dropped = %d)",
                last_msg.value, last_msg.timestamp, 
                k_uptime_get_32() - last_msg.timestamp, messages_dropped);

        LOG_INF("Sensor Logger: simulating longer work (500 ms)");
        k_msleep(500);
    }
}

/* ================================================================== */
/*  Threads                                                           */
/* ================================================================== */

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_thread_fn,
                NULL, NULL, NULL, 5, 0, 0);

K_THREAD_DEFINE(logger_thread, STACK_SIZE, logger_thread_fn,
                NULL, NULL, NULL, 6, 0, 0);

/* ================================================================== */
/*  Main                                                              */
/* ================================================================== */

int main(void)
{
    return 0;
}
