---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local base = require("wibox.layout.base")
local widget_base = require("wibox.widget.base")
local table = table
local pairs = pairs
local floor = math.floor

-- wibox.layout.flex
local flex = {}

local function round(x)
    return floor(x + 0.5)
end

--- Draw a flex layout. Each widget gets an equal share of the available space.
-- @param wibox The wibox that this widget is drawn to.
-- @param cr The cairo context to use.
-- @param width The available width.
-- @param height The available height.
-- @return The total space needed by the layout.
function flex:draw(wibox, cr, width, height)
    local pos = 0

    local num = #self.widgets
    local space_per_item
    if self.dir == "y" then
        space_per_item = height / num
    else
        space_per_item = width / num
    end

    if self._max_widget_size then
        space_per_item = math.min(space_per_item, self._max_widget_size)
    end

    for k, v in pairs(self.widgets) do
        local x, y, w, h
        if self.dir == "y" then
            x, y = 0, round(pos)
            w, h = width, floor(space_per_item)
        else
            x, y = round(pos), 0
            w, h = floor(space_per_item), height
        end
        base.draw_widget(wibox, cr, v, x, y, w, h)

        pos = pos + space_per_item

        if (self.dir == "y" and pos >= height) or
            (self.dir ~= "y" and pos >= width) then
            break
        end
    end
end

function flex:add(widget)
    widget_base.check_widget(widget)
    table.insert(self.widgets, widget)
    widget:connect_signal("widget::updated", self._emit_updated)
    self._emit_updated()
end

--- Set the maximum size the widgets in this layout will take (that is,
-- maximum width for horizontal and maximum height for vertical).
-- @param val The maximum size of the widget.
function flex:set_max_widget_size(val)
    self._max_widget_size = val
    self:emit_signal("widget::updated")
end

--- Fit the flex layout into the given space.
-- @param orig_width The available width.
-- @param orig_height The available height.
function flex:fit(orig_width, orig_height)
    local used_max = 0
    local used_in_dir = self.dir == "y" and orig_height or orig_width

    if self._max_widget_size then
        used_in_dir = math.min(used_in_dir,
            #self.widgets * self._max_widget_size)
    end

    -- Figure out the maximum size we can give out to sub-widgets
    local sub_height = self.dir == "x" and orig_height or floor(used_in_dir / #self.widgets)
    local sub_width  = self.dir == "y" and orig_width  or floor(used_in_dir / #self.widgets)

    for k, v in pairs(self.widgets) do
        local w, h = base.fit_widget(v, sub_width, sub_height)
        local max
        if self.dir == "y" then
            max = w
        else
            max = h
        end
        if max > used_max then
            used_max = max
        end
    end

    if self.dir == "y" then
        return used_max, used_in_dir
    end
    return used_in_dir, used_max
end

function flex:reset()
    for k, v in pairs(self.widgets) do
        v:disconnect_signal("widget::updated", self._emit_updated)
    end
    self.widgets = {}
    self._max_widget_size = nil
    self:emit_signal("widget::updated")
end

local function get_layout(dir)
    local ret = widget_base.make_widget()

    for k, v in pairs(flex) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret.dir = dir
    ret.widgets = {}
    ret._emit_updated = function()
        ret:emit_signal("widget::updated")
    end

    return ret
end

--- Returns a new horizontal flex layout. A flex layout shares the available space
-- equally among all widgets. Widgets can be added via :add(widget).
function flex.horizontal()
    return get_layout("x")
end

--- Returns a new vertical flex layout. A flex layout shares the available space
-- equally among all widgets. Widgets can be added via :add(widget).
function flex.vertical()
    return get_layout("y")
end

return flex

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
