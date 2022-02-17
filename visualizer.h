#ifndef _VISUALIZER_H
#define _VISUALIZER_H

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

bool init_visualizer(void);
void store_extension_key(const uint8_t *buf, int len);
void visualize_inputs(const uint8_t *buf, int len); //, const unit8_t *key);
bool exit_visualizer(void);

#endif

