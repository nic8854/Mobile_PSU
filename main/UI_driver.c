#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "ili9340.h"
#include "dfuncs.h"
#include "UI_driver.h"
#include "IO_driver.h"
#include "Button_driver.h"
#include "APA102.h"

static const char *TAG = "UI_Driver";

//Display Object
TFT_t dev;
uint16_t model = 0x7735;
char file[32];
uint16_t color;
char text[40];
uint8_t ascii[40];
uint16_t xpos;
uint16_t ypos;

//Font Files
FontxFile fx16G[2];
FontxFile fx24G[2];
FontxFile fx32G[2];

FontxFile fx16M[2];
FontxFile fx24M[2];
FontxFile fx32M[2];

//global variables
int led_test_index = 0;

int led_test_colors[5][3] ={
							{200, 100, 90},
							{100, 150, 50},
							{20, 90, 200},
							{80, 10, 250},
							{50, 150, 20}
}; 



void UI_init(int I2C_PORT, int SDA_GPIO, int SCL_GPIO)
{
	InitFontx(fx16G,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
	InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
	InitFontx(fx32G,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic

	InitFontx(fx16M,"/spiffs/ILMH16XB.FNT",""); // 8x16Dot Mincyo
	InitFontx(fx24M,"/spiffs/ILMH24XB.FNT",""); // 12x24Dot Mincyo
	InitFontx(fx32M,"/spiffs/ILMH32XB.FNT",""); // 16x32Dot Mincyo

	//Initialize SPI for Display
    spi_master_init(&dev, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
	//Iniialize Display
    lcdInit(&dev, model, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
    lcdSetFontDirection(&dev, 0);

    DF_print_fill_screen(BLACK);
	DF_VlcdUpdate(&dev);

	//Initialize Buttons and RGB LEDs
	Button_init(I2C_PORT, SDA_GPIO, SCL_GPIO);
	APA102_Init(2, VSPI_HOST);

	vTaskDelay(500 / portTICK_PERIOD_MS);
	
	//Bootup screen ------------------------------------------------
    color = WHITE;
	xpos = 10;
	ypos = 25;
	strcpy((char *)ascii, "Project:");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 45;
	strcpy((char *)ascii, "PSU_main");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 65;
	strcpy((char *)ascii, "Author:");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 85;
	strcpy((char *)ascii, "M.Maechler");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 105;
	strcpy((char *)ascii, "Version:");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 125;
	strcpy((char *)ascii, "1.0");
	DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
	ypos = 145;
	strcpy((char *)ascii, "#");
	for(int i = 1; i < 15; i++)
	{
		xpos = i*8;
		DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
		DF_VlcdUpdate(&dev);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
    DF_VlcdUpdate(&dev);
	//--------------------------------------------------------------
	ESP_LOGI(TAG, "--> UI_driver initialized successfully");
}

//Function to draw Test Screen with Parameter Values
void UI_draw_test_screen(uint8_t in_value, uint8_t out_value, double current_val, double shunt_val, int enc_val)
{
	strcpy(file, "/spiffs/background.png");
	DF_print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	color = WHITE;
	xpos = 40;
	ypos = 28;
	strcpy((char *)ascii, "TEST");
	DF_print_string(&dev, fx24G, xpos, ypos, ascii, color);
	xpos = 8;
	ypos = 55;
	strcpy((char *)ascii, "Reg 0:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 8;
	ypos = 75;
	strcpy((char *)ascii, "Reg 1:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 8;
	ypos = 95;
	strcpy((char *)ascii, "INA I:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 8;
	ypos = 115;
	strcpy((char *)ascii, "SHUNT:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 8;
	ypos = 135;
	strcpy((char *)ascii, "ENC  :");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 55;
	ypos = 55;
	sprintf(text, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(in_value));
	strcpy((char *)ascii, text);
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 55;
	ypos = 75;
	sprintf(text, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(out_value));
	strcpy((char *)ascii, text);
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 55;
	ypos = 95;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, current_val);
	xpos = 100;
	ypos = 95;
	strcpy((char *)ascii, "mA");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 55;
	ypos = 115;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, shunt_val);
	
	xpos = 100;
	ypos = 115;
	strcpy((char *)ascii, "mV");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 55;
	ypos = 135;
	DF_print_value(&dev, color, fx16G, xpos, ypos, enc_val, -1);
	DF_print_rect(5, 138, 120, 155, color);
	color = WHITE;
	DF_print_triangle(123, 80, 15, 7, 90, color);
	DF_print_line(7, 75, 7, 88, color);
	DF_print_line(7, 75, 1, 81, color);
	DF_print_line(7, 88, 1, 81, color);
	xpos = 25;
	ypos = 155;
	color = 0xFFF6;
	strcpy((char *)ascii, "OUTPUT ON");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
}

void UI_draw_main_screen(double power_val, double voltage_val, double current_val, bool output_val)
{
	strcpy(file, "/spiffs/background.png");
	DF_print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	color = WHITE;
	xpos = 40;
	ypos = 28;
	strcpy((char *)ascii, "Main");
	DF_print_string(&dev, fx24G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 55;
	strcpy((char *)ascii, "P in:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 75;
	strcpy((char *)ascii, "U in:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 95;
	strcpy((char *)ascii, "I in:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 60;
	ypos = 55;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, power_val);
	xpos = 60;
	ypos = 75;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, voltage_val);
	xpos = 60;
	ypos = 95;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, current_val);
	xpos = 100;
	ypos = 55;
	strcpy((char *)ascii, "mW");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 100;
	ypos = 75;
	strcpy((char *)ascii, "V");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 100;
	ypos = 95;
	strcpy((char *)ascii, "mA");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	DF_print_rect(5, 138, 120, 155, color);
	color = WHITE;
	DF_print_triangle(123, 80, 15, 7, 90, color);
	xpos = 25;
	ypos = 155;
	color = 0xFFF6;
	if(output_val)strcpy((char *)ascii, "OUTPUT ON");
	if(!output_val)strcpy((char *)ascii, "OUTPUT OFF");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
}

void UI_draw_voltages_screen(double out24_val, double out5_val, double outvar_val, double out33_val, bool output_val)
{
	strcpy(file, "/spiffs/background.png");
	DF_print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	color = WHITE;
	xpos = 15;
	ypos = 28;
	strcpy((char *)ascii, "Voltages");
	DF_print_string(&dev, fx24G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 55;
	strcpy((char *)ascii, " 24V:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 75;
	strcpy((char *)ascii, "  5V:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 95;
	strcpy((char *)ascii, "3.3V:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 115;
	strcpy((char *)ascii, " Var:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 60;
	ypos = 55;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, out24_val);
	xpos = 60;
	ypos = 75;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, out5_val);
	xpos = 60;
	ypos = 95;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, out33_val);
	xpos = 60;
	ypos = 115;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, outvar_val);
	xpos = 100;
	ypos = 55;
	strcpy((char *)ascii, "V");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 100;
	ypos = 75;
	strcpy((char *)ascii, "V");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 100;
	ypos = 95;
	strcpy((char *)ascii, "V");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 100;
	ypos = 115;
	strcpy((char *)ascii, "V");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	DF_print_rect(5, 138, 120, 155, color);
	color = WHITE;
	DF_print_line(7, 75, 7, 88, color);
	DF_print_line(7, 75, 1, 81, color);
	DF_print_line(7, 88, 1, 81, color);
	DF_print_triangle(123, 80, 15, 7, 90, color);
	xpos = 25;
	ypos = 155;
	color = 0xFFF6;
	if(output_val)strcpy((char *)ascii, "OUTPUT ON");
	if(!output_val)strcpy((char *)ascii, "OUTPUT OFF");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
}

void UI_draw_variable_screen(double uset_val, double ueff_val, int select_val, bool output_val)
{
	strcpy(file, "/spiffs/background.png");
	DF_print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	color = WHITE;
	xpos = 20;
	ypos = 28;
	strcpy((char *)ascii, "Variable");
	DF_print_string(&dev, fx24G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 55;
	strcpy((char *)ascii, "U set:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 75;
	strcpy((char *)ascii, "U eff:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 60;
	ypos = 55;
	if(uset_val >= 0) DF_print_value(&dev, color, fx16G, xpos, ypos, -1, uset_val);
	xpos = 60;
	ypos = 75;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, ueff_val);
	xpos = 100;
	ypos = 55;
	strcpy((char *)ascii, "V");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 100;
	ypos = 75;
	strcpy((char *)ascii, "V");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	if(!select_val) DF_print_rect(5, 38, 120, 55, color);
	if(select_val) DF_print_rect(5, 138, 120, 155, color);
	color = WHITE;
	DF_print_line(7, 75, 7, 88, color);
	DF_print_line(7, 75, 1, 81, color);
	DF_print_line(7, 88, 1, 81, color);
	DF_print_triangle(123, 80, 15, 7, 90, color);
	xpos = 25;
	ypos = 155;
	color = 0xFFF6;
	if(output_val)strcpy((char *)ascii, "OUTPUT ON");
	if(!output_val)strcpy((char *)ascii, "OUTPUT OFF");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
}

void UI_draw_statistics_screen(uint16_t p_val[100], int value_select, int division_select, bool output_val)
{
	int factor = 1;

	strcpy(file, "/spiffs/background.png");
	DF_print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	color = WHITE;
	xpos = 3;
	ypos = 28;
	strcpy((char *)ascii, "Statistics");
	DF_print_string(&dev, fx24G, xpos, ypos, ascii, color);

	switch(value_select)
	{
		case 0:
			xpos = 20;
			ypos = 50;
			strcpy((char *)ascii, "Powermeter");
			DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
			xpos = 10;
			ypos = 68;
			switch(division_select)
			{
				case 0:
					strcpy((char *)ascii, "W  3W/5ms/div");
					factor = 1;
				break;
				case 1:
					strcpy((char *)ascii, "W  1W/5ms/div");
					factor = 3;
				break;
				case 2:
					strcpy((char *)ascii, "W 0.5W/5ms/div");
					factor = 6;
				break;
				case 3:
					strcpy((char *)ascii, "W 0.25W/5ms/div");
					factor = 12;
				break;
			}
			DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
		break;
		case 1:
			xpos = 20;
			ypos = 50;
			strcpy((char *)ascii, "Voltmeter");
			DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
			xpos = 10;
			ypos = 68;
			switch(division_select)
			{
				case 0:
					strcpy((char *)ascii, "V  5V/ms/div");
					factor = 1;
				break;
				case 1:
					strcpy((char *)ascii, "V 2.5V/ms/div");
					factor = 2;
				break;
				case 2:
					strcpy((char *)ascii, "V  1V/ms/div");
					factor = 5;
				break;
				case 3:
					strcpy((char *)ascii, "V 0.5V/ms/div");
					factor = 10;
				break;
			}
			DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
		break;
		case 2:
			xpos = 20;
			ypos = 50;
			strcpy((char *)ascii, "Currentmeter");
			DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
			xpos = 10;
			ypos = 68;
			switch(division_select)
			{
				case 0:
					strcpy((char *)ascii, "A  1A/ms/div");
					factor = 1;
				break;
				case 1:
					strcpy((char *)ascii, "A 500mA/ms/div");
					factor = 2;
				break;
				case 2:
					strcpy((char *)ascii, "A 250mA/ms/div");
					factor = 4;
				break;
				case 3:
					strcpy((char *)ascii, "A 100mA/ms/div");
					factor = 10;
				break;
			}
			DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
		break;
		default:
			xpos = 20;
			ypos = 50;
			strcpy((char *)ascii, "ERROR");
			DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
		break;
	}

	xpos = 120;
	ypos = 130;
	strcpy((char *)ascii, "t");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);


	DF_print_line(15, 70, 15, 130, color);
	DF_print_line(15, 130, 115, 130, color);

	//divisions y axis
	for(int x = 0; x < 5; x++)
	{
		for(int y = 0; y < 100; y++)
		{
			if(!(y%4)) DF_print_Vpixel(y+15, x*12+70, color);
		}
	}
	//divisions x axis
	for(int x = 0; x < 5; x++)
	{
		for(int y = 0; y < 60; y++)
		{
			if(!(y%4)) DF_print_Vpixel(x*20+35, y+70, color);
		}
	}

	color = 0xFFF6;
	for(int i = 0; i < 50; i++)
	{
		uint16_t p_val_temp = p_val[i];
		//ESP_LOGI(TAG, "%d = %d", i, p_val_temp);
		if((p_val_temp*factor) < 60)
		{
			DF_print_Vpixel((i*2 + 15), (130-(p_val_temp*factor)), color);
			DF_print_Vpixel((i*2 + 16), (130-(p_val_temp*factor)), color);
		}
	}
	color = WHITE;
	DF_print_line(7, 75, 7, 88, color);
	DF_print_line(7, 75, 1, 81, color);
	DF_print_line(7, 88, 1, 81, color);
	if(value_select < 2) DF_print_triangle(123, 80, 15, 7, 90, color);
	xpos = 25;
	ypos = 155;
	color = 0xFFF6;
	if(output_val)strcpy((char *)ascii, "OUTPUT ON");
	if(!output_val)strcpy((char *)ascii, "OUTPUT OFF");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
}

void UI_draw_calibrate_screen(double INA1_S, double INA1_A, double INA2_S, double INA2_A, int select_val)
{
	strcpy(file, "/spiffs/background.png");
	DF_print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	color = WHITE;
	xpos = 10;
	ypos = 28;
	strcpy((char *)ascii, "Calibrate");
	DF_print_string(&dev, fx24G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 55;
	strcpy((char *)ascii, "INA1_S:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 75;
	strcpy((char *)ascii, "INA1_A:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 95;
	strcpy((char *)ascii, "INA2_S:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 115;
	strcpy((char *)ascii, "INA2_A:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 70;
	ypos = 55;
	if(INA1_S >= 0) DF_print_value(&dev, color, fx16G, xpos, ypos, -1, INA1_S);
	xpos = 70;
	ypos = 75;
	if(INA1_A >= 0) DF_print_value(&dev, color, fx16G, xpos, ypos, -1, INA1_A);
	xpos = 70;
	ypos = 95;
	if(INA2_S >= 0) DF_print_value(&dev, color, fx16G, xpos, ypos, -1, INA2_S);
	xpos = 70;
	ypos = 115;
	if(INA2_A >= 0) DF_print_value(&dev, color, fx16G, xpos, ypos, -1, INA2_A);
	switch(select_val)
	{
		case 0:
			DF_print_rect(5, 38, 120, 57, color);
		break;
		case 1:
			DF_print_rect(5, 58, 120, 77, color);
		break;
		case 2:
			DF_print_rect(5, 78, 120, 97, color);
		break;
		case 3:
			DF_print_rect(5, 98, 120, 117, color);
		break;
	}
	color = WHITE;
}
//Linking Function to Dfuncs
void UI_Update()
{
	DF_VlcdUpdate(&dev);
}

//Linking Function to IO_driver
void UI_GPIO_set(uint8_t GPIO_Num, bool GPIO_state)
{
	IO_GPIO_set(GPIO_Num, GPIO_state);
}

//Linking Function to IO_driver
int UI_GPIO_get(uint8_t GPIO_Num)
{
	return IO_GPIO_get(GPIO_Num);
}

//Linking Function to Expander_driver
void UI_exp_write_reg_1(uint8_t write_value)
{
	IO_exp_write_reg_1(write_value);
}

//Linking Function to Expander_driver
uint8_t UI_exp_read_reg_0()
{
	return IO_exp_read_reg_0();
}

//Linking Function to Button_driver
int UI_get_press(int button_select)
{
	return Button_get_press(button_select);
}

void UI_Buzzer_PWM(int freq)
{
	IO_Buzzer_PWM(freq);
}

void UI_Buzzer_power(bool power)
{
	IO_Buzzer_power(power);
}

//Function to test RGB LEDs
void led_test(bool mode)
{
	if(mode)
	{
		//Set LEDs to the next Color in led_test_colors
		setPixel(0, 8, led_test_colors[led_test_index][0], led_test_colors[led_test_index][1], led_test_colors[led_test_index][2]);
		setPixel(1, 8, led_test_colors[led_test_index][0], led_test_colors[led_test_index][1], led_test_colors[led_test_index][2]);
		flush();
		//Counter counts 0 - 4
		if(led_test_index < 5) led_test_index++;
		else led_test_index = 0;
	}
	else
	{
		//set LEDs to off
		setPixel(0, 0, 0, 0, 0);
		setPixel(1, 0, 0, 0, 0);
		flush();
	}
}

