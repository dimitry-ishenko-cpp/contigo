////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Dimitry Ishenko
// Contact: dimitry (dot) ishenko (at) (gee) mail (dot) com
//
// Distributed under the GNU GPL license. See the LICENSE.md file for details.

////////////////////////////////////////////////////////////////////////////////
#include "logging.hpp"
#include "vte.hpp"

#include <optional>
#include <tuple>
#include <utility> // std::swap
#include <variant>

////////////////////////////////////////////////////////////////////////////////
namespace vte
{

struct machine::dispatch
{

static int damage(VTermRect rect, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);
    if (vt->row_cb_)
    {
        auto col = rect.start_col, cols = rect.end_col - rect.start_col;
        for (auto row = rect.start_row; row < rect.end_row; ++row) vt->row_cb_(row, col, cols);
    }
    return true;
}

static int move_cursor(VTermPos pos, VTermPos old_pos, int visible, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);
    if (vt->move_cb_) vt->move_cb_(pos.row, pos.col);
    return true;
}

static constexpr enum cursor::shape to_shape[] =
{
    cursor::block, // [?]
    cursor::block, // [VTERM_PROP_CURSORSHAPE_BLOCK]
    cursor::hline, // [VTERM_PROP_CURSORSHAPE_UNDERLINE]
    cursor::vline, // [VTERM_PROP_CURSORSHAPE_BAR_LEFT]
};

static int set_prop(VTermProp prop, VTermValue* val, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);

    switch (prop)
    {
    case VTERM_PROP_CURSORBLINK:
        vt->cursor_.blink = val->boolean;
        break;
    case VTERM_PROP_CURSORSHAPE:
        vt->cursor_.shape = to_shape[val->number];
        break;
    case VTERM_PROP_CURSORVISIBLE:
        vt->cursor_.visible = val->boolean;
        break;
    default: vt = nullptr;
    }

    if (vt && vt->cursor_cb_) vt->cursor_cb_(vt->cursor_);
    return true;
}

static int bell(void* ctx)
{
    auto vt = static_cast<machine*>(ctx);
    // TODO
    return true;
}

static int resize(int rows, int cols, void* ctx)
{
    auto vt = static_cast<machine*>(ctx);
    if (vt->size_cb_) vt->size_cb_(rows, cols);
    return true;
}

static void output(const char* data, std::size_t size, void *ctx)
{
    auto vt = static_cast<machine*>(ctx);
    if (vt->send_cb_) vt->send_cb_(std::span{data, size});
}

};

////////////////////////////////////////////////////////////////////////////////
machine::machine(unsigned rows, unsigned cols) :
    vterm_{vterm_new(rows, cols), &vterm_free},
    screen_{vterm_obtain_screen(&*vterm_)}, state_{vterm_obtain_state(&*vterm_)}
{
    info() << "Virtual terminal size: " << rows << "x" << cols;

    vterm_set_utf8(&*vterm_, true);
    vterm_output_set_callback(&*vterm_, &dispatch::output, this);

    static const VTermScreenCallbacks callbacks
    {
        .damage      = dispatch::damage,
        .movecursor  = dispatch::move_cursor,
        .settermprop = dispatch::set_prop,
        .bell        = dispatch::bell,
        .resize      = dispatch::resize,
    };

    vterm_screen_set_callbacks(screen_, &callbacks, this);
    vterm_screen_set_damage_merge(screen_, VTERM_DAMAGE_SCROLL);

    vterm_screen_enable_reflow(screen_, true);
    vterm_screen_enable_altscreen(screen_, true);

    vterm_screen_reset(screen_, true);
}

namespace
{

using code_point = std::uint32_t;
using key = VTermKey;
using mod = VTermModifier;
using key_press = std::tuple<std::variant<code_point, key>, mod>;

constexpr auto operator|(mod x, mod y) { return static_cast<mod>(static_cast<int>(x) | static_cast<int>(y)); }
constexpr auto fn_key(unsigned n) { return static_cast<key>(VTERM_KEY_FUNCTION(n)); }

std::optional<key_press> parse(std::span<const char> data)
{
    mod mod = VTERM_MOD_NONE;

    auto ci = data.begin(), end = data.end();
    if (ci == data.end()) return {};

    char8_t c0 = *ci++;
    if (c0 == 0x1b)
    {
        if (ci == end) return key_press{VTERM_KEY_ESCAPE, mod};

        char8_t c1 = *ci++;
        if (ci == end)
        {
            mod = mod | VTERM_MOD_ALT;
            c0 = c1;
        }
        else if (c1 == '[')
        {
            char8_t c2 = *ci++;
            switch (c2)
            {
            case 'A': return key_press{VTERM_KEY_UP   , mod};
            case 'B': return key_press{VTERM_KEY_DOWN , mod};
            case 'C': return key_press{VTERM_KEY_RIGHT, mod};
            case 'D': return key_press{VTERM_KEY_LEFT , mod};
            case 'F': return key_press{VTERM_KEY_END  , mod};
            case 'G': return key_press{VTERM_KEY_KP_5 , mod};
            case 'H': return key_press{VTERM_KEY_HOME , mod};

            case '[':
                if (ci != end)
                {
                    char8_t c3 = *ci++;
                    if (c3 >= 'A' && c3 <= 'E') return key_press{fn_key(c3 - 'A' + 1), mod}; // f1-f5
                }
                break;

            default:
                if (ci != end)
                {
                    char8_t c3 = *ci++;
                    if (c3 == '~')
                    {
                        switch (c2)
                        {
                        case '1': return key_press{VTERM_KEY_HOME, mod};
                        case '2': return key_press{VTERM_KEY_INS, mod};
                        case '3': return key_press{VTERM_KEY_DEL, mod};
                        case '4': return key_press{VTERM_KEY_END, mod};
                        case '5': return key_press{VTERM_KEY_PAGEUP, mod};
                        case '6': return key_press{VTERM_KEY_PAGEDOWN, mod};
                        }
                    }
                    else if (ci != end)
                    {
                        char8_t c4 = *ci++;
                        if (c2 >= '0' && c2 <= '9' && c3 >= '0' && c3 <= '9' && c4 == '~')
                        {
                            auto code = (c2 - '0') * 10 + (c3 - '0');
                            switch (code)
                            {
                            case 17: case 18: case 19: case 20: case 21:
                                return key_press{fn_key(code - 11), mod}; // f6-f10

                            case 23: case 24:
                                return key_press{fn_key(code - 12), mod}; // f11-f12

                            case 25: case 26:
                                mod = mod | VTERM_MOD_SHIFT;
                                return key_press{fn_key(code - 24), mod}; // shift+f1-f2

                            case 28: case 29:
                                mod = mod | VTERM_MOD_SHIFT;
                                return key_press{fn_key(code - 25), mod}; // shift+f3-f4

                            case 31: case 32: case 33: case 34:
                                mod = mod | VTERM_MOD_SHIFT;
                                return key_press{fn_key(code - 26), mod}; // shift+f5-f8
                            }
                        }
                    }
                }
            }
            return {};
        }
    }

    switch (c0)
    {
    case 0x08:
    case 0x7f:
        return key_press{VTERM_KEY_BACKSPACE, mod};

    case 0x09:
        return key_press{VTERM_KEY_TAB, mod};

    case 0x0d:
        return key_press{VTERM_KEY_ENTER, mod};

    case 0x1b:
        return key_press{VTERM_KEY_ESCAPE, mod};

    default:
        {
            code_point cp;

            if (c0 >= 1 && c0 <= 0x1a) // ctrl+?
            {
                cp = c0 - 1 + 'a';
                mod = mod | VTERM_MOD_CTRL;
            }
            else if (c0 >= 0x80) // utf8
            {
                int n;

                     if ((c0 & 0xe0) == 0xc0) n = 1, cp = c0 & 0x1f; // 110xxxxx 10xxxxxx
                else if ((c0 & 0xf0) == 0xe0) n = 2, cp = c0 & 0x0f; // 1110xxxx 10xxxxxx 10xxxxxx
                else if ((c0 & 0xf8) == 0xf0) n = 3, cp = c0 & 0x07; // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                else return {}; // ignore

                if ((end - ci) < n) return {}; // ignore

                for (; n; --n)
                {
                    c0 = *ci++;
                    if ((c0 & 0xc0) == 0x80)
                        cp = (cp << 6) | (c0 & 0x3f);
                    else return {}; // ignore
                }
            }
            else cp = c0;

            return key_press{cp, mod};
        }
    }
}

}

void machine::recv(std::span<const char> data) { vterm_input_write(&*vterm_, data.data(), data.size()); }

void machine::send(std::span<const char> data)
{
    if (auto key_press = parse(data))
    {
        auto [val, mod] = *key_press;
        if (std::holds_alternative<code_point>(val))
        {
            auto cp = std::get<vte::code_point>(val);
            vterm_keyboard_unichar(&*vterm_, cp, mod);
        }
        else
        {
            auto key = std::get<vte::key>(val);
            vterm_keyboard_key(&*vterm_, key, mod);
        }
    }
}

void machine::commit() { vterm_screen_flush_damage(screen_); }

std::vector<vte::cell> machine::cells(int row, int col, unsigned count)
{
    std::vector<vte::cell> cells;
    cells.reserve(count);

    for (; count; ++col, --count) cells.push_back(cell(row, col));

    return cells;
}

void machine::resize(unsigned rows, unsigned cols)
{
    info() << "Resizing vte to: " << rows << "x" << cols;
    vterm_set_size(&*vterm_, rows, cols);
}

void machine::move_mouse(int row, int col) { vterm_mouse_move(&*vterm_, row, col, VTERM_MOD_NONE); }
void machine::change(vte::button button, bool state) { vterm_mouse_button(&*vterm_, button, state, VTERM_MOD_NONE); }

namespace
{

void ucs4_to_utf8(const code_point* in, char* out, std::size_t* len)
{
    auto begin = out;
    for (; *in; ++in)
    {
        auto cp = *in;
        if (cp <= 0x7f)
        {
            *out++ = cp;
        }
        else if (cp <= 0x7ff)
        {
            *out++ = 0xc0 | (cp >>  6);
            *out++ = 0x80 | (cp & 0x3f);
        }
        else if (cp <= 0xffff)
        {
            *out++ = 0xe0 | ( cp >> 12);
            *out++ = 0x80 | ((cp >>  6) & 0x3f);
            *out++ = 0x80 | ( cp & 0x3f);
        }
        else if (cp <= 0x10ffff)
        {
            *out++ = 0xf0 | ( cp >> 18);
            *out++ = 0x80 | ((cp >> 12) & 0x3f);
            *out++ = 0x80 | ((cp >>  6) & 0x3f);
            *out++ = 0x80 | ( cp & 0x3f);
        }
    }
    *out = '\0';
    *len = out - begin;
}

auto to_color(VTermState* state, VTermColor vc)
{
    vterm_state_convert_color_to_rgb(state, &vc);
    return pixman::color(vc.rgb.red << 8, vc.rgb.green << 8, vc.rgb.blue << 8, 0xffff);
}

}

vte::cell machine::cell(int row, int col)
{
    vte::cell cell;

    VTermScreenCell vtc;
    if (vterm_screen_get_cell(screen_, VTermPos{row, col}, &vtc))
    {
        ucs4_to_utf8(vtc.chars, cell.chars, &cell.len);
        cell.width = vtc.width;
        cell.attrs = vtc.attrs;

        cell.fg = to_color(state_, vtc.fg);
        cell.bg = to_color(state_, vtc.bg);
        if (cell.attrs.reverse) std::swap(cell.fg, cell.bg);
    }

    return cell;
}

////////////////////////////////////////////////////////////////////////////////
}
