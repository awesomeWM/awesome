---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.widget.base
---------------------------------------------------------------------------

local object = require("gears.object")
local cache = require("gears.cache")
local matrix = require("gears.matrix")
local protected_call = require("gears.protected_call")
local util = require("awful.util")
local setmetatable = setmetatable
local pairs = pairs
local type = type
local table = table

local base = {}

-- {{{ Functions on widgets

--- Functions available on all widgets
base.widget = {}

--- Set/get a widget's buttons.
-- @param _buttons The table of buttons that should bind to the widget.
-- @function buttons
function base.widget:buttons(_buttons)
    if _buttons then
        self._private.widget_buttons = _buttons
    end

    return self._private.widget_buttons
end

--- Set a widget's visible property
-- @tparam boolean b Wether the widget is visible at all
-- @function set_visible
function base.widget:set_visible(b)
    if b ~= self._private.visible then
        self._private.visible = b
        self:emit_signal("widget::layout_changed")
        -- In case something ignored fit and drew the widget anyway
        self:emit_signal("widget::redraw_needed")
    end
end

--- Get if the widget is visible.
-- @treturn boolean If the widget is visible
-- @function get_visible
function base.widget:get_visible()
    return self._private.visible or false
end

--- Set a widget's opacity
-- @tparam number o The opacity to use (a number from 0 to 1). 0 is fully
-- transparent while 1 is fully opaque.
-- @function set_opacity
function base.widget:set_opacity(o)
    if o ~= self._private.opacity then
        self._private.opacity = o
        self:emit_signal("widget::redraw")
    end
end

--- Get the widget opacity.
-- @treturn number The opacity (between 0 and 1)
-- @function get_opacity
function base.widget:get_opacity()
    return self._private.opacity
end

--- Set the widget's width
-- @tparam number|nil s The width that the widget has. `nil` means to apply the
-- default mechanism of calling the `:fit` method. A number overrides the result
-- from `:fit`.
-- @function set_width
function base.widget:set_forced_width(s)
    if s ~= self._private.forced_width then
        self._private.forced_width = s
        self:emit_signal("widget::layout_changed")
    end
end

--- Get the widget forced width.
-- Note that widgets instances can be placed at different places simultaneously,
-- therefore, they can have multiple width and width simultaneously. If there
-- is no forced size, then the only way to get the widget actual size is when
-- there is a `mouse::enter`, `mouse::leave` or button events.
-- @treturn nil|number The forced width (nil if automatic)
-- @function get_forced_widget
function base.widget:get_forced_width()
    return self._private.forced_width
end

--- Set the widget's height
-- @tparam number|nil s The height that the widget has. `nil` means to apply the
-- default mechanism of calling the `:fit` method. A number overrides the result
-- from `:fit`.
-- @function set_height
function base.widget:set_forced_height(s)
    if s ~= self._private.forced_height then
        self._private.forced_height = s
        self:emit_signal("widget::layout_changed")
    end
end

--- Get the widget forced height.
-- Note that widgets instances can be placed at different places simultaneously,
-- therefore, they can have multiple width and height simultaneously. If there
-- is no forced size, then the only way to get the widget actual size is when
-- there is a `mouse::enter`, `mouse::leave` or button events.
-- @treturn nil|number The forced height (nil if automatic)
-- @function get_forced_height
function base.widget:get_forced_height()
    return self._private.forced_height
end

--- Get all direct children widgets
-- This method should be re-implemented by the relevant widgets
-- @treturn table The children
-- @function get_children
function base.widget:get_children()
    return {}
end

--- Replace the layout children
-- The default implementation does nothing, this must be re-implemented by
-- all layout and container widgets.
-- @tparam table children A table composed of valid widgets
-- @function set_children
function base.widget:set_children(children) -- luacheck: no unused
    -- Nothing on purpose
end

-- It could have been merged into `get_all_children`, but it's not necessary
local function digg_children(ret, tlw)
    for _, w in ipairs(tlw:get_children()) do
        table.insert(ret, w)
        digg_children(ret, w)
    end
end

--- Get all direct and indirect children widgets.
-- This will scan all containers recursively to find widgets
-- Warning: This method it prone to stack overflow id the widget, or any of its
-- children, contain (directly or indirectly) itself.
-- @treturn table The children
-- @function get_all_children
function base.widget:get_all_children()
    local ret = {}
    digg_children(ret, self)
    return ret
end

--- Emit a signal and ensure all parent widgets in the hierarchies also
-- forward the signal. This is useful to track signals when there is a dynamic
-- set of containers and layouts wrapping the widget.
--
-- Note that this function has two flaws. First of all, the signal are only
-- forwarded once the widget tree has been built. This happens after all
-- currently scheduled functions have been executed. Therefor, it wont start
-- to work right away. In case the widget is present multiple time in a single
-- widget tree, this function will also forward the signal multiple time (one
-- per upward tree path).
--
-- @tparam string signal_name
-- @param ... Other arguments
-- @function emit_signal_recursive
function base.widget:emit_signal_recursive(signal_name, ...)
    -- This is a convenience wrapper, the real implementation is in the
    -- hierarchy.

    self:emit_signal("widget::emit_recursive", signal_name, ...)
end

--- Get a widex index.
-- @param widget The widget to look for
-- @param[opt] recursive Also check sub-widgets
-- @param[opt] ... Aditional widgets to add at the end of the "path"
-- @return The index
-- @return The parent layout
-- @return The path between "self" and "widget"
-- @function index
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

-- Indexes are widgets, allow them to be garbage-collected
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
    widget._private.widget_caches = {}
    for w in pairs(deps) do
        clear_caches(w)
    end
end

-- }}}

--- Figure out the geometry in device coordinate space. This gives only tight
-- bounds if no rotations by non-multiples of 90° are used.
-- @function wibox.widget.base.rect_to_device_geometry
function base.rect_to_device_geometry(cr, x, y, width, height)
    return matrix.transform_rectangle(cr.matrix, x, y, width, height)
end

--- Fit a widget for the given available width and height. This calls the
-- widget's `:fit` callback and caches the result for later use. Never call
-- `:fit` directly, but always through this function!
-- @param parent The parent widget which requests this information.
-- @param context The context in which we are fit.
-- @param widget The widget to fit (this uses widget:fit(context, width, height)).
-- @param width The available width for the widget
-- @param height The available height for the widget
-- @return The width and height that the widget wants to use
-- @function wibox.widget.base.fit_widget
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
-- @function wibox.widget.base.layout_widget
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

-- Handle a button event on a widget. This is used internally and should not be
-- called directly.
-- @function wibox.widget.base.handle_button
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

    -- Find all matching button objects
    local matches = {}
    for _, v in pairs(widget._private.widget_buttons) do
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
    for _, v in pairs(matches) do
        v:emit_signal(event,geometry)
    end
end

--- Create widget placement information. This should be used for a widget's
-- `:layout()` callback.
-- @param widget The widget that should be placed.
-- @param mat A matrix transforming from the parent widget's coordinate
--   system. For example, use matrix.create_translate(1, 2) to draw a
--   widget at position (1, 2) relative to the parent widget.
-- @param width The width of the widget in its own coordinate system. That is,
--   after applying the transformation matrix.
-- @param height The height of the widget in its own coordinate system. That is,
--   after applying the transformation matrix.
-- @return An opaque object that can be returned from :layout()
-- @function wibox.widget.base.place_widget_via_matrix
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
--   after applying the transformation matrix.
-- @param height The height of the widget in its own coordinate system. That is,
--   after applying the transformation matrix.
-- @return An opaque object that can be returned from :layout()
-- @function wibox.widget.base.place_widget_at
function base.place_widget_at(widget, x, y, width, height)
    return base.place_widget_via_matrix(widget, matrix.create_translate(x, y), width, height)
end

-- Read the table, separate attributes from widgets
local function parse_table(t, leave_empty)
    local max = 0
    local attributes, widgets = {}, {}
    for k,v in pairs(t) do
        if type(k) == "number" then
            if v then
                -- As `ipairs` doesn't always work on sparse tables, update the
                -- maximum
                if k > max then
                    max = k
                end

                widgets[k] = v
            end
        else
            attributes[k] = v
        end
    end

    -- Pack the sparse table if the container doesn't support sparse tables
    if not leave_empty then
        widgets = util.table.from_sparse(widgets)
        max = #widgets
    end

    return max, attributes, widgets
end

-- Recursively build a container from a declarative table
local function drill(ids, content)
    if not content then return end

    -- Alias `widget` to `layout` as they are handled the same way
    content.layout = content.layout or content.widget

    -- Make sure the layout is not indexed on a function
    local layout = type(content.layout) == "function" and  content.layout() or content.layout

    -- Create layouts based on metatable __call
    local l = layout.is_widget and layout or layout()

    -- Get the number of children widgets (including nil widgets)
    local max, attributes, widgets = parse_table(content, l.allow_empty_widget)

    -- Get the optional identifier to create a virtual widget tree to place
    -- in an "access table" to be able to retrieve the widget
    local id = attributes.id

    -- Clear the internal attributes
    attributes.id, attributes.layout, attributes.widget = nil, nil, nil

    -- Set layouts attributes, this has to be done before the widgets are added
    -- because it might affect the output.
    for attr, val in pairs(attributes) do
        if l["set_"..attr] then
            l["set_"..attr](l, val)
        elseif type(l[attr]) == "function" then
            l[attr](l, val)
        else
            l[attr] = val
        end
    end

    -- Add all widgets
    for k = 1, max do
        -- ipairs cannot be used on sparse tables
        local v, id2, e = widgets[k], id, nil
        if v then
            -- It is another declarative container, parse it
            if not v.is_widget then
                e, id2 = drill(ids, v)
                widgets[k] = e
            end
            base.check_widget(widgets[k])

            -- Place the widget in the access table
            if id2 then
                l  [id2] = e
                ids[id2] = ids[id2] or {}
                table.insert(ids[id2], e)
            end
        end
    end

    -- Replace all children (if any) with the new ones
    l:set_children(widgets)

    return l, id
end

-- Only available when the declarative system is used
local function get_children_by_id(self, name)
    if rawget(self, "_private") then
        return self._private.by_id[name] or {}
    else
        return rawget(self, "_by_id")[name] or {}
    end
end

--- Set a declarative widget hierarchy description.
-- See [The declarative layout system](../documentation/03-declarative-layout.md.html)
-- @param args An array containing the widgets disposition
-- @function setup
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

    rawset(self, "get_children_by_id", get_children_by_id)
end

--- Create a widget from a declarative description
-- See [The declarative layout system](../documentation/03-declarative-layout.md.html)
-- @param args An array containing the widgets disposition
-- @function wibox.widget.base.make_widget_declarative
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

--- Create an empty widget skeleton
-- See [Creating new widgets](../documentation/04-new-widget.md.html)
-- @param proxy If this is set, the returned widget will be a proxy for this
--   widget. It will be equivalent to this widget. This means it
--   looks the same on the screen.
-- @tparam[opt] string widget_name Name of the widget.  If not set, it will be
--   set automatically via `gears.object.modulename`.
-- @tparam[opt={}] table args Widget settings
-- @tparam[opt=false] boolean args.enable_properties Enable automatic getters and
--  setters calls.
-- @tparam[opt=nil] table args.class The widget class
-- @see fit_widget
-- @function wibox.widget.base.make_widget
function base.make_widget(proxy, widget_name, args)
    args = args or {}
    local ret = object {
        enable_properties = args.enable_properties,
        class             = args.class,
    }

    -- Backwards compatibility
    -- TODO: Remove this
    ret:connect_signal("widget::updated", function()
        ret:emit_signal("widget::layout_changed")
        ret:emit_signal("widget::redraw_needed")
    end)

    -- Create a table used to store the widgets internal data
    rawset(ret, "_private", {})

    -- No buttons yet
    ret._private.widget_buttons = {}

    -- Widget is visible
    ret._private.visible = true

    -- Widget is fully opaque
    ret._private.opacity = 1

    -- Differentiate tables from widgets
    rawset(ret, "is_widget", true)

    -- Size is not restricted/forced
    ret._private.forced_width = nil
    ret._private.forced_height = nil

    -- Make buttons work
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

    -- Set up caches
    clear_caches(ret)
    ret:connect_signal("widget::layout_changed", function()
        clear_caches(ret)
    end)

    -- Add functions
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
    return setmetatable(ret, mt)
end

--- Generate an empty widget which takes no space and displays nothing
-- @function wibox.widget.base.empty_widget
function base.empty_widget()
    return base.make_widget()
end

--- Do some sanity checking on widget. This function raises a lua error if
-- widget is not a valid widget.
-- @function wibox.widget.base.check_widget
function base.check_widget(widget)
    assert(type(widget) == "table", "Type should be table, but is " .. tostring(type(widget)))
    assert(widget.is_widget, "Argument is not a widget!")
    for _, func in pairs({ "connect_signal", "disconnect_signal" }) do
        assert(type(widget[func]) == "function", func .. " is not a function")
    end
end

return base

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
