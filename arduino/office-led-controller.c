/*
 * Arduino Nano Every WS2812B Controller w/ SSD1306 screen & two encoder knobs.
 *
 * https://www.amazon.com/Arduino-Nano-Every-Single-Board/dp/B07VX7MX27/
 * https://www.amazon.com/SSD1306-Self-Luminous-Display-Compatible-Raspberry/dp/B08QJ7BL3R/
 * https://www.amazon.com/gp/product/B0177VGSQY/
 * https://www.amazon.com/gp/product/B07LG65LSB/
 *
 * Use 100nf capacitors to debounce the encoder logic lines.
 *
 * No rights reserved, released to the public domain.
 * Written by Calvin Owens <jcalvinowens@gmail.com>
 */

#include <Wire.h>

static int decimal_txt(long d, char *out, const char *suffix = NULL)
{
  bool neg = d < 0;
  int r, c = 0;
  char buf[9];

  do {
    buf[c++] = d % 10;
    d /= 10;
  } while (d);

  if (neg)
    *out++ = '-';

  r = c;
  while (c--)
    *out++ = '0' + abs(buf[c]);

  if (suffix)
    while ((*suffix))
      *out++ = *suffix++;

  return r;
}

/*
 * Font, from lib/fonts/font_mini_4x6.c in the Linux kernel.
 * Only printable ASCII is supported, high bit is a highlight flag.
 */

static const uint8_t HIGHLIGHT = 1U << 7;
static const int font_width_px = 4;

struct fontchar {
  uint8_t r0 : 4;
  uint8_t r1 : 4;
  uint8_t r2 : 4;
  uint8_t r3 : 4;
  uint8_t r4 : 4;
  uint8_t r5 : 4;
};

static const PROGMEM struct fontchar fonttable[96] = {
  {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x4, 0x4, 0x4, 0x0, 0x4, 0x0},
  {0xa, 0xa, 0x0, 0x0, 0x0, 0x0}, {0xa, 0xf, 0xf, 0xa, 0x0, 0x0},
  {0x4, 0x6, 0xe, 0xc, 0x4, 0x0}, {0xa, 0x2, 0x4, 0x8, 0xa, 0x0},
  {0x6, 0x9, 0x6, 0xa, 0xd, 0x0}, {0x2, 0x4, 0x0, 0x0, 0x0, 0x0},
  {0x2, 0x4, 0x4, 0x4, 0x2, 0x0}, {0x4, 0x2, 0x2, 0x2, 0x4, 0x0},
  {0x0, 0xe, 0xe, 0xe, 0x0, 0x0}, {0x0, 0x4, 0xe, 0x4, 0x0, 0x0},
  {0x0, 0x0, 0x0, 0x4, 0x8, 0x0}, {0x0, 0x0, 0xe, 0x0, 0x0, 0x0},
  {0x0, 0x0, 0x0, 0x0, 0x4, 0x0}, {0x0, 0x2, 0x4, 0x8, 0x0, 0x0},
  {0x4, 0xa, 0xa, 0xa, 0x4, 0x0}, {0x4, 0xc, 0x4, 0x4, 0xe, 0x0},
  {0xc, 0x2, 0x4, 0x8, 0xe, 0x0}, {0xe, 0x2, 0x6, 0x2, 0xe, 0x0},
  {0xa, 0xa, 0xe, 0x2, 0x2, 0x0}, {0xe, 0x8, 0xe, 0x2, 0xe, 0x0},
  {0xe, 0x8, 0xe, 0xa, 0xe, 0x0}, {0xe, 0x2, 0x2, 0x2, 0x2, 0x0},
  {0xe, 0xa, 0xe, 0xa, 0xe, 0x0}, {0xe, 0xa, 0xe, 0x2, 0x2, 0x0},
  {0x0, 0x0, 0x4, 0x0, 0x4, 0x0}, {0x0, 0x0, 0x4, 0x0, 0x4, 0x8},
  {0x2, 0x4, 0x8, 0x4, 0x2, 0x0}, {0x0, 0xe, 0x0, 0xe, 0x0, 0x0},
  {0x8, 0x4, 0x2, 0x4, 0x8, 0x0}, {0xe, 0x2, 0x6, 0x0, 0x4, 0x0},
  {0x4, 0xe, 0xe, 0x8, 0x4, 0x0}, {0x4, 0xa, 0xe, 0xa, 0xa, 0x0},
  {0xc, 0xa, 0xc, 0xa, 0xc, 0x0}, {0x6, 0x8, 0x8, 0x8, 0x6, 0x0},
  {0xc, 0xa, 0xa, 0xa, 0xc, 0x0}, {0xe, 0x8, 0xe, 0x8, 0xe, 0x0},
  {0xe, 0x8, 0xe, 0x8, 0x8, 0x0}, {0x6, 0x8, 0xe, 0xa, 0x6, 0x0},
  {0xa, 0xa, 0xe, 0xa, 0xa, 0x0}, {0xe, 0x4, 0x4, 0x4, 0xe, 0x0},
  {0x2, 0x2, 0x2, 0xa, 0x4, 0x0}, {0xa, 0xa, 0xc, 0xa, 0xa, 0x0},
  {0x8, 0x8, 0x8, 0x8, 0xe, 0x0}, {0xa, 0xe, 0xe, 0xa, 0xa, 0x0},
  {0xa, 0xe, 0xe, 0xe, 0xa, 0x0}, {0x4, 0xa, 0xa, 0xa, 0x4, 0x0},
  {0xc, 0xa, 0xc, 0x8, 0x8, 0x0}, {0x4, 0xa, 0xa, 0xe, 0x6, 0x0},
  {0xc, 0xa, 0xe, 0xc, 0xa, 0x0}, {0x6, 0x8, 0x4, 0x2, 0xc, 0x0},
  {0xe, 0x4, 0x4, 0x4, 0x4, 0x0}, {0xa, 0xa, 0xa, 0xa, 0x6, 0x0},
  {0xa, 0xa, 0xa, 0x4, 0x4, 0x0}, {0xa, 0xa, 0xe, 0xe, 0xa, 0x0},
  {0xa, 0xa, 0x4, 0xa, 0xa, 0x0}, {0xa, 0xa, 0x4, 0x4, 0x4, 0x0},
  {0xe, 0x2, 0x4, 0x8, 0xe, 0x0}, {0x6, 0x4, 0x4, 0x4, 0x6, 0x0},
  {0x0, 0x8, 0x4, 0x2, 0x0, 0x0}, {0x6, 0x2, 0x2, 0x2, 0x6, 0x0},
  {0x4, 0xa, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0xf},
  {0x8, 0x4, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x6, 0xa, 0xe, 0x0},
  {0x8, 0x8, 0xc, 0xa, 0xc, 0x0}, {0x0, 0x0, 0x6, 0x8, 0x6, 0x0},
  {0x2, 0x2, 0x6, 0xa, 0x6, 0x0}, {0x0, 0xe, 0xe, 0x8, 0x6, 0x0},
  {0x2, 0x4, 0xe, 0x4, 0x4, 0x0}, {0x0, 0x6, 0xa, 0x6, 0xe, 0x0},
  {0x8, 0x8, 0xc, 0xa, 0xa, 0x0}, {0x4, 0x0, 0x4, 0x4, 0x4, 0x0},
  {0x4, 0x0, 0x4, 0x4, 0x8, 0x0}, {0x0, 0x8, 0xa, 0xc, 0xa, 0x0},
  {0x0, 0xc, 0x4, 0x4, 0xe, 0x0}, {0x0, 0x0, 0xe, 0xe, 0xa, 0x0},
  {0x0, 0x0, 0xc, 0xa, 0xa, 0x0}, {0x0, 0x4, 0xa, 0xa, 0x4, 0x0},
  {0x0, 0x0, 0xc, 0xa, 0xc, 0x8}, {0x0, 0x0, 0x6, 0xa, 0x6, 0x2},
  {0x0, 0xc, 0xa, 0x8, 0x8, 0x0}, {0x0, 0x6, 0xc, 0x2, 0xc, 0x0},
  {0x0, 0x4, 0xe, 0x4, 0x4, 0x0}, {0x0, 0x0, 0xa, 0xa, 0x6, 0x0},
  {0x0, 0x0, 0xa, 0xe, 0x4, 0x0}, {0x0, 0x0, 0xa, 0xe, 0xe, 0x0},
  {0x0, 0x0, 0xa, 0x4, 0xa, 0x0}, {0x0, 0x0, 0xa, 0xe, 0x2, 0xc},
  {0x0, 0xe, 0x6, 0xc, 0xe, 0x0}, {0x2, 0x4, 0xc, 0x4, 0x2, 0x0},
  {0x4, 0x4, 0x4, 0x4, 0x4, 0x0}, {0x8, 0x4, 0x6, 0x4, 0x8, 0x0},
  {0x5, 0xa, 0x0, 0x0, 0x0, 0x0}, {0xf, 0x9, 0x9, 0x9, 0x9, 0xf},
};

/*
 * Code to paint SSD1306 OLED screens via I2C
 */

static const int ssd1306_nr_pages = 8;
static const int ssd1306_width_px = 128;
static const int ssd1306_chars = ssd1306_width_px / font_width_px;
static const int ssd1306_i2c_addr = 0x3C;

static void ssd1306_i2c(const uint8_t *cmd, int len, bool data)
{
  int i;

  Wire.beginTransmission(ssd1306_i2c_addr);
  Wire.write(data ? 0x40 : 0x00);

  for (i = 0; i < len; i++)
    Wire.write(cmd[i]);

  if (Wire.endTransmission())
    digitalWrite(LED_BUILTIN, HIGH);
}

static void ssd1306_init_3v(void)
{
  const uint8_t ssd1306_init_sequence[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0x8D, 0x14, 0x20, 0x02, 0xA1, 0xC8, 0xDA, 0x12,
    0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,
    0x2E, 0xAF,
  };

  ssd1306_i2c(ssd1306_init_sequence, sizeof(ssd1306_init_sequence), false);
}

static void ssd1306_set_active_page(uint8_t page)
{
  uint8_t cmd[3] = {0xB0, 0x00, 0x10};

  cmd[0] |= page;
  ssd1306_i2c(cmd, sizeof(cmd), false);
}

static void ssd1306_clear_page(uint8_t page)
{
  uint8_t cmd[16] = {0};
  int i;

  ssd1306_set_active_page(page);
  for (i = 0; i < ssd1306_width_px / sizeof(cmd); i++)
    ssd1306_i2c(cmd, sizeof(cmd), true);
}

static void ssd1306_clear(void)
{
  int i;

  for (i = 0; i < ssd1306_nr_pages; i++)
    ssd1306_clear_page(i);
}

/*
 * The HIGHLIGHT bit underlines/inverts in/out of input mode
 */
static bool inputmode;

static void ssd1306_draw_character(unsigned char c, bool invert = false,
                                   bool uline = false)
{
  uint8_t bitcols[font_width_px] = {0};
  struct fontchar desc;
  int i;

  if (c & HIGHLIGHT) {
    invert = inputmode;
    uline = !inputmode;
    c &= 0x7F;
  }

  if (c < ' ' || c > '~')
    c = 127;

  memcpy_P(&desc, fonttable + c - ' ', sizeof(desc));

  for (i = 0; i < font_width_px; i++) {
    uint8_t mask = 1 << (font_width_px - i - 1);

    bitcols[i] |= !!(desc.r0 & mask) << 1;
    bitcols[i] |= !!(desc.r1 & mask) << 2;
    bitcols[i] |= !!(desc.r2 & mask) << 3;
    bitcols[i] |= !!(desc.r3 & mask) << 4;
    bitcols[i] |= !!(desc.r4 & mask) << 5;
    bitcols[i] |= !!(desc.r5 & mask) << 6;

    if (invert)
      bitcols[i] = ~bitcols[i];

    if (uline)
      bitcols[i] |= HIGHLIGHT;
  }

  ssd1306_i2c(bitcols, sizeof(bitcols), true);
}

static void ssd1306_text(int pg_row, const char *txt)
{
  int i;

  ssd1306_set_active_page(pg_row);
  for (i = 0; i < ssd1306_chars && txt[i]; i++)
    ssd1306_draw_character(txt[i]);
}

/*
 * Bit-bang the WS2812b LED protocol
 */

#define NS_TO_CYCLES(ns) \
  (((((uint64_t)F_CPU) * (ns)) + 1000000000ULL - 1ULL) / 1000000000ULL)

#define DECLARE_WS2812B_SEND_FN(port, pbit)                   \
static void ws2812b_repeat_port##port##pbit(uint8_t *data,    \
                                            int len,          \
                                            int repeats)      \
{                                                             \
  int c;                                                      \
                                                              \
  cli();                                                      \
  for (c = 0; c < repeats * len; c++) {                       \
    const uint8_t val = data[c % len];                        \
    int i;                                                    \
                                                              \
    for (i = 7; i >= 0; i--) {                                \
      if ((val >> i) & 1) {                                   \
        VPORT##port .OUT |= 1U << pbit;                       \
        __builtin_avr_delay_cycles(NS_TO_CYCLES(500));        \
        VPORT##port .OUT &= ~(1U << pbit);                    \
        __builtin_avr_delay_cycles(NS_TO_CYCLES(125));        \
      } else {                                                \
        VPORT##port .OUT |= 1U << pbit;                       \
        __builtin_avr_delay_cycles(NS_TO_CYCLES(125));        \
        VPORT##port .OUT &= ~(1U << pbit);                    \
        __builtin_avr_delay_cycles(NS_TO_CYCLES(188));        \
      }                                                       \
    }                                                         \
  }                                                           \
  sei();                                                      \
                                                              \
  delayMicroseconds(50);                                      \
}

/*
 * Wiring constants
 */

static const int gpio_k0_s = 9;
static const int gpio_k0_a = 10;
static const int gpio_k0_b = 8;
static const int gpio_k1_s = 6;
static const int gpio_k1_a = 5;
static const int gpio_k1_b = 7;

static const int gpio_led_0 = 2; // A0
DECLARE_WS2812B_SEND_FN(A, 0);
static const int gpio_led_1 = 4; // C6
DECLARE_WS2812B_SEND_FN(C, 6);
static const int lower_led_count = 144;
static const int upper_led_count = 202;

/*
 * Plumbing for two encoder knobs
 */

static volatile uint8_t dontcare;
static volatile bool screen_needs_update;

static void refresh_screen(void)
{
  screen_needs_update = true;
}

static volatile bool k0_last;
static volatile bool switch0;
static volatile uint8_t k0_mult = 1;
static volatile uint8_t *volatile knob0 = &dontcare;
static void k0_irq_s(void) { switch0 = true; refresh_screen(); }
static void k0_irq_a(void) { k0_last = digitalRead(gpio_k0_a); }
static void k0_irq_b(void)
{
  digitalRead(gpio_k0_b) == k0_last ? *knob0 += k0_mult : *knob0 -= k0_mult;
  refresh_screen();
}

static volatile bool k1_last;
static volatile bool switch1;
static volatile uint8_t k1_mult = 1;
static volatile uint8_t *volatile knob1 = &dontcare;
static void k1_irq_s(void) { switch1 = true; refresh_screen(); }
static void k1_irq_a(void) { k1_last = digitalRead(gpio_k1_a); }
static void k1_irq_b(void)
{
  digitalRead(gpio_k1_b) == k1_last ? *knob1 += k1_mult : *knob1 -= k1_mult;
  refresh_screen();
}

struct ws281x {
  uint8_t G;
  uint8_t R;
  uint8_t B;
};

/*
 * Presets
 */

struct ws281x_pattern {
  uint8_t len;
  struct ws281x v[];
};

static const struct ws281x_pattern pattern_default = {
  .len = 1,
  .v = {
    {
      .G = 83,
      .R = 230,
      .B = 25,
    },
  },
};

static const struct ws281x_pattern pattern_default2 = {
  .len = 1,
  .v = {
    {
      .G = 89,
      .R = 170,
      .B = 46,
    },
  },
};

static const struct ws281x_pattern pattern_white0 = {
  .len = 1,
  .v = {
    {
      .G = 74,
      .R = 74,
      .B = 74,
    },
  },
};

static const struct ws281x_pattern pattern_white1 = {
  .len = 3,
  .v = {
    {
      .G = 0,
      .R = 223,
      .B = 0,
    },
    {
      .G = 223,
      .R = 0,
      .B = 0,
    },
    {
      .G = 0,
      .R = 0,
      .B = 223,
    },
  },
};

static const struct ws281x_pattern pattern_white2 = {
  .len = 3,
  .v = {
    {
      .G = 112,
      .R = 112,
      .B = 0,
    },
    {
      .G = 112,
      .R = 0,
      .B = 112,
    },
    {
      .G = 0,
      .R = 112,
      .B = 112,
    },
  },
};

static const struct ws281x_pattern pattern_yellow = {
  .len = 2,
  .v = {
    {
      .G = 0,
      .R = 223,
      .B = 0,
    },
    {
      .G = 223,
      .R = 0,
      .B = 0,
    },
  },
};

static const struct ws281x_pattern pattern_purple = {
  .len = 2,
  .v = {
    {
      .G = 0,
      .R = 223,
      .B = 0,
    },
    {
      .G = 0,
      .R = 0,
      .B = 223,
    },
  },
};

static const struct ws281x_pattern pattern_teal = {
  .len = 2,
  .v = {
    {
      .G = 223,
      .R = 0,
      .B = 0,
    },
    {
      .G = 0,
      .R = 0,
      .B = 223,
    },
  },
};

static const struct ws281x_pattern pattern_rainbow = {
  .len = 9,
  .v = {
    {
      .G = 0,
      .R = 223,
      .B = 0,
    },
    {
      .G = 112,
      .R = 223,
      .B = 0,
    },
    {
      .G = 223,
      .R = 223,
      .B = 0,
    },
    {
      .G = 223,
      .R = 112,
      .B = 0,
    },
    {
      .G = 223,
      .R = 0,
      .B = 0,
    },
    {
      .G = 223,
      .R = 0,
      .B = 112,
    },
    {
      .G = 223,
      .R = 0,
      .B = 223,
    },
    {
      .G = 112,
      .R = 0,
      .B = 223,
    },
    {
      .G = 0,
      .R = 0,
      .B = 223,
    },
  },
};

static const struct ws281x_pattern *const presets[] = {
  &pattern_default,
  &pattern_default2,
  &pattern_white0,
  &pattern_white1,
  &pattern_white2,
  &pattern_yellow,
  &pattern_purple,
  &pattern_teal,
  &pattern_rainbow,
};

static const int nr_presets = sizeof(presets) / sizeof(presets[0]);

/*
 * Global variables
 */

static struct ws281x lower_leds[lower_led_count];
static struct ws281x upper_leds[upper_led_count];

static volatile uint16_t preset;
static volatile uint8_t screensel;
static volatile uint8_t fieldsel;
static uint8_t last_screensel = 255;

/*
 * Actions
 */

static unsigned long actions;

enum {
  ALL_LEDS_OFF           = (1UL << 0),
  CYCLE_PRESET           = (1UL << 1),
  APPLY_SETRGB           = (1UL << 2),
  RUN_LOWER_BRIGHTCURVE  = (1UL << 3),
  LOOP_LOWER_BRIGHTCURVE = (1UL << 4),
};

static void run_lower_brightcurve(struct ws281x s, struct ws281x e, uint8_t delay_ms,
                                  uint8_t stepping, uint8_t dither)
{
  const uint8_t pattern[16] = {0, 8, 12, 4, 2, 10, 14, 6, 11, 3, 5, 13, 9, 1, 7, 15};
  unsigned i, j;
  bool R, G, B;

  switch0 = false;
  digitalWrite(LED_BUILTIN, HIGH);
  do {
    R = s.R != e.R;
    if (s.R < e.R)
      s.R = min(s.R + stepping, e.R);
    else if (s.R > e.R)
      s.R = max(s.R - stepping, e.R);

    G = s.G != e.G;
    if (s.G < e.G)
      s.G = min(s.G + stepping, e.G);
    else if (s.G > e.G)
      s.G = max(s.G - stepping, e.G);

    B = s.B != e.B;
    if (s.B < e.B)
      s.B = min(s.B + stepping, e.B);
    else if (s.B > e.B)
      s.B = max(s.B - stepping, e.B);

    for (i = 0; i < sizeof(pattern); i++) {
      for (j = 0; R && j < lower_led_count / sizeof(pattern); j++)
        lower_leds[j * sizeof(pattern) + pattern[i]].R = s.R;
      for (j = 0; G && j < lower_led_count / sizeof(pattern); j++)
        lower_leds[j * sizeof(pattern) + pattern[i]].G = s.G;
      for (j = 0; B && j < lower_led_count / sizeof(pattern); j++)
        lower_leds[j * sizeof(pattern) + pattern[i]].B = s.B;

      if (i % dither == 0) {
        ws2812b_repeat_portA0((uint8_t *)&lower_leds, 3 * lower_led_count, 1);
        delay(delay_ms);
      }
    }
  } while ((R || G || B) && !switch0);

  digitalWrite(LED_BUILTIN, LOW);
}

static void setrgb_apply(struct ws281x v, int off, int skip, bool lower, bool upper)
{
  int i;

  if (lower) {
    for (i = off; i < lower_led_count; i++) {
      lower_leds[i].G = v.G;
      lower_leds[i].R = v.R;
      lower_leds[i].B = v.B;
      i += skip;
    }
  }

  if (upper) {
    for (i = off; i < upper_led_count; i++) {
      upper_leds[i].G = v.G;
      upper_leds[i].R = v.R;
      upper_leds[i].B = v.B;
      i += skip;
    }
  }
}

static void apply_preset(int preset_nr, bool lower, bool upper)
{
  const struct ws281x_pattern *p = presets[preset_nr];
  uint8_t len;
  unsigned i;

  for (i = 0; lower && i < lower_led_count; i++)
    lower_leds[i] = p->v[i % p->len];

  for (i = 0; upper && i < upper_led_count; i++)
    upper_leds[i] = p->v[i % p->len];
}

/*
 * Menu screens
 */

enum {
  SWITCH_OFF,
  SWITCH_ONCE = 127,
};

enum {
  FRONTPAGE,
  SETRGB,
  SETINDRGB,
  SETCURVE,

  NR_SCREENS,
};

static const PROGMEM char screen_frontpage[ssd1306_nr_pages][ssd1306_chars + 1] = {
  "   Office LED Controller v0.3   ",
  "                                ",
  "                                ",
  "    This page intentionally     ",
  "           left blank           ",
  "                                ",
  "                                ",
  "Flashed " __TIMESTAMP__,
};

static struct ws281x global_led;
static volatile uint8_t global_skip;
static volatile uint8_t global_off;
static volatile uint8_t lower_switch;
static volatile uint8_t upper_switch;
static const PROGMEM char screen_setrgb[ssd1306_nr_pages][ssd1306_chars + 1] = {
  "SET GLOBAL RGB                  ",
  "                                ",
  "  RGB:   0  /0  /0   Skip: 0    ",
  "  Upper:  NO          Off: 0    ",
  "  Lower:  NO                    ",
  "                                ",
  "                                ",
  " [CLEAR]                        ",
};

static volatile uint8_t global_strand;
static volatile uint8_t global_lednum;
static const PROGMEM char screen_setindrgb[ssd1306_nr_pages][ssd1306_chars + 1] = {
  "SET INDIVIDUAL LED RGB          ",
  "                                ",
  "                                ",
  "  Strand: LOWER                 ",
  "     LED:   0                   ",
  "     RGB:   0  /0  /0           ",
  "                                ",
  "                                ",
};

static volatile uint8_t global_delay_ms;
static volatile uint8_t global_stepping;
static volatile uint8_t global_dither;
static struct ws281x curve_start;
static struct ws281x curve_end;
static const PROGMEM char screen_brightcurve[ssd1306_nr_pages][ssd1306_chars + 1] = {
  "LOWER STRAND BRIGHTNESS CURVES  ",
  "                                ",
  "  Delay:       1ms              ",
  "  RGB Start:   0  /0  /0        ",
  "  RGB End:     0  /0  /0        ",
  "  Inc/Dither:  1  /1            ",
  "                                ",
  " [CLEAR] [SETUP] [RUN] [LOOP]   ",
};

static void run_actions(void)
{
  if (actions & ALL_LEDS_OFF) {
    memset(upper_leds, 0, upper_led_count * 3);
    memset(lower_leds, 0, lower_led_count * 3);
    preset = 0;
    actions &= ~(ALL_LEDS_OFF | CYCLE_PRESET);
  } else if (actions & CYCLE_PRESET) {
    apply_preset(preset % nr_presets, true, true);
    actions &= ~CYCLE_PRESET;
    preset++;
  }

  if (actions & APPLY_SETRGB) {
    setrgb_apply(global_led, global_off, global_skip, lower_switch,
                 upper_switch);

    if (lower_switch == SWITCH_ONCE)
      lower_switch = 0;
    if (upper_switch == SWITCH_ONCE)
      upper_switch = 0;

    actions &= ~APPLY_SETRGB;
  }

  if (actions & RUN_LOWER_BRIGHTCURVE) {
    run_lower_brightcurve(curve_start, curve_end, global_delay_ms,
                          global_stepping, global_dither);

    switch0 = false;
    actions &= ~RUN_LOWER_BRIGHTCURVE;
  }

  if (actions & LOOP_LOWER_BRIGHTCURVE) {
    do {
      struct ws281x tmp;

      run_lower_brightcurve(curve_start, curve_end, global_delay_ms,
                            global_stepping, global_dither);

      tmp = curve_start;
      curve_start = curve_end;
      curve_end = tmp;
    } while (!switch0);

    switch0 = false;
    actions &= ~LOOP_LOWER_BRIGHTCURVE;
  }
}

/*
 * Screen update logic and helpers
 */

static void update_screen(void)
{
  char *s_red, *s_grn, *s_blu, *s_off, *s_skip, *s_upper;
  char *s_lower, *s_red2, *s_grn2, *s_blu2, *s_tmp;
  char screen[ssd1306_nr_pages][ssd1306_chars + 1];
  volatile uint8_t *volatile ptr = NULL;
  struct ws281x *active;
  uint8_t this_screensel;
  int i;

  cli();
  this_screensel = screensel;
  screen_needs_update = false;
  sei();

  if (this_screensel != last_screensel) {
    last_screensel = this_screensel;
    switch0 = false;
    switch1 = false;
    fieldsel = 0;
  }

  switch (this_screensel % NR_SCREENS) {
  case FRONTPAGE:
    memcpy_P(screen, screen_frontpage, sizeof(screen_frontpage));

    if (switch0) {
      switch0 = false;
      actions |= ALL_LEDS_OFF;
    }

    if (switch1) {
      switch1 = false;
      actions |= CYCLE_PRESET;
    }

    break;

  case SETRGB:
    memcpy_P(screen, screen_setrgb, sizeof(screen));
    s_red = &screen[2][9];
    s_grn = &screen[2][13];
    s_blu = &screen[2][17];
    s_skip = &screen[2][27];
    s_upper = &screen[3][9];
    s_off = &screen[3][27];
    s_lower = &screen[4][9];

    decimal_txt(global_led.R, s_red);
    decimal_txt(global_led.G, s_grn);
    decimal_txt(global_led.B, s_blu);
    decimal_txt(global_skip, s_skip);
    decimal_txt(global_off, s_off);
    memcpy(s_upper, upper_switch & 1 ? "YES" : " NO", 3);
    memcpy(s_lower, lower_switch & 1 ? "YES" : " NO", 3);

    switch (fieldsel % 8) {
      case 0:
        s_red[0] |= HIGHLIGHT;
        s_red[1] |= HIGHLIGHT;
        s_red[2] |= HIGHLIGHT;
        ptr = &global_led.R;
        break;
      case 1:
        s_grn[0] |= HIGHLIGHT;
        s_grn[1] |= HIGHLIGHT;
        s_grn[2] |= HIGHLIGHT;
        ptr = &global_led.G;
        break;
      case 2:
        s_blu[0] |= HIGHLIGHT;
        s_blu[1] |= HIGHLIGHT;
        s_blu[2] |= HIGHLIGHT;
        ptr = &global_led.B;
        break;
      case 3:
        s_skip[0] |= HIGHLIGHT;
        s_skip[1] |= HIGHLIGHT;
        s_skip[2] |= HIGHLIGHT;
        ptr = &global_skip;
        break;
      case 4:
        s_upper[0] |= HIGHLIGHT;
        s_upper[1] |= HIGHLIGHT;
        s_upper[2] |= HIGHLIGHT;
        ptr = &upper_switch;

        if (switch0) {
          switch0 = false;
          switch1 = inputmode;
          upper_switch = SWITCH_ONCE;
          refresh_screen();
        }

        break;
      case 5:
        s_off[0] |= HIGHLIGHT;
        s_off[1] |= HIGHLIGHT;
        s_off[2] |= HIGHLIGHT;
        ptr = &global_off;
        break;
      case 6:
        s_lower[0] |= HIGHLIGHT;
        s_lower[1] |= HIGHLIGHT;
        s_lower[2] |= HIGHLIGHT;
        ptr = &lower_switch;

        if (switch0) {
          switch0 = false;
          switch1 = inputmode;
          lower_switch = SWITCH_ONCE;
          refresh_screen();
        }

        break;
      case 7:
        screen[7][2] |= HIGHLIGHT;
        screen[7][3] |= HIGHLIGHT;
        screen[7][4] |= HIGHLIGHT;
        screen[7][5] |= HIGHLIGHT;
        screen[7][6] |= HIGHLIGHT;
        ptr = &dontcare;

        if (switch0) {
          switch0 = false;
          switch1 = inputmode;
          actions |= ALL_LEDS_OFF;
          refresh_screen();
        }

        break;
    }

    actions |= APPLY_SETRGB;
    break;

  case SETINDRGB:
    memcpy_P(screen, screen_setindrgb, sizeof(screen));
    s_red = &screen[5][12];
    s_grn = &screen[5][16];
    s_blu = &screen[5][20];
    s_off = &screen[4][12];

    global_lednum = min(global_lednum, lower_led_count - 1);

    active = &lower_leds[global_lednum];
    decimal_txt(active->R, s_red);
    decimal_txt(active->G, s_grn);
    decimal_txt(active->B, s_blu);
    decimal_txt(global_lednum, s_off);

    switch (fieldsel % 4) {
      case 0:
        s_red[0] |= HIGHLIGHT;
        s_red[1] |= HIGHLIGHT;
        s_red[2] |= HIGHLIGHT;
        ptr = &active->R;
        break;
      case 1:
        s_grn[0] |= HIGHLIGHT;
        s_grn[1] |= HIGHLIGHT;
        s_grn[2] |= HIGHLIGHT;
        ptr = &active->G;
        break;
      case 2:
        s_blu[0] |= HIGHLIGHT;
        s_blu[1] |= HIGHLIGHT;
        s_blu[2] |= HIGHLIGHT;
        ptr = &active->B;
        break;
      case 3:
        s_off[0] |= HIGHLIGHT;
        s_off[1] |= HIGHLIGHT;
        s_off[2] |= HIGHLIGHT;
        ptr = &global_lednum;
        break;
    }

    /*
     * Nothing to do, IRQs will directly update settings in memory.
     */
    break;

  case SETCURVE:
    memcpy_P(screen, screen_brightcurve, sizeof(screen));
    s_red = &screen[3][15];
    s_grn = &screen[3][19];
    s_blu = &screen[3][23];
    s_red2 = &screen[4][15];
    s_grn2 = &screen[4][19];
    s_blu2 = &screen[4][23];
    s_off = &screen[2][15];
    s_skip = &screen[5][15];
    s_tmp = &screen[5][19];

    global_delay_ms = max(1, global_delay_ms);
    global_stepping = max(1, global_stepping);
    global_dither = max(1, min(15, global_dither));

    decimal_txt(curve_start.R, s_red);
    decimal_txt(curve_start.G, s_grn);
    decimal_txt(curve_start.B, s_blu);
    decimal_txt(curve_end.R, s_red2);
    decimal_txt(curve_end.G, s_grn2);
    decimal_txt(curve_end.B, s_blu2);
    decimal_txt(global_delay_ms, s_off, "ms");
    decimal_txt(global_stepping, s_skip);
    decimal_txt(global_dither, s_tmp);

    switch (fieldsel % 13) {
      case 0:
        s_off[0] |= HIGHLIGHT;
        s_off[1] |= HIGHLIGHT;
        s_off[2] |= HIGHLIGHT;
        ptr = &global_delay_ms;
        break;
      case 1:
        s_red[0] |= HIGHLIGHT;
        s_red[1] |= HIGHLIGHT;
        s_red[2] |= HIGHLIGHT;
        ptr = &curve_start.R;
        break;
      case 2:
        s_grn[0] |= HIGHLIGHT;
        s_grn[1] |= HIGHLIGHT;
        s_grn[2] |= HIGHLIGHT;
        ptr = &curve_start.G;
        break;
      case 3:
        s_blu[0] |= HIGHLIGHT;
        s_blu[1] |= HIGHLIGHT;
        s_blu[2] |= HIGHLIGHT;
        ptr = &curve_start.B;
        break;
      case 4:
        s_red2[0] |= HIGHLIGHT;
        s_red2[1] |= HIGHLIGHT;
        s_red2[2] |= HIGHLIGHT;
        ptr = &curve_end.R;
        break;
      case 5:
        s_grn2[0] |= HIGHLIGHT;
        s_grn2[1] |= HIGHLIGHT;
        s_grn2[2] |= HIGHLIGHT;
        ptr = &curve_end.G;
        break;
      case 6:
        s_blu2[0] |= HIGHLIGHT;
        s_blu2[1] |= HIGHLIGHT;
        s_blu2[2] |= HIGHLIGHT;
        ptr = &curve_end.B;
        break;
      case 7:
        s_skip[0] |= HIGHLIGHT;
        s_skip[1] |= HIGHLIGHT;
        s_skip[2] |= HIGHLIGHT;
        ptr = &global_stepping;
        break;
      case 8:
        s_tmp[0] |= HIGHLIGHT;
        s_tmp[1] |= HIGHLIGHT;
        s_tmp[2] |= HIGHLIGHT;
        ptr = &global_dither;
        break;
      case 9:
        screen[7][2] |= HIGHLIGHT;
        screen[7][3] |= HIGHLIGHT;
        screen[7][4] |= HIGHLIGHT;
        screen[7][5] |= HIGHLIGHT;
        screen[7][6] |= HIGHLIGHT;
        ptr = &dontcare;

        if (switch0) {
          switch0 = false;
          switch1 = inputmode;
          actions |= ALL_LEDS_OFF;
          refresh_screen();
        }

        break;
      case 10:
        screen[7][10] |= HIGHLIGHT;
        screen[7][11] |= HIGHLIGHT;
        screen[7][12] |= HIGHLIGHT;
        screen[7][13] |= HIGHLIGHT;
        screen[7][14] |= HIGHLIGHT;
        ptr = &dontcare;

        if (switch0) {
          switch0 = false;
          switch1 = inputmode;
          global_led = curve_start;
          global_skip = 0;
          global_off = 0;
          lower_switch = SWITCH_ONCE;
          upper_switch = 0;
          actions |= APPLY_SETRGB;
          refresh_screen();
        }

        break;
      case 11:
        screen[7][18] |= HIGHLIGHT;
        screen[7][19] |= HIGHLIGHT;
        screen[7][20] |= HIGHLIGHT;
        ptr = &dontcare;

        if (switch0) {
          switch0 = false;
          switch1 = inputmode;
          actions |= RUN_LOWER_BRIGHTCURVE;
          refresh_screen();
        }

        break;
      case 12:
        screen[7][24] |= HIGHLIGHT;
        screen[7][25] |= HIGHLIGHT;
        screen[7][26] |= HIGHLIGHT;
        screen[7][27] |= HIGHLIGHT;
        ptr = &dontcare;

        if (switch0) {
          switch0 = false;
          switch1 = inputmode;
          actions |= LOOP_LOWER_BRIGHTCURVE;
          refresh_screen();
        }

        break;
    }

    break;
  }

  /*
   * Enter/exit input mode.
   */

  if (ptr && switch1) {
    cli();

    switch0 = false;
    switch1 = false;
    if (!inputmode) {
      inputmode = true;
      knob0 = ptr;
      k0_mult = 10;
      knob1 = ptr;
    } else {
      inputmode = false;
      knob0 = &screensel;
      k0_mult = 1;
      knob1 = &fieldsel;
    }

    sei();
  }

  /*
   * Write new screen content.
   */
  for (i = 0; i < ssd1306_nr_pages; i++)
    ssd1306_text(i, &screen[i][0]);
}

/*
 * Main logic
 */

void setup(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(gpio_led_0, OUTPUT);
  pinMode(gpio_led_1, OUTPUT);
  digitalWrite(gpio_led_0, LOW);
  digitalWrite(gpio_led_1, LOW);

  pinMode(gpio_k0_a, INPUT_PULLUP);
  pinMode(gpio_k0_b, INPUT_PULLUP);
  pinMode(gpio_k0_s, INPUT_PULLUP);
  k0_irq_a();
  attachInterrupt(digitalPinToInterrupt(gpio_k0_a), k0_irq_a, CHANGE);
  attachInterrupt(digitalPinToInterrupt(gpio_k0_b), k0_irq_b, CHANGE);
  attachInterrupt(digitalPinToInterrupt(gpio_k0_s), k0_irq_s, FALLING);

  pinMode(gpio_k1_a, INPUT_PULLUP);
  pinMode(gpio_k1_b, INPUT_PULLUP);
  pinMode(gpio_k1_s, INPUT_PULLUP);
  k1_irq_a();
  attachInterrupt(digitalPinToInterrupt(gpio_k1_a), k1_irq_a, CHANGE);
  attachInterrupt(digitalPinToInterrupt(gpio_k1_b), k1_irq_b, CHANGE);
  attachInterrupt(digitalPinToInterrupt(gpio_k1_s), k1_irq_s, FALLING);

  Wire.begin();
  Wire.setClock(400000);
  ssd1306_init_3v();
  ssd1306_clear();

  cli();
  knob0 = &screensel;
  knob1 = &fieldsel;
  refresh_screen();
  sei();
}

void loop(void)
{
  bool refresh;

  cli();
  refresh = screen_needs_update;
  screen_needs_update = false;
  sei();

  if (refresh)
    update_screen();

  run_actions();

  ws2812b_repeat_portA0((uint8_t *)&lower_leds, 3 * lower_led_count, 1);
  ws2812b_repeat_portC6((uint8_t *)&upper_leds, 3 * upper_led_count, 1);
  delay(1);
}
