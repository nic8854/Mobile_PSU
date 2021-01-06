#ifndef MAIN_UI_DRIVER_H_
#define MAIN_UI_DRIVER_H_

#include "driver/spi_master.h"
#include "fontx.h"
#include "ili9340.h"
#include "pngle.h"

void enter_page();
void change_value();
void change_page();
void refresh_GUI();
void create_GUI();
void func_page_1_4();
void func_page_5();
void func_page_6();
void func_page_8();
void func_page_9();
void toggle_outs();

#endif