---------------------------------------------------------------------------
--- A layout filling all the available space. Each widget is assigned a
-- ratio (percentage) of the total space. Multiple methods are available to
-- ajust this ratio.
--
--@DOC_wibox_layout_defaults_ratio_EXAMPLE@
-- @author Emmanuel Lepage Vallee
-- @copyright 2016 Emmanuel Lepage Vallee
-- @layoutmod wibox.layout.ratio
---------------------------------------------------------------------------

local base  = require("wibox.widget.base" )
local flex  = require("wibox.layout.flex" )
local table = table
local pairs = pairs
local floor = math.floor
local gmath = require("gears.math")
local gtable = require("gears.table")
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

local ratio = {}

--@DOC_fixed_COMMON@

--- The widget used to fill the spacing between the layout elements.
--
-- By default, no widget is used.
--
--@DOC_wibox_layout_ratio_spacing_widget_EXAMPLE@
--
-- @property spacing_widget
-- @param widget

--- Add spacing between each layout widgets.
--
--@DOC_wibox_layout_ratio_spacing_EXAMPLE@
--
-- @property spacing
-- @tparam number spacing Spacing between widgets.

-- Compute the sum of all ratio (ideally, it should be 1).
local function gen_sum(self, i_s, i_e)
    local sum, new_w = 0,0

    for i = i_s or 1, i_e or #self._private.widgets do
        if self._private.ratios[i] then
            sum = sum + self._private.ratios[i]
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
    local count = #self._private.widgets
    if count == 0 then return end

    -- Instead of adding "if" everywhere, just handle this common case
    if count == 1 then
        self._private.ratios = { 1 }
        return
    end

    local sum, new_w = gen_sum(self)
    local old_count  = #self._private.widgets - new_w

    local to_add = (sum == 0) and 1 or (sum / old_count)

    -- Make sure all widgets have a ratio
    for i=1, #self._private.widgets do
        if not self._private.ratios[i] then
            self._private.ratios[i] = to_add
        end
    end

    sum = sum + to_add*new_w

    local delta, new_sum =  (1 - sum) / count,0

    -- Increase or decrease each ratio so it the sum become 1
    for i=1, #self._private.widgets do
        self._private.ratios[i] = self._private.ratios[i] + delta
        new_sum = new_sum + self._private.ratios[i]
    end

    -- Floating points is not an exact science, but it should still be close
    -- to 1.00.
    assert(new_sum > 0.99 and new_sum < 1.01)
end

function ratio:layout(context, width, height)
    local preliminary_results = {}
    local pos,spacing = 0, self._private.spacing
    local strategy = self:get_inner_fill_strategy()
    local has_stragety = strategy ~= "default"
    local to_redistribute, void_count = 0, 0
    local dir = self._private.dir or "x"
    local spacing_widget = self._private.spacing_widget
    local abspace = math.abs(spacing)
    local spoffset = spacing < 0 and 0 or spacing
    local is_y = self._private.dir == "y"
    local is_x = not is_y

    for k, v in ipairs(self._private.widgets) do
        local space, is_void
        local x, y, w, h, fit_h, fit_w

        if dir == "y" then
            space = height * self._private.ratios[k]
            x, y = 0, gmath.round(pos)
            w, h = width, floor(space)
        else
            space = width * self._private.ratios[k]
            x, y = gmath.round(pos), 0
            w, h = floor(space), height
        end

        -- Keep track of the unused entries
        if has_stragety then
            fit_h, fit_w = base.fit_widget(
                self, context, v,
                dir == "x" and floor(space) or w,
                dir == "y" and floor(space) or h
            )

            is_void = (v.visible == false)
                or (dir == "x" and fit_w == 0)
                or (dir == "y" and fit_h == 0)

            if is_void then
                to_redistribute = to_redistribute + space + spacing
                void_count = void_count + 1
            end
        end

        table.insert(preliminary_results, {v, x, y, w, h, is_void})

        pos = pos + space + spacing

        -- Make sure all widgets fit in the layout, if they aren't, something
        -- went wrong
        if (dir == "y" and gmath.round(pos) >= height) or
            (dir ~= "y" and gmath.round(pos) >= width) then
            break
        end
    end

    local active = #preliminary_results - void_count
    local result, real_pos, space_front = {}, 0, strategy == "right" and
        to_redistribute or (
            strategy == "center" and math.floor(to_redistribute/2) or 0
        )

    -- The number of spaces between `n` element is `n-1`, if there is spaces
    -- outside, then it is `n+1`
    if strategy == "spacing" then
        space_front = (space_front+to_redistribute/(active + 1))
        to_redistribute = (to_redistribute/(active + 1))*(active - 1)
    end

    spacing = strategy:match("spacing")
        and to_redistribute/(active - 1) or 0

    -- Only the `justify` strategy changes the original widget size.
    to_redistribute = (strategy == "justify") and to_redistribute or 0

    for k, entry in ipairs(preliminary_results) do
        local v, x, y, w, h, is_void = unpack(entry)

        -- Redistribute the space or move the widgets
        if strategy ~= "default" then
            if dir == "y" then
                h = is_void and 0 or h + (to_redistribute / (active))
                y = space_front + real_pos
                real_pos = real_pos + h + (is_void and 0 or spacing)

            else
                w = is_void and 0 or w + (to_redistribute / (active))
                x = space_front + real_pos
                real_pos = real_pos + w + (is_void and 0 or spacing)
            end
        end

        if k > 1 and abspace > 0 and spacing_widget then
            table.insert(result, base.place_widget_at(
                spacing_widget, is_x and (x - spoffset) or x, is_y and (y - spoffset) or y,
                is_x and abspace or w, is_y and abspace or h
            ))
        end

        table.insert(result, base.place_widget_at(v, x, y, w, h))
    end

    return result
end

--- Increase the ratio of "widget".
-- If the increment produce an invalid ratio (not between 0 and 1), the method
-- do nothing.
--
--@DOC_wibox_layout_ratio_inc_ratio_EXAMPLE@
--
-- @method inc_ratio
-- @tparam number index The widget index to change
-- @tparam number increment An floating point value between -1 and 1 where the
--   end result is within 0 and 1
function ratio:inc_ratio(index, increment)
    if #self._private.widgets ==  1 or (not index) or (not self._private.ratios[index])
      or increment < -1 or increment > 1 then
        return
    end

    assert(self._private.ratios[index])

    self:set_ratio(index, self._private.ratios[index] + increment)
end

--- Increment the ratio of the first instance of `widget`.
-- If the increment produce an invalid ratio (not between 0 and 1), the method
-- do nothing.
--
-- @method inc_widget_ratio
-- @param widget The widget to ajust
-- @tparam number increment An floating point value between -1 and 1 where the
--   end result is within 0 and 1
function ratio:inc_widget_ratio(widget, increment)
    if not widget or not increment then return end

    local index = self:index(widget)

    self:inc_ratio(index, increment)
end

--- Set the ratio of the widget at position `index`.
--
-- @method set_ratio
-- @tparam number index The index of the widget to change
-- @tparam number percent An floating point value between 0 and 1
function ratio:set_ratio(index, percent)
    if not percent or #self._private.widgets ==  1 or not index or not self._private.widgets[index]
        or percent < 0 or percent > 1 then
        return
    end

    local old = self._private.ratios[index]

    -- Remove what has to be cleared from all widget
    local delta = ( (percent-old) / (#self._private.widgets-1) )

    for k in pairs(self._private.widgets) do
        self._private.ratios[k] = self._private.ratios[k] - delta
    end

    -- Set the new ratio
    self._private.ratios[index] = percent

    -- As some widgets may now have a slightly negative ratio, normalize again
    normalize(self)

    self:emit_signal("widget::layout_changed")
end

--- Get the ratio at `index`.
--
-- @method get_ratio
-- @tparam number index The widget index to query
-- @treturn number The index (between 0 and 1)
function ratio:get_ratio(index)
    if not index then return end
    return self._private.ratios[index]
end

--- Set the ratio of `widget` to `percent`.
--
-- @method set_widget_ratio
-- @tparam widget widget The widget to ajust.
-- @tparam number percent A floating point value between 0 and 1.
function ratio:set_widget_ratio(widget, percent)
    local index = self:index(widget)

    self:set_ratio(index, percent)
end

--- Update all widgets to match a set of a ratio.
-- The sum of before, itself and after must be 1 or nothing will be done.
--
--@DOC_wibox_layout_ratio_ajust_ratio_EXAMPLE@
--
-- @method ajust_ratio
-- @tparam number index The index of the widget to change
-- @tparam number before The sum of the ratio before the widget
-- @tparam number itself The ratio for "widget"
-- @tparam number after The sum of the ratio after the widget
function ratio:ajust_ratio(index, before, itself, after)
    if not self._private.widgets[index] or not before or not itself or not after then
        return
    end

    local sum = before + itself + after

    -- As documented, it is the caller job to come up with valid numbers
    if math.min(before, itself, after) < 0 then return end
    if sum > 1.01 or sum < -0.99 then return end

    -- Compute the before and after offset to be applied to each widgets
    local before_count, after_count = index-1, #self._private.widgets - index

    local b, a = gen_sum(self, 1, index-1), gen_sum(self, index+1)

    local db, da = (before - b)/before_count, (after - a)/after_count

    -- Apply the new ratio
    self._private.ratios[index] = itself

    -- Equality split the delta among widgets before and after
    for i = 1, index -1 do
        self._private.ratios[i] = self._private.ratios[i] + db
    end
    for i = index+1, #self._private.widgets do
        self._private.ratios[i] = self._private.ratios[i] + da
    end

    -- Remove potential negative ratio
    normalize(self)

    self:emit_signal("widget::layout_changed")
end

--- Update all widgets to match a set of a ratio.
--
-- @method ajust_widget_ratio
-- @param widget The widget to ajust
-- @tparam number before The sum of the ratio before the widget
-- @tparam number itself The ratio for "widget"
-- @tparam number after The sum of the ratio after the widget
function ratio:ajust_widget_ratio(widget, before, itself, after)
    local index = self:index(widget)
    self:ajust_ratio(index, before, itself, after)
end

--- Add some widgets to the given fixed layout.
--
-- **Signal:** widget::added The argument are the widgets
-- @method add
-- @tparam widget ... Widgets that should be added (must at least be one)
function ratio:add(...)
    -- No table.pack in Lua 5.1 :-(
    local args = { n=select('#', ...), ... }
    assert(args.n > 0, "need at least one widget to add")
    for i=1, args.n do
        local w = base.make_widget_from_value(args[i])
        base.check_widget(w)
        table.insert(self._private.widgets, w)
    end

    normalize(self)
    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::added", ...)
end

--- Remove a widget from the layout.
--
-- **Signal:** widget::removed The arguments are the widget and the index
-- @method remove
-- @tparam number index The widget index to remove
-- @treturn boolean index If the operation is successful
function ratio:remove(index)
    if not index or not self._private.widgets[index] then return false end

    local w = self._private.widgets[index]

    table.remove(self._private.ratios, index)
    table.remove(self._private.widgets, index)

    normalize(self)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::removed", w, index)

    return true
end

--- Insert a new widget in the layout at position `index`.
-- **Signal:** widget::inserted The arguments are the widget and the index
--
-- @method insert
-- @tparam number index The position
-- @param widget The widget
function ratio:insert(index, widget)
    if not index or index < 1 or index > #self._private.widgets + 1 then return false end

    base.check_widget(widget)

    table.insert(self._private.widgets, index, widget)

    normalize(self)

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::inserted", widget, #self._private.widgets)
end

--- Set how the space of invisible or `0x0` sized widget is redistributed.
--
-- Possible values:
--
-- * "default": Honor the ratio and do not redistribute the space.
-- * "justify": Distribute the space among remaining widgets.
-- * "center": Squash remaining widgets and leave equal space on both side.
-- * "inner_spacing": Add equal spacing between all widgets.
-- * "spacing": Add equal spacing between all widgets and on the outside.
-- * "left": Squash remaining widgets and leave empty space on the left.
-- * "right": Squash remaining widgets and leave empty space on the right.
--
--@DOC_wibox_layout_ratio_strategy_EXAMPLE@
--
-- @property inner_fill_strategy
-- @tparam string inner_fill_strategy One of the value listed above.

function ratio:get_inner_fill_strategy()
    return self._private.inner_fill_strategy or "default"
end

local valid_strategies = {
    default       = true,
    justify       = true,
    center        = true,
    inner_spacing = true,
    spacing       = true,
    left          = true,
    right         = true
}

function ratio:set_inner_fill_strategy(strategy)
    assert(valid_strategies[strategy] ~= nil, "Invalid strategy: "..(strategy or ""))

    self._private.inner_fill_strategy = strategy
    self:emit_signal("widget::layout_changed")
end

local function get_layout(dir, widget1, ...)
    local ret = flex[dir](widget1, ...)

    gtable.crush(ret, ratio, true)

    ret._private.fill_space = nil

    ret._private.ratios = {}

    return ret
end

--- Returns a new horizontal ratio layout. A ratio layout shares the available space.
-- equally among all widgets. Widgets can be added via :add(widget).
-- @constructorfct wibox.layout.ratio.horizontal
-- @tparam widget ... Widgets that should be added to the layout.
function ratio.horizontal(...)
    return get_layout("horizontal", ...)
end

--- Returns a new vertical ratio layout. A ratio layout shares the available space.
-- equally among all widgets. Widgets can be added via :add(widget).
-- @constructorfct wibox.layout.ratio.vertical
-- @tparam widget ... Widgets that should be added to the layout.
function ratio.vertical(...)
    return get_layout("vertical", ...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return ratio

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
