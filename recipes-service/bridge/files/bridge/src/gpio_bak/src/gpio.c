#include <zmq.h>
#include <assert.h>

#define GPIO_CHIP_PATH "/dev/gpiochip6" //gpio bank for PI*
//PI0 is connected to NOT_INT pin from radio

#define NOT_IRQ_OFFSET 3
#define LINE_REQUEST_TIMEOUT_NS 50000000 ///<50ms

#include "gpio.h"

#define INPROC_CON_STR "inproc://gpio_event"

//#define DEBUG_GPIO_EVENT

bool gpio_global_event_set = false;
bool gpio_global_event_ack = false;

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

static struct gpiod_request_config * create_line_request(void){
	struct gpiod_request_config * to_return = gpiod_request_config_new();
	assert(to_return != NULL);
	
	gpiod_request_config_set_consumer(to_return, "iris");
	//gpiod_request_config_set_event_buffer_size(to_return, 0);
	return to_return;
}

static void set_gpio_event(void * event_sock){
#ifdef DEBUG_GPIO_EVENT
	printf("DEBUG: set_gpio_event()\n");
#endif
	int rc = zmq_send(event_sock, "I", 1, 0); //send a message on the socket here to indicate the event
	if (rc != 1) {printf("zmq_send() error\n");}
	
	if (!gpio_global_event_ack){
		gpio_global_event_set = true;
	}
}

//loop for the gpio event monitor thread
static void * gpio_event_monitor_run(void * zmq_ctx){
	static uint8_t rx_buf[8];
	int rc;
	void * event_sock = zmq_socket(zmq_ctx, ZMQ_PAIR);
	rc = zmq_connect(event_sock, INPROC_CON_STR);
	assert(rc == 0);
	
	unsigned int line_offsets[1] = {NOT_IRQ_OFFSET};

	//gpiod calls
	
	struct gpiod_request_config * line_request_config = create_line_request();
	assert(line_request_config != NULL);
	
	struct gpiod_line_config * line_config = gpiod_line_config_new();
	assert(line_config != NULL);
	
    struct gpiod_line_settings * line_settings = create_line_settings_for_falling_edge_events_line();
	assert(line_settings != NULL);
	
	gpiod_line_config_reset(line_config);
	rc = gpiod_line_config_add_line_settings(line_config, line_offsets, 1, line_settings);
	assert(rc==0);
	
	struct gpiod_chip * chip = gpiod_chip_open(GPIO_CHIP_PATH);
	assert(chip != NULL);
	
	//request the line settings
	struct gpiod_line_request * line_request = gpiod_chip_request_lines(chip, line_request_config, line_config);
	assert(line_request != NULL);
	
	struct gpiod_edge_event_buffer * events = gpiod_edge_event_buffer_new(1);
	assert(events != NULL);
	
	bool stopped = false;
	while(!stopped){
		if (gpiod_line_request_get_value(line_request, NOT_IRQ_OFFSET)==GPIOD_LINE_VALUE_ACTIVE){
			set_gpio_event(event_sock);
		} else {
			gpio_global_event_set = false;
		}
		rc = gpiod_line_request_wait_edge_events(line_request, LINE_REQUEST_TIMEOUT_NS);
		if 		(rc < 0) {break;} //error ocurred
		else if (rc == 1) { //an event is pending
			rc = gpiod_line_request_read_edge_events(line_request, events, 1);
			struct gpiod_edge_event * event = gpiod_edge_event_buffer_get_event(events, 0);
			if (gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_FALLING_EDGE){
				set_gpio_event(event_sock);
			}
		}
		
		//check for a message from the pair socket here
		rc = zmq_recv(event_sock, rx_buf, sizeof(rx_buf), ZMQ_DONTWAIT);
		if (rc > 0){
			if (rx_buf[0] == 'S'){
				printf("INFO: gpio_monitor shutting down\n");
				stopped = true;
			}
		}
	}
	
	gpiod_edge_event_buffer_free(events);
	gpiod_line_request_release(line_request); //should free line_request_config and line_config as well
	gpiod_request_config_free(line_request_config);
	gpiod_line_config_free(line_config);
	gpiod_line_settings_free(line_settings);
	gpiod_chip_close(chip);
	zmq_close(event_sock);
    return NULL;
}

static void * gpio_monitor_socket = NULL;

//set zmq_ctx to null to not create sockets and just poll the gpios
void * gpio_init(void * zmq_ctx){
	static pthread_t gpio_monitor;
	
	
	if (zmq_ctx != NULL){
		gpio_monitor_socket = zmq_socket(zmq_ctx, ZMQ_PAIR);
		int rc;
		rc = zmq_bind(gpio_monitor_socket, INPROC_CON_STR);
		assert(rc==0);
	}
    
    pthread_create(&gpio_monitor, NULL, gpio_event_monitor_run, zmq_ctx);
	zclock_sleep(250);
	return gpio_monitor_socket;
}

void gpio_deinit(void){
	if (gpio_monitor_socket != NULL){
		zmq_send(gpio_monitor_socket, "S", 1, 0);
		zclock_sleep(500);	
		zmq_close(gpio_monitor_socket);
	}
}
