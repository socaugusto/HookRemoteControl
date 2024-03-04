#include "system.h"
#include "remote.h"
#include "lcd_spiModule.h"

void system_init(const void *lcd_dev, const void *cs_dev)
{
    lcd_init(lcd_dev, cs_dev);
    remote_init();
}

void system_thread(void)
{
    // comm_init();

    // database_run();
    // command_run();
    // radio_run();
}

void system_updateButtons(uint32_t buttons)
{
    remote_updateButtons(buttons);
}

void system_receiveUpdate(const uint8_t *data, uint32_t length)
{
    comm_addToRadioBuffer(data, length);
}

void system_updateUi(void)
{
    remote_updateUi();
}
