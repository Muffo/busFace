#include <pebble.h>
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_bus_layer[3];

static GFont s_time_font;
static GFont s_bus_font;

static char s_time_buffer[8];
static char s_bus_buffer[16];


static void main_window_load(Window *window) {
    // Get information about the Window
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
  
    // ==================== Time ====================
    s_time_layer = text_layer_create(
        GRect(0, 5, bounds.size.w, 50));
  
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BRACIOLA_B_48));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorGreen);
    text_layer_set_font(s_time_layer, s_time_font);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

    // Add it as a child layer to the Window's root layer
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));


    // ==================== Bus ====================
    s_bus_layer[0] = text_layer_create(
        GRect(0, 75, bounds.size.w, 25));

    s_bus_layer[1] = text_layer_create(
        GRect(0, 100, bounds.size.w, 25));

    s_bus_layer[2] = text_layer_create(
        GRect(0, 125, bounds.size.w, 25));
  
    int i;
    s_bus_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BRACIOLA_R_24));
    for (i = 0; i < 3; ++i) {
        text_layer_set_background_color(s_bus_layer[i], GColorClear);
        text_layer_set_text_color(s_bus_layer[i], GColorWhite);
        text_layer_set_font(s_bus_layer[i], s_bus_font);
        text_layer_set_text_alignment(s_bus_layer[i], GTextAlignmentLeft);

        // Add it as a child layer to the Window's root layer
        layer_add_child(window_layer, text_layer_get_layer(s_bus_layer[i]));
    }

    text_layer_set_text(s_bus_layer[0], "Next bus in");
    text_layer_set_text(s_bus_layer[2], "(scheduled)");

    text_layer_set_text_color(s_bus_layer[2], GColorRed);
    layer_set_hidden(text_layer_get_layer(s_bus_layer[2]), true);

}

static void main_window_unload(Window *window) { 
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
    fonts_unload_custom_font(s_time_font);

    int i;
    for (i = 0; i < 3; ++i) {
        text_layer_destroy(s_bus_layer[i]);
    }
    fonts_unload_custom_font(s_bus_font);
}

static void update_time() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
  
    // Write the current hours and minutes into a buffer
    strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ?
                                            "%H:%M" : "%I:%M", tick_time);
    text_layer_set_text(s_time_layer, s_time_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    // Update the time as soon as we enter in the next minute
    if (tick_time->tm_sec == 0) {
        update_time();  
    }

    // Get bus update every 20 seconds
    if (tick_time->tm_sec % 20 == 0) {
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        dict_write_uint8(iter, 0, 0);
        app_message_outbox_send();   
    }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

    APP_LOG(APP_LOG_LEVEL_DEBUG, "[%s] Received stuff", __func__);

    // Read tuples for data
    Tuple *due_in_tuple = dict_find(iterator, MESSAGE_KEY_DUEIN);
    Tuple *scheduled_tuple = dict_find(iterator, MESSAGE_KEY_SCHEDULED);

    if (due_in_tuple && scheduled_tuple ) {
        snprintf(s_bus_buffer, sizeof(s_bus_buffer), "%s", due_in_tuple->value->cstring);
        text_layer_set_text(s_bus_layer[1], s_bus_buffer);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "[%s] Updating time: %s", __func__, s_bus_buffer);
        layer_set_hidden(text_layer_get_layer(s_bus_layer[2]), !scheduled_tuple->value->int8);
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
    // Create main Window element and assign to pointer
    s_main_window = window_create();
  
    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    
    window_set_background_color(s_main_window, GColorBlack);

    // Show the Window on the watch, with animated=true
    window_stack_push(s_main_window, true);

    // Make sure the time is displayed from the start
    update_time();

    // Register with TickTimerService
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);


    // Register app message callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    // Open AppMessage
    const int inbox_size = 128;
    const int outbox_size = 128;
    app_message_open(inbox_size, outbox_size);
}

static void deinit() {
    // Destroy Window
    window_destroy(s_main_window);
}

int main(void) {
    init();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_main_window);
    app_event_loop();
    deinit();
}
