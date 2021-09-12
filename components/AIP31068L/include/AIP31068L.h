#ifndef AIP31068L_H
#define AIP31068L_H


// Struct for wider configurability of the LCD unit
struct LCD_config {
    uint8_t rows; // Number of rows
    uint8_t cols; // Number of columns
    uint8_t bitmode_flag; // Set 8bit mode?
    uint8_t two_line_flag; // Set two-line mode?
    uint8_t display_flag; // Enable displayEnable display?? 
    uint8_t pixel_flag; // Set 5x8 (if 0) or 5x11 (if 1) dots per cell? 
    uint8_t cursor_flag; // Enable cursor?
    uint8_t blink_flag; // Enable blink in current cell?
    uint8_t entrymode_flag; // If 1, writes from left to right. Opposite direction if 0
    uint8_t shiftmode_flag; // No idea what this does.
};

void LCD_quick_init(uint8_t addr, uint8_t sdaPin, uint8_t clkPin, uint8_t cols, uint8_t rows);
void LCD_init(uint8_t addr, uint8_t sdaPin, uint8_t clkPin, struct LCD_config* LCD_conf);
void LCD_setCursor(uint8_t col, uint8_t row);
void LCD_writeChar(char c);
void LCD_writeStr(char* str);
void LCD_home(void);
void LCD_clearScreen(void);



#endif
