#include <zmq.h>
#include <assert.h>

/*
//PI0 is connected to NOT_INT pin from radio
#define GPIO_CHIP_PATH "/dev/gpiochip6" //gpio bank for PI*
#define NOT_IRQ_OFFSET 3
*/

//PF9 is connected to NOT_INT pin from radio
#define GPIO_CHIP_PATH "/dev/gpiochip5" //gpio bank for PF*
#define NOT_IRQ_OFFSET 9

#define SHUTDOWN_GPIO_CHIP_PATH "/dev/gpiochip4" //gpio bank for PE*
#define SHUTDOWN_OFFSET 10

//#define LINE_REQUEST_TIMEOUT_NS  500000 ///<0.5ms
//#define LINE_REQUEST_TIMEOUT_NS 1000000 ///<1ms
//#define LINE_REQUEST_TIMEOUT_NS   1500000 ///<1.5ms
#define LINE_REQUEST_TIMEOUT_NS   2000000 ///<1.5ms

#include "gpio.h"
#include <czmq.h>
#include "os_signal.h"

//#define DEBUG_GPIO_EVENT


unsigned int line_offsets[1] = {NOT_IRQ_OFFSET};

//gpiod objects
struct gpiod_request_config * line_request_config;
struct gpiod_line_config * line_config;
struct gpiod_line_settings * line_settings;
struct gpiod_chip * chip;
struct gpiod_line_request * line_request;
struct gpiod_edge_event_buffer * events;

unsigned int shutdown_line_offsets[1] = {SHUTDOWN_OFFSET};
struct gpiod_request_config * shutdown_line_request_config;
struct gpiod_line_config * shutdown_line_config;
struct gpiod_line_settings * shutdown_line_settings;
struct gpiod_chip * chip_E;
struct gpiod_line_request * line_request_E10;


//creates and object that must be freed with gpiod_line_settings_free(line_settings)
static struct gpiod_line_settings * create_line_settings_for_falling_edge_events_line(void){
	int rc;
	struct gpiod_line_settings * to_return = gpiod_line_settings_new();
	assert(to_return != NULL);
	
	rc = gpiod_line_settings_set_direction(to_return, GPIOD_LINE_DIRECTION_INPUT);
	assert(rc==0);
	
	rc = gpiod_line_settings_set_edge_detection(to_return, GPIOD_LINE_EDGE_FALLING);
	assert(rc==0);
	
	rc = gpiod_line_settings_set_bias(to_return, GPIOD_LINE_BIAS_PULL_UP);
	assert(rc==0);
	
	gpiod_line_settings_set_active_low(to_return, true);
	
	rc = gpiod_line_settings_set_event_clock(to_return, GPIOD_LINE_CLOCK_MONOTONIC);
	assert(rc==0);	
	
	return to_return;
}

static struct gpiod_line_settings * create_line_settings_for_output(void){
	int rc;
	struct gpiod_line_settings * to_return = gpiod_line_settings_new();
	assert(to_return != NULL);

	rc = gpiod_line_settings_set_direction(to_return, GPIOD_LINE_DIRECTION_OUTPUT);
	assert(rc==0);
	
	gpiod_line_settings_set_output_value(to_return, GPIOD_LINE_VALUE_INACTIVE);
	
	return to_return;
	
}

static struct gpiod_request_config * create_line_request(void){
	struct gpiod_request_config * to_return = gpiod_request_config_new();
	assert(to_return != NULL);
	
	gpiod_request_config_set_consumer(to_return, "iris");
	//gpiod_request_config_set_event_buffer_size(to_return, 0);
	return to_return;
}

bool gpio_init(void){
	int rc;
	//////////////////////////////////////////
	//gpiod calls to setup the interrupt input
	//////////////////////////////////////////
	line_request_config = create_line_request();
	assert(line_request_config != NULL);
	
	line_config = gpiod_line_config_new();
	assert(line_config != NULL);
	
    line_settings = create_line_settings_for_falling_edge_events_line();
	assert(line_settings != NULL);
	
	gpiod_line_config_reset(line_config);
	rc = gpiod_line_config_add_line_settings(line_config, line_offsets, 1, line_settings);
	assert(rc==0);
	
	chip = gpiod_chip_open(GPIO_CHIP_PATH);
	assert(chip != NULL);
	
	//request the line settings
	line_request = gpiod_chip_request_lines(chip, line_request_config, line_config);
	assert(line_request != NULL);
	
	events = gpiod_edge_event_buffer_new(1);
	assert(events != NULL);

	//////////////////////////////////////////
	//gpiod calls to setup shutdown output
	//////////////////////////////////////////	
	
	shutdown_line_request_config = create_line_request();
	assert(shutdown_line_request_config != NULL);
	
	shutdown_line_config = gpiod_line_config_new();
	assert(shutdown_line_config != NULL);
	
	shutdown_line_settings = create_line_settings_for_output();
	assert(shutdown_line_settings != NULL);
	
	gpiod_line_config_reset(shutdown_line_config);
	rc = gpiod_line_config_add_line_settings(shutdown_line_config, shutdown_line_offsets, 1, shutdown_line_settings);
	assert(rc==0);
	
	chip_E = gpiod_chip_open(SHUTDOWN_GPIO_CHIP_PATH);
	assert(chip_E != NULL);
	
	//request the line settings
	line_request_E10 = gpiod_chip_request_lines(chip_E, shutdown_line_request_config, shutdown_line_config);
	assert(line_request_E10 != NULL);
	
	assert(gpio_set_shutdown(false)==true);
	
	return true;
}

bool gpio_poll(void){
	if (gpiod_line_request_get_value(line_request, NOT_IRQ_OFFSET)==GPIOD_LINE_VALUE_ACTIVE){
		return true;
	}
	
	int rc = gpiod_line_request_wait_edge_events(line_request, LINE_REQUEST_TIMEOUT_NS);
	if (rc < 0) {
		printf("ERROR: gpiod_line_request_wait_edge_events() failed!\n");
		os_interrupted = 1;
		return false;
	} //error ocurred
	else if (rc >= 1) { //an event is pending
		gpiod_line_request_read_edge_events(line_request, events, 1);
		if (gpiod_edge_event_get_event_type(gpiod_edge_event_buffer_get_event(events, 0)) == GPIOD_EDGE_EVENT_FALLING_EDGE){
			//printf("-\n");
			return true;
		}
		
		//struct gpiod_edge_event * event = gpiod_edge_event_buffer_get_event(events, 0);
		//if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_FALLING_EDGE){
		//	return true;
		//}
	}
	return false;
}

bool gpio_get(void){
	return (gpiod_line_request_get_value(line_request, NOT_IRQ_OFFSET)==GPIOD_LINE_VALUE_ACTIVE);
}

bool gpio_set_shutdown(bool value){
	int rc;
	enum gpiod_line_value line_value = (value==true) ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
	rc = gpiod_line_request_set_value(line_request_E10, shutdown_line_offsets[0], line_value);
	return (rc==0);
}


void gpio_deinit(void){
	gpiod_edge_event_buffer_free(events);
	gpiod_line_request_release(line_request); //should free line_request_config and line_config as well
	gpiod_request_config_free(line_request_config);
	gpiod_line_config_free(line_config);
	gpiod_line_settings_free(line_settings);
	gpiod_chip_close(chip);
	
	gpiod_line_request_release(line_request_E10);
	gpiod_request_config_free(shutdown_line_request_config);
	gpiod_line_config_free(shutdown_line_config);
	gpiod_line_settings_free(shutdown_line_settings);
	gpiod_chip_close(chip_E);
}
