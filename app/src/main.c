#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

#define PRIO_LOW  (7)
#define PRIO_MED  (5)
#define PRIO_HIGH (3)
#define PRIO_COOP (-1)

void t_low_fn(void *p1, void *p2, void *p3)
{
    while (1) 
    {
        LOG_INF("T_LOW running");
        k_msleep(300);
    }
}

void t_med_fn(void *p1, void *p2, void *p3)
{
    while (1) 
    {
        LOG_INF("T_MED running");
        k_msleep(200);
    }
}

void t_high_fn(void *p1, void *p2, void *p3)
{
    while (1) 
    {
        LOG_INF("T_HIGH running");
        k_msleep(100);
    }
}

void t_coop_fn(void *p1, void *p2, void *p3)
{
    const int iterations_per_run = 5;

    while (1)
    {
        int i = 0;

        while (i++ < iterations_per_run)
        {
            LOG_INF("T_COOP iteration %d/%d", i, iterations_per_run);
            k_busy_wait(500000);
        }

        k_sleep(K_MSEC(2000));
    }
}

K_THREAD_DEFINE(t_low, STACK_SIZE, t_low_fn,
                NULL, NULL, NULL, PRIO_LOW, 0, 0);
K_THREAD_DEFINE(t_med, STACK_SIZE, t_med_fn,
                NULL, NULL, NULL, PRIO_MED, 0, 0);
K_THREAD_DEFINE(t_high, STACK_SIZE, t_high_fn,
                NULL, NULL, NULL, PRIO_HIGH, 0, 0);
K_THREAD_DEFINE(t_coop, STACK_SIZE, t_coop_fn,
                NULL, NULL, NULL, PRIO_COOP, 0, 0);

int main(void)
{
    return 0;
}
