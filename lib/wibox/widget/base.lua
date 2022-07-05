---------------------------------------------------------------------------
-- Base class of every widgets, containers and layouts,
--
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.widget.base
-- @supermodule gears.object
---------------------------------------------------------------------------

local object = require("gears.object")
local cache = require("gears.cache")
local matrix = require("gears.matrix")
local gdebug = require("gears.debug")
local protected_call = require("gears.protected_call")
local gtable = require("gears.table")
local setmetatable = setmetatable
local pairs = pairs
local type = type
local table = table

local base = {}

-- {{{ Properties available on all widgets

--- Get or set the children elements.
-- @property children
-- @tparam table children The children.
-- @baseclass wibox.widget.base

--- Get all direct and indirect children widgets.
-- This will scan all containers recursively to find widgets
-- Warning: This method it prone to stack overflow if there is a loop in the
-- widgets hierarchy. A hierarchy loop is when a widget, or any of its
-- children, contain (directly or indirectly) itself.
-- @property all_children
-- @tparam table all_children The children.
-- @baseclass wibox.widget.base

--- Force a widget height.
-- @property forced_height
-- @tparam number|nil forced_height The height (`nil` for automatic)
-- @baseclass wibox.widget.base

--- Force a widget width.
-- @property forced_width
-- @tparam number|nil forced_width The width (`nil` for automatic)
-- @baseclass wibox.widget.base

--- The widget opacity (transparency).
-- @property opacity
-- @tparam[opt=1] number opacity The opacity (between 0 and 1)
-- @baseclass wibox.widget.base

--- The widget visibility.
-- @property visible
-- @tparam[opt=true] boolean visible
-- @baseclass wibox.widget.base

--- The widget buttons.
--
-- The table contains a list of `awful.button` objects.
-- @property buttons
-- @tparam table buttons
-- @see awful.button
-- @baseclass wibox.widget.base

-- }}}

-- {{{ Signals available on the widgets.

--- When the layout (size) change.
-- This signal is emitted when the previous results of `:layout()` and `:fit()`
-- are no longer valid.  Unless this signal is emitted, `:layout()` and `:fit()`
-- must return the same result when called with the same arguments.
-- @signal widget::layout_changed
-- @see widget::redraw_needed
-- @baseclass wibox.widget.base

--- When the widget content changed.
-- This signal is emitted when the content of the widget changes. The widget will
-- be redrawn, it is not re-layouted. Put differently, it is assumed that
-- `:layout()` and `:fit()` would still return the same results as before.
-- @signal widget::redraw_needed
-- @see widget::layout_changed
-- @baseclass wibox.widget.base

--- When a mouse button is pressed over the widget.
-- @signal button::press
-- @tparam table self The current object instance itself.
-- @tparam number lx The horizontal position relative to the (0,0) position in
-- the widget.
-- @tparam number ly The vertical position relative to the (0,0) position in the
-- widget.
-- @tparam number button The button number.
-- @tparam table mods The modifiers (mod4, mod1 (alt), Control, Shift)
-- @tparam table find_widgets_result The entry from the result of
-- @{wibox:find_widgets} for the position that the mouse hit.
-- @tparam wibox.drawable find_widgets_result.drawable The drawable containing
-- the widget.
-- @tparam widget find_widgets_result.widget The widget being displayed.
-- @tparam wibox.hierarchy find_widgets_result.hierarchy The hierarchy
-- managing the widget's geometry.
-- @tparam number find_widgets_result.x An approximation of the X position that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.y An approximation of the Y position that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.width An approximation of the width that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.height An approximation of the height that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.widget_width The exact width of the widget
-- in its local coordinate system.
-- @tparam number find_widgets_result.widget_height The exact height of the widget
-- in its local coordinate system.
-- @see mouse
-- @baseclass wibox.widget.base

--- When a mouse button is released over the widget.
-- @signal button::release
-- @tparam table self The current object instance itself.
-- @tparam number lx The horizontal position relative to the (0,0) position in
-- the widget.
-- @tparam number ly The vertical position relative to the (0,0) position in the
-- widget.
-- @tparam number button The button number.
-- @tparam table mods The modifiers (mod4, mod1 (alt), Control, Shift)
-- @tparam table find_widgets_result The entry from the result of
-- @{wibox:find_widgets} for the position that the mouse hit.
-- @tparam wibox.drawable find_widgets_result.drawable The drawable containing
-- the widget.
-- @tparam widget find_widgets_result.widget The widget being displayed.
-- @tparam wibox.hierarchy find_widgets_result.hierarchy The hierarchy
-- managing the widget's geometry.
-- @tparam number find_widgets_result.x An approximation of the X position that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.y An approximation of the Y position that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.width An approximation of the width that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.height An approximation of the height that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.widget_width The exact width of the widget
-- in its local coordinate system.
-- @tparam number find_widgets_result.widget_height The exact height of the widget
-- in its local coordinate system.
-- @see mouse
-- @baseclass wibox.widget.base

--- When the mouse enter a widget.
-- @signal mouse::enter
-- @tparam table self The current object instance itself.
-- @tparam table find_widgets_result The entry from the result of
-- @{wibox:find_widgets} for the position that the mouse hit.
-- @tparam wibox.drawable find_widgets_result.drawable The drawable containing
-- the widget.
-- @tparam widget find_widgets_result.widget The widget being displayed.
-- @tparam wibox.hierarchy find_widgets_result.hierarchy The hierarchy
-- managing the widget's geometry.
-- @tparam number find_widgets_result.x An approximation of the X position that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.y An approximation of the Y position that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.width An approximation of the width that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.height An approximation of the height that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.widget_width The exact width of the widget
-- in its local coordinate system.
-- @tparam number find_widgets_result.widget_height The exact height of the widget
-- in its local coordinate system.
-- @see mouse
-- @baseclass wibox.widget.base

--- When the mouse leave a widget.
-- @signal mouse::leave
-- @tparam table self The current object instance itself.
-- @tparam table find_widgets_result The entry from the result of
-- @{wibox:find_widgets} for the position that the mouse hit.
-- @tparam wibox.drawable find_widgets_result.drawable The drawable containing
-- the widget.
-- @tparam widget find_widgets_result.widget The widget being displayed.
-- @tparam wibox.hierarchy find_widgets_result.hierarchy The hierarchy
-- managing the widget's geometry.
-- @tparam number find_widgets_result.x An approximation of the X position that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.y An approximation of the Y position that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.width An approximation of the width that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.height An approximation of the height that
-- the widget is visible at on the surface.
-- @tparam number find_widgets_result.widget_width The exact width of the widget
-- in its local coordinate system.
-- @tparam number find_widgets_result.widget_height The exact height of the widget
-- in its local coordinate system.
-- @see mouse
-- @baseclass wibox.widget.base

-- }}}

-- {{{ Functions on widgets

-- Functions available on all widgets.
base.widget = {}

object.properties._legacy_accessors(base.widget, "buttons", nil, true, function(new_btns)
    return new_btns[1] and (
        type(new_btns[1]) == "button" or new_btns[1]._is_capi_button
    ) or false
end, true)

--- Set a widget's visibility.
-- @tparam boolean b Whether the widget is visible.
-- @method wibox.widget.base:set_visible
-- @hidden
function base.widget:set_visible(b)
    if b ~= self._private.visible then
        self._private.visible = b
        self:emit_signal("widget::layout_changed")
        -- In case something ignored fit and drew the widget anyway.
        self:emit_signal("widget::redraw_needed")
    end
end

--- Add a new `awful.button` to this widget.
-- @tparam awful.button button The button to add.
-- @method wibox.widget.base:add_button
-- @noreturn
function base.widget:add_button(button)
    if not button then return end

    -- Simple case
    if not self._private.buttons then
        self:set_buttons({button})
        return
    end

    -- This could happen if something accidentally uses rawset
    assert(self._private.buttons_formatted)

    -- an `awful.button` is a tupple of `capi.button`
    self._private.buttons_formatted = gtable.join(
        self._private.buttons_formatted,
        button
    )

    table.insert(self._private.buttons, button)
end

--- Is the widget visible?
-- @treturn boolean
-- @method wibox.widget.base:get_visible
-- @hidden
function base.widget:get_visible()
    return self._private.visible or false
end

--- Set a widget's opacity.
-- @tparam number o The opacity to use (a number from 0 (transparent) to 1
-- (opaque)).
-- @method wibox.widget.base:set_opacity
-- @hidden
function base.widget:set_opacity(o)
    if o ~= self._private.opacity then
        self._private.opacity = o
        self:emit_signal("widget::redraw_needed")
    end
end

--- Get the widget's opacity.
-- @treturn number The opacity (between 0 (transparent) and 1 (opaque)).
-- @method wibox.widget.base:get_opacity
-- @hidden
function base.widget:get_opacity()
    return self._private.opacity
end

--- Set the widget's forced width.
-- @tparam[opt] number width With `nil` the default mechanism of calling the
--   `:fit` method is used.
-- @see wibox.widget.base:fit_widget
-- @method wibox.widget.base:set_forced_width
-- @hidden
function base.widget:set_forced_width(width)
    if width ~= self._private.forced_width then
        self._private.forced_width = width
        self:emit_signal("widget::layout_changed")
    end
end

--- Get the widget's forced width.
--
-- Note that widget instances can be used in different places simultaneously,
-- and therefore can have multiple dimensions.
-- If there is no forced width/height, then the only way to get the widget's
-- actual size is during a `mouse::enter`, `mouse::leave` or button event.
-- @treturn[opt] number The forced width (nil if automatic).
-- @see fit_widget
-- @method wibox.widget.base:get_forced_width
-- @hidden
function base.widget:get_forced_width()
    return self._private.forced_width
end

--- Set the widget's forced height.
-- @tparam[opt] number height With `nil` the default mechanism of calling the
--   `:fit` method is used.
-- @see wibox.widget.base:fit_widget
-- @method wibox.widget.base:set_height
-- @hidden
function base.widget:set_forced_height(height)
    if height ~= self._private.forced_height then
        self._private.forced_height = height
        self:emit_signal("widget::layout_changed")
    end
end

--- Get the widget's forced height.
--
-- Note that widget instances can be used in different places simultaneously,
-- and therefore can have multiple dimensions.
-- If there is no forced width/height, then the only way to get the widget's
-- actual size is during a `mouse::enter`, `mouse::leave` or button event.
-- @treturn[opt] number The forced height (nil if automatic).
-- @method wibox.widget.base:get_forced_height
-- @hidden
function base.widget:get_forced_height()
    return self._private.forced_height
end

--- Get the widget's direct children widgets.
--
-- This method should be re-implemented by the relevant widgets.
-- @treturn table children The children.
-- @method wibox.widget.base:get_children
-- @hidden
function base.widget:get_children()
    return {}
end

--- Replace the layout children.
--
-- The default implementation does nothing, this must be re-implemented by
-- all layout and container widgets.
-- @tparam table children A table composed of valid widgets.
-- @method wibox.widget.base:set_children
-- @hidden
function base.widget:set_children(children) -- luacheck: no unused
    -- Nothing on purpose
end

-- It could have been merged into `get_all_children`, but it's not necessary.
local function digg_children(ret, tlw)
    for _, w in ipairs(tlw:get_children()) do
        table.insert(ret, w)
        digg_children(ret, w)
    end
end

--- Get all direct and indirect children widgets.
--
-- This will scan all containers recursively to find widgets.
--
-- *Warning*: This method it prone to stack overflow if the widget, or any of
-- its children, contains (directly or indirectly) itself.
-- @treturn table children The children.
-- @method wibox.widget.base:get_all_children
-- @hidden
function base.widget:get_all_children()
    local ret = {}
    digg_children(ret, self)
    return ret
end

--- Common implementation of the `:set_widget()` method exposed by many
-- other widgets.
--
-- Use this if your widget has no custom logic when setting the widget.
--
-- @usage
--   rawset(my_custom_widget, "set_widget", wibox.widget.base.set_widget_common)
function base.set_widget_common(self, widget)
    local w = widget and base.make_widget_from_value(widget)

    if w then
        base.check_widget(w)
    end

    self._private.widget = w

    self:emit_signal("property::widget")

    self:emit_signal("widget::layout_changed")
end

--- Emit a signal and ensure all parent widgets in the hierarchies also
-- forward the signal.
--
-- This is useful to track signals when there is a dynamic set of containers
--   and layouts wrapping the widget.
--
-- Note that this function has some flaws:
--
-- 1. The signal is only forwarded once the widget tree has been built. This
--    happens after all currently scheduled functions have been executed.
--    Therefore, it will not start to work right away.
-- 2. In case the widget is present multiple times in a single widget tree,
--    this function will also forward the signal multiple times (once per upward
--    tree path).
-- 3. If the widget is removed from the widget tree, the signal is still
--    forwarded for some time, similar to the first case.
--
-- @tparam string signal_name
-- @param ... Other arguments
-- @method wibox.widget.base:emit_signal_recursive
-- @noreturn
function base.widget:emit_signal_recursive(signal_name, ...)
    -- This is a convenience wrapper, the real implementation is in the
    -- hierarchy.

    self:emit_signal("widget::emit_recursive", signal_name, ...)
end

--- Get the index of a widget.
-- @tparam widget widget The widget to look for.
-- @tparam[opt] boolean recursive Recursively check accross the sub-widgets
--   hierarchy.
-- @tparam[opt] widget ... Additional widgets to add at the end of the
--   sub-widgets hierarchy "path".
-- @treturn number The widget index.
-- @treturn widget The parent widget.
-- @treturn table The hierarchy path between "self" and "widget".
-- @method wibox.widget.base:index
function base.widget:index(widget, recursive, ...)
    local widgets = self:get_children()
    for idx, w in ipairs(widgets) do
        if w == widget then
            return idx, self, {...}
        elseif recursive then
            local child_idx, l, path = w:index(widget, true, self, ...)
            if child_idx and l then
                return child_idx, l, path
            end
        end
    end
    return nil, self, {}
end
-- }}}

-- {{{ Caches

-- Indexes are widgets, allow them to be garbage-collected.
local widget_dependencies = setmetatable({}, { __mode = "kv" })

-- Get the cache of the given kind for this widget. This returns a gears.cache
-- that calls the callback of kind `kind` on the widget.
local function get_cache(widget, kind)
    if not widget._private.widget_caches[kind] then
        widget._private.widget_caches[kind] = cache.new(function(...)
            return protected_call(widget[kind], widget, ...)
        end)
    end
    return widget._private.widget_caches[kind]
end

-- Special value to skip the dependency recording that is normally done by
-- base.fit_widget() and base.layout_widget(). The caller must ensure that no
-- caches depend on the result of the call and/or must handle the children's
-- widget::layout_changed signal correctly when using this.
base.no_parent_I_know_what_I_am_doing = {}

-- Record a dependency from parent to child: The layout of `parent` depends on
-- the layout of `child`.
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
    widget._private.widget_caches = {}
    for w in pairs(deps) do
        clear_caches(w)
    end
end

-- }}}

--- Figure out the geometry in the device coordinate space.
--
-- This gives only tight bounds if no rotations by non-multiples of 90° are
-- used.
-- @staticfct wibox.widget.base.rect_to_device_geometry
-- @param cr The cairo context.
-- @tparam number x The `x` value.
-- @tparam number y The `y` value.
-- @tparam number width The `width` value.
-- @tparam number height The `height` value.
-- @treturn number The new `x` value.
-- @treturn number The new `y` value.
-- @treturn number The new `width` value.
-- @treturn number The new `height` value.
function base.rect_to_device_geometry(cr, x, y, width, height)
    return matrix.transform_rectangle(cr.matrix, x, y, width, height)
end

--- Fit a widget for the given available width and height.
--
-- This calls the widget's `:fit` callback and caches the result for later use.
-- Never call `:fit` directly, but always through this function!
-- @tparam widget parent The parent widget which requests this information.
-- @tparam table context The context in which we are fit.
-- @tparam widget widget The widget to fit (this uses
--   `widget:fit(context, width, height)`).
-- @tparam number width The available width for the widget.
-- @tparam number height The available height for the widget.
-- @treturn number The width that the widget wants to use.
-- @treturn number The height that the widget wants to use.
-- @staticfct wibox.widget.base.fit_widget
function base.fit_widget(parent, context, widget, width, height)
    record_dependency(parent, widget)

    if not widget._private.visible then
        return 0, 0
    end

    -- Sanitize the input. This also filters out e.g. NaN.
    width = math.max(0, width)
    height = math.max(0, height)

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

    -- Apply forced size and handle nil's
    w = widget._private.forced_width or w or 0
    h = widget._private.forced_height or h or 0

    -- Also sanitize the output.
    w = math.max(0, math.min(w, width))
    h = math.max(0, math.min(h, height))
    return w, h
end

--- Lay out a widget for the given available width and height.
--
-- This calls the widget's `:layout` callback and caches the result for later
-- use.  Never call `:layout` directly, but always through this function!
-- However, normally there shouldn't be any reason why you need to use this
-- function.
-- @tparam widget parent The parent widget which requests this information.
-- @tparam table context The context in which we are laid out.
-- @tparam widget widget The widget to layout (this uses
--   `widget:layout(context, width, height)`).
-- @tparam number width The available width for the widget.
-- @tparam number height The available height for the widget.
-- @treturn[opt] table The result from the widget's `:layout` callback.
-- @staticfct wibox.widget.base.layout_widget
function base.layout_widget(parent, context, widget, width, height)
    record_dependency(parent, widget)

    if not widget._private.visible then
        return
    end

    -- Sanitize the input. This also filters out e.g. NaN.
    width = math.max(0, width)
    height = math.max(0, height)

    if widget.layout then
        return get_cache(widget, "layout"):get(context, width, height)
    end
end

--- Handle a button event on a widget.
--
-- This is used internally and should not be called directly.
-- @staticfct wibox.widget.base.handle_button
-- @noreturn
function base.handle_button(event, widget, x, y, button, modifiers, geometry)
    x = x or y -- luacheck: no unused
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

    -- Find all matching button objects.
    local matches = {}
    for _, v in pairs(widget._private.buttons_formatted or {}) do
        local match = true
        -- Is it the right button?
        if v.button ~= 0 and v.button ~= button then match = false end
        -- Are the correct modifiers pressed?
        if (not is_any(v.modifiers)) and (not tables_equal(v.modifiers, modifiers)) then match = false end
        if match then
            table.insert(matches, v)
        end
    end

    -- Emit the signals.
    for _, v in pairs(matches) do
        v:emit_signal(event,geometry)
    end
end

--- Create widget placement information. This should be used in a widget's
-- `:layout()` callback.
-- @tparam widget widget The widget that should be placed.
-- @param mat A matrix transforming from the parent widget's coordinate
--   system. For example, use matrix.create_translate(1, 2) to draw a
--   widget at position (1, 2) relative to the parent widget.
-- @tparam number width The width of the widget in its own coordinate system.
--   That is, after applying the transformation matrix.
-- @tparam number height The height of the widget in its own coordinate system.
--   That is, after applying the transformation matrix.
-- @treturn table An opaque object that can be returned from `:layout()`.
-- @staticfct wibox.widget.base.place_widget_via_matrix
function base.place_widget_via_matrix(widget, mat, width, height)
    assert(width >= 0, "A widget's width cannot be negative: " ..  tostring(width))
    assert(height >= 0, "A widget's height cannot be negative: " ..  tostring(height))
    return {
        _widget = widget,
        _width = width,
        _height = height,
        _matrix = mat
    }
end

--- Create widget placement information. This should be used for a widget's
-- `:layout()` callback.
-- @tparam widget widget The widget that should be placed.
-- @tparam number x The x coordinate for the widget.
-- @tparam number y The y coordinate for the widget.
-- @tparam number width The width of the widget in its own coordinate system.
--   That is, after applying the transformation matrix.
-- @tparam number height The height of the widget in its own coordinate system.
--   That is, after applying the transformation matrix.
-- @treturn table An opaque object that can be returned from `:layout()`.
-- @staticfct wibox.widget.base.place_widget_at
function base.place_widget_at(widget, x, y, width, height)
    return base.place_widget_via_matrix(widget, matrix.create_translate(x, y), width, height)
end

-- Check if `obj` can be called (either using the metacall or as a function)
local function is_callable(obj)
    local t = type(obj)
    return t == "function" or (
        t == "table" and getmetatable(obj) and getmetatable(obj).__call
    ), t
end

-- Read the table, separate attributes from widgets.
local function parse_table(t, leave_empty)
    local max = 0
    local attributes, widgets = {}, {}
    for k,v in pairs(t) do
        if type(k) == "number" then
            if v then
                -- Since `ipairs` doesn't always work on sparse tables, update
                -- the maximum.
                if k > max then
                    max = k
                end

                widgets[k] = v
            end
        else
            attributes[k] = v
        end
    end

    -- Pack the sparse table, if the container doesn't support sparse tables.
    if not leave_empty then
        widgets = gtable.from_sparse(widgets)
        max = #widgets
    end

    return max, attributes, widgets
end

-- Recursively build a container from a declarative table.
local function drill(ids, content)
    if not content then return end

    -- Alias `widget` to `layout` as they are handled the same way.
    content.layout = content.layout or content.widget

    -- Make sure the layout is not indexed on a function.
    local layout = type(content.layout) == "function" and  content.layout() or content.layout

    -- Create layouts based on metatable's __call.
    local l = layout.is_widget and layout or layout()

    -- Get the number of children widgets (including nil widgets).
    local max, attributes, widgets = parse_table(content, l.allow_empty_widget)

    -- Get the optional identifier to create a virtual widget tree to place
    -- in an "access table" to be able to retrieve the widget.
    local id = attributes.id

    -- Clear the internal attributes.
    attributes.id, attributes.layout, attributes.widget = nil, nil, nil

    -- Set layout attributes.
    -- This has to be done before the widgets are added because it might affect
    -- the output.
    for attr, val in pairs(attributes) do
        if l["set_"..attr] then
            l["set_"..attr](l, val)
        elseif type(l[attr]) == "function" then
            l[attr](l, val)
        else
            l[attr] = val
        end
    end

    if widgets and max > 0 then
        -- Add all widgets.
        for k = 1, max do
            -- ipairs cannot be used on sparse tables.
            local v, id2, e = widgets[k], id, nil
            if v then
                -- It is another declarative container, parse it.
                if (not v.is_widget) and (v.widget or v.layout) then
                    e, id2 = drill(ids, v)
                    widgets[k] = e
                elseif (not v.is_widget) and is_callable(v) then
                     widgets[k] = v()
                end
                base.check_widget(widgets[k])

                -- Place the widget in the access table.
                if id2 then
                    l  [id2] = e
                    ids[id2] = ids[id2] or {}
                    table.insert(ids[id2], e)
                end
            end
        end

        -- Replace all children (if any) with the new ones.
        l:set_children(widgets)
    end

    return l, id
end

-- Only available when the declarative system is used.
local function get_children_by_id(self, name)
    if rawget(self, "_private") then
        return self._private.by_id[name] or {}
    else
        return rawget(self, "_by_id")[name] or {}
    end
end

--- Set a declarative widget hierarchy description.
--
-- See [The declarative layout system](../documentation/03-declarative-layout.md.html).
-- @tparam table args A table containing the widget's disposition.
-- @method wibox.widget.base:setup
-- @hidden
function base.widget:setup(args)
    local f,ids = self.set_widget or self.add or self.set_first,{}
    local w, id = drill(ids, args)
    f(self,w)
    if id then
        -- Avoid being dropped by wibox metatable -> drawin
        rawset(self, id, w)
        ids[id] = ids[id] or {}
        table.insert(ids[id], 1, w)
    end

    if rawget(self, "_private") then
        self._private.by_id = ids
    else
        rawset(self, "_by_id", ids)
    end

    if not rawget(self, "get_children_by_id") then
        rawset(self, "get_children_by_id", get_children_by_id)
    end
end

--- Create a widget from a declarative description.
--
-- See [The declarative layout system](../documentation/03-declarative-layout.md.html).
-- @tparam table args A table containing the widgets disposition.
-- @constructorfct wibox.widget.base.make_widget_declarative
function base.make_widget_declarative(args)
    local ids = {}

    if (not args.layout) and (not args.widget) then
        args.widget = base.make_widget(nil, args.id)
    end

    local w, id = drill(ids, args)

    local mt = getmetatable(w) or {}
    local orig_string = tostring(w)

    -- Add the main id (if any)
    if id then
        ids[id] = ids[id] or {}
        table.insert(ids[id], 1, w)
    end

    if rawget(w, "_private") then
        w._private.by_id = ids
    else
        rawset(w, "_by_id", ids)
    end

    rawset(w, "get_children_by_id", get_children_by_id)

    mt.__tostring = function()
        return string.format("%s (%s)", id or w.widget_name or "N/A", orig_string)
    end

    return setmetatable(w, mt)
end

--- Create a widget from an undetermined value.
--
-- The value can be:
--
-- * A widget (in which case nothing new is created)
-- * A declarative construct
-- * A constructor function
-- * A metaobject
--
-- @param wdg The value.
-- @param[opt=nil] ... Arguments passed to the contructor (if any).
-- @constructorfct wibox.widget.base.make_widget_from_value
-- @treturn widget|nil The new widget or `nil` in case of failure.
function base.make_widget_from_value(wdg, ...)
    if not wdg then return nil end

    local is_function, t = is_callable(wdg)

    if is_function then
        wdg = wdg(...)
    elseif t == "table" then
        wdg = wdg.is_widget and wdg or base.make_widget_declarative(wdg)
    else
        gdebug.print_warning(
            "The argument is not a function, table, or widget."
        )
        return nil
    end

    return wdg
end

--- Create an empty widget skeleton.
--
-- See [Creating new widgets](../documentation/04-new-widgets.md.html).
-- @tparam[opt] widget proxy If this is set, the returned widget will be a
--   proxy for this widget. It will be equivalent to this widget.
--   This means it looks the same on the screen.
-- @tparam[opt] string widget_name Name of the widget.  If not set, it will be
--   set automatically via @{gears.object.modulename}.
-- @tparam[opt={}] table args Widget settings
-- @tparam[opt=false] boolean args.enable_properties Enable automatic getter
--   and setter methods.
-- @tparam[opt=nil] table args.class The widget class
-- @see fit_widget
-- @constructorfct wibox.widget.base.make_widget
function base.make_widget(proxy, widget_name, args)
    args = args or {}

    local prop_default = args.enable_properties ~= false

    if awesome.api_level < 5 then
        prop_default = args.enable_properties
    end

    local ret = object {
        enable_properties = prop_default,
        class             = args.class,
    }

    -- Backwards compatibility.
    -- TODO: Remove this
    ret:connect_signal("widget::updated", function()
        ret:emit_signal("widget::layout_changed")
        ret:emit_signal("widget::redraw_needed")
    end)

    -- Create a table used to store the widgets internal data.
    rawset(ret, "_private", {})

    -- Widget is visible.
    ret._private.visible = true

    -- Widget is fully opaque.
    ret._private.opacity = 1

    -- Differentiate tables from widgets.
    rawset(ret, "is_widget", true)

    -- Size is not restricted/forced.
    ret._private.forced_width = nil
    ret._private.forced_height = nil

    -- Make buttons work.
    ret:connect_signal("button::press", function(...)
        return base.handle_button("press", ...)
    end)
    ret:connect_signal("button::release", function(...)
        return base.handle_button("release", ...)
    end)

    if proxy then
        rawset(ret, "fit", function(_, context, width, height)
            return base.fit_widget(ret, context, proxy, width, height)
        end)
        rawset(ret, "layout", function(_, _, width, height)
            return { base.place_widget_at(proxy, 0, 0, width, height) }
        end)
        proxy:connect_signal("widget::layout_changed", function()
            ret:emit_signal("widget::layout_changed")
        end)
        proxy:connect_signal("widget::redraw_needed", function()
            ret:emit_signal("widget::redraw_needed")
        end)
    end

    -- Set up caches.
    clear_caches(ret)
    ret:connect_signal("widget::layout_changed", function()
        clear_caches(ret)
    end)

    -- Add functions.
    for k, v in pairs(base.widget) do
        rawset(ret, k, v)
    end

    -- Add __tostring method to metatable.
    rawset(ret, "widget_name", widget_name or object.modulename(3))
    local mt = getmetatable(ret) or {}
    local orig_string = tostring(ret)
    mt.__tostring = function()
        return string.format("%s (%s)", ret.widget_name, orig_string)
    end

    -- Even when properties are disabled, buttons is required for backward
    -- compatibility.
    --TODO v6 Remove this
    if not args.enable_properties then
        mt.__index = function(_, key)
            if key == "buttons" then
                return base.widget.get_buttons(ret)
            end

            return rawget(ret, key)
        end
        mt.__newindex = function(_, key, value)
            if key == "buttons" then
                base.widget.set_buttons(ret, value)
            else
                rawset(ret, key, value)
            end
        end
    end

    return setmetatable(ret, mt)
end

--- Generate an empty widget which takes no space and displays nothing.
-- @constructorfct wibox.widget.base.empty_widget
function base.empty_widget()
    return base.make_widget()
end

--- Do some sanity checking on a widget.
--
-- This function raises an error if the widget is not valid.
-- @staticfct wibox.widget.base.check_widget
-- @noreturn
function base.check_widget(widget)
    assert(type(widget) == "table", "Type should be table, but is " .. tostring(type(widget)))
    assert(widget.is_widget, "Argument is not a widget!")
    for _, func in pairs({ "connect_signal", "disconnect_signal" }) do
        assert(type(widget[func]) == "function", func .. " is not a function")
    end
end

return base

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
