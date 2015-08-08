---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.align
---------------------------------------------------------------------------

local setmetatable = setmetatable
local table = table
local pairs = pairs
local type = type
local floor = math.floor
local base = require("wibox.layout.base")
local widget_base = require("wibox.widget.base")

local align = {}

--- Draw an align layout.
-- @param context The context in which we are drawn.
-- @param cr The cairo context to use.
-- @param width The available width.
-- @param height The available height.
function align:draw(context, cr, width, height)
    -- Draw will have to deal with all three align modes and should work in a
    -- way that makes sense if one or two of the widgets are missing (if they
    -- are all missing, it won't draw anything.) It should also handle the case
    -- where the fit something that isn't set to expand (for instance the
    -- outside widgets when the expand mode is "inside" or any of the widgets
    -- when the expand mode is "none" wants to take up more space than is
    -- allowed.
    local size_first = 0
    -- start with all the space given by the parent, subtract as we go along
    local size_remains = self.dir == "y" and height or width

    -- we will prioritize the middle widget unless the expand mode is "inside"
    --  if it is, we prioritize the first widget by not doing this block also,
    --  if the second widget doesn't exist, we will prioritise the first one
    --  instead
    if self._expand ~= "inside" and self.second then
        local w, h = base.fit_widget(self.second, width, height)
        local size_second = self.dir == "y" and h or w
        -- if all the space is taken, skip the rest, and draw just the middle
        -- widget
        if size_second >= size_remains then
            base.draw_widget(context, cr, self.second, 0, 0, width, height)
            return
        else
            -- the middle widget is sized first, the outside widgets are given
            --  the remaining space if available we will draw later
            size_remains = floor((size_remains - size_second) / 2)
        end
    end
    if self.first then
        local w, h, _ = width, height, nil
        -- we use the fit function for the "inside" and "none" modes, but
        --  ignore it for the "outside" mode, which will force it to expand
        --  into the remaining space
        if self._expand ~= "outside" then
            if self.dir == "y" then
                _, h = base.fit_widget(self.first, width, size_remains)
                size_first = h
                -- for "inside", the third widget will get a chance to use the
                --  remaining space, then the middle widget. For "none" we give
                --  the third widget the remaining space if there was no second
                --  widget to take up any space (as the first if block is skipped
                --  if this is the case)
                if self._expand == "inside" or not self.second then
                    size_remains = size_remains - h
                end
            else
                w, _ = base.fit_widget(self.first, size_remains, height)
                size_first = w
                if self._expand == "inside" or not self.second then
                    size_remains = size_remains - w
                end
            end
        else
            if self.dir == "y" then
                h = size_remains
            else
                w = size_remains
            end
        end
        base.draw_widget(context, cr, self.first, 0, 0, w, h)
    end
    -- size_remains will be <= 0 if first used all the space
    if self.third and size_remains > 0 then
        local w, h, _ = width, height, nil
        if self._expand ~= "outside" then
            if self.dir == "y" then
                _, h = base.fit_widget(self.third, width, size_remains)
                -- give the middle widget the rest of the space for "inside" mode
                if self._expand == "inside" then
                    size_remains = size_remains - h
                end
            else
                w, _ = base.fit_widget(self.third, size_remains, height)
                if self._expand == "inside" then
                    size_remains = size_remains - w
                end
            end
        else
            if self.dir == "y" then
                h = size_remains
            else
                w = size_remains
            end
        end
        local x, y = width - w, height - h
        base.draw_widget(context, cr, self.third, x, y, w, h)
    end
    -- here we either draw the second widget in the space set aside for it
    -- in the beginning, or in the remaining space, if it is "inside"
    if self.second and size_remains > 0 then
        local x, y, w, h = 0, 0, width, height
        if self._expand == "inside" then
            if self.dir == "y" then
                h = size_remains
                x, y = 0, size_first
            else
                w = size_remains
                x, y = size_first, 0
            end
        else
            if self.dir == "y" then
                _, h = base.fit_widget(self.second, width, size_remains)
                y = floor( (height - h)/2 )
            else
                w, _ = base.fit_widget(self.second, width, size_remains)
                x = floor( (width -w)/2 )
            end
        end
        base.draw_widget(context, cr, self.second, x, y, w, h)
    end
end

local function widget_changed(layout, old_w, new_w)
    if old_w then
        old_w:disconnect_signal("widget::updated", layout._emit_updated)
    end
    if new_w then
        widget_base.check_widget(new_w)
        new_w:weak_connect_signal("widget::updated", layout._emit_updated)
    end
    layout._emit_updated()
end

--- Set the layout's first widget. This is the widget that is at the left/top
function align:set_first(widget)
    widget_changed(self, self.first, widget)
    self.first = widget
end

--- Set the layout's second widget. This is the centered one.
function align:set_second(widget)
    widget_changed(self, self.second, widget)
    self.second = widget
end

--- Set the layout's third widget. This is the widget that is at the right/bottom
function align:set_third(widget)
    widget_changed(self, self.third, widget)
    self.third = widget
end

--- Fit the align layout into the given space. The align layout will
-- ask for the sum of the sizes of its sub-widgets in its direction
-- and the largest sized sub widget in the other direction.
-- @param orig_width The available width.
-- @param orig_height The available height.
function align:fit(orig_width, orig_height)
    local used_in_dir = 0
    local used_in_other = 0

    for k, v in pairs{self.first, self.second, self.third} do
        local w, h = base.fit_widget(v, orig_width, orig_height)

        local max = self.dir == "y" and w or h
        if max > used_in_other then
            used_in_other = max
        end

        used_in_dir = used_in_dir + (self.dir == "y" and h or w)
    end

    if self.dir == "y" then
        return used_in_other, used_in_dir
    end
    return used_in_dir, used_in_other
end
--- Set the expand mode which determines how sub widgets expand to take up
-- unused space. Options are:
--  "inside" - Default option. Size of outside widgets is determined using their
--              fit function. Second, middle, or center widget expands to fill
--              remaining space.
-- "outside" - Center widget is sized using its fit function and placed in the
--              center of the allowed space. Outside widgets expand (or contract)
--              to fill remaining space on their side.
--    "none" - All widgets are sized using their fit function, drawn to only the
--              returned space, or remaining space, whichever is smaller. Center
--              widget gets priority.
-- @param mode How to use unused space. "inside" (default) "outside" or "none"
function align:set_expand(mode)
    if mode == "none" or mode == "outside" then
        self._expand = mode
    else
        self._expand = "inside"
    end
    self:emit_signal("widget::updated")
end

function align:reset()
    for k, v in pairs({ "first", "second", "third" }) do
        self[v] = nil
    end
    self:emit_signal("widget::updated")
end

local function get_layout(dir)
    local ret = widget_base.make_widget()
    ret.dir = dir
    ret._emit_updated = function()
        ret:emit_signal("widget::updated")
    end

    for k, v in pairs(align) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret:set_expand("inside")

    return ret
end

--- Returns a new horizontal align layout. An align layout can display up to
-- three widgets. The widget set via :set_left() is left-aligned. :set_right()
-- sets a widget which will be right-aligned. The remaining space between those
-- two will be given to the widget set via :set_middle().
function align.horizontal()
    local ret = get_layout("x")

    ret.set_left = ret.set_first
    ret.set_middle = ret.set_second
    ret.set_right = ret.set_third

    return ret
end

--- Returns a new vertical align layout. An align layout can display up to
-- three widgets. The widget set via :set_top() is top-aligned. :set_bottom()
-- sets a widget which will be bottom-aligned. The remaining space between those
-- two will be given to the widget set via :set_middle().
function align.vertical()
    local ret = get_layout("y")

    ret.set_top = ret.set_first
    ret.set_middle = ret.set_second
    ret.set_bottom = ret.set_third

    return ret
end

return align

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
