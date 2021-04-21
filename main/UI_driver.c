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
uint8_t Reg1_value = 0;
uint8_t Reg1_value_last = 0;

/**
 * Initializiation function for display, Buttons and RGB LEDs. Starts SPI target for Display. Also draws boot screen
 * Starts Button_init() and APA102_init().
 * 
 * @param I2C_PORT sets I2c_port for Button init
 * @param SDA_GPIO sets SDA GPIO for Button init
 * @param SCL_GPIO sets SCL GPIO for Button init
 *  
 * @endcode
 * \ingroup UI_draw
 */
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
		if(i%2) UI_set_LED0(1);
		else UI_set_LED0(0);
		xpos = i*8;
		DF_print_string(&dev, fx16M, xpos, ypos, ascii, color);
		DF_VlcdUpdate(&dev);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}

	//startup
	UI_GPIO_set(LED_0, 1);
	UI_Buzzer_power(1);
	UI_Buzzer_PWM(100);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	UI_Buzzer_PWM(300);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	UI_Buzzer_PWM(500);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	UI_Buzzer_PWM(700);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	UI_Buzzer_PWM(900);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	UI_Buzzer_PWM(1100);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	UI_Buzzer_power(0);

    DF_VlcdUpdate(&dev);
	ESP_LOGI(TAG, "--> UI_driver initialized successfully");
}

/**
 * Function to generate a variable Screen using the dfuncs library.
 *
 * Display must be initialized to use this function. Initialize using UI_init.
 * 
 * @param power_val Input power value in Watts.
 * @param voltage_val Input voltage value in Volts.
 * @param current_val Input current value in Amps.
 * @param output_val Sets output display to on or off(1 or 0).
 *  
 * @endcode
 * \ingroup UI_draw
 */
void UI_draw_main_screen(double power_val, double voltage_val, double current_val, bool output_val)
{
	//getting Background from Spiffs and printing it
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

/**
 * Function to generate a variable Screen using the dfuncs library.
 *
 * Display must be initialized to use this function. Initialize using UI_init.
 * 
 * @param out24_val 24V output value in Volts.
 * @param out5_val 5V output value in Volts.
 * @param outvar_val Variable Output Value in Volts
 * @param out33_val 3.3V output value in Volts.
 * @param output_val Sets output display to on or off(1 or 0).
 *  
 * @endcode
 * \ingroup UI_draw
 */
void UI_draw_voltages_screen(double out24_val, double out5_val, double outvar_val, double out33_val, bool output_val)
{
	//getting Background from Spiffs and printing it
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

/**
 * Function to generate a variable Screen using the dfuncs library.
 *
 * Display must be initialized to use this function. Initialize using UI_init.
 * 
 * @param uset_val The set value of the variable out in Volts.
 * @param ueff_val The effective value of the varible out in Volts.
 * @param select_val Value to select which parameter should be selected. Draws Rectangle around selected Value.
 * @param output_val Sets output display to on or off(1 or 0).
 *  
 * @endcode
 * \ingroup UI_draw
 */
void UI_draw_variable_screen(double uset_val, double ueff_val, int select_val, bool output_val)
{
	//getting Background from Spiffs and printing it
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

/**
 * Function to generate a Statistics Screen using the dfuncs library.
 *
 * Display must be initialized to use this function. Initialize using UI_init.
 * 
 * @param p_val Array of 100 values, that should be displayed on display. Values need to be scaled from 0-60.
 * @param screen_select Selects between Power, Voltage and Current. Only changes displayed value.
 * @param division_select Selects the value for the divisions.
 * @param select_val Value to select which parameter should be selected. Draws Rectangle around selected Value.
 * @param output_val Selects if output is on or off(1 or 0).
 *  
 * @endcode
 * \ingroup UI_draw
 */
void UI_draw_statistics_screen(uint16_t p_val[100], int screen_select, int division_select, int select_val, bool output_val)
{
	//scaling factor for divisions
	int factor = 1;
	//getting Background from Spiffs and printing it
	strcpy(file, "/spiffs/background.png");
	DF_print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	color = WHITE;
	xpos = 3;
	ypos = 28;
	strcpy((char *)ascii, "Statistics");
	DF_print_string(&dev, fx24G, xpos, ypos, ascii, color);

	//select if power, volts, or current
	switch(screen_select)
	{
		case 0:
			xpos = 20;
			ypos = 50;
			strcpy((char *)ascii, "Powermeter");
			DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
			xpos = 10;
			ypos = 68;
			//selects what division to use
			switch(division_select)
			{
				case 0:
					strcpy((char *)ascii, "W  3W/2s/div");
					factor = 1;
				break;
				case 1:
					strcpy((char *)ascii, "W  1W/2s/div");
					factor = 3;
				break;
				case 2:
					strcpy((char *)ascii, "W 0.5W/2s/div");
					factor = 6;
				break;
				case 3:
					strcpy((char *)ascii, "W 1/4W/2s/div");
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
			//selects what division to use
			switch(division_select)
			{
				case 0:
					strcpy((char *)ascii, "V  5V/2s/div");
					factor = 1;
				break;
				case 1:
					strcpy((char *)ascii, "V 2.5V/2s/div");
					factor = 2;
				break;
				case 2:
					strcpy((char *)ascii, "V  1V/2s/div");
					factor = 5;
				break;
				case 3:
					strcpy((char *)ascii, "V 0.5V/2s/div");
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
			xpos = 7;
			ypos = 68;
			//selects what division to use
			switch(division_select)
			{
				case 0:
					strcpy((char *)ascii, "A  1A/2s/div");
					factor = 1;
				break;
				case 1:
					strcpy((char *)ascii, "A 500mA/2s/div");
					factor = 2;
				break;
				case 2:
					strcpy((char *)ascii, "A 250mA/2s/div");
					factor = 4;
				break;
				case 3:
					strcpy((char *)ascii, "A 100mA/2s/div");
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

	//draw graph
	color = 0xFFF6;
	for(int i = 0; i < 50; i++)
	{
		uint16_t p_val_temp = p_val[i];
		if((p_val_temp*factor) < 60)
		{
			//draw selected Pixel and the one right from it
			DF_print_Vpixel((i*2 + 15), (130-(p_val_temp*factor)), color);
			DF_print_Vpixel((i*2 + 16), (130-(p_val_temp*factor)), color);
		}
	}

	if(!select_val) DF_print_rect(20, 52, 118, 67, color);
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
/**
 * Function to generate a Calibration Screen using the dfuncs library.
 *
 * Display must be initialized to use this function. Initialize using UI_init.
 * 
 * @param INA1_S The Shunt Value in Ohms for INA 1.
 * @param INA1_A Max Current Value in mA for INA 1.
 * @param INA2_S The Shunt Value in Ohms for INA 2.
 * @param INA2_A Max Current Value in mA for INA 2.
 * @param select_val Value to select which parameter should be selected. Draws Rectangle around slected Value.
 *  
 * @endcode
 * \ingroup UI_draw
 */
void UI_draw_calibrate_screen_1(double INA1_S, double INA1_A, double INA2_S, double INA2_A, int select_val)
{
	//getting Background from Spiffs and printing it
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
	xpos = 60;
	ypos = 55;
	DF_print_value(&dev, color, fx16G, xpos, ypos, INA1_S, -1);
	xpos = 60;
	ypos = 75;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, INA1_A);
	xpos = 60;
	ypos = 95;
	DF_print_value(&dev, color, fx16G, xpos, ypos, INA2_S, -1);
	xpos = 60;
	ypos = 115;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, INA2_A);
	xpos = 95;
	ypos = 55;
	strcpy((char *)ascii, "mOhm");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 100;
	ypos = 75;
	strcpy((char *)ascii, "A");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 95;
	ypos = 95;
	strcpy((char *)ascii, "mOhm");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 100;
	ypos = 115;
	strcpy((char *)ascii, "A");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	switch(select_val)
	{
		case 0:
			DF_print_rect(5, 38, 127, 57, color);
		break;
		case 1:
			DF_print_rect(5, 58, 127, 77, color);
		break;
		case 2:
			DF_print_rect(5, 78, 127, 97, color);
		break;
		case 3:
			DF_print_rect(5, 98, 127, 117, color);
		break;
	}
	color = WHITE;
}
void UI_draw_calibrate_screen_2(double out24, double out5, double out33, double outvar, int select_val)
{
	//getting Background from Spiffs and printing it
	strcpy(file, "/spiffs/background.png");
	DF_print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	color = WHITE;
	xpos = 10;
	ypos = 28;
	strcpy((char *)ascii, "Calibrate");
	DF_print_string(&dev, fx24G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 55;
	strcpy((char *)ascii, "   24V:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 75;
	strcpy((char *)ascii, "    5V:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 95;
	strcpy((char *)ascii, "  3.3V:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 5;
	ypos = 115;
	strcpy((char *)ascii, "   Var:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 60;
	ypos = 55;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, out24);
	xpos = 60;
	ypos = 75;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, out5);
	xpos = 60;
	ypos = 95;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, out33);
	xpos = 60;
	ypos = 115;
	DF_print_value(&dev, color, fx16G, xpos, ypos, -1, outvar);

	switch(select_val)
	{
		case 0:
			DF_print_rect(5, 38, 127, 57, color);
		break;
		case 1:
			DF_print_rect(5, 58, 127, 77, color);
		break;
		case 2:
			DF_print_rect(5, 78, 127, 97, color);
		break;
		case 3:
			DF_print_rect(5, 98, 127, 117, color);
		break;
	}
	color = WHITE;
}

/**
 * Function to generate a tcbus Screen using the dfuncs library.
 * Still under construction
 * @param TC_EN_val Enable Pin value for TC Bus
 * @param TC_NFON_val Low frequency mode Pin for TC Bus
 * @param output_val Value of output
 * @param select_val chooses which value to select on screen
 * Updates LCD from Virtual Screen
 * @endcode
 */
void UI_draw_tcbus_screen(bool TC_EN_val, bool TC_NFON_val, bool output_val, int select_val)
{
	//getting Background from Spiffs and printing it
	strcpy(file, "/spiffs/background.png");
	DF_print_png(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	color = WHITE;
	xpos = 25;
	ypos = 28;
	strcpy((char *)ascii, "TC Bus");
	DF_print_string(&dev, fx24G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 55;
	strcpy((char *)ascii, "TC_EN:");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 10;
	ypos = 75;
	strcpy((char *)ascii, "NFON :");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 20;
	ypos = 115;
	strcpy((char *)ascii, "more coming");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 40;
	ypos = 130;
	strcpy((char *)ascii, "soon");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 65;
	ypos = 55;
	if(TC_EN_val)strcpy((char *)ascii, "ON");
	if(!TC_EN_val)strcpy((char *)ascii, "OFF");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
	xpos = 65;
	ypos = 75;
	if(TC_NFON_val)strcpy((char *)ascii, "ON");
	if(!TC_NFON_val)strcpy((char *)ascii, "OFF");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);

	switch(select_val)
	{
		case 0:
			DF_print_rect(5, 38, 120, 57, color);
		break;
		case 1:
			DF_print_rect(5, 58, 120, 77, color);
		break;
		case 2:
			DF_print_rect(5, 138, 120, 155, color);
		break;
	}

	color = WHITE;
	DF_print_line(7, 75, 7, 88, color);
	DF_print_line(7, 75, 1, 81, color);
	DF_print_line(7, 88, 1, 81, color);

	xpos = 25;
	ypos = 155;
	color = 0xFFF6;
	if(output_val)strcpy((char *)ascii, "OUTPUT ON");
	if(!output_val)strcpy((char *)ascii, "OUTPUT OFF");
	DF_print_string(&dev, fx16G, xpos, ypos, ascii, color);
}
/**
 * Linking Function to Dfuncs
 * Updates LCD from Virtual Screen
 * @endcode
 */
void UI_Update()
{
	DF_VlcdUpdate(&dev);
}

/**
 * Linking Function to IO driver
 * Sets level of specified GPIO Pin.
 * @param GPIO_Num selects GPIO from a List of defines(see header)
 * @param GPIO_state selects what state the GPIO should change to (HIGH or LOW)
 * @endcode
 */
void UI_GPIO_set(uint8_t GPIO_Num, bool GPIO_state)
{
	IO_GPIO_set(GPIO_Num, GPIO_state);
}

/**
 * Linking Function to IO driver
 * Gets level of specified GPIO Pin.
 * @param GPIO_Num selects GPIO from a List of defines(see header)
 * @return returns state of GPIO as an int
 * @endcode
 */
int UI_GPIO_get(uint8_t GPIO_Num)
{
	return IO_GPIO_get(GPIO_Num);
}

/**
 * Linking Function to Expander driver
 * Sets levels of Reg 1.
 * @param write_value Bit Pattern to set Reg 1 to.
 * @return returns state of GPIO as an int
 * @endcode
 */
void UI_exp_write_reg_1(uint8_t write_value)
{
	IO_exp_write_reg_1(write_value);
}

/**
 * Linking Function to Expander driver
 * Gets levels of reg 0
 * @return returns states of reg 0 as a bit pattern
 * @endcode
 */
uint8_t UI_exp_read_reg_0()
{
	return IO_exp_read_reg_0();
}

/**
 * Linking Function to Button driver.
 * get press state of selected button
 * @param button_select Selects which buttons value to return
 * @return returns button state as an int. 0 = no press, 1 = short press, 2 = long press
 * @endcode
 */
int UI_get_press(int button_select)
{
	return Button_get_press(button_select);
}

/**
 * Linking Function to Button driver.
 * Resets all Button states. Used when changing to new screen.
 * 
 * @endcode
 */
void UI_reset_all_states()
{
	Button_reset_all_states();
}


/**
 * Linking Function to Button driver.
 * Returns the value of the encoder counter.
 * 
 * @return Returns Counter value as an Intiture
 *  
 * @endcode
 */
int UI_get_ENC()
{
	return Button_get_ENC();
}

/**
 * Linking Function to Button driver.
 * Sets encoder counter to specified number.
 * 
 * @param value value which counter should be set to
 *  
 * @endcode
 */
void UI_set_ENC(int value)
{
	Button_set_ENC(value);
}

/**
 * Linking Function to IO driver
 * Sets Buzzer PWM to specified frequency
 * @param freq set frequency. Needs to be within 100 - 10000Hz.
 * @endcode
 */
void UI_Buzzer_PWM(int freq)
{
	IO_Buzzer_PWM(freq);
}

/**
 * Linking Function to IO driver
 * Sets Buzzer PWM to on or off
 * @param power sets power state. 1 = on, 0 = off.
 * @endcode
 */
void UI_Buzzer_power(bool power)
{
	IO_Buzzer_power(power);
}

/**
 * Short Beep of Buzzer
 * 
 * @endcode
 */
void UI_Buzzer_beep()
{
	IO_Buzzer_PWM(150);
	IO_Buzzer_power(1);
	vTaskDelay(25 / portTICK_PERIOD_MS);
	IO_Buzzer_power(0);
}

/**
 * Linking Function to IO driver
 * Sets LED0 to on or off
 * @param value State as a boolean
 * @endcode
 */
void UI_set_LED0(bool value)
{
	IO_GPIO_set(LED_0, value);
}

/**
 * Linking Function to APA102. 
 * Sets Sets color and brihtness of RGB LEDs. 
 * Both LEDs always have to be set.
 * @param index selects which LED to write to. In this case 0 or 1.
 * @param bright sets brightness setting form 0 to 31.
 * @param red set red channnel from 0 to 255.
 * @param green set green channnel from 0 to 255.
 * @param blue set blue channnel from 0 to 255.
 * @endcode
 */
void UI_set_RGB(uint8_t index, int bright, int red, int green, int blue)
{
	setPixel(index, bright, green, blue, red);
	flush();
}

void UI_set_TC_EN(bool value)
{
	if(value) Reg1_value = (Reg1_value | 0x01);
	else Reg1_value = (Reg1_value & 0xFE);
	if(Reg1_value != Reg1_value_last) Button_write_reg_1(Reg1_value);
	Reg1_value_last = Reg1_value;
}
void UI_set_TC_NFON(bool value)
{
	if(value) Reg1_value = (Reg1_value | 0x02);
	else Reg1_value = (Reg1_value & 0xFD);
	if(Reg1_value != Reg1_value_last) Button_write_reg_1(Reg1_value);
	Reg1_value_last = Reg1_value;
}