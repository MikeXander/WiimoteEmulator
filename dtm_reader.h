#ifndef DTM_READER_H
#define DTM_READER_H

#include <stdio.h>
#include <stdint.h>
#include "wiimote.h"
#include "wm_crypto.h"

const char *DEFAULT_TAS = "./tas.dtm";
const char *DEFAULT_KEY = "./taskey.txt";

// returns 0 on error or input sequence end
int next_dtm_report(struct wiimote_state * state, uint8_t *buf);

#endif // _DTM_READER_H