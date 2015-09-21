---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @module wibox.widget.base
---------------------------------------------------------------------------

local debug = require("gears.debug")
local object = require("gears.object")
local cache = require("gears.cache")
local matrix = require("gears.matrix")
local setmetatable = setmetatable
local pairs = pairs
local type = type
local table = table

local base = {}

-- {{{ Caches

-- Indexes are widgets, allow them to be garbage-collected
local widget_dependencies = setmetatable({}, { __mode = "k" })

-- Get the cache of the given kind for this widget. This returns a gears.cache
-- that calls the callback of kind `kind` on the widget.
local function get_cache(widget, kind)
    if not widget._widget_caches[kind] then
        widget._widget_caches[kind] = cache.new(function(...)
            return widget[kind](widget, ...)
        end)
    end
    return widget._widget_caches[kind]
end

-- Special value to skip the dependency recording that is normally done by
-- base.fit_widget() and base.layout_widget(). The caller must ensure that no
-- caches depend on the result of the call and/or must handle the childs
-- widget::layout_changed signal correctly when using this.
base.no_parent_I_know_what_I_am_doing = {}

-- Record a dependency from parent to child: The layout of parent depends on the
-- layout of child.
local function record_dependency(parent, child)
    if parent == base.no_parent_I_know_what_I_am_doing then
        return
    end

    base.check_widget(parent)
    base.check_widget(child)

    local deps = widget_dependencies[child] or {}
    deps[parent] = true
    widget_dependencies[child] = deps
end

-- Clear the caches for `widget` and all widgets that depend on it.
local clear_caches
function clear_caches(widget)
    local deps = widget_dependencies[widget] or {}
    widget_dependencies[widget] = {}
    widget._widget_caches = {}
    for w in pairs(deps) do
        clear_caches(w)
    end
end
-- }}}

--- Figure out the geometry in device coordinate space. This gives only tight
-- bounds if no rotations by non-multiples of 90° are used.
function base.rect_to_device_geometry(cr, x, y, width, height)
    return matrix.transform_rectangle(cr.matrix, x, y, width, height)
end

--- Fit a widget for the given available width and height. This calls the
-- widget's `:fit` callback and caches the result for later use. Never call
-- `:fit` directly, but always through this function!
-- @param parent The parent widget which requests this information.
-- @param context The context in which we are fit.
-- @param widget The widget to fit (this uses widget:fit(width, height)).
-- @param width The available width for the widget
-- @param height The available height for the widget
-- @return The width and height that the widget wants to use
function base.fit_widget(parent, context, widget, width, height)
    record_dependency(parent, widget)

    if not widget.visible then
        return 0, 0
    end

    -- Sanitize the input. This also filters out e.g. NaN.
    local width = math.max(0, width)
    local height = math.max(0, height)

    local w, h = 0, 0
    if widget.fit then
        w, h = get_cache(widget, "fit"):get(context, width, height)
    else
        -- If it has no fit method, calculate based on the size of children
        local children = base.layout_widget(parent, context, widget, width, height)
        for _, info in ipairs(children or {}) do
            local x, y, w2, h2 = matrix.transform_rectangle(info._matrix,
                0, 0, info._width, info._height)
            w, h = math.max(w, x + w2), math.max(h, y + h2)
        end
    end

    -- Also sanitize the output.
    w = math.max(0, math.min(w, width))
    h = math.max(0, math.min(h, height))
    return w, h
end

--- Lay out a widget for the given available width and height. This calls the
-- widget's `:layout` callback and caches the result for later use. Never call
-- `:layout` directly, but always through this function! However, normally there
-- shouldn't be any reason why you need to use this function.
-- @param parent The parent widget which requests this information.
-- @param context The context in which we are laid out.
-- @param widget The widget to layout (this uses widget:layout(context, width, height)).
-- @param width The available width for the widget
-- @param height The available height for the widget
-- @return The result from the widget's `:layout` callback.
function base.layout_widget(parent, context, widget, width, height)
    record_dependency(parent, widget)

    if not widget.visible then
        return
    end

    -- Sanitize the input. This also filters out e.g. NaN.
    local width = math.max(0, width)
    local height = math.max(0, height)

    if widget.layout then
        return get_cache(widget, "layout"):get(context, width, height)
    end
end

--- Set/get a widget's buttons.
-- This function is available on widgets created by @{make_widget}.
function base:buttons(_buttons)
    if _buttons then
        self.widget_buttons = _buttons
    end

    return self.widget_buttons
end

-- Handle a button event on a widget. This is used internally and should not be
-- called directly.
function base.handle_button(event, widget, x, y, button, modifiers, geometry)
    local function is_any(mod)
        return #mod == 1 and mod[1] == "Any"
    end

    local function tables_equal(a, b)
        if #a ~= #b then
            return false
        end
        for k, v in pairs(b) do
            if a[k] ~= v then
                return false
            end
        end
        return true
    end

    -- Find all matching button objects
    local matches = {}
    for k, v in pairs(widget.widget_buttons) do
        local match = true
        -- Is it the right button?
        if v.button ~= 0 and v.button ~= button then match = false end
        -- Are the correct modifiers pressed?
        if (not is_any(v.modifiers)) and (not tables_equal(v.modifiers, modifiers)) then match = false end
        if match then
            table.insert(matches, v)
        end
    end

    -- Emit the signals
    for k, v in pairs(matches) do
        v:emit_signal(event,geometry)
    end
end

--- Create widget placement information. This should be used for a widget's
-- `:layout()` callback.
-- @param widget The widget that should be placed.
-- @param mat A matrix transforming from the parent widget's coordinate
--        system. For example, use matrix.create_translate(1, 2) to draw a
--        widget at position (1, 2) relative to the parent widget.
-- @param width The width of the widget in its own coordinate system. That is,
--        after applying the transformation matrix.
-- @param height The height of the widget in its own coordinate system. That is,
--        after applying the transformation matrix.
-- @return An opaque object that can be returned from :layout()
function base.place_widget_via_matrix(widget, mat, width, height)
    return {
        _widget = widget,
        _width = width,
        _height = height,
        _matrix = mat
    }
end

--- Create widget placement information. This should be used for a widget's
-- `:layout()` callback.
-- @param widget The widget that should be placed.
-- @param x The x coordinate for the widget.
-- @param y The y coordinate for the widget.
-- @param width The width of the widget in its own coordinate system. That is,
--        after applying the transformation matrix.
-- @param height The height of the widget in its own coordinate system. That is,
--        after applying the transformation matrix.
-- @return An opaque object that can be returned from :layout()
function base.place_widget_at(widget, x, y, width, height)
    return base.place_widget_via_matrix(widget, matrix.create_translate(x, y), width, height)
end

--[[--
Create a new widget. All widgets have to be generated via this function so that
the needed signals are added and mouse input handling is set up.

The returned widget will have a :buttons member function that can be used to
register a set of mouse button events with the widget.

To implement your own widget, you can implement some member functions on a
freshly-created widget. Note that all of these functions should be deterministic
in the sense that they will show the same behavior if they are repeatedly called
with the same arguments (same width and height). If your widget is updated and
needs to change, suitable signals have to be emitted. This will be explained
later.

The first callback is :fit. This function is called to select the size of your
widget. The arguments to this function is the available space and it should
return its desired size. Note that this function only provides a hint which is
not necessarily followed. The widget must also be able to draw itself at
different sizes than the one requested.
<pre><code>function widget:fit(context, width, height)
  -- Find the maximum square available
  local m = math.min(width, height)
  return m, m
end</code></pre>

The next callback is :draw. As the name suggests, this function is called to
draw the widget. The arguments to this widget are the context that the widget is
drawn in, the cairo context on which it should be drawn and the widget's size.
The cairo context is set up in such a way that the widget as its top-left corner
at (0, 0) and its bottom-right corner at (width, height). In other words, no
special transformation needs to be done. Note that during this callback a
suitable clip will already be applied to the cairo context so that this callback
will not be able to draw outside of the area that was registered for the widget
by the layout that placed this widget. You should not call
<code>cr:reset_clip()</code>, as redraws will not be handled correctly in this
case.
<pre><code>function widget:draw(wibox, cr, width, height)
    cr:move_to(0, 0)
    cr:line_to(width, height)
    cr:move_to(0, height)
    cr:line_to(width, 0)
    cr:stroke()
end</code></pre>

There are two signals configured for a widget. When the result that :fit would
return changes, the <code>widget::layout_changed</code> signal has to be
emitted. If this actually causes layout changes, the affected areas will be
redrawn. The other signal is <code>widget::redraw_needed</code>. This signal
signals that :draw has to be called to redraw the widget, but it is safe to
assume that :fit does still return the same values as before. If in doubt, you
can emit both signals to be safe.

If your widget only needs to draw something to the screen, the above is all that
is needed. The following callbacks can be used when implementing layouts which
place other widgets on the screen.

The :layout callback is used to figure out which other widgets should be drawn
relative to this widget. Note that it is allowed to place widgets outside of the
extents of your own widget, for example at a negative position or at twice the
size of this widget. Use this mechanism if your widget needs to draw outside of
its own extents. If the result of this callback changes,
<code>widget::layout_changed</code> has to be emitted. You can use @{fit_widget}
to call the `:fit` callback of other widgets. Never call `:fit` directly!  For
example, if you want to place another widget <code>child</code> inside of your
widget, you can do it like this:
<pre><code>-- For readability
local base = wibox.widget.base
function widget:layout(width, height)
    local result = {}
    table.insert(result, base.place_widget_at(child, width/2, 0, width/2, height)
    return result
end</code></pre>

Finally, if you want to influence how children are drawn, there are four
callbacks available that all get similar arguments:
<pre><code>function widget:before_draw_children(context, cr, width, height)
function widget:after_draw_children(context, cr, width, height)
function widget:before_draw_child(context, index, child, cr, width, height)
function widget:after_draw_child(context, index, child, cr, width, height)</code></pre>

All of these are called with the same arguments as the :draw() method. Please
note that a larger clip will be active during these callbacks that also contains
the area of all children. These callbacks can be used to influence the way in
which children are drawn, but they should not cause the drawing to cover a
different area. As an example, these functions can be used to draw children
translucently:
<pre><code>function widget:before_draw_children(wibox, cr, width, height)
    cr:push_group()
end
function widget:after_draw_children(wibox, cr, width, height)
    cr:pop_group_to_source()
    cr:paint_with_alpha(0.5)
end</code></pre>

In pseudo-code, the call sequence for the drawing callbacks during a redraw
looks like this:
<pre><code>widget:draw(wibox, cr, width, height)
widget:before_draw_children(wibox, cr, width, height)
for child do
    widget:before_draw_child(wibox, cr, child_index, child, width, height)
    cr:save()
    -- Draw child and all of its children recursively, taking into account the
    -- position and size given to base.place_widget_at() in :layout().
    cr:restore()
    widget:after_draw_child(wibox, cr, child_index, child, width, height)
end
widget:after_draw_children(wibox, cr, width, height)</code></pre>
@param proxy If this is set, the returned widget will be a proxy for this
             widget. It will be equivalent to this widget. This means it
             looks the same on the screen.
@tparam[opt] string widget_name Name of the widget.  If not set, it will be
             set automatically via `gears.object.modulename`.
@see fit_widget
--]]--
function base.make_widget(proxy, widget_name)
    local ret = object()

    -- This signal is used by layouts to find out when they have to update.
    ret:add_signal("widget::layout_changed")
    ret:add_signal("widget::redraw_needed")
    -- Mouse input, oh noes!
    ret:add_signal("button::press")
    ret:add_signal("button::release")
    ret:add_signal("mouse::enter")
    ret:add_signal("mouse::leave")

    -- Backwards compatibility
    -- TODO: Remove this
    ret:add_signal("widget::updated")
    ret:connect_signal("widget::updated", function()
        ret:emit_signal("widget::layout_changed")
        ret:emit_signal("widget::redraw_needed")
    end)

    -- No buttons yet
    ret.widget_buttons = {}
    ret.buttons = base.buttons

    -- Make buttons work
    ret:connect_signal("button::press", function(...)
        return base.handle_button("press", ...)
    end)
    ret:connect_signal("button::release", function(...)
        return base.handle_button("release", ...)
    end)

    if proxy then
        ret.fit = function(_, context, width, height)
            return base.fit_widget(ret, context, proxy, width, height)
        end
        ret.layout = function(_, context, width, height)
            return { base.place_widget_at(proxy, 0, 0, width, height) }
        end
        proxy:connect_signal("widget::layout_changed", function()
            ret:emit_signal("widget::layout_changed")
        end)
        proxy:connect_signal("widget::redraw_needed", function()
            ret:emit_signal("widget::redraw_needed")
        end)
    end

    -- Set up caches
    clear_caches(ret)
    ret:connect_signal("widget::layout_changed", function()
        clear_caches(ret)
    end)

    -- Add visible property and setter.
    ret.visible = true
    function ret:set_visible(b)
        if b ~= self.visible then
            self.visible = b
            self:emit_signal("widget::layout_changed")
            -- In case something ignored fit and drew the widget anyway
            self:emit_signal("widget::redraw_needed")
        end
    end

    -- Add opacity property and setter.
    ret.opacity = 1
    function ret:set_opacity(b)
        if b ~= self.opacity then
            self.opacity = b
            self:emit_signal("widget::redraw")
        end
    end

    -- Add __tostring method to metatable.
    ret.widget_name = widget_name or object.modulename(3)
    local mt = {}
    local orig_string = tostring(ret)
    mt.__tostring = function(o)
        return string.format("%s (%s)", ret.widget_name, orig_string)
    end
    return setmetatable(ret, mt)
end

--- Generate an empty widget which takes no space and displays nothing
function base.empty_widget()
    return base.make_widget()
end

--- Do some sanity checking on widget. This function raises a lua error if
-- widget is not a valid widget.
function base.check_widget(widget)
    debug.assert(type(widget) == "table")
    for k, func in pairs({ "add_signal", "connect_signal", "disconnect_signal" }) do
        debug.assert(type(widget[func]) == "function", func .. " is not a function")
    end
end

return base

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
