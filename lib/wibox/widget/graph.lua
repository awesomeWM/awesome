---------------------------------------------------------------------------
--- A graph widget.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @classmod wibox.widget.graph
---------------------------------------------------------------------------

local setmetatable = setmetatable
local ipairs = ipairs
local math = math
local table = table
local type = type
local color = require("gears.color")
local base = require("wibox.widget.base")

local graph = { mt = {} }

--- Set the graph border color.
-- If the value is nil, no border will be drawn.
--
-- @function set_border_color
-- @param graph The graph.
-- @param color The border color to set.

--- Set the graph foreground color.
--
-- @function set_color
-- @param graph The graph.
-- @param color The graph color.
-- @see gears.color.create_pattern

--- Set the graph background color.
--
-- @function set_background_color
-- @param graph The graph.
-- @param color The graph background color.

--- Set the maximum value the graph should handle.
-- If "scale" is also set, the graph never scales up below this value, but it
-- automatically scales down to make all data fit.
--
-- @function set_max_value
-- @param graph The graph.
-- @param value The value.

--- Set the graph to automatically scale its values. Default is false.
--
-- @function set_scale
-- @param graph The graph.
-- @param scale A boolean value

--- Set the graph to draw stacks. Default is false.
--
-- @function set_stack
-- @param graph The graph.
-- @param stack A boolean value.

--- Set the graph stacking colors. Order matters.
--
-- @function set_stack_colors
-- @param graph The graph.
-- @param stack_colors A table with stacking colors.

local properties = { "width", "height", "border_color", "stack",
                     "stack_colors", "color", "background_color",
                     "max_value", "scale" }

function graph.draw(_graph, _, cr, width, height)
    local max_value = _graph._data.max_value
    local values = _graph._data.values

    cr:set_line_width(1)

    -- Draw the background first
    cr:set_source(color(_graph._data.background_color or "#000000aa"))
    cr:paint()

    -- Account for the border width
    cr:save()
    if _graph._data.border_color then
        cr:translate(1, 1)
        width, height = width - 2, height - 2
    end

    -- Draw a stacked graph
    if _graph._data.stack then

        if _graph._data.scale then
            for _, v in ipairs(values) do
                for _, sv in ipairs(v) do
                    if sv > max_value then
                        max_value = sv
                    end
                end
            end
        end

        for i = 0, width do
            local rel_i = 0
            local rel_x = i + 0.5

            if _graph._data.stack_colors then
                for idx, col in ipairs(_graph._data.stack_colors) do
                    local stack_values = values[idx]
                    if stack_values and i < #stack_values then
                        local value = stack_values[#stack_values - i] + rel_i
                        cr:move_to(rel_x, height * (1 - (rel_i / max_value)))
                        cr:line_to(rel_x, height * (1 - (value / max_value)))
                        cr:set_source(color(col or "#ff0000"))
                        cr:stroke()
                        rel_i = value
                    end
                end
            end
        end
    else
        if _graph._data.scale then
            for _, v in ipairs(values) do
                if v > max_value then
                    max_value = v
                end
            end
        end

        -- Draw the background on no value
        if #values ~= 0 then
            -- Draw reverse
            for i = 0, #values - 1 do
                local value = values[#values - i]
                if value >= 0 then
                    value = value / max_value
                    cr:move_to(i + 0.5, height * (1 - value))
                    cr:line_to(i + 0.5, height)
                end
            end
            cr:set_source(color(_graph._data.color or "#ff0000"))
            cr:stroke()
        end

    end

    -- Undo the cr:translate() for the border
    cr:restore()

    -- Draw the border last so that it overlaps already drawn values
    if _graph._data.border_color then
        -- We decremented these by two above
        width, height = width + 2, height + 2

        -- Draw the border
        cr:rectangle(0.5, 0.5, width - 1, height - 1)
        cr:set_source(color(_graph._data.border_color or "#ffffff"))
        cr:stroke()
    end
end

function graph.fit(_graph)
    return _graph._data.width, _graph._data.height
end

--- Add a value to the graph
--
-- @param value The value to be added to the graph
-- @param group The stack color group index.
function graph:add_value(value, group)
    value = value or 0
    local values = self._data.values
    local max_value = self._data.max_value
    value = math.max(0, value)
    if not self._data.scale then
        value = math.min(max_value, value)
    end

    if self._data.stack and group then
        if not  self._data.values[group]
        or type(self._data.values[group]) ~= "table"
        then
            self._data.values[group] = {}
        end
        values = self._data.values[group]
    end
    table.insert(values, value)

    local border_width = 0
    if self._data.border_color then border_width = 2 end

    -- Ensure we never have more data than we can draw
    while #values > self._data.width - border_width do
        table.remove(values, 1)
    end

    self:emit_signal("widget::redraw_needed")
    return self
end

--- Clear the graph.
function graph:clear()
    self._data.values = {}
    self:emit_signal("widget::redraw_needed")
    return self
end

--- Set the graph height.
-- @param height The height to set.
function graph:set_height(height)
    if height >= 5 then
        self._data.height = height
        self:emit_signal("widget::layout_changed")
    end
    return self
end

--- Set the graph width.
-- @param width The width to set.
function graph:set_width(width)
    if width >= 5 then
        self._data.width = width
        self:emit_signal("widget::layout_changed")
    end
    return self
end

-- Build properties function
for _, prop in ipairs(properties) do
    if not graph["set_" .. prop] then
        graph["set_" .. prop] = function(_graph, value)
            if _graph._data[prop] ~= value then
                _graph._data[prop] = value
                _graph:emit_signal("widget::redraw_needed")
            end
            return _graph
        end
    end
    if not graph["get_" .. prop] then
        graph["get_" .. prop] = function(_graph)
            return _graph._data[prop]
        end
    end
end

--- Create a graph widget.
-- @param args Standard widget() arguments. You should add width and height
-- key to set graph geometry.
-- @return A new graph widget.
-- @function wibox.widget.graph
function graph.new(args)
    args = args or {}

    local width = args.width or 100
    local height = args.height or 20

    if width < 5 or height < 5 then return end

    local _graph = base.make_widget()

    _graph._data = { width = width, height = height, values = {}, max_value = 1 }

    -- Set methods
    _graph.add_value = graph["add_value"]
    _graph.clear = graph["clear"]
    _graph.draw = graph.draw
    _graph.fit = graph.fit

    for _, prop in ipairs(properties) do
        _graph["set_" .. prop] = graph["set_" .. prop]
        _graph["get_" .. prop] = graph["get_" .. prop]
    end

    return _graph
end

function graph.mt:__call(...)
    return graph.new(...)
end

return setmetatable(graph, graph.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
