#include <pebble.h>

/*
 * Code Watch — VS Code theme
 * Uses system fonts: GOTHIC_14 for code, GOTHIC_18_BOLD for result
 *   Key 0: KEY_MODE  0=Dark  1=Light
 */

#define KEY_MODE   0
#define MODE_DARK  0
#define MODE_LIGHT 1

static int s_mode = MODE_DARK;
static Window *s_window;
static Layer  *s_canvas;

static int  s_hour, s_min;
static bool s_is_am;
static char s_day_buf[12];
static char s_result_buf[24];

/* ── colours ── */
static GColor ck(void){ return s_mode==MODE_DARK?GColorMagenta        :GColorPurple;       }
static GColor cf(void){ return s_mode==MODE_DARK?GColorYellow          :GColorChromeYellow; }
static GColor cv(void){ return s_mode==MODE_DARK?GColorPictonBlue      :GColorBlue;         }
static GColor cs(void){ return s_mode==MODE_DARK?GColorOrange          :GColorRed;          }
static GColor cn(void){ return s_mode==MODE_DARK?GColorGreen           :GColorIslamicGreen; }
static GColor co(void){ return s_mode==MODE_DARK?GColorLightGray       :GColorDarkGray;     }
static GColor cc(void){ return s_mode==MODE_DARK?GColorMediumSpringGreen:GColorIslamicGreen;}
static GColor cl(void){ return s_mode==MODE_DARK?GColorDarkGray        :GColorLightGray;    }

/* ── draw state ── */
static int s_cx, s_cy, s_lno;
#define LH     13
#define GUTTER 20
#define LM     23

static GFont s_font_code;
static GFont s_font_result;

static void draw_lno(GContext *ctx) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%d", s_lno++);
  graphics_context_set_text_color(ctx, cl());
  graphics_draw_text(ctx, buf, s_font_code,
    GRect(1, s_cy, GUTTER-3, LH),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
  s_cx = LM;
}

static void tok(GContext *ctx, const char *text, GColor col) {
  if (!text || !text[0]) return;
  graphics_context_set_text_color(ctx, col);
  graphics_draw_text(ctx, text, s_font_code,
    GRect(s_cx, s_cy, 144-s_cx, LH),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  GSize sz = graphics_text_layout_get_content_size(
    text, s_font_code, GRect(0,0,200,LH),
    GTextOverflowModeWordWrap, GTextAlignmentLeft);
  s_cx += sz.w;
}

static void nl(GContext *ctx) {
  s_cy += LH;
  draw_lno(ctx);
}

/* ── canvas ── */
static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  /* background */
  graphics_context_set_fill_color(ctx,
    s_mode==MODE_DARK ? GColorBlack : GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* gutter */
  graphics_context_set_fill_color(ctx,
    s_mode==MODE_DARK ? GColorOxfordBlue : GColorLightGray);
  graphics_fill_rect(ctx, GRect(0,0,GUTTER,168), 0, GCornerNone);

  /* values */
  char h_str[4], m_str[4];
  int h12 = s_hour % 12; if (!h12) h12=12;
  snprintf(h_str, sizeof(h_str), "%d",  h12);
  snprintf(m_str, sizeof(m_str), "%02d", s_min);
  char *per  = s_is_am ? "\"AM\"" : "\"PM\"";
  char *peri = s_is_am ? "\"PM\"" : "\"AM\"";

  s_cy=2; s_lno=1;
  draw_lno(ctx);

  /* 1: # Monday */
  tok(ctx,"# ",cc()); tok(ctx,s_day_buf,cc());
  nl(ctx);

  /* 2: def get_time(): */
  tok(ctx,"def ",ck()); tok(ctx,"get_time",cf()); tok(ctx,"():",co());
  nl(ctx);

  /* 3:   if hour < 12: */
  tok(ctx,"  if ",ck()); tok(ctx,"hour",cv());
  tok(ctx,"<",co()); tok(ctx,"12",cn()); tok(ctx,":",co());
  nl(ctx);

  /* 4:     period = "AM" */
  tok(ctx,"   period",cv()); tok(ctx,"=",co()); tok(ctx,per,cs());
  nl(ctx);

  /* 5:   else: */
  tok(ctx,"  else",ck()); tok(ctx,":",co());
  nl(ctx);

  /* 6:     period = "PM" */
  tok(ctx,"   period",cv()); tok(ctx,"=",co()); tok(ctx,peri,cs());
  nl(ctx);

  /* 7:   hour = 10 */
  tok(ctx,"  hour",cv()); tok(ctx,"=",co()); tok(ctx,h_str,cn());
  nl(ctx);

  /* 8:   mins = 33 */
  tok(ctx,"  mins",cv()); tok(ctx,"=",co()); tok(ctx,m_str,cn());
  nl(ctx);

  /* 9:   return { */
  tok(ctx,"  return ",ck()); tok(ctx,"{",co());
  nl(ctx);

  /* 10:    "h":10, "m":33 */
  tok(ctx,"   \"h\"",cs()); tok(ctx,":",co()); tok(ctx,h_str,cn());
  tok(ctx," \"m\"",cs()); tok(ctx,":",co()); tok(ctx,m_str,cn());
  nl(ctx);

  /* 11:    "p":"AM" } */
  tok(ctx,"   \"p\"",cs()); tok(ctx,":",co());
  tok(ctx,per,cs()); tok(ctx," }",co());

  /* ── result bar ── */
  int result_y = s_cy + LH + 2;

  graphics_context_set_fill_color(ctx,
    s_mode==MODE_DARK ? GColorOxfordBlue : GColorLightGray);
  graphics_fill_rect(ctx, GRect(0, result_y, 144, 22), 0, GCornerNone);

  /* >> prompt */
  graphics_context_set_text_color(ctx, cn());
  graphics_draw_text(ctx, ">>", s_font_code,
    GRect(3, result_y+3, 18, 16),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  /* result value */
  graphics_context_set_text_color(ctx,
    s_mode==MODE_DARK ? GColorWhite : GColorBlack);
  graphics_draw_text(ctx, s_result_buf, s_font_result,
    GRect(22, result_y+2, 120, 18),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void update_time(struct tm *t) {
  s_hour  = t->tm_hour;
  s_min   = t->tm_min;
  s_is_am = s_hour < 12;
  strftime(s_day_buf, sizeof(s_day_buf), "%A", t);

  char time_part[8], date_part[10];
  strftime(time_part, sizeof(time_part),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  strftime(date_part, sizeof(date_part), "%a %d", t);
  snprintf(s_result_buf, sizeof(s_result_buf), "%s %s %s",
           time_part, s_is_am?"AM":"PM", date_part);

  layer_mark_dirty(s_canvas);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *t = dict_find(iter, KEY_MODE);
  if (t) {
    s_mode = (int)t->value->int32;
    persist_write_int(KEY_MODE, s_mode);
    layer_mark_dirty(s_canvas);
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);

  /* system fonts — no custom font needed */
  s_font_code   = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_font_result = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, canvas_draw);
  layer_add_child(root, s_canvas);

  time_t now = time(NULL);
  update_time(localtime(&now));
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
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

int main(void) { init(); app_event_loop(); deinit(); }
