---------------------------------------------------------------------------
--- A layout filling all the available space. Each widget is assigned a
-- ratio (percentage) of the total space. Multiple methods are available to
-- ajust this ratio.
-- @author Emmanuel Lepage Vallee
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.ratio
---------------------------------------------------------------------------

local base  = require("wibox.widget.base" )
local flex  = require("wibox.layout.flex" )
local table = table
local pairs = pairs
local floor = math.floor
local util  = require("awful.util")

local ratio = {}

--- Set a widget at a specific index, replace the current one
-- @tparam number index A widget or a widget index
-- @param widget2 The widget to take the place of the first one
-- @treturn boolean If the operation is successful
-- @name set
-- @class function

--- Replace the first instance of `widget` in the layout with `widget2`
-- @param widget The widget to replace
-- @param widget2 The widget to replace `widget` with
-- @tparam[opt=false] boolean recursive Digg in all compatible layouts to find the widget.
-- @treturn boolean If the operation is successful
-- @name replace_widget
-- @class function

--- Swap 2 widgets in a layout
-- @tparam number index1 The first widget index
-- @tparam number index2 The second widget index
-- @treturn boolean If the operation is successful
-- @name swap
-- @class function

--- Swap 2 widgets in a layout
-- If widget1 is present multiple time, only the first instance is swapped
-- @param widget1 The first widget
-- @param widget2 The second widget
-- @tparam[opt=false] boolean recursive Digg in all compatible layouts to find the widget.
-- @treturn boolean If the operation is successful
-- @name swap_widgets
-- @class function

--- Get all children of this layout.
-- @param layout The layout you are modifying.
-- @return a list of all widgets
-- @name get_children
-- @class function

--- Fit the ratio layout into the given space
-- @param layout The layout you are modifying.
-- @param context The context in which we are fit.
-- @tparam number orig_width The available width.
-- @tparam number orig_height The available height.
-- @name fit
-- @class function

--- Reset a ratio layout. This removes all widgets from the layout.
-- @param layout The layout you are modifying.
-- @name reset
-- @class function

-- Compute the sum of all ratio (ideally, it should be 1)
local function gen_sum(self, i_s, i_e)
    local sum, new_w = 0,0

    for i = i_s or 1, i_e or #self.widgets do
        if self._ratios[i] then
            sum = sum + self._ratios[i]
        else
            new_w = new_w + 1
        end
    end

    return sum, new_w
end

-- The ratios are expressed as percentages. For this to work, the sum of all
-- ratio must be 1. This function attempt to ajust them. Space can be taken
-- from or added to a ratio when widgets are being added or removed. If a
-- specific ratio must be enforced for a widget, it has to be done with the
-- `ajust_ratio` method after each insertion or deletion
local function normalize(self)
    local count = #self.widgets
    if count == 0 then return end

    -- Instead of adding "if" everywhere, just handle this common case
    if count == 1 then
        self._ratios = { 1 }
        return
    end

    local sum, new_w = gen_sum(self)
    local old_count  = #self.widgets - new_w

    local to_add = (sum == 0) and 1 or (sum / old_count)

    -- Make sure all widgets have a ratio
    for i=1, #self.widgets do
        if not self._ratios[i] then
            self._ratios[i] = to_add
        end
    end

    sum = sum + to_add*new_w

    local delta, new_sum =  (1 - sum) / count,0

    -- Increase or decrease each ratio so it the sum become 1
    for i=1, #self.widgets do
        self._ratios[i] = self._ratios[i] + delta
        new_sum = new_sum + self._ratios[i]
    end

    -- Floating points is not an exact science, but it should still be close
    -- to 1.00.
    assert(new_sum > 0.99 and new_sum < 1.01)
end

function ratio:layout(_, width, height)
    local result = {}
    local pos,spacing = 0, self._spacing

    for k, v in ipairs(self.widgets) do
        local space
        local x, y, w, h

        if self.dir == "y" then
            space = height * self._ratios[k]
            x, y = 0, util.round(pos)
            w, h = width, floor(space)
        else
            space = width * self._ratios[k]
            x, y = util.round(pos), 0
            w, h = floor(space), height
        end

        table.insert(result, base.place_widget_at(v, x, y, w, h))

        pos = pos + space + spacing

        -- Make sure all widgets fit in the layout, if they aren't, something
        -- went wrong
        if (self.dir == "y" and util.round(pos) >= height) or
            (self.dir ~= "y" and util.round(pos) >= width) then
            break
        end
    end

    return result
end

--- Increase the ratio of "widget"
-- If the increment produce an invalid ratio (not between 0 and 1), the method
-- do nothing.
-- @tparam number index The widget index to change
-- @tparam number increment An floating point value between -1 and 1 where the
--   end result is within 0 and 1
function ratio:inc_ratio(index, increment)
    if #self.widgets ==  1 or (not index) or (not self._ratios[index])
      or increment < -1 or increment > 1 then
        return
    end

    assert(self._ratios[index])

    self:set_ratio(index, self._ratios[index] + increment)
end

--- Increment the ratio of the first instance of `widget`
-- If the increment produce an invalid ratio (not between 0 and 1), the method
-- do nothing.
-- @param widget The widget to ajust
-- @tparam number increment An floating point value between -1 and 1 where the
--   end result is within 0 and 1
function ratio:inc_widget_ratio(widget, increment)
    if not widget or not increment then return end

    local index = self:index(widget)

    self:inc_ratio(index, increment)
end

--- Set the ratio of the widget at position `index`
-- @tparam number index The index of the widget to change
-- @tparam number percent An floating point value between 0 and 1
function ratio:set_ratio(index, percent)
    if not percent or #self.widgets ==  1 or not index or not self.widgets[index]
        or percent < 0 or percent > 1 then
        return
    end

    local old = self._ratios[index]

    -- Remove what has to be cleared from all widget
    local delta = ( (percent-old) / (#self.widgets-1) )

    for k in pairs(self.widgets) do
        self._ratios[k] = self._ratios[k] - delta
    end

    -- Set the new ratio
    self._ratios[index] = percent

    -- As some widgets may now have a slightly negative ratio, normalize again
    normalize(self)

    self:emit_signal("widget::layout_changed")
end

--- Get the ratio at `index`.
-- @tparam number index The widget index to query
-- @treturn number The index (between 0 and 1)
function ratio:get_ratio(index)
    if not index then return end
    return self._ratios[index]
end

--- Set the ratio of `widget` to `percent`.
-- @tparam widget widget The widget to ajust.
-- @tparam number percent A floating point value between 0 and 1.
function ratio:set_widget_ratio(widget, percent)
    local index = self:index(widget)

    self:set_ratio(index, percent)
end

--- Update all widgets to match a set of a ratio.
-- The sum of before, itself and after must be 1 or nothing will be done
-- @tparam number index The index of the widget to change
-- @tparam number before The sum of the ratio before the widget
-- @tparam number itself The ratio for "widget"
-- @tparam number after The sum of the ratio after the widget
function ratio:ajust_ratio(index, before, itself, after)
    if not self.widgets[index] or not before or not itself or not after then
        return
    end

    local sum = before + itself + after

    -- As documented, it is the caller job to come up with valid numbers
    if math.min(before, itself, after) < 0 then return end
    if sum > 1.01 or sum < -0.99 then return end

    -- Compute the before and after offset to be applied to each widgets
    local before_count, after_count = index-1, #self.widgets - index

    local b, a = gen_sum(self, 1, index-1), gen_sum(self, index+1)

    local db, da = (before - b)/before_count, (after - a)/after_count

    -- Apply the new ratio
    self._ratios[index] = itself

    -- Equality split the delta among widgets before and after
    for i = 1, index -1 do
        self._ratios[i] = self._ratios[i] + db
    end
    for i = index+1, #self.widgets do
        self._ratios[i] = self._ratios[i] + da
    end

    -- Remove potential negative ratio
    normalize(self)

    self:emit_signal("widget::layout_changed")
end

--- Update all widgets to match a set of a ratio
-- @param widget The widget to ajust
-- @tparam number before The sum of the ratio before the widget
-- @tparam number itself The ratio for "widget"
-- @tparam number after The sum of the ratio after the widget
function ratio:ajust_widget_ratio(widget, before, itself, after)
    local index = self:index(widget)
    self:ajust_ratio(index, before, itself, after)
end

--- Add some widgets to the given fixed layout
-- @tparam widget ... Widgets that should be added (must at least be one)
function ratio:add(...)
    -- No table.pack in Lua 5.1 :-(
    local args = { n=select('#', ...), ... }
    assert(args.n > 0, "need at least one widget to add")
    for i=1, args.n do
        base.check_widget(args[i])
        table.insert(self.widgets, args[i])
    end

    normalize(self)
    self:emit_signal("widget::layout_changed")
end

--- Remove a widget from the layout
-- @tparam number index The widget index to remove
-- @treturn boolean index If the operation is successful
function ratio:remove(index)
    if not index or not self.widgets[index] then return false end

    table.remove(self._ratios, index)
    table.remove(self.widgets, index)

    normalize(self)

    self:emit_signal("widget::layout_changed")

    return true
end

--- Insert a new widget in the layout at position `index`
-- @tparam number index The position
-- @param widget The widget
function ratio:insert(index, widget)
    if not index or index < 1 or index > #self.widgets + 1 then return false end

    base.check_widget(widget)

    table.insert(self.widgets, index, widget)

    normalize(self)

    self:emit_signal("widget::layout_changed")
end

local function get_layout(dir, widget1, ...)
    local ret = flex[dir](widget1, ...)

    util.table.crush(ret, ratio)

    ret.fill_space = nil

    ret._ratios = {}

    return ret
end

--- Returns a new horizontal ratio layout. A ratio layout shares the available space
-- equally among all widgets. Widgets can be added via :add(widget).
-- @tparam widget ... Widgets that should be added to the layout.
function ratio.horizontal(...)
    return get_layout("horizontal", ...)
end

--- Returns a new vertical ratio layout. A ratio layout shares the available space
-- equally among all widgets. Widgets can be added via :add(widget).
-- @tparam widget ... Widgets that should be added to the layout.
function ratio.vertical(...)
    return get_layout("vertical", ...)
end

return ratio

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
