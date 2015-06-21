---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.fixed
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local table = table
local pairs = pairs

local fixed = {}

--- Layout a fixed layout. Each widget gets just the space it asks for.
-- @param context The context in which we are drawn.
-- @param width The available width.
-- @param height The available height.
function fixed:layout(context, width, height)
    local result = {}
    local pos,spacing = 0, self._spacing

    for k, v in pairs(self.widgets) do
        local x, y, w, h, _
        local in_dir
        if self.dir == "y" then
            x, y = 0, pos
            w, h = width, height - pos
            if k ~= #self.widgets or not self._fill_space then
                _, h = base.fit_widget(context, v, w, h);
            end
            pos = pos + h + spacing
            in_dir = h
        else
            x, y = pos, 0
            w, h = width - pos, height
            if k ~= #self.widgets or not self._fill_space then
                w, _ = base.fit_widget(context, v, w, h);
            end
            pos = pos + w + spacing
            in_dir = w
        end

        if (self.dir == "y" and pos-spacing > height) or
            (self.dir ~= "y" and pos-spacing > width) then
            break
        end
        table.insert(result, base.place_widget_at(v, x, y, w, h))
    end
    return result
end

--- Add a widget to the given fixed layout
function fixed:add(widget)
    base.check_widget(widget)
    table.insert(self.widgets, widget)
    self:emit_signal("widget::layout_changed")
end

--- Fit the fixed layout into the given space
-- @param context The context in which we are fit.
-- @param orig_width The available width.
-- @param orig_height The available height.
function fixed:fit(context, orig_width, orig_height)
    local width, height = orig_width, orig_height
    local used_in_dir, used_max = 0, 0

    for k, v in pairs(self.widgets) do
        local w, h = base.fit_widget(context, v, width, height)
        local in_dir, max
        if self.dir == "y" then
            max, in_dir = w, h
            height = height - in_dir
        else
            in_dir, max = w, h
            width = width - in_dir
        end
        if max > used_max then
            used_max = max
        end
        used_in_dir = used_in_dir + in_dir

        if width <= 0 or height <= 0 then
            if self.dir == "y" then
                used_in_dir = orig_height
            else
                used_in_dir = orig_width
            end
            break
        end
    end

    local spacing = self._spacing * (#self.widgets-1)

    if self.dir == "y" then
        return used_max, used_in_dir + spacing
    end
    return used_in_dir + spacing, used_max
end

--- Reset a fixed layout. This removes all widgets from the layout.
function fixed:reset()
    self.widgets = {}
    self:emit_signal("widget::layout_changed")
end

--- Set the layout's fill_space property. If this property is true, the last
-- widget will get all the space that is left. If this is false, the last widget
-- won't be handled specially and there can be space left unused.
function fixed:fill_space(val)
    if self._fill_space ~= val then
        self._fill_space = not not val
        self:emit_signal("widget::layout_changed")
    end
end

local function get_layout(dir)
    local ret = base.make_widget()

    for k, v in pairs(fixed) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret.dir = dir
    ret.widgets = {}
    ret:set_spacing(0)
    ret:fill_space(false)

    return ret
end

--- Returns a new horizontal fixed layout. Each widget will get as much space as it
-- asks for and each widget will be drawn next to its neighboring widget.
-- Widgets can be added via :add().
function fixed.horizontal()
    return get_layout("x")
end

--- Returns a new vertical fixed layout. Each widget will get as much space as it
-- asks for and each widget will be drawn next to its neighboring widget.
-- Widgets can be added via :add().
function fixed.vertical()
    return get_layout("y")
end

--- Add spacing between each layout widgets
-- @param spacing Spacing between widgets.
function fixed:set_spacing(spacing)
    if self._spacing ~= spacing then
        self._spacing = spacing
        self:emit_signal("widget::layout_changed")
    end
end

return fixed

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
