#ifndef BEEP_H
#define BEEP_H

#include "esp_err.h"

void megolavania(void);
void mcdonalds(void);
void megolavania_task(void);
void ascent_beep(void);
void break_beep(void);
void digit_beep(void);
void high_beep(void);
void low_beep(void);
void battery_beep(void);
void error_beep(void);
void wait_beep(void);
esp_err_t r3_init_beep(void);

#endif