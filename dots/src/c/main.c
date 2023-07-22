#include <pebble.h>
#include "pebble-gbc-graphics/pebble-gbc-graphics.h"
#include "numbers.h"

#define NUMBER_OF_VRAM_BANKS_TO_GENERATE 1
#define NUMBER_OF_TILES 2
#define NUMBER_OF_PALETTES 1

static Window *s_window;
static GBC_Graphics *s_gbc_graphics;

#if defined(PBL_ROUND)
const int POSITIONS[][2] = {
    {5, 2},
    {12, 2},
    {5, 12},
    {12, 12},
    {7, 10},
    {14, 10},
};
#else
const int POSITIONS[][2] = {
    {3, 2},
    {10, 2},
    {3, 12},
    {10, 12},
    {5, 10},
    {12, 10},
};
#endif

/**
 * Loads a tilesheet from the resources into a VRAM bank
 */
static void load_tilesheet() {
    // Calculate how many tiles are on the tilesheet
    ResHandle handle = resource_get_handle(RESOURCE_ID_DATA_DOT_TILESHEET);
    size_t res_size = resource_size(handle);
    uint16_t tiles_to_load = res_size / GBC_TILE_NUM_BYTES;

    uint8_t tilesheet_start_offset = 0; // This is the tile on the tilesheet we want to start loading from
    uint8_t vram_start_offset = 0; // This is the tile in the VRAM we want to start loading into
    uint8_t vram_bank = 0; // The VRAM bank we want to store the tiles into
    GBC_Graphics_load_from_tilesheet_into_vram(s_gbc_graphics, RESOURCE_ID_DATA_DOT_TILESHEET, tilesheet_start_offset, 
                                                tiles_to_load, vram_start_offset, vram_bank);
}

/**
 * Sets palettes for the background in various gradients. You may find this link helpful: https://developer.rebble.io/developer.pebble.com/guides/tools-and-resources/color-picker/index.html
 */
static void create_palettes() {
#if defined(PBL_COLOR) // Pebbles with color screens use the ARGB8 Pebble color definitions for palettes
    GBC_Graphics_set_bg_palette(s_gbc_graphics, 0, GColorBlackARGB8, GColorWhiteARGB8, GColorWhiteARGB8, GColorWhiteARGB8);
#else // Pebbles with black and white screens use the GBC_COLOR definitions for palettes
    GBC_Graphics_set_bg_palette(s_gbc_graphics, 0, GBC_COLOR_BLACK, GBC_COLOR_WHITE, GBC_COLOR_WHITE, GBC_COLOR_WHITE);
#endif
}

static void draw_number(int number, int start_x, int start_y) {
    const uint8_t *number_pixels = NUMBERS[number];
    for (int x = 0; x < NUMBER_WIDTH; x++) {
        for (int y = 0; y < NUMBER_HEIGHT; y++) {
            GBC_Graphics_bg_set_tile(s_gbc_graphics, x + start_x, y + start_y, number_pixels[x + y * NUMBER_WIDTH]);
        }
    }
}

static void draw_number_from_char(char number, int start_x, int start_y) {
    draw_number(number - '0', start_x, start_y);
}

static void draw_dot(int x, int y) {
    GBC_Graphics_bg_set_tile(s_gbc_graphics, x, y, 1);
}

static void draw_blank(int x, int y) {
    GBC_Graphics_bg_set_tile(s_gbc_graphics, x, y, 0);
}

/**
 * Sets the background tiles in a test pattern to demonstrate palettes
 */
static void generate_background() {
    for (uint8_t y = 0; y < GBC_TILEMAP_HEIGHT; y++) {
        for (uint8_t x = 0; x < GBC_TILEMAP_WIDTH; x++) {
            GBC_Graphics_bg_set_tile(s_gbc_graphics, x, y, 0);
            GBC_Graphics_bg_set_tile_palette(s_gbc_graphics, x, y, 0);
        }
    }
#if defined(PBL_ROUND)
    GBC_Graphics_bg_move(s_gbc_graphics, -2, -6);
#endif
}

static void update_time() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Write the current hours and minutes into a buffer
    static char s_buffer[8];
    strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                            "%H%M" : "%I%M", tick_time);

    draw_number_from_char(s_buffer[0], POSITIONS[0][0], POSITIONS[0][1]);
    draw_number_from_char(s_buffer[1], POSITIONS[1][0], POSITIONS[1][1]);
    draw_number_from_char(s_buffer[2], POSITIONS[2][0], POSITIONS[2][1]);
    draw_number_from_char(s_buffer[3], POSITIONS[3][0], POSITIONS[3][1]);
    draw_dot(POSITIONS[4][0], POSITIONS[4][1]);
    draw_dot(POSITIONS[5][0], POSITIONS[5][1]);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

/**
 * Hides the window layer from view by pushing it off of the screen
 */
static void hide_window_layer() {
    GBC_Graphics_window_set_offset_y(s_gbc_graphics, GBC_Graphics_get_screen_height(s_gbc_graphics));
}

/**
 * Execute all of the graphics functions
 */
static void window_load(Window *window) {
    // Create the GBC_Graphics object
    s_gbc_graphics = GBC_Graphics_ctor(s_window, NUMBER_OF_VRAM_BANKS_TO_GENERATE);

    load_tilesheet();
    create_palettes();
    generate_background();
    hide_window_layer();
    update_time();

    // Display the graphics
    GBC_Graphics_render(s_gbc_graphics);
}

static void window_unload(Window *window) {
    // Destroy the GBC_Graphics object
    GBC_Graphics_destroy(s_gbc_graphics);
}

static void init(void) {
    s_window = window_create();
    
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    const bool animated = true;
    window_stack_push(s_window, animated);
}

static void deinit(void) {
    window_destroy(s_window);
}

int main(void) {
    init();

    app_event_loop();
    deinit();
}
