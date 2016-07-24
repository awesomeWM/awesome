---------------------------------------------------------------------------
--- A progressbar widget.
--
-- By default, this widget will take all the available size. To prevent this,
-- a `wibox.container.constraint` widget or the `forced_width`/`forced_height`
-- properties have to be used.
--
--@DOC_wibox_widget_defaults_progressbar_EXAMPLE@
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @classmod wibox.widget.progressbar
---------------------------------------------------------------------------

local setmetatable = setmetatable
local ipairs = ipairs
local math = math
local util =  require("awful.util")
local base = require("wibox.widget.base")
local color = require("gears.color")
local beautiful = require("beautiful")

local progressbar = { mt = {} }

--- The progressbar border color.
-- If the value is nil, no border will be drawn.
--
-- @property border_color
-- @tparam gears.color color The border color to set.

--- The progressbar foreground color.
--
-- @property color
-- @tparam gears.color color The progressbar color.

--- The progressbar background color.
--
-- @property background_color
-- @tparam gears.color color The progressbar background color.

--- Set the progressbar to draw vertically.
-- This doesn't do anything anymore, use a `wibox.container.rotate` widget.
-- @deprecated set_vertical
-- @tparam boolean vertical

--- The progressbar to draw ticks. Default is false.
--
-- @property ticks
-- @param boolean

--- The progressbar ticks gap.
--
-- @property ticks_gap
-- @param number

--- The progressbar ticks size.
--
-- @property ticks_size
-- @param number

--- The maximum value the progressbar should handle.
--
-- @property max_value
-- @param number

--- The progressbar background color.
-- @beautiful beautiful.progressbar_bg

--- The progressbar foreground color.
-- @beautiful beautiful.progressbar_fg

--- The progressbar border color.
-- @beautiful beautiful.progressbar_border_color

local properties = { "border_color", "color"    , "background_color",
                     "value"       , "max_value", "ticks",
                     "ticks_gap"   , "ticks_size" }

function progressbar.draw(pbar, _, cr, width, height)
    local ticks_gap = pbar._private.ticks_gap or 1
    local ticks_size = pbar._private.ticks_size or 4

    -- We want one pixel wide lines
    cr:set_line_width(1)

    local max_value = pbar._private.max_value

    local value = math.min(max_value, math.max(0, pbar._private.value))

    if value >= 0 then
        value = value / max_value
    end

    local over_drawn_width = width
    local over_drawn_height = height
    local border_width = 0
    local col = pbar._private.border_color or beautiful.progressbar_border_color
    if col then
        -- Draw border
        cr:rectangle(0.5, 0.5, width - 1, height - 1)
        cr:set_source(color(col))
        cr:stroke()

        over_drawn_width = width - 2 -- remove 2 for borders
        over_drawn_height = height - 2 -- remove 2 for borders
        border_width = 1
    end

    cr:rectangle(border_width, border_width,
                 over_drawn_width, over_drawn_height)
    cr:set_source(color(pbar._private.color or beautiful.progressbar_fg or "#ff0000"))
    cr:fill()

    -- Cover the part that is not set with a rectangle
    local rel_x = over_drawn_width * value
    cr:rectangle(border_width + rel_x,
                    border_width,
                    over_drawn_width - rel_x,
                    over_drawn_height)
    cr:set_source(color(pbar._private.background_color or "#000000aa"))
    cr:fill()

    if pbar._private.ticks then
        for i=0, width / (ticks_size+ticks_gap)-border_width do
            local rel_offset = over_drawn_width / 1 - (ticks_size+ticks_gap) * i

            if rel_offset <= rel_x then
                cr:rectangle(rel_offset,
                                border_width,
                                ticks_gap,
                                over_drawn_height)
            end
        end
        cr:set_source(color(pbar._private.background_color or "#000000aa"))
        cr:fill()
    end
end

function progressbar:fit(_, width, height)
    return width, height
end

--- Set the progressbar value.
-- @param value The progress bar value between 0 and 1.
function progressbar:set_value(value)
    value = value or 0

    self._private.value = value

    self:emit_signal("widget::redraw_needed")
    return self
end

function progressbar:set_max_value(max_value)

    self._private.max_value = max_value

    self:emit_signal("widget::redraw_needed")
end

--- Set the progressbar height.
-- This method is deprecated, it no longer do anything.
-- Use a `wibox.container.constraint` widget or `forced_height`.
-- @param height The height to set.
-- @deprecated set_height
function progressbar:set_height(height) --luacheck: no unused_args
    util.deprecate("Use a `wibox.container.constraint` widget or forced_height")
end

--- Set the progressbar width.
-- This method is deprecated, it no longer do anything.
-- Use a `wibox.container.constraint` widget or `forced_width`.
-- @param width The width to set.
-- @deprecated set_width
function progressbar:set_width(width) --luacheck: no unused_args
    util.deprecate("Use a `wibox.container.constraint` widget or forced_width")
end

-- Build properties function
for _, prop in ipairs(properties) do
    if not progressbar["set_" .. prop] then
        progressbar["set_" .. prop] = function(pbar, value)
            pbar._private[prop] = value
            pbar:emit_signal("widget::redraw_needed")
            return pbar
        end
    end
end

function progressbar:set_vertical(value) --luacheck: no unused_args
    util.deprecate("Use a `wibox.container.rotate` widget")
end


--- Create a progressbar widget.
-- @param args Standard widget() arguments. You should add width and height
-- key to set progressbar geometry.
-- @return A progressbar widget.
-- @function wibox.widget.progressbar
function progressbar.new(args)
    args = args or {}

    local pbar = base.make_widget(nil, nil, {
        enable_properties = true,
    })

    pbar._private.width     = args.width or 100
    pbar._private.height    = args.height or 20
    pbar._private.value     = 0
    pbar._private.max_value = 1

    util.table.crush(pbar, progressbar, true)

    return pbar
end

function progressbar.mt:__call(...)
    return progressbar.new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(progressbar, progressbar.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
