//standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <czmq.h>

#include "print_util.h"
#include "radio.h"

#define RADIO_CONFIG_DIR "/bridge/conf/si4463/active/"

//#define RADIO_CONFIG_REGEX "^\\s*#define\\s+(RF_[^ ]*)\\s+(0x[^\\t\\r\\n]*)"
//#define RADIO_MODEM_CONFIG_REGEX "^\\s*#define\\s+(RF_MODEM_[^ ]*|RF_PA_[^ ]*|RF_SYNTH_[^ ]*|RF_MATCH_[^ ]*|RF_FREQ_[^ ]*)\\s+(0x[^\\t\\r\\n]*)"

int main(void) {
	
	//zhashx_t * config = parse_radio_config("./test_radio_config.h", NULL);
	//assert(config != NULL);
	
	zhashx_t * config = radio_load_configs(RADIO_CONFIG_DIR, NULL, RADIO_MODEM_CONFIG_REGEX);
	assert(config != NULL);
	
	zchunk_t * c = zhashx_first(config);
	while (c != NULL){
		printArrHex(zchunk_data(c), zchunk_size(c));
		c = zhashx_next(config);
	}
	
}
