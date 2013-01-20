---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @author dodo
-- @copyright 2010, 2011 Uli Schlachter, dodo
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local beautiful = require("beautiful")
local lgi = require("lgi")
local cairo = lgi.cairo
local Pango = lgi.Pango
local PangoCairo = lgi.PangoCairo
local type = type
local setmetatable = setmetatable
local pairs = pairs
local error = error

-- wibox.widget.textbox
local textbox = { mt = {} }

-- Setup a pango layout for the given textbox and cairo context
local function setup_layout(box, width, height)
    local layout = box._layout
    layout.width = Pango.units_from_double(width)
    layout.height = Pango.units_from_double(height)
end

--- Draw the given textbox on the given cairo context in the given geometry
function textbox:draw(wibox, cr, width, height)
    cr:update_layout(self._layout)
    setup_layout(self, width, height)
    local ink, logical = self._layout:get_pixel_extents()
    local offset = 0
    if self._valign == "center" then
        offset = (height - logical.height) / 2
    elseif self._valign == "bottom" then
        offset = height - logical.height
    end
    cr:move_to(0, offset)
    cr:show_layout(self._layout)
end

--- Fit the given textbox
function textbox:fit(width, height)
    setup_layout(self, width, height)
    local ink, logical = self._layout:get_pixel_extents()

    if logical.width == 0 or logical.height == 0 then
        return 0, 0
    end

    return logical.width, logical.height
end

--- Set a textbox' text.
-- @param text The text to set. This can contain pango markup (e.g. <b>bold</b>)
function textbox:set_markup(text)
    local attr, parsed = Pango.parse_markup(text, -1, 0)
    -- In case of error, attr is false and parsed is an error message
    if not attr then error(parsed) end

    self._layout.text = parsed
    self._layout.attributes = attr
    self:emit_signal("widget::updated")
end

--- Set a textbox' text.
-- @param text The text to display. Pango markup is ignored and shown as-is.
function textbox:set_text(text)
    self._layout.text = text
    self._layout.attributes = nil
    self:emit_signal("widget::updated")
end

--- Set a textbox' ellipsize mode.
-- @param mode Where should long lines be shortened? "start", "middle" or "end"
function textbox:set_ellipsize(mode)
    local allowed = { none = "NONE", start = "START", middle = "MIDDLE", ["end"] = "END" }
    if allowed[mode] then
        self._layout:set_ellipsize(allowed[mode])
        self:emit_signal("widget::updated")
    end
end

--- Set a textbox' wrap mode.
-- @param mode Where to wrap? After "word", "char" or "word_char"
function textbox:set_wrap(mode)
    local allowed = { word = "WORD", char = "CHAR", word_char = "WORD_CHAR" }
    if allowed[mode] then
        self._layout:set_wrap(allowed[mode])
        self:emit_signal("widget::updated")
    end
end

--- Set a textbox' vertical alignment
-- @param mode Where should the textbox be drawn? "top", "center" or "bottom"
function textbox:set_valign(mode)
    local allowed = { top = true, center = true, bottom = true }
    if allowed[mode] then
        self._valign = mode
        self:emit_signal("widget::updated")
    end
end

--- Set a textbox' horizontal alignment
-- @param mode Where should the textbox be drawn? "left", "center" or "right"
function textbox:set_align(mode)
    local allowed = { left = "LEFT", center = "CENTER", right = "RIGHT" }
    if allowed[mode] then
        self._layout:set_alignment(allowed[mode])
        self:emit_signal("widget::updated")
    end
end

--- Set a textbox' font
-- @param font The font description as string
function textbox:set_font(font)
    self._layout:set_font_description(beautiful.get_font(font))
end

-- Returns a new textbox
local function new(text, ignore_markup)
    local ret = base.make_widget()

    for k, v in pairs(textbox) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    local ctx = PangoCairo.font_map_get_default():create_context()
    ret._layout = Pango.Layout.new(ctx)

    ret:set_ellipsize("end")
    ret:set_wrap("word_char")
    ret:set_valign("center")
    ret:set_align("left")
    ret:set_font()

    if text then
        if ignore_markup then
            ret:set_text(text)
        else
            ret:set_markup(text)
        end
    end

    return ret
end

function textbox.mt:__call(...)
    return new(...)
end

return setmetatable(textbox, textbox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
