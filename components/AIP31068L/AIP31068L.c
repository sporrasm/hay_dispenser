#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "rom/ets_sys.h"
#include <esp_log.h>
#include "AIP31068L.h"

// Byte constants for the LCD:
#define LCD_SET_DDRAM_ADDR 0x80
#define LCD_RESET 0x20
#define LCD_8BIT_MODE 0x10 // LCD interface in 8-bit mode
#define LCD_LINES 0x08 // Two-line mode
#define LCD_PIXELS 0x04 // 5x11 pixels
#define LCD_DISPCTRL 0x08
#define LCD_DISPLAY_ON 0x04
#define LCD_CURSOR_ON 0x02
#define LCD_BLINK_ON 0x01
#define LCD_ENTRYMODE 0x04
#define LCD_ENTRYMODE_INCR 0x02
#define LCD_ENTRYMODE_SHIFT 0x01

// Clear and home codes
#define LCD_CLEAR 0x01
#define LCD_HOME 0x02

// RS bits
#define RS_CMD 0x00
#define RS_DATA 0x40

// Table of addresses for display RAM. Address at
// Each index represents the start of each row
uint8_t rowAddresses[4] = {0x00, 0x40, 0x14, 0x54};

static char* TAG = "LCD Driver";
static uint8_t LCD_addr;
static uint8_t SDA_pin;
static uint8_t SCL_pin;
static uint8_t LCD_cols;
static uint8_t LCD_rows;

static void LCD_command(uint8_t val); 
static void LCD_writeByte(uint8_t data, uint8_t mode);

static esp_err_t i2c_init(void)
{
    // Initialize i2c interface with esp32 in master mode
  i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = SDA_pin,
    .scl_io_num = SCL_pin,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000
  };
  i2c_param_config(I2C_NUM_0, &conf);
  i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
  return ESP_OK;
}

void LCD_quick_init(uint8_t addr, uint8_t sdaPin, uint8_t clkPin, uint8_t cols, uint8_t rows) {
  /*
     NOTE: This is a convience function provided to make life easier! LCD can be configured
     to a greater extent by calling LCD_init (see below)

     Initialization sequence for AIP31068L: wait for 40 ms or more for supplies to settle
     then send reset command, which enables 8-bit mode (required?), two-line mode and sets size of cell to 5x11
     enable display
     clear the LCD
     and toggle the entrymode to write from left to right

     See you specific datasheet, whether or not the initialization is similar.
  */
  LCD_addr = addr;
  SDA_pin = sdaPin;
  SCL_pin = clkPin;
  LCD_cols = cols;
  LCD_rows = rows;
  i2c_init();
  // Wait 4 ticks
  vTaskDelay(40 / portTICK_RATE_MS);
  // Reset the LCD controller (apply 8-bit, two-line and 5x10 character modes):
  LCD_command(LCD_RESET | LCD_8BIT_MODE | LCD_LINES | LCD_PIXELS);
  ets_delay_us(100); // 100 us delay
  // Turn on display
  LCD_command(LCD_DISPCTRL | LCD_DISPLAY_ON);
  ets_delay_us(100);
  // Clear the LCD screen
  LCD_command(LCD_CLEAR);
  // Wait 2 ticks
  vTaskDelay(20 / portTICK_RATE_MS);
  // Set entry mode (from left to right) 
  LCD_command(LCD_ENTRYMODE | LCD_ENTRYMODE_INCR);
}

void LCD_init(uint8_t addr, uint8_t sdaPin, uint8_t clkPin, struct LCD_config* LCD_conf) {
  /*
     Initialization which consider all possible flags for LCD. The flags are
     set from the LCD_conf structure (see AIP31068L.h and datasheete for details).
  */
  LCD_addr = addr;
  SDA_pin = sdaPin;
  SCL_pin = clkPin;
  LCD_cols = LCD_conf->cols;
  LCD_rows = LCD_conf->rows;

  uint8_t reset_byte=LCD_RESET;
  uint8_t display_byte=LCD_DISPCTRL;
  uint8_t entry_byte=LCD_ENTRYMODE;
  if (LCD_conf->bitmode_flag)
    reset_byte |= LCD_8BIT_MODE;
  if (LCD_conf->two_line_flag)
    reset_byte |= LCD_LINES;
  if (LCD_conf->display_flag)
    display_byte |= LCD_DISPLAY_ON;
  if (LCD_conf->pixel_flag)
    display_byte |= LCD_PIXELS;
  if (LCD_conf->cursor_flag)
    display_byte |= LCD_CURSOR_ON;
  if (LCD_conf->blink_flag)
    display_byte |= LCD_BLINK_ON;
  if (LCD_conf->entrymode_flag)
    entry_byte |= LCD_ENTRYMODE_INCR;
  if (LCD_conf->shiftmode_flag)
    entry_byte |= LCD_ENTRYMODE_SHIFT;

  i2c_init();
  // Wait 4 ticks
  vTaskDelay(40 / portTICK_RATE_MS);
  // Reset the LCD controller (apply 8-bit, two-line and 5x10 character modes):
  LCD_command(reset_byte);
  ets_delay_us(100); // 100 us delay
  // Turn on display
  LCD_command(display_byte);
  ets_delay_us(100);
  // Clear the LCD screen
  LCD_command(LCD_CLEAR);
  // Wait 2 ticks
  vTaskDelay(20 / portTICK_RATE_MS);
  // Set entry mode (from left to right) 
  LCD_command(entry_byte);
}

void LCD_setCursor(uint8_t col, uint8_t row) {
  if (row > LCD_rows - 1) {
    ESP_LOGE(TAG, "Cannot write to row %d. Please select a row in the range (0, %d)", row, LCD_rows-1);
    row = LCD_rows - 1;
  }
  // Lower bits indicate the address
  LCD_command(LCD_SET_DDRAM_ADDR | (col + rowAddresses[row]));
}

void LCD_writeChar(char c) {
  //Writing characters requires RS==1. Delay should be 43 microseconds.
  LCD_writeByte(c, RS_DATA);
  ets_delay_us(43);
}

void LCD_writeStr(char* str) {
  // Write characters by writing them byte by byte
  while (*str) {
    LCD_writeChar(*str++);
  }
}

void LCD_home(void) {
  // According to datasheet, this commands takes 1.53ms
  LCD_writeByte(LCD_HOME, RS_CMD);
  vTaskDelay(20 / portTICK_RATE_MS);
}

void LCD_clearScreen(void) {
  // According to datasheet, this commands takes 1.53ms
  LCD_writeByte(LCD_CLEAR, RS_CMD);
  vTaskDelay(20 / portTICK_RATE_MS);
}

void LCD_command(uint8_t val) {
  // These comamnds should take 39 us
  LCD_writeByte(val, RS_CMD);
  ets_delay_us(39);
}


static void LCD_writeByte(uint8_t byte, uint8_t mode) {
  // read or write is determined I2C_MASTER_WRITE BIT
  uint8_t addr = (LCD_addr << 1) | I2C_MASTER_WRITE;
  // Create I2C Link
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  // I2C Handshake:
  ESP_ERROR_CHECK(i2c_master_start(cmd));
  // Write address to buffer
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, addr, 1));
  // Write control byte to buffer
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, mode, 1));
  // Write command byte to buffer
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, byte, 1));
  // Set stop bit 
  ESP_ERROR_CHECK(i2c_master_stop(cmd));
  // Send to slave
  ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS));
  // Free link
  i2c_cmd_link_delete(cmd);   
}

