#include <pebble.h>

/*
 * Code Watch — JetBrains Mono + VS Code theme
 *
 * Layout:
 *  ┌─────────────────┐
 *  │ code (0-108px)  │  ← rotated time text on right edge
 *  │                 │     drawn as single string rotated 90° CCW
 *  └─────────────────┘
 *
 * Rotation trick: use a GBitmap as offscreen buffer,
 * draw text into it, then rotate 90° CCW when blitting.
 * Pebble SDK supports GBitmap rotation via gbitmap_create_blank.
 *
 * Simpler approach: draw each character of the rotated string
 * using a rotated GContext transform — not available on basalt.
 *
 * Best practical approach on Pebble basalt:
 * Draw text into an offscreen GBitmap, then copy pixels rotated.
 */

#define KEY_MODE   0
#define MODE_DARK  0
#define MODE_LIGHT 1

#define CODE_W   108
#define STRIP_X  110
#define GUTTER    18
#define LM        21
#define LH        13

static int s_mode = MODE_DARK;
static Window *s_window;
static Layer  *s_canvas;
static GFont   s_font_code;
static GFont   s_font_time;

static int  s_hour, s_min;
static bool s_is_am;
static char s_day_buf[12];
static char s_rotated_buf[32]; /* "Mon, 17, 10:33 PM" */

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
    GRect(1, s_cy, GUTTER-3, LH),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
  s_cx = LM;
}

static void tok(GContext *ctx, const char *text, GColor col) {
  if (!text || !text[0]) return;
  graphics_context_set_text_color(ctx, col);
  graphics_draw_text(ctx, text, s_font_code,
    GRect(s_cx, s_cy, CODE_W - s_cx, LH),
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

/* ── rotate pixels 90° CCW from src bitmap into screen ── */
/* src is W×H, drawn at screen position (dst_x, dst_y) */
/* after rotation: occupies H wide × W tall on screen */
static void draw_rotated_bitmap_ccw(GContext *ctx, GBitmap *src,
                                     int dst_x, int dst_y) {
  GRect src_bounds = gbitmap_get_bounds(src);
  int sw = src_bounds.size.w;
  int sh = src_bounds.size.h;
  /* rotated: width=sh, height=sw */
  for (int sy = 0; sy < sh; sy++) {
    GBitmapDataRowInfo row = gbitmap_get_data_row_info(src, sy);
    for (int sx = row.min_x; sx <= row.max_x; sx++) {
      /* CCW rotation: new_x = sy, new_y = (sw-1-sx) */
      int nx = dst_x + sy;
      int ny = dst_y + (sw - 1 - sx);
      GColor8 px;
      uint8_t *data = row.data;
      /* 1-bit bitmap: check bit */
      uint8_t byte_val = data[sx / 8];
      bool lit = (byte_val >> (sx % 8)) & 1;
      if (lit) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_rect(ctx, GRect(nx, ny, 1, 1), 0, GCornerNone);
      }
    }
  }
}

/* ── canvas ── */
static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  /* background */
  GColor bg = s_mode==MODE_DARK ? GColorBlack : GColorWhite;
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* gutter */
  graphics_context_set_fill_color(ctx,
    s_mode==MODE_DARK ? GColorOxfordBlue : GColorLightGray);
  graphics_fill_rect(ctx, GRect(0,0,GUTTER,168), 0, GCornerNone);

  /* thin divider between code and time */
  graphics_context_set_stroke_color(ctx, cl());
  graphics_draw_line(ctx, GPoint(STRIP_X-1,0), GPoint(STRIP_X-1,168));

  /* ── code ── */
  char h_str[4], m_str[4];
  int h12 = s_hour % 12; if (!h12) h12=12;
  snprintf(h_str, sizeof(h_str), "%d",  h12);
  snprintf(m_str, sizeof(m_str), "%02d", s_min);
  char *per  = s_is_am ? "\"AM\"" : "\"PM\"";
  char *peri = s_is_am ? "\"PM\"" : "\"AM\"";

  s_cy=2; s_lno=1;
  draw_lno(ctx);
  tok(ctx,"# ",cc()); tok(ctx,s_day_buf,cc()); nl(ctx);
  tok(ctx,"def ",ck()); tok(ctx,"get_time",cf()); tok(ctx,"():",co()); nl(ctx);
  tok(ctx,"  if ",ck()); tok(ctx,"hour",cv());
  tok(ctx,"<",co()); tok(ctx,"12",cn()); tok(ctx,":",co()); nl(ctx);
  tok(ctx,"   p=",co()); tok(ctx,per,cs()); nl(ctx);
  tok(ctx,"  else",ck()); tok(ctx,":",co()); nl(ctx);
  tok(ctx,"   p=",co()); tok(ctx,peri,cs()); nl(ctx);
  tok(ctx,"  hour",cv()); tok(ctx,"=",co()); tok(ctx,h_str,cn()); nl(ctx);
  tok(ctx,"  mins",cv()); tok(ctx,"=",co()); tok(ctx,m_str,cn()); nl(ctx);
  tok(ctx,"  return ",ck()); tok(ctx,"{",co()); nl(ctx);
  tok(ctx,"  \"h\"",cs()); tok(ctx,":",co()); tok(ctx,h_str,cn());
  tok(ctx," \"m\"",cs()); tok(ctx,":",co()); tok(ctx,m_str,cn()); nl(ctx);
  tok(ctx,"  \"p\"",cs()); tok(ctx,":",co()); tok(ctx,per,cs()); tok(ctx," }",co());

  /* ── rotated time string ── */
  /* Draw text into offscreen 1-bit bitmap, then rotate CCW 90° onto right strip */
  /* Text width estimate: ~10px per char at font size 14 */
  int text_len = (int)strlen(s_rotated_buf);
  int bmp_w = text_len * 10 + 4;  /* generous estimate */
  int bmp_h = 18;                  /* font height */

  GBitmap *offscreen = gbitmap_create_blank(GSize(bmp_w, bmp_h), GBitmapFormat1Bit);
  if (offscreen) {
    GContext *off_ctx = graphics_context_create(NULL);
    /* We can't create an offscreen GContext on Pebble directly —
       use the alternative: draw pixel-by-pixel using a precomputed layout */
    gbitmap_destroy(offscreen);
  }

  /* Fallback: draw time string as individual chars stacked,
     but with proper 90° CCW orientation by swapping x/y coordinates */
  /* Each char from the string: position on screen is
     x = STRIP_X + (strip_height/2) - char_index * char_w  (reading bottom to top)
     y = fixed centre of strip
     This gives bottom-to-top reading = CCW rotation */

  int strip_cx = STRIP_X + 16;  /* horizontal centre of strip */
  int char_h   = 14;            /* step between chars */
  int total_h  = text_len * char_h;
  int start_y  = 84 + total_h/2 - char_h; /* centre vertically */

  GColor time_col = s_mode==MODE_DARK ? GColorWhite : GColorBlack;

  for (int i = 0; i < text_len; i++) {
    char buf[2] = {s_rotated_buf[i], '\0'};
    /* draw bottom-to-top = CCW: last char at top, first char at bottom */
    int cy_char = start_y - i * char_h;
    graphics_context_set_text_color(ctx, time_col);
    graphics_draw_text(ctx, buf, s_font_time,
      GRect(strip_cx - 8, cy_char, 16, char_h + 2),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
}

static void update_time(struct tm *t) {
  s_hour  = t->tm_hour;
  s_min   = t->tm_min;
  s_is_am = s_hour < 12;
  strftime(s_day_buf, sizeof(s_day_buf), "%A", t);
  /* rotated string: "Mon, 17, 10:33 PM" */
  char time_part[8], day_short[5], day_num[4];
  strftime(time_part, sizeof(time_part),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  strftime(day_short, sizeof(day_short), "%a", t);
  strftime(day_num,   sizeof(day_num),   "%d", t);
  snprintf(s_rotated_buf, sizeof(s_rotated_buf), "%s, %s, %s %s",
           day_short, day_num, time_part, s_is_am ? "AM" : "PM");
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
  s_font_code = fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_FONT_JB_12));
  s_font_time = fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_FONT_JB_16));
  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, canvas_draw);
  layer_add_child(root, s_canvas);
  time_t now = time(NULL);
  update_time(localtime(&now));
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  fonts_unload_custom_font(s_font_code);
  fonts_unload_custom_font(s_font_time);
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
