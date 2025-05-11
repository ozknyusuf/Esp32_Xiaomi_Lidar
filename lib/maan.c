#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lvgl/src/core/lv_global.h"

#if LV_USE_WAYLAND
#include "backends/interface.h"
#endif

uint16_t window_width;
uint16_t window_height;
bool fullscreen;
bool maximize;

static void configure_simulator(int argc, char **argv);

static const char *getenv_default(const char *name, const char *dflt)
{
    return getenv(name) ? : dflt;
}

#if LV_USE_EVDEV
static void indev_deleted_cb(lv_event_t * e)
{
    if(LV_GLOBAL_DEFAULT()->deinit_in_progress) return;
    lv_obj_t * cursor_obj = lv_event_get_user_data(e);
    lv_obj_delete(cursor_obj);
}

static void discovery_cb(lv_indev_t * indev, lv_evdev_type_t type, void * user_data)
{
    LV_LOG_USER("new '%s' device discovered", type == LV_EVDEV_TYPE_REL ? "REL" :
                                              type == LV_EVDEV_TYPE_ABS ? "ABS" :
                                              type == LV_EVDEV_TYPE_KEY ? "KEY" :
                                              "unknown");

    lv_display_t * disp = user_data;
    lv_indev_set_display(indev, disp);

    if(type == LV_EVDEV_TYPE_REL) {
        /* Set the cursor icon */
        LV_IMAGE_DECLARE(mouse_cursor_icon);
        lv_obj_t * cursor_obj = lv_image_create(lv_display_get_screen_active(disp));
        lv_image_set_src(cursor_obj, &mouse_cursor_icon);
        lv_indev_set_cursor(indev, cursor_obj);

        /* delete the mouse cursor icon if the device is removed */
        lv_indev_add_event_cb(indev, indev_deleted_cb, LV_EVENT_DELETE, cursor_obj);
    }
}

static void lv_linux_init_input_pointer(lv_display_t *disp)
{
    /* Enables a pointer (touchscreen/mouse) input device
     * Use 'evtest' to find the correct input device. /dev/input/by-id/ is recommended if possible
     * Use /dev/input/by-id/my-mouse-or-touchscreen or /dev/input/eventX
     * 
     * If LV_LINUX_EVDEV_POINTER_DEVICE is not set, automatic evdev disovery will start
     */
    const char *input_device = getenv("LV_LINUX_EVDEV_POINTER_DEVICE");

    if (input_device == NULL) {
        LV_LOG_USER("the LV_LINUX_EVDEV_POINTER_DEVICE environment variable is not set. using evdev automatic discovery.");
        lv_evdev_discovery_start(discovery_cb, disp);
        return;
    }

    lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, input_device);
    lv_indev_set_display(touch, disp);

    /* Set the cursor icon */
    LV_IMAGE_DECLARE(mouse_cursor_icon);
    lv_obj_t * cursor_obj = lv_image_create(lv_display_get_screen_active(disp));
    lv_image_set_src(cursor_obj, &mouse_cursor_icon);
    lv_indev_set_cursor(touch, cursor_obj);
}
#endif

#if LV_USE_LINUX_FBDEV
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");
    lv_display_t * disp = lv_linux_fbdev_create();

#if LV_USE_EVDEV
    lv_linux_init_input_pointer(disp);
#endif

    lv_linux_fbdev_set_file(disp, device);
}
#elif LV_USE_LINUX_DRM
static void lv_linux_disp_init(void)
{
    const char *device = getenv_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    lv_display_t * disp = lv_linux_drm_create();

#if LV_USE_EVDEV
    lv_linux_init_input_pointer(disp);
#endif

    lv_linux_drm_set_file(disp, device, -1);
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{

    lv_sdl_window_create(window_width, window_height);

}
#elif LV_USE_WAYLAND
    /* see backend/wayland.c */
#else
#error Unsupported configuration
#endif

#if LV_USE_WAYLAND == 0
void lv_linux_run_loop(void)
{
    uint32_t idle_time;

    /*Handle LVGL tasks*/
    while(1) {

        idle_time = lv_timer_handler(); /*Returns the time to the next timer execution*/
        usleep(idle_time * 1000);
    }
}
#endif

/*
 * Process command line arguments and environment
 * variables to configure the simulator
 */
static void configure_simulator(int argc, char **argv)
{

    int opt = 0;
    bool err = false;

    /* Default values */
    fullscreen = maximize = false;
    window_width = atoi(getenv("LV_SIM_WINDOW_WIDTH") ? : "800");
    window_height = atoi(getenv("LV_SIM_WINDOW_HEIGHT") ? : "480");

    /* Parse the command-line options. */
    while ((opt = getopt (argc, argv, "fmw:h:")) != -1) {
        switch (opt) {
        case 'f':
            fullscreen = true;
            if (LV_USE_WAYLAND == 0) {
                fprintf(stderr, "The SDL driver doesn't support fullscreen mode on start\n");
                exit(1);
            }
            break;
        case 'm':
            maximize = true;
            if (LV_USE_WAYLAND == 0) {
                fprintf(stderr, "The SDL driver doesn't support maximized mode on start\n");
                exit(1);
            }
            break;
        case 'w':
            window_width = atoi(optarg);
            break;
        case 'h':
            window_height = atoi(optarg);
            break;
        case ':':
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            exit(1);
        case '?':
            fprintf (stderr, "Unknown option -%c.\n", optopt);
            exit(1);
        }
    }
}
static lv_obj_t * screen;

// Renk tanımlamaları
static lv_color_t color_orange;
static lv_color_t color_pink;
static lv_color_t color_teal;
static lv_color_t color_lime;

// Fonksiyon prototipleri
void create_temperature_section(int16_t x, int16_t y, const char* temp_text, const char* symbol);
void create_menu_item(int16_t x, int16_t y, const char* text, const char* symbol, lv_color_t bar_color, lv_color_t symbol_color);
bool is_color_black(lv_color_t color);

void create_printer_ui(void) {
    // Ana renkleri tanımla
    color_orange = lv_color_hex(0xFF5500);
    color_pink = lv_color_hex(0xFF00AA);
    color_teal = lv_color_hex(0x00AAAA);
    color_lime = lv_color_hex(0xAAFF00);
    
    // Arka plan rengi (koyu siyah)
    static lv_style_t style_screen;
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_hex(0x151515));
    
    // Ana ekran oluştur
    screen = lv_obj_create(NULL);
    lv_obj_add_style(screen, &style_screen, 0);
    lv_obj_set_size(screen, 800, 480);  // Ekran boyutunu 800x480 olarak güncelle
    
    // Sol taraftaki ısı bilgisi bölümü
    create_temperature_section(150, 120, "17°C / 0°C", LV_SYMBOL_MINUS);
    create_temperature_section(150, 320, "23°C / 0°C", LV_SYMBOL_SETTINGS);
    
    // Sağ taraftaki menü öğeleri - yatay düzende daha eşit dağıtıldı
    create_menu_item(400, 120, "Home", LV_SYMBOL_HOME, color_orange, color_pink);
    create_menu_item(400, 320, "Filament", LV_SYMBOL_SETTINGS, color_teal, lv_color_black());
    create_menu_item(650, 120, "Actions", LV_SYMBOL_LIST, color_pink, lv_color_black());
    create_menu_item(650, 320, "Configuration", LV_SYMBOL_SETTINGS, color_lime, lv_color_black());
    
    // Print butonu daha yukarıya alındı (420 -> 220)
    create_menu_item(525, 220, "Print", LV_SYMBOL_SETTINGS, color_pink, lv_color_black());
    
    lv_scr_load(screen);
}

void create_temperature_section(int16_t x, int16_t y, const char* temp_text, const char* symbol) {
    // Sembol için container
    lv_obj_t * cont = lv_obj_create(screen);
    lv_obj_set_size(cont, 100, 100);
    lv_obj_set_pos(cont, x - 50, y - 50);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    
    // Sembol
    lv_obj_t * icon = lv_label_create(cont);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_obj_center(icon);
    
    // Sıcaklık metni
    lv_obj_t * temp_label = lv_label_create(screen);
    lv_label_set_text(temp_label, temp_text);
    lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_22, 0);
    lv_obj_align_to(temp_label, cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

// İki rengin eşit olup olmadığını kontrol et
bool is_color_black(lv_color_t color) {
    lv_color_t black = lv_color_black();
    return lv_color_brightness(color) == lv_color_brightness(black);
}

void create_menu_item(int16_t x, int16_t y, const char* text, const char* symbol, lv_color_t bar_color, lv_color_t symbol_color) {
    // Menü öğesi için container
    lv_obj_t * cont = lv_obj_create(screen);
    lv_obj_set_size(cont, 110, 110);
    lv_obj_set_pos(cont, x - 55, y - 55);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    
    // Sembol
    lv_obj_t * icon = lv_label_create(cont);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_obj_center(icon);
    
    // Menü metni
    lv_obj_t * menu_label = lv_label_create(screen);
    lv_label_set_text(menu_label, text);
    lv_obj_set_style_text_color(menu_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(menu_label, &lv_font_montserrat_20, 0);
    lv_obj_align_to(menu_label, cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    
    // Renkli çubuk
    if (!is_color_black(bar_color)) {
        lv_obj_t * bar = lv_obj_create(screen);
        lv_obj_set_size(bar, 110, 6);  // Daha geniş çizgiler
        
        if (strcmp(text, "Home") == 0 || strcmp(text, "Actions") == 0) {
            // Home ve Actions için üstte çizgi
            lv_obj_align_to(bar, cont, LV_ALIGN_OUT_TOP_MID, 0, -15);
        } else if (strcmp(text, "Filament") == 0 || strcmp(text, "Configuration") == 0) {
            // Filament ve Configuration için altta çizgi
            lv_obj_align_to(bar, menu_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        } else if (strcmp(text, "Print") == 0) {
            // Print için altta daha uzun çizgi
            lv_obj_set_size(bar, 300, 6);
            lv_obj_align_to(bar, menu_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        }
        
        lv_obj_set_style_bg_color(bar, bar_color, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 0, 0);
    }
}


int main(int argc, char **argv)
{

    configure_simulator(argc, argv);

    /* Initialize LVGL. */
    lv_init();

    /* Initialize the configured backend SDL2, FBDEV, libDRM or wayland */
    lv_linux_disp_init();

    /*Create a Demo*/
    create_printer_ui();


    lv_linux_run_loop();

    return 0;
}