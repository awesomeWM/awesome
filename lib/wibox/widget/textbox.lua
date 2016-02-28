---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @author dodo
-- @copyright 2010, 2011 Uli Schlachter, dodo
-- @release @AWESOME_VERSION@
-- @classmod wibox.widget.textbox
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local gdebug = require("gears.debug")
local beautiful = require("beautiful")
local lgi = require("lgi")
local Pango = lgi.Pango
local PangoCairo = lgi.PangoCairo
local type = type
local setmetatable = setmetatable
local pairs = pairs

local textbox = { mt = {} }

--- Set the DPI of a Pango layout
local function setup_dpi(box, dpi)
    if box.dpi ~= dpi then
        box.dpi = dpi
        box._ctx:set_resolution(dpi)
        box._layout:context_changed()
    end
end

--- Setup a pango layout for the given textbox and dpi
local function setup_layout(box, width, height, dpi)
    box._layout.width = Pango.units_from_double(width)
    box._layout.height = Pango.units_from_double(height)
    setup_dpi(box, dpi)
end

--- Draw the given textbox on the given cairo context in the given geometry
function textbox:draw(context, cr, width, height)
    setup_layout(self, width, height, context.dpi)
    cr:update_layout(self._layout)
    local _, logical = self._layout:get_pixel_extents()
    local offset = 0
    if self._valign == "center" then
        offset = (height - logical.height) / 2
    elseif self._valign == "bottom" then
        offset = height - logical.height
    end
    cr:move_to(0, offset)
    cr:show_layout(self._layout)
end

local function do_fit_return(self)
    local _, logical = self._layout:get_pixel_extents()
    if logical.width == 0 or logical.height == 0 then
        return 0, 0
    end
    return logical.width, logical.height
end

--- Fit the given textbox
function textbox:fit(context, width, height)
    setup_layout(self, width, height, context.dpi)
    return do_fit_return(self)
end

--- Get the preferred size of a textbox.
-- This returns the size that the textbox would use if infinite space were
-- available.
-- @tparam integer|screen s The screen on which the textbox will be displayed.
-- @treturn number The preferred width.
-- @treturn number The preferred height.
function textbox:get_preferred_size(s)
    return self:get_preferred_size_at_dpi(beautiful.xresources.get_dpi(s))
end

--- Get the preferred height of a textbox at a given width.
-- This returns the height that the textbox would use when it is limited to the
-- given width.
-- @tparam number width The available width.
-- @tparam integer|screen s The screen on which the textbox will be displayed.
-- @treturn number The needed height.
function textbox:get_height_for_width(width, s)
    return self:get_height_for_width_at_dpi(width, beautiful.xresources.get_dpi(s))
end

--- Get the preferred size of a textbox.
-- This returns the size that the textbox would use if infinite space were
-- available.
-- @tparam number dpi The DPI value to render at.
-- @treturn number The preferred width.
-- @treturn number The preferred height.
function textbox:get_preferred_size_at_dpi(dpi)
    local max_lines = 2^20
    setup_dpi(self, dpi)
    self._layout.width = -1 -- no width set
    self._layout.height = -max_lines -- show this many lines per paragraph
    return do_fit_return(self)
end

--- Get the preferred height of a textbox at a given width.
-- This returns the height that the textbox would use when it is limited to the
-- given width.
-- @tparam number width The available width.
-- @tparam number dpi The DPI value to render at.
-- @treturn number The needed height.
function textbox:get_height_for_width_at_dpi(width, dpi)
    local max_lines = 2^20
    setup_dpi(self, dpi)
    self._layout.width = Pango.units_from_double(width)
    self._layout.height = -max_lines -- show this many lines per paragraph
    local _, h = do_fit_return(self)
    return h
end

--- Set the text of the textbox (with
-- [Pango markup](https://developer.gnome.org/pango/stable/PangoMarkupFormat.html)).
-- @tparam string text The text to set. This can contain pango markup (e.g.
--   `<b>bold</b>`). You can use `awful.util.escape` to escape
--   parts of it.
-- @treturn[1] boolean true
-- @treturn[2] boolean false
-- @treturn[2] string Error message explaining why the markup was invalid.
function textbox:set_markup_silently(text)
    if self._markup == text then
        return true
    end

    local attr, parsed = Pango.parse_markup(text, -1, 0)
    -- In case of error, attr is false and parsed is a GLib.Error instance.
    if not attr then
        return false, parsed.message or tostring(parsed)
    end

    self._markup = text
    self._layout.text = parsed
    self._layout.attributes = attr
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
    return true
end

--- Set the text of the textbox (with
-- [Pango markup](https://developer.gnome.org/pango/stable/PangoMarkupFormat.html)).
-- @tparam string text The text to set. This can contain pango markup (e.g.
--   `<b>bold</b>`). You can use `awful.util.escape` to escape
--   parts of it.
function textbox:set_markup(text)
    local success, message = self:set_markup_silently(text)
    if not success then
        gdebug.print_error(message)
    end
end

--- Set a textbox' text.
-- @param text The text to display. Pango markup is ignored and shown as-is.
function textbox:set_text(text)
    if self._layout.text == text and self._layout.attributes == nil then
        return
    end
    self._markup = nil
    self._layout.text = text
    self._layout.attributes = nil
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
end

--- Set a textbox' ellipsize mode.
-- @param mode Where should long lines be shortened? "start", "middle" or "end"
function textbox:set_ellipsize(mode)
    local allowed = { none = "NONE", start = "START", middle = "MIDDLE", ["end"] = "END" }
    if allowed[mode] then
        if self._layout:get_ellipsize() == allowed[mode] then
            return
        end
        self._layout:set_ellipsize(allowed[mode])
        self:emit_signal("widget::redraw_needed")
        self:emit_signal("widget::layout_changed")
    end
end

--- Set a textbox' wrap mode.
-- @param mode Where to wrap? After "word", "char" or "word_char"
function textbox:set_wrap(mode)
    local allowed = { word = "WORD", char = "CHAR", word_char = "WORD_CHAR" }
    if allowed[mode] then
        if self._layout:get_wrap() == allowed[mode] then
            return
        end
        self._layout:set_wrap(allowed[mode])
        self:emit_signal("widget::redraw_needed")
        self:emit_signal("widget::layout_changed")
    end
end

--- Set a textbox' vertical alignment
-- @param mode Where should the textbox be drawn? "top", "center" or "bottom"
function textbox:set_valign(mode)
    local allowed = { top = true, center = true, bottom = true }
    if allowed[mode] then
        if self._valign == mode then
            return
        end
        self._valign = mode
        self:emit_signal("widget::redraw_needed")
        self:emit_signal("widget::layout_changed")
    end
end

--- Set a textbox' horizontal alignment
-- @param mode Where should the textbox be drawn? "left", "center" or "right"
function textbox:set_align(mode)
    local allowed = { left = "LEFT", center = "CENTER", right = "RIGHT" }
    if allowed[mode] then
        if self._layout:get_alignment() == allowed[mode] then
            return
        end
        self._layout:set_alignment(allowed[mode])
        self:emit_signal("widget::redraw_needed")
        self:emit_signal("widget::layout_changed")
    end
end

--- Set a textbox' font
-- @param font The font description as string
function textbox:set_font(font)
    self._layout:set_font_description(beautiful.get_font(font))
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
end

-- Returns a new textbox
local function new(text, ignore_markup)
    local ret = base.make_widget()

    for k, v in pairs(textbox) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret._dpi = -1
    ret._ctx = PangoCairo.font_map_get_default():create_context()
    ret._layout = Pango.Layout.new(ret._ctx)

    ret:set_ellipsize("end")
    ret:set_wrap("word_char")
    ret:set_valign("center")
    ret:set_align("left")
    ret:set_font(beautiful and beautiful.font)

    if text then
        if ignore_markup then
            ret:set_text(text)
        else
            ret:set_markup(text)
        end
    end

    return ret
end

function textbox.mt.__call(_, ...)
    return new(...)
end

return setmetatable(textbox, textbox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
