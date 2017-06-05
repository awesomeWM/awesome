---------------------------------------------------------------------------
-- This container scrolls its inner widget inside of the available space. An
-- example usage would be a text widget that displays information about the
-- currently playing song without using too much space for long song titles.
--
-- Please note that mouse events do not propagate to widgets inside of the
-- scroll container. Also, if this widget is causing too high CPU usage, you can
-- use @{set_fps} to make it update less often.
-- @usage
-- wibox.widget {
--    layout = wibox.container.scroll.horizontal,
--    max_size = 100,
--    step_function = wibox.container.scroll.step_functions
--                    .waiting_nonlinear_back_and_forth,
--    speed = 100,
--    {
--        widget = wibox.widget.textbox,
--        text = "This is a " .. string.rep("very, ", 10) ..  " very long text",
--    },
-- }
-- @author Uli Schlachter (based on ideas from Saleur Geoffrey)
-- @copyright 2015 Uli Schlachter
-- @classmod wibox.container.scroll
---------------------------------------------------------------------------

local cache = require("gears.cache")
local timer = require("gears.timer")
local hierarchy = require("wibox.hierarchy")
local base = require("wibox.widget.base")
local gtable = require("gears.table")
local lgi = require("lgi")
local GLib = lgi.GLib

local scroll = {}
local _need_scroll_redraw

-- "Strip" a context so that we can use it for our own drawing
local function cleanup_context(context)
    local skip = { wibox = true, drawable = true, client = true, position = true }
    local res = {}
    for k, v in pairs(context) do
        if not skip[k] then
            res[k] = v
        end
    end
    return res
end

-- Create a hierarchy (and some more stuff) for drawing the given widget. This
-- allows "some stuff" to be re-used instead of re-created all the time.
local hierarchy_cache = cache.new(function(context, widget, width, height)
    context = cleanup_context(context)
    local layouts = setmetatable({}, { __mode = "k" })

    -- Create a widget hierarchy and update when needed
    local hier
    local function do_pending_updates(layout)
        layouts[layout] = true
        hier:update(context, widget, width, height, nil)
    end
    local function emit(signal)
        -- Make the scroll layouts redraw
        for w in pairs(layouts) do
            w:emit_signal(signal)
        end
    end
    local function redraw_callback()
        emit("widget::redraw_needed")
    end
    local function layout_callback()
        emit("widget::redraw_needed")
        emit("widget::layout_changed")
    end
    hier = hierarchy.new(context, widget, width, height, redraw_callback, layout_callback, nil)

    return hier, do_pending_updates, context
end)

--- Calculate all the information needed for scrolling.
-- @param self The instance of the scrolling layout.
-- @param context A widget context under which we are fit/drawn.
-- @param width The available width
-- @param height The available height
-- @return A table with the following entries
-- @field fit_width The width that should be returned from :fit
-- @field fit_height The height that should be returned from :fit
-- @field surface_width The width for showing the child widget
-- @field surface_height The height for showing the child widget
-- @field first_x The x offset for drawing the child the first time
-- @field first_y The y offset for drawing the child the first time
-- @field[opt] second_x The x offset for drawing the child the second time
-- @field[opt] second_y The y offset for drawing the child the second time
-- @field hierarchy The wibox.hierarchy instance representing "everything"
-- @field context The widget context for drawing the hierarchy
local function calculate_info(self, context, width, height)
    local result = {}
    assert(self._private.widget)

    -- First, get the size of the widget (and the size of extra space)
    local surface_width, surface_height = width, height
    local extra_width, extra_height, extra = 0, 0, self._private.expand and self._private.extra_space or 0
    local w, h
    if self._private.dir == "h" then
        w, h = base.fit_widget(self, context, self._private.widget, self._private.space_for_scrolling, height)
        surface_width = w
        extra_width = extra
    else
        w, h = base.fit_widget(self, context, self._private.widget, width, self._private.space_for_scrolling)
        surface_height = h
        extra_height = extra
    end
    result.fit_width, result.fit_height = w, h
    if self._private.dir == "h" then
        if self._private.max_size then
            result.fit_width = math.min(w, self._private.max_size)
        end
    else
        if self._private.max_size then
            result.fit_height = math.min(h, self._private.max_size)
        end
    end
    if w > width or h > height then
        -- There is less space available than we need, we have to scroll
        _need_scroll_redraw(self)

        surface_width, surface_height = surface_width + extra_width, surface_height + extra_height

        local x, y = 0, 0
        local function get_scroll_offset(size, visible_size)
            return self._private.step_function(self._private.timer:elapsed(),
                                               size,
                                               visible_size,
                                               self._private.speed,
                                               self._private.extra_space)
        end
        if self._private.dir == "h" then
            x = -get_scroll_offset(surface_width - extra, width)
        else
            y = -get_scroll_offset(surface_height - extra, height)
        end
        result.first_x, result.first_y = x, y
        -- Was the extra space already included elsewhere?
        local extra_spacer = self._private.expand and 0 or self._private.extra_space
        if self._private.dir == "h" then
            x = x + surface_width + extra_spacer
        else
            y = y + surface_height + extra_spacer
        end
        result.second_x, result.second_y = x, y
    else
        result.first_x, result.first_y = 0, 0
    end
    result.surface_width, result.surface_height = surface_width, surface_height

    -- Get the hierarchy and subscribe ourselves to updates
    local hier, do_pending_updates, ctx = hierarchy_cache:get(context,
            self._private.widget, surface_width, surface_height)
    result.hierarchy = hier
    result.context = ctx
    do_pending_updates(self)

    return result
end

-- Draw this scrolling layout.
-- @param context The context in which we are drawn.
-- @param cr The cairo context to draw to.
-- @param width The available width.
-- @param height The available height.
function scroll:draw(context, cr, width, height)
    if not self._private.widget then
        return
    end

    local info = calculate_info(self, context, width, height)

    -- Draw the first instance of the child
    cr:save()
    cr:translate(info.first_x, info.first_y)
    cr:rectangle(0, 0, info.surface_width, info.surface_height)
    cr:clip()
    info.hierarchy:draw(info.context, cr)
    cr:restore()

    -- If there is one, draw the second instance (same code as above, minus the
    -- clip)
    if info.second_x and info.second_y then
        cr:translate(info.second_x, info.second_y)
        cr:rectangle(0, 0, info.surface_width, info.surface_height)
        cr:clip()
        info.hierarchy:draw(info.context, cr)
    end
end

-- Fit the scroll layout into the given space.
-- @param context The context in which we are fit.
-- @param width The available width.
-- @param height The available height.
function scroll:fit(context, width, height)
    if not self._private.widget then
        return 0, 0
    end
    local info = calculate_info(self, context, width, height)
    return info.fit_width, info.fit_height
end

-- Internal function used for triggering redraws for scrolling.
-- The purpose is to start a timer for redrawing the widget for scrolling.
-- Redrawing works by simply emitting the `widget::redraw_needed` signal.
-- Pausing is implemented in this function: We just don't start a timer.
-- This function must be idempotent (calling it multiple times right after
-- another does not make a difference).
_need_scroll_redraw = function(self)
    if not self._private.paused and not self._private.scroll_timer then
        self._private.scroll_timer = timer.start_new(1 / self._private.fps, function()
            self._private.scroll_timer = nil
            self:emit_signal("widget::redraw_needed")
        end)
    end
end

--- Pause the scrolling animation.
-- @see continue
function scroll:pause()
    if self._private.paused then
        return
    end
    self._private.paused = true
    self._private.timer:stop()
end

--- Continue the scrolling animation.
-- @see pause
function scroll:continue()
    if not self._private.paused then
        return
    end
    self._private.paused = false
    self._private.timer:continue()
    self:emit_signal("widget::redraw_needed")
end

--- Reset the scrolling state to its initial condition.
-- For must scroll step functions, the effect of this function should be to
-- display the widget without any scrolling applied.
-- This function does not undo the effect of @{pause}.
function scroll:reset_scrolling()
    self._private.timer:start()
    if self._private.paused then
        self._private.timer:stop()
    end
end

--- Set the direction in which this widget scroll.
-- @param dir Either "h" for horizontal scrolling or "v" for vertical scrolling
function scroll:set_direction(dir)
    if dir == self._private.dir then
        return
    end
    if dir ~= "h" and dir ~= "v" then
        error("Invalid direction, can only be 'h' or 'v'")
    end
    self._private.dir = dir
    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
end

--- The widget to be scrolled.
-- @property widget
-- @tparam widget widget The widget

function scroll:set_widget(widget)
    if widget == self._private.widget then
        return
    end
    if widget then
        base.check_widget(widget)
    end
    self._private.widget = widget
    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
end

function scroll:get_widget()
    return self._private.widget
end

--- Get the number of children element
-- @treturn table The children
function scroll:get_children()
    return {self._private.widget}
end

--- Replace the layout children
-- This layout only accept one children, all others will be ignored
-- @tparam table children A table composed of valid widgets
function scroll:set_children(children)
    self:set_widget(children[1])
end

--- Specify the expand mode that is used for extra space.
-- @tparam boolean expand If true, the widget is expanded to include the extra
-- space. If false, the extra space is simply left empty.
-- @see set_extra_space
function scroll:set_expand(expand)
    if expand == self._private.expand then
        return
    end
    self._private.expand = expand
    self:emit_signal("widget::redraw_needed")
end

--- Set the number of frames per second that this widget should draw.
-- @tparam number fps The number of frames per second
function scroll:set_fps(fps)
    if fps == self._private.fps then
        return
    end
    self._private.fps = fps
    -- No signal needed: If we are scrolling, the next redraw will apply the new
    -- FPS, else it obviously doesn't make a difference.
end

--- Set the amount of extra space that should be included in the scrolling. This
-- extra space will likely be left empty between repetitions of the widgets.
-- @tparam number extra_space The amount of extra space
-- @see set_expand
function scroll:set_extra_space(extra_space)
    if extra_space == self._private.extra_space then
        return
    end
    self._private.extra_space = extra_space
    self:emit_signal("widget::redraw_needed")
end

--- Set the speed of the scrolling animation. The exact meaning depends on the
-- step function that is used, but for the simplest step functions, this will be
-- in pixels per second.
-- @tparam number speed The speed for the animation
function scroll:set_speed(speed)
    if speed == self._private.speed then
        return
    end
    self._private.speed = speed
    self:emit_signal("widget::redraw_needed")
end

--- Set the maximum size of this widget in the direction set by
-- @{set_direction}. If the child widget is smaller than this size, no scrolling
-- is done. If the child widget is larger, then only this size will be visible
-- and the rest is made visible via scrolling.
-- @tparam number max_size The maximum size of this widget or nil for unlimited.
function scroll:set_max_size(max_size)
    if max_size == self._private.max_size then
        return
    end
    self._private.max_size = max_size
    self:emit_signal("widget::layout_changed")
end

--- Set the step function that determines the exact behaviour of the scrolling
-- animation.
-- The step function is called with five arguments:
--
-- * The time in seconds since the state of the animation
-- * The size of the child widget
-- * The size of the visible part of the widget
-- * The speed of the animation. This should have a linear effect on this
--   function's behaviour.
-- * The extra space configured by @{set_extra_space}. This was not yet added to
--   the size of the child widget, but should likely be added to it in most
--   cases.
--
-- The step function should return a single number. This number is the offset at
-- which the widget is drawn and should be between 0 and `size+extra_space`.
-- @tparam function step_function A step function.
-- @see step_functions
function scroll:set_step_function(step_function)
    -- Call the step functions once to see if it works
    step_function(0, 42, 10, 10, 5)
    if step_function == self._private.step_function then
        return
    end
    self._private.step_function = step_function
    self:emit_signal("widget::redraw_needed")
end

--- Set an upper limit for the space for scrolling.
-- This restricts the child widget's maximal size.
-- @tparam number space_for_scrolling The space for scrolling
function scroll:set_space_for_scrolling(space_for_scrolling)
    if space_for_scrolling == self._private.space_for_scrolling then
        return
    end
    self._private.space_for_scrolling = space_for_scrolling
    self:emit_signal("widget::layout_changed")
end

local function get_layout(dir, widget, fps, speed, extra_space, expand, max_size, step_function, space_for_scrolling)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    ret._private.paused = false
    ret._private.timer = GLib.Timer()
    ret._private.scroll_timer = nil

    gtable.crush(ret, scroll, true)

    ret:set_direction(dir)
    ret:set_widget(widget)
    ret:set_fps(fps or 20)
    ret:set_speed(speed or 10)
    ret:set_extra_space(extra_space or 0)
    ret:set_expand(expand)
    ret:set_max_size(max_size)
    ret:set_step_function(step_function or scroll.step_functions.linear_increase)
    ret:set_space_for_scrolling(space_for_scrolling or 2^1024)

    return ret
end

--- Get a new horizontal scrolling container.
-- @param[opt] widget The widget that should be scrolled
-- @param[opt=20] fps The number of frames per second
-- @param[opt=10] speed The speed of the animation
-- @param[opt=0] extra_space The amount of extra space to include
-- @tparam[opt=false] boolean expand Should the widget be expanded to include the
-- extra space?
-- @param[opt] max_size The maximum size of the child widget
-- @param[opt=step_functions.linear_increase] step_function The step function to be used
-- @param[opt=2^1024] space_for_scrolling The space for scrolling
function scroll.horizontal(widget, fps, speed, extra_space, expand, max_size, step_function, space_for_scrolling)
    return get_layout("h", widget, fps, speed, extra_space, expand, max_size, step_function, space_for_scrolling)
end

--- Get a new vertical scrolling container.
-- @param[opt] widget The widget that should be scrolled
-- @param[opt=20] fps The number of frames per second
-- @param[opt=10] speed The speed of the animation
-- @param[opt=0] extra_space The amount of extra space to include
-- @tparam[opt=false] boolean expand Should the widget be expanded to include the
-- extra space?
-- @param[opt] max_size The maximum size of the child widget
-- @param[opt=step_functions.linear_increase] step_function The step function to be used
-- @param[opt=2^1024] space_for_scrolling The space for scrolling
function scroll.vertical(widget, fps, speed, extra_space, expand, max_size, step_function, space_for_scrolling)
    return get_layout("v", widget, fps, speed, extra_space, expand, max_size, step_function, space_for_scrolling)
end

--- A selection of step functions
-- @see set_step_function
scroll.step_functions = {}

--- A step function that scrolls the widget in an increasing direction with
-- constant speed.
function scroll.step_functions.linear_increase(elapsed, size, _, speed, extra_space)
    return (elapsed * speed) % (size + extra_space)
end

--- A step function that scrolls the widget in an decreasing direction with
-- constant speed.
function scroll.step_functions.linear_decrease(elapsed, size, _, speed, extra_space)
    return (-elapsed * speed) % (size + extra_space)
end

--- A step function that scrolls the widget to its end and back to its
-- beginning, then back to its end, etc. The speed is constant.
function scroll.step_functions.linear_back_and_forth(elapsed, size, visible_size, speed)
    local state = ((elapsed * speed) % (2 * size)) / size
    state = state <= 1 and state or 2 - state
    return (size - visible_size) * state
end

--- A step function that scrolls the widget to its end and back to its
-- beginning, then back to its end, etc. The speed is null at the ends and
-- maximal in the middle.
function scroll.step_functions.nonlinear_back_and_forth(elapsed, size, visible_size, speed)
    local state = ((elapsed * speed) % (2 * size)) / size
    local negate = false
    if state > 1 then
        negate = true
        state = state - 1
    end
    if state < 1/3 then
        -- In the first 1/3rd of time, do a quadratic increase in speed
        state = 2 * state * state
    elseif state < 2/3 then
        -- In the center, do a linear increase. That means we need:
        -- If state is 1/3, result is 2/9 = 2 * 1/3 * 1/3
        -- If state is 2/3, result is 7/9 = 1 - 2 * (1 - 2/3) * (1 - 2/3)
        state = 5/3*state - 3/9
    else
        -- In the last 1/3rd of time, do a quadratic decrease in speed
        state = 1 - 2 * (1 - state) * (1 - state)
    end
    if negate then
        state = 1 - state
    end
    return (size - visible_size) * state
end

--- A step function that scrolls the widget to its end and back to its
-- beginning, then back to its end, etc. The speed is null at the ends and
-- maximal in the middle. At both ends the widget stands still for a moment.
function scroll.step_functions.waiting_nonlinear_back_and_forth(elapsed, size, visible_size, speed)
    local state = ((elapsed * speed) % (2 * size)) / size
    local negate = false
    if state > 1 then
        negate = true
        state = state - 1
    end
    if state < 1/5 or state > 4/5 then
        -- One fifth of time, nothing moves
        state = state < 1/5 and 0 or 1
    else
        state = (state - 1/5) * 5/3
        if state < 1/3 then
            -- In the first 1/3rd of time, do a quadratic increase in speed
            state = 2 * state * state
        elseif state < 2/3 then
            -- In the center, do a linear increase. That means we need:
            -- If state is 1/3, result is 2/9 = 2 * 1/3 * 1/3
            -- If state is 2/3, result is 7/9 = 1 - 2 * (1 - 2/3) * (1 - 2/3)
            state = 5/3*state - 3/9
        else
            -- In the last 1/3rd of time, do a quadratic decrease in speed
            state = 1 - 2 * (1 - state) * (1 - state)
        end
    end
    if negate then
        state = 1 - state
    end
    return (size - visible_size) * state
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return scroll

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
