#ifndef MAIN_DFUNCS_H_
#define MAIN_DFUNCS_H_

#include "driver/spi_master.h"
#include "fontx.h"
#include "ili9340.h"
#include "pngle.h"

#define RED			    0xf800
#define GREEN			0x07e0
#define BLUE			0x001f
#define BLACK			0x0000
#define WHITE			0xffff
#define GRAY			0x8c51
#define YELLOW			0xFFE0
#define CYAN			0x07FF
#define PURPLE			0xF81F


#define DIRECTION0		    0
#define DIRECTION90		    1
#define DIRECTION180		2
#define DIRECTION270		3

int DF_print_value(TFT_t * dev, uint16_t color, FontxFile font[2], uint16_t xpos, uint16_t ypos, int int_value, float float_value);
int DF_print_string(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, uint8_t * ascii, uint16_t color);
int DF_print_char(TFT_t * dev, FontxFile *fxs, uint16_t x, uint16_t y, uint8_t ascii, uint16_t color);
TickType_t DF_print_png(TFT_t * dev, char * file, int width, int height);
void DF_print_png_init(pngle_t *pngle, uint32_t w, uint32_t h);
void DF_print_png_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]);
void DF_print_png_finish(pngle_t *pngle);
void DF_print_Vpixel(uint16_t x, uint16_t y, uint16_t color);
void DF_VlcdUpdate(TFT_t * dev);
void DF_print_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void DF_print_fill_screen(uint16_t color);
void DF_print_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void DF_print_triangle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color);

#endif