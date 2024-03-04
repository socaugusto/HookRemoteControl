#include "system.h"
#include "remote.h"
#include "lcd_spiModule.h"
#include "communications.h"
#include "database.h"
#include "commands.h"

void system_init(const void *lcd_dev, const void *cs_dev)
{
    comm_init();
    remote_init();
    lcd_init(lcd_dev, cs_dev);
}

void system_thread(void)
{
    remote_run();
    database_run();
    command_run();
}

void system_updateButtons(uint32_t button_state, uint32_t has_changed)
{
    remote_updateButtons(button_state, has_changed);
}

void system_receiveUpdate(const uint8_t *data, uint32_t length)
{
    comm_addToMotorBuffer(data, length);
}

void system_updateUi(void)
{
    remote_updateUi();
}
