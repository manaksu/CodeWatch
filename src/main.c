#include <pebble.h>
#include <string.h>

/*
 * Code Watch — Source Code Pro + VS Code theme
 * Code as backdrop (small, muted), big terminal result at bottom
 *   Key 0: KEY_MODE   0=Dark  1=Light
 *   Key 1: KEY_STYLE  0=Terminal  1=ASCII Box
 */

#define KEY_MODE   0
#define KEY_STYLE  1
#define MODE_DARK  0
#define MODE_LIGHT 1
#define STYLE_TERMINAL 0
#define STYLE_ASCII    1

#define LH_CODE  11   /* backdrop code line height — small */
#define LH_RES   22   /* result bar height */

static int s_mode  = MODE_DARK;
static int s_style = STYLE_TERMINAL;

static Window *s_window;
static Layer  *s_canvas;
static GFont   s_font_code;   /* SCP 12 — backdrop */
static GFont   s_font_result; /* system GOTHIC_28_BOLD — result */

static int  s_hour, s_min;
static bool s_is_am;
static char s_day_buf[12];
static char s_result_buf[20]; /* "10:33 AM  Mon 17" */

/* ── muted backdrop colours ── */
/* All at ~40% opacity feel — dimmed versions of VS Code colours */
static GColor ck(void){ return s_mode==MODE_DARK?GColorDarkCandyAppleRed :GColorLightGray; }
static GColor cf(void){ return s_mode==MODE_DARK?GColorBulgarianRose      :GColorLightGray; }
static GColor cv(void){ return s_mode==MODE_DARK?GColorOxfordBlue         :GColorLightGray; }
static GColor cs(void){ return s_mode==MODE_DARK?GColorWindsorTan         :GColorLightGray; }
static GColor cn(void){ return s_mode==MODE_DARK?GColorDarkGreen          :GColorLightGray; }
static GColor co(void){ return s_mode==MODE_DARK?GColorDarkGray           :GColorLightGray; }
static GColor cc(void){ return s_mode==MODE_DARK?GColorDarkGray           :GColorLightGray; }

/* result colours — full brightness */
static GColor cr_num(void){ return s_mode==MODE_DARK?GColorGreen           :GColorIslamicGreen; }
static GColor cr_str(void){ return s_mode==MODE_DARK?GColorOrange          :GColorRed;          }
static GColor cr_ink(void){ return s_mode==MODE_DARK?GColorWhite           :GColorBlack;        }
static GColor cr_dim(void){ return s_mode==MODE_DARK?GColorLightGray       :GColorDarkGray;     }

/* ── draw state ── */
static int s_cx, s_cy, s_lno;

static void draw_lno(GContext *ctx) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%d", s_lno++);
  graphics_context_set_text_color(ctx, co());
  graphics_draw_text(ctx, buf, s_font_code,
    GRect(0, s_cy, 14, LH_CODE),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
  s_cx = 16;
}

static void tok(GContext *ctx, const char *text, GColor col) {
  if (!text || !text[0]) return;
  graphics_context_set_text_color(ctx, col);
  graphics_draw_text(ctx, text, s_font_code,
    GRect(s_cx, s_cy, 144-s_cx, LH_CODE),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  GSize sz = graphics_text_layout_get_content_size(
    text, s_font_code, GRect(0,0,200,LH_CODE),
    GTextOverflowModeWordWrap, GTextAlignmentLeft);
  s_cx += sz.w;
}

static void nl(GContext *ctx) { s_cy += LH_CODE; draw_lno(ctx); }

/* ── canvas ── */
static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GColor bg = s_mode==MODE_DARK ? GColorBlack : GColorWhite;
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  char h_str[4], m_str[4];
  int h12 = s_hour%12; if(!h12) h12=12;
  snprintf(h_str, sizeof(h_str), "%d",  h12);
  snprintf(m_str, sizeof(m_str), "%02d", s_min);
  char *per  = s_is_am ? "\"AM\"" : "\"PM\"";
  char *peri = s_is_am ? "\"PM\"" : "\"AM\"";

  /* ── backdrop code — muted, small ── */
  s_cy=2; s_lno=1; draw_lno(ctx);
  tok(ctx,"# ",cc()); tok(ctx,s_day_buf,cc()); nl(ctx);
  tok(ctx,"def ",ck()); tok(ctx,"get_time",cf()); tok(ctx,"():",co()); nl(ctx);
  tok(ctx,"  if ",ck()); tok(ctx,"hour",cv()); tok(ctx,"<",co()); tok(ctx,"12",cn()); tok(ctx,":",co()); nl(ctx);
  tok(ctx,"    period",cv()); tok(ctx,"=",co()); tok(ctx,per,cs()); nl(ctx);
  tok(ctx,"  else",ck()); tok(ctx,":",co()); nl(ctx);
  tok(ctx,"    period",cv()); tok(ctx,"=",co()); tok(ctx,peri,cs()); nl(ctx);
  tok(ctx,"  hour",cv()); tok(ctx,"=",co()); tok(ctx,h_str,cn()); nl(ctx);
  tok(ctx,"  mins",cv()); tok(ctx,"=",co()); tok(ctx,m_str,cn()); nl(ctx);
  tok(ctx,"  return ",ck()); tok(ctx,"{",co()); nl(ctx);
  tok(ctx,"    \"h\"",cs()); tok(ctx,":",co()); tok(ctx,h_str,cn());
  tok(ctx,"  \"m\"",cs()); tok(ctx,":",co()); tok(ctx,m_str,cn()); nl(ctx);
  tok(ctx,"    \"p\"",cs()); tok(ctx,":",co()); tok(ctx,per,cs()); tok(ctx," }",co());

  /* ── result area ── */
  int result_y = 128;

  if (s_style == STYLE_ASCII) {
    /* ASCII box */
    graphics_context_set_text_color(ctx, cr_dim());
    /* top */
    graphics_draw_text(ctx,
      "\xe2\x94\x8c\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
      "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
      "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
      "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x90",
      s_font_code, GRect(0, result_y, 144, LH_CODE),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    /* left │ */
    graphics_draw_text(ctx, "\xe2\x94\x82", s_font_code,
      GRect(0, result_y+LH_CODE, 8, LH_CODE),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    /* right │ */
    graphics_draw_text(ctx, "\xe2\x94\x82", s_font_code,
      GRect(136, result_y+LH_CODE, 8, LH_CODE),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    /* >> */
    graphics_context_set_text_color(ctx, cr_num());
    graphics_draw_text(ctx, ">>", s_font_code,
      GRect(6, result_y+LH_CODE, 18, LH_CODE),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    /* result big */
    graphics_context_set_text_color(ctx, cr_ink());
    graphics_draw_text(ctx, s_result_buf, s_font_result,
      GRect(24, result_y+4, 110, 28),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    /* bottom */
    graphics_context_set_text_color(ctx, cr_dim());
    graphics_draw_text(ctx,
      "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
      "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
      "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
      "\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x98",
      s_font_code, GRect(0, result_y+38, 144, LH_CODE),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  } else {
    /* Terminal style */
    /* divider line */
    graphics_context_set_fill_color(ctx, cr_dim());
    graphics_fill_rect(ctx, GRect(0, result_y, 144, 1), 0, GCornerNone);

    /* cursor block */
    graphics_context_set_fill_color(ctx, cr_num());
    graphics_fill_rect(ctx, GRect(2, result_y+6, 5, 12), 0, GCornerNone);

    /* $ prompt */
    graphics_context_set_text_color(ctx, cr_num());
    graphics_draw_text(ctx, "$", s_font_code,
      GRect(9, result_y+6, 10, LH_CODE),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    /* big result */
    graphics_context_set_text_color(ctx, cr_ink());
    graphics_draw_text(ctx, s_result_buf, s_font_result,
      GRect(20, result_y+2, 122, 36),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
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
  snprintf(s_result_buf, sizeof(s_result_buf), "%s %s",
           time_s, day_s);
  layer_mark_dirty(s_canvas);
}

static void tick_handler(struct tm *tick_time, TimeUnits u) { update_time(tick_time); }

static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *t = dict_find(iter, KEY_MODE);
  if (t) { s_mode=(int)t->value->int32; persist_write_int(KEY_MODE,s_mode); layer_mark_dirty(s_canvas); }
  Tuple *st = dict_find(iter, KEY_STYLE);
  if (st) { s_style=(int)st->value->int32; persist_write_int(KEY_STYLE,s_style); layer_mark_dirty(s_canvas); }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);
  s_font_code   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SCP_12));
  s_font_result = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
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

static void deinit(void) { tick_timer_service_unsubscribe(); window_destroy(s_window); }
int main(void) { init(); app_event_loop(); deinit(); }
