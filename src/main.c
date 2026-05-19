#include <pebble.h>

/*
 * Code Watch
 * Displays time as Python syntax with VS Code colour scheme
 *   Key 0: KEY_MODE  0=Dark  1=Light
 */

#define KEY_MODE  0
#define MODE_DARK  0
#define MODE_LIGHT 1

static int s_mode = MODE_DARK;

static Window *s_window;
static Layer  *s_canvas;
static GFont   s_font;

static int  s_hour, s_min;
static bool s_is_am;
static char s_day_buf[12];
static char s_date_buf[12];

/* ── VS Code colour palette ── */
/* Dark mode */
#define D_BG    GColorBlack
#define D_KW    GColorMagenta        /* if/else/return/def — purple */
#define D_FN    GColorYellow         /* function name */
#define D_VAR   GColorPictonBlue     /* variables */
#define D_STR   GColorOrange         /* strings */
#define D_NUM   GColorGreen          /* numbers */
#define D_OP    GColorLightGray      /* operators/punct */
#define D_CMT   GColorMediumSpringGreen /* comments */
#define D_LNO   GColorDarkGray       /* line numbers */

/* Light mode */
#define L_BG    GColorWhite
#define L_KW    GColorPurple
#define L_FN    GColorChromeYellow
#define L_VAR   GColorBlue
#define L_STR   GColorRed
#define L_NUM   GColorGreen
#define L_OP    GColorDarkGray
#define L_CMT   GColorIslamicGreen
#define L_LNO   GColorLightGray

/* ── helpers ── */
static GColor col(GColor dark, GColor light) {
  return s_mode == MODE_DARK ? dark : light;
}

static int s_x, s_y;
static int s_lno;

static void draw_lno(GContext *ctx) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%d", s_lno++);
  graphics_context_set_text_color(ctx, col(D_LNO, L_LNO));
  graphics_draw_text(ctx, buf, s_font,
    GRect(0, s_y, 16, 14),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
  s_x = 20;
}

static void tok(GContext *ctx, const char *text, GColor dark, GColor light) {
  graphics_context_set_text_color(ctx, col(dark, light));
  graphics_draw_text(ctx, text, s_font,
    GRect(s_x, s_y, 144 - s_x, 14),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  /* advance x by text width */
  GSize sz = graphics_text_layout_get_content_size(
    text, s_font, GRect(0,0,144,14),
    GTextOverflowModeWordWrap, GTextAlignmentLeft);
  s_x += sz.w;
}

static void nl(GContext *ctx) {
  s_y += 13;
  draw_lno(ctx);
}

/* ── canvas draw ── */
static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  /* background */
  graphics_context_set_fill_color(ctx, col(D_BG, L_BG));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* gutter background in dark mode */
  if (s_mode == MODE_DARK) {
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(0, 0, 18, 168), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(18, 0, 126, 168), 0, GCornerNone);
  }

  /* build hour/min strings */
  char h_str[4], m_str[4], period[5];
  snprintf(h_str, sizeof(h_str), "%d", s_hour % 12 == 0 ? 12 : s_hour % 12);
  snprintf(m_str, sizeof(m_str), "%02d", s_min);
  snprintf(period, sizeof(period), s_is_am ? "\"AM\"" : "\"PM\"");
  char period_inv[5];
  snprintf(period_inv, sizeof(period_inv), s_is_am ? "\"PM\"" : "\"AM\"");

  s_y = 4; s_lno = 1;
  draw_lno(ctx);

  /* # Sunday */
  tok(ctx, "# ", D_CMT, L_CMT);
  tok(ctx, s_day_buf, D_CMT, L_CMT);
  nl(ctx);

  /* def get_time(): */
  tok(ctx, "def ", D_KW, L_KW);
  tok(ctx, "get_time", D_FN, L_FN);
  tok(ctx, "():", D_OP, L_OP);
  nl(ctx);

  /* if hour < 12: */
  tok(ctx, "  if ", D_KW, L_KW);
  tok(ctx, "hour", D_VAR, L_VAR);
  tok(ctx, " < ", D_OP, L_OP);
  tok(ctx, "12", D_NUM, L_NUM);
  tok(ctx, ":", D_OP, L_OP);
  nl(ctx);

  /* period = "AM" */
  tok(ctx, "    period", D_VAR, L_VAR);
  tok(ctx, " = ", D_OP, L_OP);
  tok(ctx, period, D_STR, L_STR);
  nl(ctx);

  /* else: */
  tok(ctx, "  else", D_KW, L_KW);
  tok(ctx, ":", D_OP, L_OP);
  nl(ctx);

  /* period = "PM" */
  tok(ctx, "    period", D_VAR, L_VAR);
  tok(ctx, " = ", D_OP, L_OP);
  tok(ctx, period_inv, D_STR, L_STR);
  nl(ctx);

  /* blank */
  nl(ctx);

  /* hour = 10 */
  tok(ctx, "  hour", D_VAR, L_VAR);
  tok(ctx, " = ", D_OP, L_OP);
  tok(ctx, h_str, D_NUM, L_NUM);
  nl(ctx);

  /* mins = 33 */
  tok(ctx, "  mins", D_VAR, L_VAR);
  tok(ctx, " = ", D_OP, L_OP);
  tok(ctx, m_str, D_NUM, L_NUM);
  nl(ctx);

  /* blank */
  nl(ctx);

  /* return { */
  tok(ctx, "  return ", D_KW, L_KW);
  tok(ctx, "{", D_OP, L_OP);
  nl(ctx);

  /* "h": 10, */
  tok(ctx, "    \"h\"", D_STR, L_STR);
  tok(ctx, ": ", D_OP, L_OP);
  tok(ctx, h_str, D_NUM, L_NUM);
  tok(ctx, ",", D_OP, L_OP);
  nl(ctx);

  /* "m": 33, */
  tok(ctx, "    \"m\"", D_STR, L_STR);
  tok(ctx, ": ", D_OP, L_OP);
  tok(ctx, m_str, D_NUM, L_NUM);
  tok(ctx, ",", D_OP, L_OP);
  nl(ctx);

  /* "p": "AM" */
  tok(ctx, "    \"p\"", D_STR, L_STR);
  tok(ctx, ": ", D_OP, L_OP);
  tok(ctx, period, D_STR, L_STR);
  nl(ctx);

  /* } */
  tok(ctx, "  }", D_OP, L_OP);

  /* date bottom right */
  graphics_context_set_text_color(ctx, col(D_LNO, L_LNO));
  graphics_draw_text(ctx, s_date_buf, s_font,
    GRect(0, 154, 140, 14),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
}

/* ── update ── */
static void update_time(struct tm *t) {
  s_hour  = t->tm_hour;
  s_min   = t->tm_min;
  s_is_am = s_hour < 12;
  strftime(s_day_buf,  sizeof(s_day_buf),  "%A", t);
  strftime(s_date_buf, sizeof(s_date_buf), "%a %d", t);
  layer_mark_dirty(s_canvas);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

/* ── AppMessage ── */
static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *t = dict_find(iter, KEY_MODE);
  if (t) {
    s_mode = (int)t->value->int32;
    persist_write_int(KEY_MODE, s_mode);
    layer_mark_dirty(s_canvas);
  }
}

/* ── window ── */
static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);

  s_font = fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_FONT_CODE_10));

  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, canvas_draw);
  layer_add_child(root, s_canvas);

  time_t now = time(NULL);
  update_time(localtime(&now));
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  fonts_unload_custom_font(s_font);
}

static void init(void) {
  s_mode = persist_exists(KEY_MODE) ? persist_read_int(KEY_MODE) : MODE_DARK;

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  app_message_open(64, 64);
  app_message_register_inbox_received(inbox_received);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
