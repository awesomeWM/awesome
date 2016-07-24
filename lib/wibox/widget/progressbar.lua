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
local shape = require("gears.shape")

local progressbar = { mt = {} }

--- The progressbar border color.
-- If the value is nil, no border will be drawn.
--
-- @property border_color
-- @tparam gears.color color The border color to set.

--- The progressbar border width.
-- @property border_width

--- The progressbar inner border color.
-- If the value is nil, no border will be drawn.
--
-- @property bar_border_color
-- @tparam gears.color color The border color to set.

--- The progressbar inner border width.
-- @property bar_border_width

--- The progressbar foreground color.
--
-- @property color
-- @tparam gears.color color The progressbar color.

--- The progressbar background color.
--
-- @property background_color
-- @tparam gears.color color The progressbar background color.

--- The progressbar inner shape.
-- @property bar_shape
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape

--- The progressbar shape.
-- @property shape
-- @tparam[opt=gears.shape.rectangle] gears.shape shape
-- @see gears.shape

--- Set the progressbar to draw vertically.
-- This doesn't do anything anymore, use a `wibox.container.rotate` widget.
-- @deprecated set_vertical
-- @tparam boolean vertical

--- Force the inner part (the bar) to fit in the background shape.
--
-- @property clip
-- @tparam[opt=true] boolean clip

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

--- The progressbar shape.
-- @beautiful beautiful.progressbar_shape
-- @see gears.shape

--- The progressbar border color.
-- @beautiful beautiful.progressbar_border_color

--- The progressbar outer border width.
-- @beautiful beautiful.progressbar_border_width

--- The progressbar inner shape.
-- @beautiful beautiful.progressbar_bar_shape
-- @see gears.shape

--- The progressbar bar border width.
-- @beautiful beautiful.progressbar_bar_border_width

--- The progressbar bar border color.
-- @beautiful beautiful.progressbar_bar_border_color

local properties = { "border_color", "color"     , "background_color",
                     "value"       , "max_value" , "ticks",
                     "ticks_gap"   , "ticks_size", "border_width",
                     "shape"       , "bar_shape" , "bar_border_width",
                     "clip"        , "bar_border_color"
                   }

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
    local border_width = pbar._private.border_width
        or beautiful.progressbar_border_width or 0

    local bcol = pbar._private.border_color or beautiful.progressbar_border_color

    border_width = bcol and border_width or 0

    local bg = pbar._private.background_color or
        beautiful.progressbar_bg or "#ff0000aa"

    local bg_width, bg_height = width, height

    -- Draw the background shape
    if border_width > 0 then
        -- Cairo draw half of the border outside of the path area
        cr:translate(border_width/2, border_width/2)
        bg_width, bg_height = bg_width - border_width, bg_height - border_width
        cr:set_line_width(border_width)
    end

    local background_shape = pbar._private.shape or
        beautiful.progressbar_shape or shape.rectangle

    background_shape(cr, bg_width, bg_height)

    cr:set_source(color(bg))

    if border_width > 0 then
        cr:fill_preserve()

        -- Draw the border
        cr:set_source(color(bcol))

        cr:stroke()

        over_drawn_width  = width  - 2*border_width
        over_drawn_height = height - 2*border_width
    else
        cr:fill()
    end

    -- Undo the translation
    if border_width > 0 then
        cr:translate(-border_width/2, -border_width/2)
    end

    local rel_x = over_drawn_width * value

    cr:translate(border_width, border_width)

    -- Make sure the bar stay in the shape
    if pbar._private.clip ~= false and beautiful.progressbar_clip ~= false then
        background_shape(cr, over_drawn_width, over_drawn_height)
        cr:clip()
    end

    -- Draw the progressbar shape

    local bar_shape = pbar._private.bar_shape or
        beautiful.progressbar_bar_shape or shape.rectangle

    local bar_border_width = pbar._private.bar_border_width or
        beautiful.progressbar_bar_border_width or pbar._private.border_width or
        beautiful.progressbar_border_width or 0

    local bar_border_color = pbar._private.bar_border_color or
        beautiful.progressbar_bar_border_color

    bar_border_width = bar_border_color and bar_border_width or 0

    over_drawn_width  = over_drawn_width  - bar_border_width
    over_drawn_height = over_drawn_height - bar_border_width
    cr:translate(bar_border_width/2, bar_border_width/2)

    bar_shape(cr, rel_x, over_drawn_height)

    cr:set_source(color(pbar._private.color or beautiful.progressbar_fg or "#ff0000"))

    if bar_border_width > 0 then
        cr:fill_preserve()
        cr:set_source(color(bar_border_color))
        cr:set_line_width(bar_border_width)
        cr:stroke()
    else
        cr:fill()
    end

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
