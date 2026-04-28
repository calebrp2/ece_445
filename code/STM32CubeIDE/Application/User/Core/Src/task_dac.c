#include "task_dac.h"
#include "dac.h"

static const uint16_t k_levels[] = { 4095, 2048, 0 };  /* 3.3 V, 1.65 V, 0 V */

void Task_DAC_Run(void *argument)
{
    (void)argument;

    uint32_t idx = 0;
    for (;;)
    {
        DAC_SignalGen_Write(k_levels[idx]);
        idx = (idx + 1) % 3;
        osDelay(1000);
    }
}
