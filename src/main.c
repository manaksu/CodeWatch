#include <pebble.h>
#include <string.h>

/*
 * Code Watch — Source Code Pro + VS Code theme
 * Full width code, result line at bottom
 *   Key 0: KEY_MODE  0=Dark  1=Light
 */

#define KEY_MODE   0
#define KEY_STYLE  1
#define MODE_DARK  0
#define MODE_LIGHT 1
#define STYLE_TERMINAL 0
#define STYLE_ASCII    1

#define LH    12   /* line height */
#define LM     4   /* left margin (no gutter — full width) */

static int s_mode = MODE_DARK;
static int s_style = STYLE_TERMINAL;
static Window *s_window;
static Layer  *s_canvas;
static GFont   s_font_code;

static int  s_hour, s_min;
static bool s_is_am;
static char s_day_buf[12];
static char s_result_buf[24]; /* >> 10:33 AM  Mon 17 */

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

static void draw_lno(GContext *ctx) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%d", s_lno++);
  graphics_context_set_text_color(ctx, cl());
  graphics_draw_text(ctx, buf, s_font_code,
    GRect(0, s_cy, 16, LH),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
  s_cx = 20; /* after line number */
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

static void nl(GContext *ctx) { s_cy += LH; draw_lno(ctx); }

/* ── canvas ── */
static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, s_mode==MODE_DARK?GColorBlack:GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* thin gutter line */
  graphics_context_set_stroke_color(ctx, cl());
  graphics_draw_line(ctx, GPoint(17,0), GPoint(17,135));

  char h_str[4], m_str[4];
  int h12 = s_hour%12; if(!h12) h12=12;
  snprintf(h_str, sizeof(h_str), "%d",  h12);
  snprintf(m_str, sizeof(m_str), "%02d", s_min);
  char *per  = s_is_am ? "\"AM\"" : "\"PM\"";
  char *peri = s_is_am ? "\"PM\"" : "\"AM\"";

  s_cy=2; s_lno=1; draw_lno(ctx);

  /* 1: # Monday */
  tok(ctx,"# ",cc()); tok(ctx,s_day_buf,cc()); nl(ctx);

  /* 2: def get_time(): */
  tok(ctx,"def ",ck()); tok(ctx,"get_time",cf()); tok(ctx,"():",co()); nl(ctx);

  /* 3:   if hour < 12: */
  tok(ctx,"  if ",ck()); tok(ctx,"hour",cv());
  tok(ctx," < ",co()); tok(ctx,"12",cn()); tok(ctx,":",co()); nl(ctx);

  /* 4:     period = "AM" */
  tok(ctx,"    period",cv()); tok(ctx," = ",co()); tok(ctx,per,cs()); nl(ctx);

  /* 5:   else: */
  tok(ctx,"  else",ck()); tok(ctx,":",co()); nl(ctx);

  /* 6:     period = "PM" */
  tok(ctx,"    period",cv()); tok(ctx," = ",co()); tok(ctx,peri,cs()); nl(ctx);

  /* 7:   hour = 10 */
  tok(ctx,"  hour",cv()); tok(ctx," = ",co()); tok(ctx,h_str,cn()); nl(ctx);

  /* 8:   mins = 33 */
  tok(ctx,"  mins",cv()); tok(ctx," = ",co()); tok(ctx,m_str,cn()); nl(ctx);

  /* 9:   return { */
  tok(ctx,"  return ",ck()); tok(ctx,"{",co()); nl(ctx);

  /* 10:    "h":10, "m":33 */
  tok(ctx,"    \"h\"",cs()); tok(ctx,":",co()); tok(ctx,h_str,cn());
  tok(ctx,"  \"m\"",cs()); tok(ctx,":",co()); tok(ctx,m_str,cn()); nl(ctx);

  /* 11:    "p":"AM" } */
  tok(ctx,"    \"p\"",cs()); tok(ctx,":",co()); tok(ctx,per,cs()); tok(ctx," }",co());

  /* ── result display ── */
  int result_y = 136;
  GColor ink = s_mode==MODE_DARK ? GColorWhite : GColorBlack;

  if (s_style == STYLE_ASCII) {
    /* ASCII box style */
    /* top border: ┌──...──┐ */
    graphics_context_set_text_color(ctx, co());
    graphics_draw_text(ctx,
      "âââââââââââââââââââââââââ",
      s_font_code, GRect(0, result_y, 144, LH),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    /* middle: │ >> result │ */
    graphics_context_set_text_color(ctx, co());
    graphics_draw_text(ctx, "â", s_font_code,
      GRect(0, result_y+LH, 8, LH),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, "â", s_font_code,
      GRect(136, result_y+LH, 8, LH),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    /* >> prompt */
    graphics_context_set_text_color(ctx, cn());
    graphics_draw_text(ctx, ">>", s_font_code,
      GRect(8, result_y+LH, 16, LH),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    /* result text */
    graphics_context_set_text_color(ctx, ink);
    graphics_draw_text(ctx, s_result_buf, s_font_code,
      GRect(24, result_y+LH, 110, LH),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    /* bottom border: └──...──┘ */
    graphics_context_set_text_color(ctx, co());
    graphics_draw_text(ctx,
      "âââââââââââââââââââââââââ",
      s_font_code, GRect(0, result_y+LH*2, 144, LH),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  } else {
    /* Terminal style */
    graphics_context_set_fill_color(ctx,
      s_mode==MODE_DARK ? GColorDarkGray : GColorLightGray);
    graphics_fill_rect(ctx, GRect(0, result_y, 144, 1), 0, GCornerNone);
    graphics_context_set_fill_color(ctx,
      s_mode==MODE_DARK ? GColorBlack : GColorWhite);
    graphics_fill_rect(ctx, GRect(0, result_y+1, 144, 18), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, cn());
    graphics_fill_rect(ctx, GRect(2, result_y+3, 6, 11), 0, GCornerNone);
    graphics_context_set_text_color(ctx, cn());
    graphics_draw_text(ctx, "$", s_font_code,
      GRect(10, result_y+2, 10, LH),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    graphics_context_set_text_color(ctx, ink);
    graphics_draw_text(ctx, s_result_buf, s_font_code,
      GRect(20, result_y+2, 122, LH),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    GSize rw = graphics_text_layout_get_content_size(
      s_result_buf, s_font_code, GRect(0,0,200,LH),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
    int cur_x = 20 + rw.w + 1;
    if (cur_x < 140) {
      graphics_context_set_fill_color(ctx, ink);
      graphics_fill_rect(ctx, GRect(cur_x, result_y+3, 5, 10), 0, GCornerNone);
    }
  }
}

static void update_time(struct tm *t) {
  s_hour  = t->tm_hour;
  s_min   = t->tm_min;
  s_is_am = s_hour < 12;
  strftime(s_day_buf, sizeof(s_day_buf), "%A", t);

  char time_s[8], day_s[10];
  strftime(time_s, sizeof(time_s),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  strftime(day_s, sizeof(day_s), "%a %d", t);
  snprintf(s_result_buf, sizeof(s_result_buf), "%s %s  %s",
           time_s, s_is_am?"AM":"PM", day_s);

  layer_mark_dirty(s_canvas);
}

static void tick_handler(struct tm *tick_time, TimeUnits u) {
  update_time(tick_time);
}

static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *t = dict_find(iter, KEY_MODE);
  if (t) {
    s_mode=(int)t->value->int32;
    persist_write_int(KEY_MODE, s_mode);
    layer_mark_dirty(s_canvas);
  }
  Tuple *st = dict_find(iter, KEY_STYLE);
  if (st) {
    s_style=(int)st->value->int32;
    persist_write_int(KEY_STYLE, s_style);
    layer_mark_dirty(s_canvas);
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);
  s_font_code = fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_FONT_SCP_12));
  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, canvas_draw);
  layer_add_child(root, s_canvas);
  time_t now = time(NULL);
  update_time(localtime(&now));
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  fonts_unload_custom_font(s_font_code);
}

static void init(void) {
  s_mode  = persist_exists(KEY_MODE)  ? persist_read_int(KEY_MODE)  : MODE_DARK;
  s_style = persist_exists(KEY_STYLE) ? persist_read_int(KEY_STYLE) : STYLE_TERMINAL;
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load=window_load, .unload=window_unload
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
