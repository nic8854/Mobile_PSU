#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "ili9340.h"
#include "ui_driver.h"
#include "dfuncs.h"



/*
page_select
-----------------------------
0 = 24V  OUT
1 = 5V   OUT
2 = VAR  OUT
3 = 3.3V OUT
4 = TC-Bus
5 = Options
6 = More Measurements
7 = INA220 calibratrion
8 = change Display Settings

vaule_select page 0 - 4
-----------------------------
0 = nothing
1 = Output On/Off
2 = More Measurements

vaule_select page 5
-----------------------------
0 = nothing
1 = INA220 calibration
2 = change display options
3 = Output On/Off
4 = More Measurements

vaule_select page 6
-----------------------------
0 = nothing

vaule_select page 7
-----------------------------
0 = nothing
1 = calibrate INA1
2 = calibrate INA2
3 = calibrate INA3

vaule_select page 7
-----------------------------
0 = nothing
1 = digits displayed
2 = Output On/Off
3 = More Measurements
*/

#define OUT24   0
#define OUT5    1
#define OUTVAR  2
#define OUT33   3
#define TCBUS   4
#define OPTIONS 5
#define MEASURE 6
#define INA220  7
#define DISPSET 8

int page_select = 0;
int value_select = 0;
double value_selected = 0;
int output_state = 0;
int value_select_max = 0;

int up_state = 0;
int down_state = 0;
int left_state = 0;
int right_state = 0;
int middle_state = 0;
int clkwise_state = 0;
int cclkwise_state = 0;

int up_state_last = 0;
int down_state_last = 0;
int left_state_last = 0;
int right_state_last = 0;
int middle_state_last = 0;
int clkwise_state_last = 0;
int cclkwise_state_last = 0;

void enter_page()
{
    
}
void change_sel_value()
{
    if(up_state)
    {
        if(value_select != 0) value_select--;
        else value_select = value_select_max;
    }
    if(down_state)
    {
        if(value_select != value_select_max) value_select++;
        else value_select = 0;
    }
}

void change_page()
{
    if(right_state) page_select++;
    if(left_state) page_select--;
}

void refresh_GUI()
{

}


void create_GUI()
{
    xTaskCreate(ui_driver, "ui_driver", 1024*6, NULL, 2, NULL);
}

void ui_driver(void *pvParameters)
{
    //read_io();

    if(right_state_last < right_state) change_page();
    if(left_state_last < left_state) change_page();
    if(up_state_last < up_state) change_value();
    if(down_state_last < down_state) change_value();
    if(middle_state_last < middle_state) enter_page();

    refresh_GUI();

}