#include <pebble.h>

#define START_HOUR  0
#define START_MINUTE    1
#define STOP_HOUR   2
#define STOP_MINUTE 3
#define START_ALARM 4
#define STOP_ALARM  5

#define SECONDS (1)
#define MINUTES (60 * SECONDS)
#define HOURS (60 * MINUTES)
#define DAYS (24 * HOURS)

#define FONT_HEIGHT 30
#define FONT_WIDTH 30
#define TIME_SELECT_HOURS 1
#define TIME_SELECT_MINUTES 2
#define TIME_START 0
#define TIME_STOP 1
#define num_top_menu_sections 1

static Window *window;

SimpleMenuLayer *top_menu_layer;

void top_menu_callback(int index, void *context);
static SimpleMenuItem top_menu_items[] = {
    {"Light off", "Toggle raise for light", NULL, (SimpleMenuLayerSelectCallback)top_menu_callback},
    {"Set Start", NULL, NULL, (SimpleMenuLayerSelectCallback)top_menu_callback},
    {"Set Stop", NULL, NULL, (SimpleMenuLayerSelectCallback)top_menu_callback},
};
#define num_top_menu_items (sizeof(top_menu_items) / sizeof(*top_menu_items))

const SimpleMenuSection top_menu_sections = {
    .title="Main menu",
    .items=top_menu_items,
    .num_items=num_top_menu_items
};

Window *time_window=NULL;
TextLayer *time_layer=NULL;

char time_select_text[30];
int time_select_pointer;
int time_select_hours;
int time_select_minutes;
int time_setting;
char *which[2] = {"start", "stop"};
int start_hour;
int start_min;
int stop_hour;
int stop_min;
WakeupId start_alarm_id;
WakeupId stop_alarm_id;

void schedule_wakeup(WakeupId *alarm_id, int hour, int min, int which, int which_mem) {
    time_t now;
    struct tm *now_tick;
    time_t alarm_time;
    int32_t time_inc;

    now = time(0L);
    now_tick = localtime(&now);

    wakeup_cancel(*alarm_id);
    time_inc = ((hour - now_tick->tm_hour) * HOURS) +
               ((min - now_tick->tm_hour) * MINUTES) -
               now_tick->tm_sec;
    alarm_time = now + time_inc;
    *alarm_id = wakeup_schedule(alarm_time, which, false);
    persist_write_int(which_mem, (uint32_t)(*alarm_id));
}

void save_and_initiate_timer(int which) {
    if (which == TIME_START) {
    	persist_write_int(START_HOUR, (uint32_t)start_hour);
    	persist_write_int(START_MINUTE, (uint32_t)start_min);
    	schedule_wakeup(&start_alarm_id, start_hour, start_min, TIME_START, START_ALARM);
    } else {
    	persist_write_int(STOP_HOUR, (uint32_t)stop_hour);
    	persist_write_int(STOP_MINUTE, (uint32_t)stop_min);
    	schedule_wakeup(&stop_alarm_id, stop_hour, stop_min, TIME_STOP, STOP_ALARM);
    }
}

void format_time(void) {
    snprintf(time_select_text, sizeof(time_select_text),
             "Setting\n%s\n%02u:%02u",
             which[time_setting],
             time_select_hours,
             time_select_minutes);
}

static void select_time_handler(ClickRecognizerRef recognizer, void *context) {
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "Time Click select");

    if (time_select_pointer == TIME_SELECT_HOURS) {
    	time_select_pointer = TIME_SELECT_MINUTES;
    } else {
        app_log(APP_LOG_LEVEL_WARNING,
    	        __FILE__,
    	        __LINE__,
    	        "Time selected is '%s'", time_select_text);
    	if (time_setting == TIME_START) {
    	    start_hour = time_select_hours;
    	    start_min = time_select_minutes;
    	    save_and_initiate_timer(TIME_START);
    	} else {
    	    stop_hour = time_select_hours;
    	    stop_min = time_select_minutes;
    	    save_and_initiate_timer(TIME_START);
    	}

    	window_stack_pop(true);
    }
}

static void up_time_handler(ClickRecognizerRef recognizer, void *context) {
    if (time_select_pointer == TIME_SELECT_HOURS) {
    	time_select_hours++;
    	if (time_select_hours > 23) {
    	    time_select_hours = 0;
    	} else if (time_select_hours < 0) {
    	    time_select_hours = 23;
        }
    } else {
    	time_select_minutes++;
    	if (time_select_minutes > 59) {
    	    time_select_minutes = 0;
    	} else if (time_select_minutes < 0) {
    	    time_select_minutes = 59;
        }
    }
    format_time();
    text_layer_set_text(time_layer, time_select_text);
}

static void down_time_handler(ClickRecognizerRef recognizer, void *context) {
    if (time_select_pointer == TIME_SELECT_HOURS) {
    	time_select_hours--;
    	if (time_select_hours > 23) {
    	    time_select_hours = 0;
    	} else if (time_select_hours < 0) {
    	    time_select_hours = 23;
        }
    } else {
    	time_select_minutes--;
    	if (time_select_minutes > 59) {
    	    time_select_minutes = 0;
        } else if (time_select_minutes < 0) {
    	    time_select_minutes = 59;
        }
    }

    format_time();
    text_layer_set_text(time_layer, time_select_text);
}

static void time_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_time_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_time_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_time_handler);

    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_time_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_time_handler);
}

void set_time(int which) {

    if (!time_window) {
    	time_window = window_create();
    }

    if (!time_layer) {
    	time_layer = text_layer_create(GRect(0, 0, 144, 168));
    }

    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(time_window), (Layer *)time_layer);

    time_setting = which;
    time_select_pointer = TIME_SELECT_HOURS;
    time_select_hours = (which == TIME_START) ? start_hour : stop_hour;
    time_select_minutes = (which == TIME_START) ? start_min : stop_min;
    format_time();
    text_layer_set_text(time_layer, time_select_text);
    window_set_click_config_provider(time_window, time_config_provider);

    window_stack_push(time_window, true);
}

void top_menu_callback(int index, void *context) {
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "Main Menu callback on row %d\n", index);

    switch (index) {
        case 0: // toggle auto-backlight
        	if (app_worker_is_running()) {
                app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Lights off");
        	    top_menu_items[index].title = "Light off";
                layer_mark_dirty(simple_menu_layer_get_layer(top_menu_layer));
        	    app_worker_kill();
        	} else {
                app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Lights on");
        	    top_menu_items[index].title = "Light on";
                layer_mark_dirty(simple_menu_layer_get_layer(top_menu_layer));
        	    app_worker_launch();
        	}
        	break;
        case 1: // Set Start Time
        	set_time(TIME_START);
        	return;
        case 2: // Set Start Time
        	set_time(TIME_STOP);
        	return;
    }
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    if (app_worker_is_running()) {
        top_menu_items[0].title = "Light on";
    } else {
        top_menu_items[0].title = "Light off";
    }

    top_menu_layer = simple_menu_layer_create(layer_get_frame(window_layer), window, &top_menu_sections, num_top_menu_sections, NULL);
    #ifdef PBL_COLOR
        menu_layer_set_normal_colors(simple_menu_layer_get_menu_layer(top_menu_layer), GColorBlack, GColorWhite);
        menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(top_menu_layer), GColorGreen, GColorBlack);
    #endif
    layer_add_child(window_layer, simple_menu_layer_get_layer(top_menu_layer));
}

static void window_unload(Window *window) {
    simple_menu_layer_destroy(top_menu_layer);
}

void read_alarm_data(void) {
    uint32_t val;

    val = persist_read_int(START_ALARM);
    if (val) {
    	start_alarm_id = (WakeupId)val;
    	APP_LOG(APP_LOG_LEVEL_DEBUG, "start_alarm_id=%u", (uint)val);
    }

    val = persist_read_int(STOP_ALARM);
    if (val) {
    	stop_alarm_id = (WakeupId)val;
    	APP_LOG(APP_LOG_LEVEL_DEBUG, "stop_alarm_id=%u", (uint)val);
    }

    val = persist_read_int(START_HOUR);
    if (val) {
    	start_hour = (int)val;
    	APP_LOG(APP_LOG_LEVEL_DEBUG, "start_hour=%u", (uint)val);
    }
    val = persist_read_int(START_MINUTE);
    if (val) {
    	start_min = (int)val;
    	APP_LOG(APP_LOG_LEVEL_DEBUG, "start_min=%u", (uint)val);
    }

    val = persist_read_int(STOP_HOUR);
    if (val) {
    	stop_hour = (int)val;
    	APP_LOG(APP_LOG_LEVEL_DEBUG, "stop_hour=%u", (uint)val);
    }

    val = persist_read_int(STOP_MINUTE);
    if (val) {
    	stop_min = (int)val;
    	APP_LOG(APP_LOG_LEVEL_DEBUG, "stop_min=%u", (uint)val);
    }
}

void handle_wakeup(WakeupId id, int32_t cookie) {
    if (cookie == TIME_START) {
    	app_worker_launch();
    	wakeup_cancel(start_alarm_id);
    	schedule_wakeup(&start_alarm_id, start_hour, start_min, TIME_START, START_ALARM);
    } else {
    	app_worker_kill();
    	wakeup_cancel(stop_alarm_id);
    	schedule_wakeup(&stop_alarm_id, stop_hour, stop_min, TIME_STOP, STOP_ALARM);
    }
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
    	.load = window_load,
    	.unload = window_unload,
    });
    #ifdef PBL_COLOR
        window_set_background_color(window, GColorBlack);
    #endif
    window_stack_push(window, true);
}

static void deinit(void) {
    window_destroy(window);
}

int main(void) {
    AppLaunchReason reason;
    WakeupId id = 0;
    int32_t cookie;
    reason = launch_reason();

    if (reason == APP_LAUNCH_WAKEUP) {
    	wakeup_get_launch_event(&id, &cookie);
    	read_alarm_data();
    	handle_wakeup(id, cookie);
    } else {
    	init();
    	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
    	app_event_loop();
    	deinit();
    }
}
