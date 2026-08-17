#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define THREAD_STACK_SIZE (1024)
#define THREAD_PRIORITY  (5)

#define COUNTER_INCREMENTS (100000)

static volatile int shared_counter = 0;

K_MUTEX_DEFINE(counter_mutex);

void thread_func(void *p1, void *p2, void *p3)
{
    const char* name = k_thread_name_get(k_current_get());
    LOG_INF("[%s] started", name);

    k_mutex_lock(&counter_mutex, K_FOREVER);

    for(int i = 0; i < COUNTER_INCREMENTS; i++)
    {
        ++shared_counter;
    }

    k_mutex_unlock(&counter_mutex);

    LOG_INF("[%s] done", name);
}

K_THREAD_DEFINE(thread_alpha, THREAD_STACK_SIZE, thread_func,
                NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread_bravo, THREAD_STACK_SIZE, thread_func,
                NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);

int main(void)
{
    k_thread_join(thread_alpha, K_FOREVER);
    k_thread_join(thread_bravo, K_FOREVER);

    LOG_INF("Counter = %d", shared_counter);

    if (shared_counter != (2 * COUNTER_INCREMENTS))
    {
        LOG_ERR("Counter value invalid, race condition occured!");
    }
    else
    {
        LOG_INF("Counter value valid");
    }
    
    return 0;
}
