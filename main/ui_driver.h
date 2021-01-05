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

#endif