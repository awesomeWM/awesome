---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local setmetatable = setmetatable
local ipairs = ipairs
local math = math
local base = require("wibox.widget.base")
local color = require("gears.color")

--- A progressbar widget.
-- awful.widget.progressbar
local progressbar = { mt = {} }

local data = setmetatable({}, { __mode = "k" })

--- Set the progressbar border color.
-- If the value is nil, no border will be drawn.
-- @name set_border_color
-- @class function
-- @param progressbar The progressbar.
-- @param color The border color to set.

--- Set the progressbar foreground color.
-- @name set_color
-- @class function
-- @param progressbar The progressbar.
-- @param color The progressbar color.

--- Set the progressbar background color.
-- @name set_background_color
-- @class function
-- @param progressbar The progressbar.
-- @param color The progressbar background color.

--- Set the progressbar to draw vertically. Default is false.
-- @name set_vertical
-- @class function
-- @param progressbar The progressbar.
-- @param vertical A boolean value.

--- Set the progressbar to draw ticks. Default is false.
-- @name set_ticks
-- @class function
-- @param progressbar The progressbar.
-- @param ticks A boolean value.

--- Set the progressbar ticks gap.
-- @name set_ticks_gap
-- @class function
-- @param progressbar The progressbar.
-- @param value The value.

--- Set the progressbar ticks size.
-- @name set_ticks_size
-- @class function
-- @param progressbar The progressbar.
-- @param value The value.

--- Set the maximum value the progressbar should handle.
-- @name set_max_value
-- @class function
-- @param progressbar The progressbar.
-- @param value The value.

local properties = { "width", "height", "border_color",
                     "color", "background_color",
                     "vertical", "value", "max_value",
                     "ticks", "ticks_gap", "ticks_size" }

function progressbar.draw(pbar, wibox, cr, width, height)
    local ticks_gap = data[pbar].ticks_gap or 1
    local ticks_size = data[pbar].ticks_size or 4

    -- We want one pixel wide lines
    cr:set_line_width(1)

    local value = data[pbar].value
    local max_value = data[pbar].max_value
    if value >= 0 then
        value = value / max_value
    end

    local over_drawn_width = width
    local over_drawn_height = height
    local border_width = 0
    if data[pbar].border_color then
        -- Draw border
        cr:rectangle(0.5, 0.5, width - 1, height - 1)
        cr:set_source(color(data[pbar].border_color))
        cr:stroke()

        over_drawn_width = width - 2 -- remove 2 for borders
        over_drawn_height = height - 2 -- remove 2 for borders
        border_width = 1
    end

    cr:rectangle(border_width, border_width,
                 over_drawn_width, over_drawn_height)
    cr:set_source(color(data[pbar].color or "#ff0000"))
    cr:fill()

    -- Cover the part that is not set with a rectangle
    if data[pbar].vertical then
        local rel_height = over_drawn_height * (1 - value)
        cr:rectangle(border_width,
                     border_width,
                     over_drawn_width,
                     rel_height)
        cr:set_source(color(data[pbar].background_color or "#000000aa"))
        cr:fill()

        -- Place smaller pieces over the gradient if ticks are enabled
        if data[pbar].ticks then
            for i=0, height / (ticks_size+ticks_gap)-border_width do
                local rel_offset = over_drawn_height / 1 - (ticks_size+ticks_gap) * i

                if rel_offset >= rel_height then
                    cr:rectangle(border_width,
                                 rel_offset,
                                 over_drawn_width,
                                 ticks_gap)
                end
            end
            cr:set_source(color(data[pbar].background_color or "#000000aa"))
            cr:fill()
        end
    else
        local rel_x = over_drawn_width * value
        cr:rectangle(border_width + rel_x,
                     border_width,
                     over_drawn_width - rel_x,
                     over_drawn_height)
        cr:set_source(color(data[pbar].background_color or "#000000aa"))
        cr:fill()

        if data[pbar].ticks then
            for i=0, width / (ticks_size+ticks_gap)-border_width do
                local rel_offset = over_drawn_width / 1 - (ticks_size+ticks_gap) * i

                if rel_offset <= rel_x then
                    cr:rectangle(rel_offset,
                                 border_width,
                                 ticks_gap,
                                 over_drawn_height)
                end
            end
            cr:set_source(color(data[pbar].background_color or "#000000aa"))
            cr:fill()
        end
    end
end

function progressbar.fit(pbar, width, height)
    return data[pbar].width, data[pbar].height
end

--- Set the progressbar value.
-- @param value The progress bar value between 0 and 1.
function progressbar:set_value(value)
    local value = value or 0
    local max_value = data[self].max_value
    data[self].value = math.min(max_value, math.max(0, value))
    self:emit_signal("widget::updated")
    return self
end

--- Set the progressbar height.
-- @param height The height to set.
function progressbar:set_height(height)
    data[self].height = height
    self:emit_signal("widget::updated")
    return self
end

--- Set the progressbar width.
-- @param width The width to set.
function progressbar:set_width(width)
    data[self].width = width
    self:emit_signal("widget::updated")
    return self
end

-- Build properties function
for _, prop in ipairs(properties) do
    if not progressbar["set_" .. prop] then
        progressbar["set_" .. prop] = function(pbar, value)
            data[pbar][prop] = value
            pbar:emit_signal("widget::updated")
            return pbar
        end
    end
end

--- Create a progressbar widget.
-- @param args Standard widget() arguments. You should add width and height
-- key to set progressbar geometry.
-- @return A progressbar widget.
function progressbar.new(args)
    local args = args or {}
    local width = args.width or 100
    local height = args.height or 20

    args.type = "imagebox"

    local pbar = base.make_widget()

    data[pbar] = { width = width, height = height, value = 0, max_value = 1 }

    -- Set methods
    for _, prop in ipairs(properties) do
        pbar["set_" .. prop] = progressbar["set_" .. prop]
    end

    pbar.draw = progressbar.draw
    pbar.fit = progressbar.fit

    return pbar
end

function progressbar.mt:__call(...)
    return progressbar.new(...)
end

return setmetatable(progressbar, progressbar.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
