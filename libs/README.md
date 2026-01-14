# null Language Libraries

Reusable library modules for the null programming language.

## Available Libraries

| Library | Description |
|---------|-------------|
| `string/string.null` | String manipulation and character utilities |
| `random/random.null` | Random number generation (LCG-based) |
| `format/format.null` | Output formatting (padding, progress bars) |
| `color/color.null` | ANSI terminal colors and cursor control |

## Usage

Import libraries using the `@use` directive:

```null
@use "libs/string/string.null"
@use "libs/random/random.null"
@use "libs/format/format.null"
@use "libs/color/color.null"
```

## Library Reference

### string/string.null

Character and string utilities:
- `str_len(s)` - Get string length
- `print_char_n(c, n)` - Print character n times
- `print_spaces(n)` - Print n spaces
- `print_line(width)` - Print horizontal line of dashes
- `print_box_line(width)` - Print box border (+---+)
- `is_digit(c)`, `is_alpha(c)`, `is_alnum(c)` - Character tests
- `to_upper(c)`, `to_lower(c)` - Case conversion

### random/random.null

Random number generation:
- `rand_seed_time()` - Seed from system time
- `rand_seed(n)` - Set specific seed
- `rand()` - Get next random number
- `rand_max(max)` - Random in [0, max)
- `rand_range(min, max)` - Random in [min, max]
- `rand_bool()` - Random boolean
- `roll_die(sides)`, `roll_d6()`, `roll_d20()` - Dice rolls
- `flip_coin()` - Coin flip

### format/format.null

Output formatting:
- `fmt_int(n)` - Print integer
- `fmt_int_width(n, w)` - Right-aligned with width
- `fmt_int_left(n, w)` - Left-aligned with width
- `fmt_int_zero(n, w)` - Zero-padded
- `fmt_percent(n)` - Print with % sign
- `fmt_progress(current, total, width)` - Progress bar
- `count_digits(n)` - Count decimal digits

### color/color.null

ANSI terminal colors:
- `color_reset()` - Reset all formatting
- `color_bold()`, `color_dim()`, `color_underline()` - Text styles
- `color_red()`, `color_green()`, `color_blue()`, etc. - Foreground colors
- `color_bright_*()` - Bright/bold colors
- `bg_red()`, `bg_green()`, etc. - Background colors
- `print_red(s)`, `print_green(s)`, etc. - Print colored and reset
- `clear_screen()`, `cursor_home()` - Screen control
- `cursor_hide()`, `cursor_show()` - Cursor visibility
