#ifndef SSD1306_H
#define SSD1306_H
#define SPI_BUS_NUM               (0)    //SPI 0
// #define SSD1306_RST_PIN           (5)   //Reset pin is GPIO24
// #define SSD1306_DC_PIN            (22)   //Data/Command pin is GPIO23
#define SSD1306_MAX_SEG           (128)  //Maxinum segment
#define SSD1306_MAX_LINE          (7)
#define SSD1306_DEF_FONT_SIZE     (5)    //Default font size

extern int ETX_SSD1306_DisplayInit(void);
extern int etx_spi_write(uint8_t data);
extern int ETX_SSD1306_GpiodInit(struct spi_device *spi);
extern void ETX_SSD1306_GpiodDeInit(void);

void ETX_SSD1306_SetCursor(uint8_t lineNo, uint8_t cursorPos);
void ETX_SSD1306_GoToNextLine(void);
void ETX_SSD1306_PrintChar(unsigned char c);
void ETX_SSD1306_String(char *str);
void ETX_SSD1306_InvertDisplay(bool need_to_invert);
void ETX_SSD1306_SetBrightness(uint8_t brightnessValue);
void ETX_SSD1306_StartScrollHorizontal(bool is_left_scroll, 
                                uint8_t start_line_no,
                                uint8_t end_line_no);
void ETX_SSD1306_StartScrollVerticalHorizontal(
                                                bool is_vertical_left_scroll,
                                                uint8_t start_line_no,
                                                uint8_t end_line_no,
                                                uint8_t vertical_area,
                                                uint8_t rows
);

void ETX_SSD1306_DeactivateScroll(void);
void ETX_SSD1306_fill(uint8_t data);
void ETX_SSD1306_ClearDisplay(void);
void ETX_SSD1306_PrintLogo(void);

#endif