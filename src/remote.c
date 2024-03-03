#include "remote.h"
#include "lcd_spiModule.h"

void update_ui(void)
{
    lcd_set_cursor(1, 1);
    lcd_print(">Batt(V):%0.2f", 23.25f);
    lcd_clear_eol();
    lcd_set_cursor(2, 1);
    lcd_print(">Load(A):%0.2f", 1.25f);
    lcd_clear_eol();
    lcd_set_cursor(3, 1);
    lcd_send_string(">HookPos:");
    lcd_send_string("Unknown");
    lcd_clear_eol();
    lcd_set_cursor(4, 1);
    lcd_print(">RSSI(dBm):%.0f", -25);
    lcd_clear_eol();
}
