#include <pebble_worker.h>

void light_enable(bool enable);

bool auto_backlight = false;
bool light_on = false;
time_t watch_level_start = 0;

void
light_callback (void *data) 
{ 

    light_on = false;
    light_enable(false);
}


#define X_RANGE_LOW -250
#define X_RANGE_HIGH 250
#define Y_RANGE_LOW -1000
#define Y_RANGE_HIGH -300

void
handle_accel(AccelData *data, uint32_t num_samples)
{
    static bool watch_outside_range = true;

    if (data->x < X_RANGE_HIGH && data->x > X_RANGE_LOW &&
	data->y > Y_RANGE_LOW && data->y < Y_RANGE_HIGH) {
	if (light_on == false &&
	    watch_level_start == 0 &&
	    watch_outside_range) {
	    watch_level_start = time(0L); /* record when level started */
	    watch_outside_range = false;
	}
    } else if ((data->x > (X_RANGE_HIGH + 50) || data->x < (X_RANGE_LOW - 50)) ||
	       (data->y < (Y_RANGE_LOW - 100) || data->y > (Y_RANGE_HIGH + 100))) {
	watch_level_start = 0;
	watch_outside_range = true;
    }

    if (watch_level_start != 0 && (time(0L) - watch_level_start) > 0) {
	light_enable(true);
	light_on = true;
	app_timer_register(5000, light_callback, NULL);
	watch_level_start = 0;
    }
}




#ifdef notdef

void
handle_accel(AccelData *data, uint32_t num_samples) 
{

    if (data->x < 250 && data->x > -250 &&
	data->y > -1000 && data->y < 0) {
//	if (light_on == false && watch_level_start == false) {
	if (!light_on && watch_level_start == false) {
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Start backlight\n");
	    light_enable(true);
	    light_on = true;
	    app_timer_register(5000, light_callback, NULL);
	    watch_level_start = true;
	}
    } else {
	if (watch_level_start) {
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "Reset backlight x=%d, y=%d\n", data->x, data->y);
	}
	watch_level_start = false;
	light_on = false;
	light_enable(false);
    }
}

#endif


int main(void) {
    accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
    accel_data_service_subscribe(1, handle_accel);
    auto_backlight = true;

    worker_event_loop();

    
}
