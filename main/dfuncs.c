#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "ili9340.h"
#include "pngle.h"
#include "decode_image.h"
#include "dfuncs.h"

uint16_t vscreen[128][160]; // [x][y]

int DF_print_value(TFT_t * dev, uint16_t color, FontxFile font[2], uint16_t xpos, uint16_t ypos, int int_value, float float_value)
{
	lcdSetFontDirection(&dev, 0);
	char text[40];
	uint8_t ascii[40];
	if(int_value != -1)
	{
		ESP_LOGW(__FUNCTION__, "Int Value: %d", int_value);
		sprintf(text, "%d", int_value);
	}
	if(float_value != -1)
	{
		ESP_LOGW(__FUNCTION__, "Float Value: %.2f", float_value);
		sprintf(text, "%.2f", float_value);
	}
	strcpy((char *)ascii, text);
	return DF_print_string(dev, font, xpos, ypos, ascii, color);

	
}

int DF_print_string(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, uint8_t * ascii, uint16_t color) {
	int length = strlen((char *)ascii);
	bool error = 0;
	//ESP_LOGW(__FUNCTION__,"lcdDrawString length=%d",length);
	for(int i=0;i<length;i++) {
		if(x > 128 || y > 160) 
		{
			error = 1;
			break;
		}
		//printf("ascii[%d]=%x x=%d y=%d\n",i,ascii[i],x,y);
		if (dev->_font_direction == 0)
			x = DF_print_char(dev, fx, x, y, ascii[i], color);
		if (dev->_font_direction == 1)
			y = DF_print_char(dev, fx, x, y, ascii[i], color);
		if (dev->_font_direction == 2)
			x = DF_print_char(dev, fx, x, y, ascii[i], color);
		if (dev->_font_direction == 3)
			y = DF_print_char(dev, fx, x, y, ascii[i], color);
	}
	if(error == 1) return -1;
	if (dev->_font_direction == 0) return x;
	if (dev->_font_direction == 2) return x;
	if (dev->_font_direction == 1) return y;
	if (dev->_font_direction == 3) return y;
	return 0;
}

int DF_print_char(TFT_t * dev, FontxFile *fxs, uint16_t x, uint16_t y, uint8_t ascii, uint16_t color) {
	uint16_t xx,yy,bit,ofs;
	unsigned char fonts[128]; // font pattern
	unsigned char pw, ph;
	int h,w;
	uint16_t mask;
	bool rc;

	//printf("_font_direction=%d\n",dev->_font_direction);
	rc = GetFontx(fxs, ascii, fonts, &pw, &ph);
	//printf("GetFontx rc=%d pw=%d ph=%d\n",rc,pw,ph);
	if (!rc) return 0;

	uint16_t xd1 = 0;
	uint16_t yd1 = 0;
	uint16_t xd2 = 0;
	uint16_t yd2 = 0;
	uint16_t xss = 0;
	uint16_t yss = 0;
	uint16_t xsd = 0;
	uint16_t ysd = 0;
	int next = 0;
        uint16_t x0  = 0;
        uint16_t x1  = 0;
        uint16_t y0  = 0;
        uint16_t y1  = 0;
	if (dev->_font_direction == 0) {
		xd1 = +1;
		yd1 = +1; //-1;
		xd2 =  0;
		yd2 =  0;
		xss =  x;
		yss =  y - (ph - 1);
		xsd =  1;
		ysd =  0;
		next = x + pw;

                x0  = x;
                y0  = y - (ph-1);
                x1  = x + (pw-1);
                y1  = y;
	} else if (dev->_font_direction == 2) {
		xd1 = -1;
		yd1 = -1; //+1;
		xd2 =  0;
		yd2 =  0;
		xss =  x;
		yss =  y + ph + 1;
		xsd =  1;
		ysd =  0;
		next = x - pw;

                x0  = x - (pw-1);
                y0  = y;
                x1  = x;
                y1  = y + (ph-1);
	} else if (dev->_font_direction == 1) {
		xd1 =  0;
		yd1 =  0;
		xd2 = -1;
		yd2 = +1; //-1;
		xss =  x + ph;
		yss =  y;
		xsd =  0;
		ysd =  1;
		next = y + pw; //y - pw;

                x0  = x;
                y0  = y;
                x1  = x + (ph-1);
                y1  = y + (pw-1);
	} else if (dev->_font_direction == 3) {
		xd1 =  0;
		yd1 =  0;
		xd2 = +1;
		yd2 = -1; //+1;
		xss =  x - (ph - 1);
		yss =  y;
		xsd =  0;
		ysd =  1;
		next = y - pw; //y + pw;

                x0  = x - (ph-1);
                y0  = y - (pw-1);
                x1  = x;
                y1  = y;
	}

        if (dev->_font_fill) DF_print_rect(x0, y0, x1, y1, dev->_font_fill_color);

	int bits;
	//printf("xss=%d yss=%d\n",xss,yss);
	ofs = 0;
	yy = yss;
	xx = xss;
	for(h=0;h<ph;h++) {
		if(xsd) xx = xss;
		if(ysd) yy = yss;
		//    for(w=0;w<(pw/8);w++) {
		bits = pw;
		for(w=0;w<((pw+4)/8);w++) {
			mask = 0x80;
			for(bit=0;bit<8;bit++) {
				bits--;
				if (bits < 0) continue;
				//if(_DEBUG_)printf("xx=%d yy=%d mask=%02x fonts[%d]=%02x\n",xx,yy,mask,ofs,fonts[ofs]);
				if (fonts[ofs] & mask) 
				{
					//lcdDrawPixel(dev, xx, yy, color);
					DF_print_Vpixel(xx, yy, color);
				} 
				else 
				{
					//if (dev->_font_fill) lcdDrawPixel(dev, xx, yy, dev->_font_fill_color);
				}
				if (h == (ph-2) && dev->_font_underline)
				{
					//lcdDrawPixel(dev, xx, yy, dev->_font_underline_color);
					DF_print_Vpixel(xx, yy, color);
				}
				if (h == (ph-1) && dev->_font_underline)
				{
					//lcdDrawPixel(dev, xx, yy, dev->_font_underline_color);
					DF_print_Vpixel(xx, yy, color);
				}
				xx = xx + xd1;
				yy = yy + yd2;
				mask = mask >> 1;
			}
			ofs++;
		}
		yy = yy + yd1;
		xx = xx + xd2;
	}

	if (next < 0) next = 0;
	return next;
}

TickType_t DF_print_png(TFT_t * dev, char * file, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdSetFontDirection(dev, 0);

	int _width = width;
	if (width > 240) _width = 240;
	int _height = height;
	if (height > 320) _height = 320;

	// open PNG file
	FILE* fp = fopen(file, "rb");
	if (fp == NULL) {
		ESP_LOGW(__FUNCTION__, "File not found [%s]", file);
		return 0;
	}

	char buf[1024];
	size_t remain = 0;
	int len;

	pngle_t *pngle = pngle_new(_width, _height);

	pngle_set_init_callback(pngle, DF_print_png_init);
	pngle_set_draw_callback(pngle, DF_print_png_draw);
	pngle_set_done_callback(pngle, DF_print_png_finish);

	double display_gamma = 3;
	pngle_set_display_gamma(pngle, display_gamma);


	while (!feof(fp)) {
		if (remain >= sizeof(buf)) {
			ESP_LOGE(__FUNCTION__, "Buffer exceeded");
			while(1) vTaskDelay(1);
		}

		len = fread(buf + remain, 1, sizeof(buf) - remain, fp);
		if (len <= 0) {
			//printf("EOF\n");
			break;
		}

		int fed = pngle_feed(pngle, buf, remain + len);
		if (fed < 0) {
			ESP_LOGE(__FUNCTION__, "ERROR; %s", pngle_error(pngle));
			while(1) vTaskDelay(1);
		}

		remain = remain + len - fed;
		if (remain > 0) memmove(buf, buf + fed, remain);
	}

	fclose(fp);

	uint16_t pngWidth = width;
	uint16_t offsetX = 0;
	if (width > pngle->imageWidth) {
		pngWidth = pngle->imageWidth;
		offsetX = (width - pngle->imageWidth) / 2;
	}
	ESP_LOGD(__FUNCTION__, "pngWidth=%d offsetX=%d", pngWidth, offsetX);

	uint16_t pngHeight = height;
	uint16_t offsetY = 0;
	if (height > pngle->imageHeight) {
		pngHeight = pngle->imageHeight;
		offsetY = (height - pngle->imageHeight) / 2;
	}
	ESP_LOGD(__FUNCTION__, "pngHeight=%d offsetY=%d", pngHeight, offsetY);
	uint16_t *colors = (uint16_t*)malloc(sizeof(uint16_t) * pngWidth);

#if 0
	for(int y = 0; y < pngHeight; y++){
		for(int x = 0;x < pngWidth; x++){
			pixel_png pixel = pngle->pixels[y][x];
			uint16_t color = rgb565_conv(pixel.red, pixel.green, pixel.blue);
			lcdDrawPixel(dev, x+offsetX, y+offsetY, color);
		}
	}
#endif

	for(int y = 0; y < pngHeight; y++){
		for(int x = 0;x < pngWidth; x++){
			pixel_png pixel = pngle->pixels[y][x];
			colors[x] = rgb565_conv(pixel.blue, pixel.green, pixel.red);
			//uint16_t color = rgb565_conv(pixel.red, pixel.green, pixel.blue);
			//colors[x] = ~color;
			//vscreen[x][y] = colors[x];
			DF_print_Vpixel(x, y, colors[x]);
		}
		//lcdDrawMultiPixels(dev, offsetX, y+offsetY, pngWidth, colors);
	}
	free(colors);
	pngle_destroy(pngle, _width, _height);
	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	//ESP_LOGW(__FUNCTION__, "Drawing Image");
	return diffTick;
}

void DF_print_png_init(pngle_t *pngle, uint32_t w, uint32_t h)
{
	ESP_LOGD(__FUNCTION__, "print_png_init w=%d h=%d", w, h);
	ESP_LOGD(__FUNCTION__, "screenWidth=%d screenHeight=%d", pngle->screenWidth, pngle->screenHeight);
	pngle->imageWidth = w;
	pngle->imageHeight = h;
	pngle->reduction = false;
	pngle->scale_factor = 1.0;

	// Calculate Reduction
	if (pngle->screenWidth < pngle->imageWidth || pngle->screenHeight < pngle->imageHeight) {
		pngle->reduction = true;
		double factorWidth = (double)pngle->screenWidth / (double)pngle->imageWidth;
		double factorHeight = (double)pngle->screenHeight / (double)pngle->imageHeight;
		pngle->scale_factor = factorWidth;
		if (factorHeight < factorWidth) pngle->scale_factor = factorHeight;
		pngle->imageWidth = pngle->imageWidth * pngle->scale_factor;
		pngle->imageHeight = pngle->imageHeight * pngle->scale_factor;
	}
	ESP_LOGD(__FUNCTION__, "reduction=%d scale_factor=%f", pngle->reduction, pngle->scale_factor);
	ESP_LOGD(__FUNCTION__, "imageWidth=%d imageHeight=%d", pngle->imageWidth, pngle->imageHeight);
		
}

void DF_print_png_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
	ESP_LOGD(__FUNCTION__, "print_png_draw x=%d y=%d w=%d h=%d", x,y,w,h);
#if 0
	uint8_t r = rgba[0];
	uint8_t g = rgba[1];
	uint8_t b = rgba[2];
#endif

	// image reduction
	uint32_t _x = x;
	uint32_t _y = y;
	if (pngle->reduction) {
		_x = x * pngle->scale_factor;
		_y = y * pngle->scale_factor;
	}
	if (_y < pngle->screenHeight && _x < pngle->screenWidth) {
		pngle->pixels[_y][_x].red = rgba[0];
		pngle->pixels[_y][_x].green = rgba[1];
		pngle->pixels[_y][_x].blue = rgba[2];
	}

}

void DF_print_png_finish(pngle_t *pngle) 
{
	ESP_LOGD(__FUNCTION__, "print_png_finish");
}

void DF_VlcdUpdate(TFT_t * dev)
{
	uint16_t pngHeight = 160;
	uint16_t pngWidth = 128;
	uint16_t offsetX = 0;
	uint16_t offsetY = 0;
	uint16_t *colors = (uint16_t*)malloc(sizeof(uint16_t) * pngWidth);

	for(int y = 0; y < pngHeight; y++){
		for(int x = 0;x < pngWidth; x++){
			colors[x] = vscreen[x][y];
		}
		lcdDrawMultiPixels(dev, offsetX, y+offsetY, pngWidth, colors);
	}
	free(colors);
}

void DF_print_Vpixel(uint16_t x, uint16_t y, uint16_t color)
{
	vscreen[x][y] = color;
}

void DF_print_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	DF_print_line(x1, y1, x2, y1, color);
	DF_print_line(x2, y1, x2, y2, color);
	DF_print_line(x2, y2, x1, y2, color);
	DF_print_line(x1, y2, x1, y1, color);
}

void DF_print_fill_screen(uint16_t color) {
	uint16_t Height = 160;
	uint16_t Width = 128;
	for(int x = 0; x < Width; x++)
	{
		for(int y = 0; y < Height; y++)
		{
			vscreen[x][y] = color;
		}
	}
}


void DF_print_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	int i;
	int dx,dy;
	int sx,sy;
	int E;

	/* distance between two points */
	dx = ( x2 > x1 ) ? x2 - x1 : x1 - x2;
	dy = ( y2 > y1 ) ? y2 - y1 : y1 - y2;

	/* direction of two point */
	sx = ( x2 > x1 ) ? 1 : -1;
	sy = ( y2 > y1 ) ? 1 : -1;

	/* inclination < 1 */
	if ( dx > dy ) {
		E = -dx;
		for ( i = 0 ; i <= dx ; i++ ) {
			DF_print_Vpixel(x1, y1, color);
			x1 += sx;
			E += 2 * dy;
			if ( E >= 0 ) {
			y1 += sy;
			E -= 2 * dx;
		}
	}

/* inclination >= 1 */
	} else {
		E = -dy;
		for ( i = 0 ; i <= dy ; i++ ) {
			DF_print_Vpixel(x1, y1, color);
			y1 += sy;
			E += 2 * dx;
			if ( E >= 0 ) {
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}
}

