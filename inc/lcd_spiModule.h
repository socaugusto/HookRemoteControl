#ifndef LCD_SPIMODULE_H_
#define LCD_SPIMODULE_H_

#include <stdint.h>

/**@brief Function for lcd default init with spi init. */
void lcd_init(const void *lcd_dev, const void *cs_dev);

/**@brief Function for setting lcd set-command. */
void lcd_set_command(uint8_t cmd);

/**@brief Function for writing, setting lcd data-command. */
void lcd_write_data(uint8_t data);

/**@brief Function for cleaning lcd monitor. */
void lcd_clear(void);

/**@brief Function for changing lcd cursor point. */
void lcd_set_cursor(uint8_t line_x, uint8_t chr_x);

/**@brief Function for sending string data to lcd. */
void lcd_send_string(char str[]);

/**@brief Function for sending string, int, float, ... values to lcd. */
void lcd_print(char const *ch, float value);

/**@brief Function to clear end of line*/
void lcd_clear_eol(void);

#endif
