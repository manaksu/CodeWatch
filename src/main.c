#include <pebble.h>
#include <string.h>

/*
 * Code Watch — Source Code Pro + VS Code theme
 *   Key 0: KEY_MODE  0=Dark  1=Light
 */

#define KEY_MODE   0
#define MODE_DARK  0
#define MODE_LIGHT 1

#define CODE_W   108
#define STRIP_X  118
#define GUTTER    18
#define LM        21
#define LH        13

static int s_mode = MODE_DARK;
static Window *s_window;
static Layer  *s_canvas;
static GFont   s_font_code;

static int  s_hour, s_min;
static bool s_is_am;
static char s_day_buf[12];
static char s_vert_buf[32]; /* vertical time string */

/* ── colours ── */
static GColor ck(void){ return s_mode==MODE_DARK?GColorMagenta        :GColorPurple;       }
static GColor cf(void){ return s_mode==MODE_DARK?GColorYellow          :GColorChromeYellow; }
static GColor cv(void){ return s_mode==MODE_DARK?GColorPictonBlue      :GColorBlue;         }
static GColor cs(void){ return s_mode==MODE_DARK?GColorOrange          :GColorRed;          }
static GColor cn(void){ return s_mode==MODE_DARK?GColorGreen           :GColorIslamicGreen; }
static GColor co(void){ return s_mode==MODE_DARK?GColorLightGray       :GColorDarkGray;     }
static GColor cc(void){ return s_mode==MODE_DARK?GColorMediumSpringGreen:GColorIslamicGreen;}
static GColor cl(void){ return s_mode==MODE_DARK?GColorDarkGray        :GColorLightGray;    }

/* ── 5×7 pixel font ── */
static const uint8_t s_f5x7[][5] = {
  {0x00,0x00,0x00,0x00,0x00}, /* space */
  {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
  {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
  {0x42,0x61,0x51,0x49,0x46}, /* 2 */
  {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
  {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
  {0x27,0x45,0x45,0x45,0x39}, /* 5 */
  {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
  {0x01,0x71,0x09,0x05,0x03}, /* 7 */
  {0x36,0x49,0x49,0x49,0x36}, /* 8 */
  {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
  {0x7E,0x11,0x11,0x11,0x7E}, /* A */
  {0x7F,0x49,0x49,0x49,0x36}, /* B */
  {0x3E,0x41,0x41,0x41,0x22}, /* C */
  {0x7F,0x41,0x41,0x22,0x1C}, /* D */
  {0x7F,0x49,0x49,0x49,0x41}, /* E */
  {0x7F,0x09,0x09,0x09,0x01}, /* F */
  {0x3E,0x41,0x49,0x49,0x7A}, /* G */
  {0x7F,0x08,0x08,0x08,0x7F}, /* H */
  {0x00,0x41,0x7F,0x41,0x00}, /* I */
  {0x20,0x40,0x41,0x3F,0x01}, /* J */
  {0x7F,0x08,0x14,0x22,0x41}, /* K */
  {0x7F,0x40,0x40,0x40,0x40}, /* L */
  {0x7F,0x02,0x04,0x02,0x7F}, /* M */
  {0x7F,0x04,0x08,0x10,0x7F}, /* N */
  {0x3E,0x41,0x41,0x41,0x3E}, /* O */
  {0x7F,0x09,0x09,0x09,0x06}, /* P */
  {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
  {0x7F,0x09,0x19,0x29,0x46}, /* R */
  {0x46,0x49,0x49,0x49,0x31}, /* S */
  {0x01,0x01,0x7F,0x01,0x01}, /* T */
  {0x3F,0x40,0x40,0x40,0x3F}, /* U */
  {0x1F,0x20,0x40,0x20,0x1F}, /* V */
  {0x3F,0x40,0x38,0x40,0x3F}, /* W */
  {0x63,0x14,0x08,0x14,0x63}, /* X */
  {0x07,0x08,0x70,0x08,0x07}, /* Y */
  {0x61,0x51,0x49,0x45,0x43}, /* Z */
  {0x08,0x08,0x08,0x08,0x08}, /* - */
  {0x00,0x36,0x36,0x00,0x00}, /* : */
  {0x00,0x50,0x30,0x00,0x00}, /* , */
  {0x00,0x60,0x60,0x00,0x00}, /* . */
  {0x41,0x22,0x14,0x08,0x00}, /* > */
};

static int fidx(char c) {
  if (c==' ')             return 0;
  if (c>='0'&&c<='9')     return 1+(c-'0');
  if (c>='A'&&c<='Z')     return 11+(c-'A');
  if (c>='a'&&c<='z')     return 11+(c-'a');
  if (c=='-')             return 37;
  if (c==':')             return 38;
  if (c==',')             return 39;
  if (c=='.')             return 40;
  if (c=='>')             return 41;
  return 0;
}

/* ── draw vertical text using 5x7 pixel font ── */
static void draw_vert(GContext *ctx, const char *text,
                      int sx, int sy, int max_h, GColor col) {
  graphics_context_set_fill_color(ctx, col);
  int cur_y = sy;
  for (int ci = 0; text[ci]; ci++) {
    int cy = cur_y;
    cur_y += (text[ci]==' ') ? 5 : 9;
    if (cy > sy + max_h) break;
    const uint8_t *g = s_f5x7[fidx(text[ci])];
    for (int c=0;c<5;c++) {
      for (int r=0;r<7;r++) {
        if (g[c] & (1<<(6-r))) {
          int px = sx + (6-r);
          int py = cy + (4-c);
          if (py>=sy && py<=sy+max_h)
            graphics_fill_rect(ctx, GRect(px,py,2,2), 0, GCornerNone);
        }
      }
    }
  }
}

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
    GRect(s_cx, s_cy, CODE_W-s_cx, LH),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  GSize sz = graphics_text_layout_get_content_size(
    text, s_font_code, GRect(0,0,200,LH),
    GTextOverflowModeWordWrap, GTextAlignmentLeft);
  s_cx += sz.w;
}

static void nl(GContext *ctx) { s_cy+=LH; draw_lno(ctx); }

/* ── canvas ── */
static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, s_mode==MODE_DARK?GColorBlack:GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  char h_str[4], m_str[4];
  int h12 = s_hour%12; if(!h12) h12=12;
  snprintf(h_str, sizeof(h_str), "%d",  h12);
  snprintf(m_str, sizeof(m_str), "%02d", s_min);
  char *per  = s_is_am ? "\"AM\"" : "\"PM\"";
  char *peri = s_is_am ? "\"PM\"" : "\"AM\"";

  s_cy=2; s_lno=1; draw_lno(ctx);
  tok(ctx,"# ",cc());   tok(ctx,s_day_buf,cc()); nl(ctx);
  tok(ctx,"def ",ck()); tok(ctx,"get_time",cf()); tok(ctx,"():",co()); nl(ctx);
  tok(ctx,"  if ",ck()); tok(ctx,"hour",cv()); tok(ctx,"<",co()); tok(ctx,"12",cn()); tok(ctx,":",co()); nl(ctx);
  tok(ctx,"   p=",co()); tok(ctx,per,cs()); nl(ctx);
  tok(ctx,"  else",ck()); tok(ctx,":",co()); nl(ctx);
  tok(ctx,"   p=",co()); tok(ctx,peri,cs()); nl(ctx);
  tok(ctx,"  hour",cv()); tok(ctx,"=",co()); tok(ctx,h_str,cn()); nl(ctx);
  tok(ctx,"  mins",cv()); tok(ctx,"=",co()); tok(ctx,m_str,cn()); nl(ctx);
  tok(ctx,"  return ",ck()); tok(ctx,"{",co()); nl(ctx);
  tok(ctx,"  \"h\"",cs()); tok(ctx,":",co()); tok(ctx,h_str,cn());
  tok(ctx," \"m\"",cs()); tok(ctx,":",co()); tok(ctx,m_str,cn()); nl(ctx);
  tok(ctx,"  \"p\"",cs()); tok(ctx,":",co()); tok(ctx,per,cs()); tok(ctx," }",co());

  /* vertical time using 5x7 pixel font — no custom font involved */
  GColor tcol = s_mode==MODE_DARK ? GColorWhite : GColorBlack;
  draw_vert(ctx, s_vert_buf, STRIP_X, 4, 160, tcol);
}

static void update_time(struct tm *t) {
  s_hour  = t->tm_hour;
  s_min   = t->tm_min;
  s_is_am = s_hour < 12;
  strftime(s_day_buf, sizeof(s_day_buf), "%A", t);

  char day_s[5], day_n[4], time_s[8];
  strftime(day_s,  sizeof(day_s),  "%a", t);
  strftime(day_n,  sizeof(day_n),  "%d", t);
  strftime(time_s, sizeof(time_s),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  snprintf(s_vert_buf, sizeof(s_vert_buf), ">> %s %s %s",
           time_s, day_s, day_n);

  /* uppercase */
  for (int i=0; s_vert_buf[i]; i++)
    if (s_vert_buf[i]>='a' && s_vert_buf[i]<='z')
      s_vert_buf[i] -= 32;

  /* reverse for top-to-bottom rendering */
  int len = (int)strlen(s_vert_buf);
  for (int i=0; i<len/2; i++) {
    char tmp = s_vert_buf[i];
    s_vert_buf[i] = s_vert_buf[len-1-i];
    s_vert_buf[len-1-i] = tmp;
  }
  layer_mark_dirty(s_canvas);
}

static void tick_handler(struct tm *tick_time, TimeUnits u) { update_time(tick_time); }

static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *t = dict_find(iter, KEY_MODE);
  if (t) { s_mode=(int)t->value->int32; persist_write_int(KEY_MODE,s_mode); layer_mark_dirty(s_canvas); }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);
  /* Source Code Pro for code lines only */
  s_font_code = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SCP_12));
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
  s_mode = persist_exists(KEY_MODE) ? persist_read_int(KEY_MODE) : MODE_DARK;
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
