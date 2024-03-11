#include "lcd_spiModule.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>

static struct spi_config spi_cfg = {
	.frequency = 125000U,
	.operation = (SPI_OP_MODE_MASTER | SPI_TRANSFER_LSB | SPI_WORD_SET(8)),
	.slave = 0,
	.cs = {{0}},
};

#define LCD_FUNC_SET_RE0 0x20 // IS:0, RE:0, SD:0
#define LCD_FUNC_SET_RE1 0x22 // IS:0, RE:1, SD:0

#define LCD_CLEAR_DISPLAY 0x01		 // <RE:0>
#define LCD_RETURN_HOME 0x02		 // <RE:0>
#define LCD_ENTRYMODE_SET 0x04		 // <RE:0> , (<RE:1> not added.)
#define LCD_DISPLAY_CONTROL 0x08	 // <RE:0>
#define LCD_CURSORDISPLAY_SHIFT 0x10 // <RE:0>
#define LCD_SET_CGRAMADDR 0x40		 // <RE:0>
#define LCD_SET_DDRAMADDR 0x80		 // <RE:0>

//***Extended Command Set
#define LCD_FUNC_SELECT_A 0x71	// <RE:1>
#define DISABLE_INT_VDD_5V 0x00 //  data(x)
#define ENABLE_INT_VDD_5V 0x5C	//  data(x)

#define LCD_FUNC_SELECT_B 0x72 //  <RE:1>
#define CGROM 0x00			   // default

#define LCD_OLED_CHAR_ENABLE 0x79
#define LCD_OLED_CHAR_DISABLE 0x78

// Entry mode set sub-options <RE:0>
typedef enum
{
	ENTRY_INCREMENT = 0x02,
	ENTRY_DECREMENT = 0x00,
	ENTRY_DISPLAY_SHIFT = 0x01,
	ENTRY_DISPLAY_NOSHIFT = 0x00
} lcd_EntryMode_set_t;

// Display control sub-options <RE:0>
typedef enum
{
	DISPLAY_ON = 0x04,
	DISPLAY_OFF = 0x00,
	CURSOR_ON = 0x02,
	CURSOR_OFF = 0x00,
	BLINK_ON = 0x01,
	BLINK_OFF = 0x00
} lcd_display_control_t;

// Cursor or display shift sub-options <RE:0>
typedef enum
{
	DISPLAY_SHIFT = 0x08,
	CURSOR_SHIFT = 0x00,
	SHIFT_TO_RIGHT = 0x04,
	SHIFT_TO_LEFT = 0x00
} lcd_CursorDisplay_shift_t;

// Function set sub-options <RE:0>
typedef enum
{
	MODE_2L_4L = 0x08,
	MODE_1L_3L = 0x00,
	MODE_2L_DH_FONT_ENABLE = 0x04,
	MODE_2L_DH_FONT_DISABLE = 0x00
} lcd_function_set_t;

// Extended Function set sub-options <RE:1>
typedef enum
{
	MODE_4L = 0x08,
	MODE_6DOT = 0x04,
	MODE_5DOT = 0x00,
	MODE_BW_INV_CURSOR_ENABLE = 0x02,
	MODE_BW_INV_CURSOR_DISABLE = 0x00
} lcd_extfunction_set_t;

#define line_MAX ((uint8_t)4)
#define chr_MAX ((uint8_t)20)

static const uint8_t cursor_data[line_MAX][chr_MAX] = {
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13}, // 1. line DDRAM address
	{0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33}, // 2. line DDRAM address
	{0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53}, // 3. line DDRAM address
	{0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73}, // 4. line DDRAM address
};

static struct device *lcd;
static struct gpio_dt_spec *cs;
static uint8_t tx_buff[3];

/**@brief Function for lcd default init with spi init. */
void lcd_init(const void *lcd_dev, const void *cs_dev)
{
	if (!lcd_dev || !cs_dev)
	{
		return;
	}

	lcd = (struct device *)lcd_dev;
	cs = (struct gpio_dt_spec *)cs_dev;

	gpio_pin_set_dt(cs, 1);
	k_sleep(K_MSEC(1));
	gpio_pin_set_dt(cs, 0);
	k_sleep(K_MSEC(1));

	lcd_set_command(0x2A); // function set (extended command set)
	k_sleep(K_MSEC(1));
	lcd_set_command(0x71); // function selection A
	k_sleep(K_MSEC(1));
	lcd_write_data(0x5C); // disable internal VDD regulator (2.8V I/O). data(0x5C) = enable regulator (5V I/O)
	k_sleep(K_MSEC(1));
	lcd_set_command(0x28); // function set (fundamental command set)
	k_sleep(K_MSEC(1));
	lcd_set_command(0x08); // display off, cursor off, blink off
	k_sleep(K_MSEC(1));
	lcd_set_command(0x2A); // function set (extended command set)
	k_sleep(K_MSEC(1));
	lcd_set_command(0x79); // OLED command set enabled
	k_sleep(K_MSEC(1));
	lcd_set_command(0xD5); // set display clock divide ratio/oscillator frequency
	k_sleep(K_MSEC(1));
	lcd_set_command(0x70); // set display clock divide ratio/oscillator frequency
	k_sleep(K_MSEC(1));
	lcd_set_command(0x78); // OLED command set disabled
	k_sleep(K_MSEC(1));
	lcd_set_command(0x09); // extended function set (4-lines)
	k_sleep(K_MSEC(1));
	lcd_set_command(0x05); // COM SEG direction
	k_sleep(K_MSEC(1));
	lcd_set_command(0x72); // function selection B
	k_sleep(K_MSEC(1));
	lcd_write_data(0x00); // ROM CGRAM selection
	k_sleep(K_MSEC(1));
	lcd_set_command(0x2A); // function set (extended command set)
	k_sleep(K_MSEC(1));
	lcd_set_command(0x79); // OLED command set enabled
	k_sleep(K_MSEC(1));
	lcd_set_command(0xDA); // set SEG pins hardware configuration
	k_sleep(K_MSEC(1));
	lcd_set_command(0x10); // set SEG pins hardware configuration
	k_sleep(K_MSEC(1));
	lcd_set_command(0xDC); // function selection C
	k_sleep(K_MSEC(1));
	lcd_set_command(0x00); // function selection C
	k_sleep(K_MSEC(1));
	lcd_set_command(0x81); // set contrast control
	k_sleep(K_MSEC(1));
	lcd_set_command(0x7F); // set contrast control
	k_sleep(K_MSEC(1));
	lcd_set_command(0xD9); // set phase length
	k_sleep(K_MSEC(1));
	lcd_set_command(0xF1); // set phase length
	k_sleep(K_MSEC(1));
	lcd_set_command(0xDB); // set VCOMH deselect level
	k_sleep(K_MSEC(1));
	lcd_set_command(0x40); // set VCOMH deselect level
	k_sleep(K_MSEC(1));
	lcd_set_command(0x78); // OLED command set disabled
	k_sleep(K_MSEC(1));
	lcd_set_command(0x28); // function set (fundamental command set)
	k_sleep(K_MSEC(1));
	lcd_set_command(0x01); // clear display
	k_sleep(K_MSEC(1));
	lcd_set_command(0x80); // set DDRAM address to 0x00
	k_sleep(K_MSEC(1));
	lcd_set_command(0x0C); // display ON

	k_sleep(K_MSEC(500));
}

/**@brief Function for setting lcd set-command. */
void lcd_set_command(uint8_t cmd)
{
	tx_buff[0] = 0x1F;
	tx_buff[1] = cmd & 0x0F;
	tx_buff[2] = (cmd & 0xF0) >> 4;

	struct spi_buf tx_buf = {.buf = tx_buff, .len = sizeof(tx_buff)};
	struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1};

	gpio_pin_set_dt(cs, 1);
	spi_write(lcd, &spi_cfg, &tx_bufs);
	gpio_pin_set_dt(cs, 0);
}

/**@brief Function for writing, setting lcd data-command. */
void lcd_write_data(uint8_t data)
{
	tx_buff[0] = 0x5F;
	tx_buff[1] = data & 0x0F;
	tx_buff[2] = (data & 0xF0) >> 4;

	struct spi_buf tx_buf = {.buf = tx_buff, .len = sizeof(tx_buff)};
	struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1};

	gpio_pin_set_dt(cs, 1);
	spi_write(lcd, &spi_cfg, &tx_bufs);
	gpio_pin_set_dt(cs, 0);
}

/**@brief Function for cleaning lcd monitor. */
void lcd_clear(void)
{
	lcd_set_command(LCD_CLEAR_DISPLAY);
	k_sleep(K_MSEC(1));
}

static uint32_t xpos;

/**@brief Function for changing lcd cursor point. */
void lcd_set_cursor(uint8_t line_x, uint8_t chr_x)
{
	if (((line_x >= 1 && line_x <= line_MAX) && (chr_x >= 1 && chr_x <= chr_MAX)))
	{
		lcd_set_command(0x28);
		k_sleep(K_MSEC(1));
		lcd_set_command(LCD_SET_DDRAMADDR | cursor_data[line_x - 1][chr_x - 1]);
		k_sleep(K_MSEC(1));
		xpos = chr_x;
	}
}

/**@brief Function for sending string data to lcd. */
void lcd_send_string(char str[])
{
	while (*str)
	{
		lcd_write_data(*str++);
		k_sleep(K_MSEC(1));
		xpos++;
	}
}

void lcd_clear_eol(void)
{
	while (xpos <= 20)
	{
		lcd_write_data(' ');
		k_sleep(K_MSEC(1));
		xpos++;
	}
}

/**@brief Function for sending string, int, float, ... values to lcd. */
void lcd_print(char const *ch, float value)
{
	const uint8_t lcd_buff_size = 21;
	char data_ch[lcd_buff_size]; // default data size:100.

	snprintf(data_ch, lcd_buff_size, ch, value);
	data_ch[lcd_buff_size - 1] = 0;
	lcd_send_string(data_ch);
}
