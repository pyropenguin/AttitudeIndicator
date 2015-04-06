#include <pebble.h>

Window *window;
Layer *bgLayer;
Layer *horizonLayer;
static TextLayer *timeLayer;

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

#define NUM_TICK_LINES 9

static GPath *skyPath = NULL;
static GPath *earthPath = NULL;
static GPath *tickLinesPath[NUM_TICK_LINES] = {NULL};

static GBitmap *bezelBitmap;
static BitmapLayer *bezelBitmapLayer;

int rotateAngle = 0;
static char s_time_buffer[] = "00:00 MAR 00";

static GPathInfo skyPathInfo = {
  .num_points = 4,
  .points = (GPoint []) {{-SCREEN_WIDTH, -SCREEN_HEIGHT},
                         {SCREEN_WIDTH, -SCREEN_HEIGHT},
                         {SCREEN_WIDTH, 0},
                         {-SCREEN_WIDTH, 0}}
};
static GPathInfo earthPathInfo = {
  .num_points = 4,
  .points = (GPoint []) {{-SCREEN_WIDTH, 0},
                         {SCREEN_WIDTH, 0},
                         {SCREEN_WIDTH, SCREEN_HEIGHT},
                         {-SCREEN_WIDTH, SCREEN_HEIGHT}}
};

// Using gpath_move_to in horizonUpdateProc, we define 0,0 to be at the center
// of the display. The coordinate frame directionality remains the same, so a
// positive x (or width) is right, a positive y (or height) is down.
static GPathInfo tickLinesInfo[NUM_TICK_LINES] = {
  {.num_points = 4, .points = (GPoint []) {{-30, 36}, {30, 36}, {30, 33}, {-30, 33}}}, // 20 deg down
  {.num_points = 4, .points = (GPoint []) {{ -6, 27}, { 6, 27}, { 6, 24}, { -6, 24}}}, // 15 deg down
  {.num_points = 4, .points = (GPoint []) {{-18, 18}, {18, 18}, {18, 15}, {-18, 15}}}, // 10 deg down
  {.num_points = 4, .points = (GPoint []) {{ -6,  9}, { 6,  9}, { 6,  6}, { -6,  6}}}, // 5 deg down
  {.num_points = 4, .points = (GPoint []) {{-SCREEN_WIDTH,-2}, {SCREEN_WIDTH,-2},
                                           {SCREEN_WIDTH,2}, {-SCREEN_WIDTH,2}}},         // Horizon
  {.num_points = 4, .points = (GPoint []) {{ -6, -9}, { 6, -9}, { 6,  -6}, { -6,  -6}}},  // 5 deg up
  {.num_points = 4, .points = (GPoint []) {{-18,-18}, {18,-18}, {18, -15}, {-18, -15}}},  // 10 deg up
  {.num_points = 4, .points = (GPoint []) {{ -6,-27}, { 6,-27}, { 6, -24}, { -6, -24}}},  // 15 deg up
  {.num_points = 4, .points = (GPoint []) {{-30,-36}, {30,-36}, {30, -33}, {-30, -33}}},  // 20 deg up
};

void initHorizon(void)
{
  skyPath   = gpath_create(&skyPathInfo);
  earthPath = gpath_create(&earthPathInfo);
  
  for (int i = 0; i < NUM_TICK_LINES; i++)
  {
    tickLinesPath[i] = gpath_create(&tickLinesInfo[i]);
  }
}

void horizonUpdateProc(Layer *horizonLayer, GContext* ctx) {
  int i;
  
  // Fill the path
  gpath_move_to(skyPath, GPoint(SCREEN_WIDTH/2, SCREEN_HEIGHT/2));
  gpath_move_to(earthPath, GPoint(SCREEN_WIDTH/2, SCREEN_HEIGHT/2));
  gpath_rotate_to(skyPath, (int)rotateAngle);
  gpath_rotate_to(earthPath, (int)rotateAngle);
  
  for (i = 0; i < NUM_TICK_LINES; i++)
  {
    gpath_move_to(tickLinesPath[i], GPoint(SCREEN_WIDTH/2, SCREEN_HEIGHT/2));
    gpath_rotate_to(tickLinesPath[i], (int)rotateAngle);
  }
  
  graphics_context_set_fill_color(ctx, GColorVividCerulean);
  gpath_draw_filled(ctx, skyPath);
  graphics_context_set_fill_color(ctx, GColorWindsorTan);
  gpath_draw_filled(ctx, earthPath);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  for (i = 0; i < NUM_TICK_LINES; i++)
  {
    gpath_draw_filled(ctx, tickLinesPath[i]);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {

  // Set text layers for the next event
  if (clock_is_24h_style()) {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%H:%M %h %d", tick_time);
  } else {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%I:%M %h %d", tick_time);
  }
  text_layer_set_text(timeLayer, s_time_buffer);
  
  rotateAngle = (tick_time->tm_sec % 60) * (TRIG_MAX_ANGLE / 60);
  
  layer_mark_dirty(horizonLayer);
}

void handle_init(void) {
  static GRect bgLayerRect = {{0,0},{SCREEN_WIDTH,SCREEN_HEIGHT}};
  window = window_create();
  
  bgLayer = window_get_root_layer(window);
  layer_set_frame(bgLayer, bgLayerRect);
  
  bezelBitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BEZEL);
  bezelBitmapLayer = bitmap_layer_create(layer_get_frame(bgLayer));
  bitmap_layer_set_bitmap(bezelBitmapLayer, bezelBitmap);
  bitmap_layer_set_compositing_mode(bezelBitmapLayer, GCompOpSet);
  
  horizonLayer = layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
  initHorizon();
  
  timeLayer = text_layer_create(GRect(14,132,93,23));
  text_layer_set_text_color(timeLayer, GColorWhite);
  text_layer_set_text_alignment(timeLayer,GTextAlignmentCenter);
  text_layer_set_background_color(timeLayer, GColorBlack);
  text_layer_set_font(timeLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(timeLayer, s_time_buffer);
  
  layer_set_update_proc(horizonLayer, horizonUpdateProc);
  
  layer_add_child(bgLayer, horizonLayer);
  layer_add_child(bgLayer, bitmap_layer_get_layer(bezelBitmapLayer));
  layer_add_child(bgLayer, text_layer_get_layer(timeLayer));

  window_stack_push(window, true);
  
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  text_layer_set_text(timeLayer, s_time_buffer);
}

void handle_deinit(void) {
  gpath_destroy(skyPath);
  gpath_destroy(earthPath);
  for(int i = 0; i < NUM_TICK_LINES; i++)
  {
    gpath_destroy(tickLinesPath[i]);
  }
  layer_destroy(horizonLayer);
  bitmap_layer_destroy(bezelBitmapLayer);
  text_layer_destroy(timeLayer);
  gbitmap_destroy(bezelBitmap);
  window_destroy(window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
