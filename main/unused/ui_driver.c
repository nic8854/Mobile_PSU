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
1 = 24V  OUT
2 = 5V   OUT
3 = VAR  OUT
4 = 3.3V OUT
5 = TC-Bus
6 = Options
7 = More Measurements
8 = INA220 calibratrion
9 = change Display Settings

value_select page 1 - 5
-----------------------------
0 = nothing
1 = Output On/Off
2 = More Measurements

value_select page 6
-----------------------------
0 = nothing
1 = INA220 calibration
2 = change display options
3 = Output On/Off
4 = More Measurements

value_select page 7
-----------------------------
0 = nothing

value_select page 8
-----------------------------
0 = nothing
1 = calibrate INA1
2 = calibrate INA2
3 = calibrate INA3

vaule_select page 9
-----------------------------
0 = nothing
1 = digits displayed
2 = Output On/Off
3 = More Measurements
*/

#define OUT24   1
#define OUT5    2
#define OUTVAR  3
#define OUT33   4
#define TCBUS   5
#define OPTIONS 6
#define MEASURE 7
#define INA220  8
#define DISPSET 9

int page_select = 0;
int page_select_last = 0;
int value_select = 0;
double value_selected = 0;
int output_state = 0;
int value_select_max = 0;
int page_select_max = 5;
int digit_select = 0;

void enter_page()
{
    switch(page_select)
    {
        case OUT24:     func_page_1_4();
        case OUT5:      func_page_1_4();
        case OUTVAR:    func_page_1_4();
        case OUT33:     func_page_1_4();
        case TCBUS:     func_page_5();
        case OPTIONS:   func_page_6();
        case MEASURE:   func_page_7();
        case INA220:    func_page_8();
        case DISPSET:   func_page_9();
    }
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
    if(right_state)
    {
       if(page_select != page_select_max) page_select++;
       else value_select = 0; 
    } 
    if(left_state)
    {
        if(page_select != 0) page_select--;
        else value_select = page_select_max;
    } 
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

void func_page_1_4()
{
    switch(value_select)
    {
        case 1: toggle_outs(); break;
        case 2: page_select = OPTIONS; break;
    }
}

void func_page_6()
{
    switch(value_select)
    {
        case 1: page_select = INA220; break;
        case 2: page_select = DISPSET; break;
        case 3: toggle_outs(); break;
        case 4: page_select = OPTIONS; break;
    }
}

void func_page_8()
{
    switch(value_select)
    {
        case 1: INA_cal(1); break;
        case 2: INA_cal(2); break;
        case 3: INA_cal(3); break;
    }
}

void func_page_9()
{
    switch(value_select)
    {
        case 1: change_digit(); break;
        case 2: toggle_outs(); break;
        case 3: page_select = OPTIONS; break;
    }
}

void toggle_outs()
{
    /*
    output_state
    -------------
    0 = OFF
    1 = ON
    */

    if(output_state) output_state = 0;
    else output_state = 1;
}

void change_digit()
{
    /*
    digit_select
    -------------
    0 = .00
    1 = .0
    2 = .
    */

    if(digit_select < 2) digit_select++;
    else digit_select = 0;
}
*/