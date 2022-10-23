---------------------------------------------------------------------------
-- An abstract widget that handles a preset of concrete widget.
--
-- The `wibox.template` widget is an abstraction layer that contains a
-- concrete widget definition. The template widget can be used to build widgets
-- that the user can customize at their will, thanks to the template mechanism.
--
-- Common usage examples
-- =====================
--
-- A basic implementation of the template widget needs a widget definition and
-- a callback to manage widget updates.
--
-- The `:update()` can be called later to update the widget template. Arguments
-- can be provided to the `:update()` method, it will be forwarded to the
-- `update_callback` function.
--
--@DOC_wibox_widget_template_basic_textbox_EXAMPLE@
--
-- Usage in libraries
-- ==================
--
-- (this part is to be completed according to awful.widget comming PRs to implement the widget_template usage)
--
-- This widget was designed to be used as a standard way to offer customization
-- over concrete widget implementation. We use the `wibox.template` as a
-- base to implement widgets from the `awful.widget` library. This way, it is
-- easy for the final user to customize the standard widget offered by awesome!
--
-- It is possible to use the template widget as a base for a custom 3rd party
-- widget module to offer more customization to the final user.
--
-- Here is an example of implementation for a custom widget inheriting from
-- `wibox.template` :
--
-- The module definition should include a default widget and a builder function
-- that can build the widget with either, the user values or the default.
--
--@DOC_wibox_widget_template_concrete_implementation_module_EXAMPLE@
--
-- On the user side, it only requires to call the builder function to get an
-- instance of the widget.
--
--@DOC_wibox_widget_template_concrete_implementation_user_EXAMPLE@
--
-- @author Aire-One
-- @copyright 2021 Aire-One <aireone@aireone.xyz>
--
-- @widgetmod wibox.template
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local wbase = require("wibox.widget.base")
local gtable = require("gears.table")
local gtimer = require("gears.timer")

local template = {
    mt = {},
    queued_updates = {},
}

local function lazy_load_child(self)
    if self._private.widget then return self._private.widget end

    local widget_instance = wbase.make_widget_from_value(self._private.widget_template)

    if widget_instance then
        wbase.check_widget(widget_instance)
    end

    self._private.widget = widget_instance

    local rem = {}

    for k, conn in ipairs(self._private.connections) do
        conn.apply()

        if conn.once then
            if conn.src_obj then
                conn.src_obj:disconnect_signal("property::"..conn.src_prop, conn.apply)
            end
            table.insert(rem, k)
        end
    end

    for i = #rem, 1, -1 do
        table.remove(self._private.connections, rem[i])
    end

    return self._private.widget
end

-- Layout this layout.
-- @method layout
-- @hidden
function template:layout(_, width, height)
    local w = lazy_load_child(self)

    if not w then return end

    return { wbase.place_widget_at(w, 0, 0, width, height) }
end

-- Fit this layout into the given area.
-- @method fit
-- @hidden
function template:fit(context, width, height)
    local w = lazy_load_child(self)

    if not w then
        return 0, 0
    end

    return wbase.fit_widget(self, context, w, width, height)
end

function template:draw() end

-- Call the update widget method now and clean the queue for this widget
-- instance.
-- @method _do_update_now
-- @hidden
function template:_do_update_now()
    if type(self._private.update_callback) == "function" then
        self._private.update_callback(self, self._private.update_args)
    end

    self._private.update_args = nil
    template.queued_updates[self] = nil
end

--- Update the widget.
--
-- This function will call the `update_callback` function at the end of the
-- current GLib event loop. Updates are batched by event loop, it means that the
-- widget can only be update once by event loop. If the `template:update` method
-- is called multiple times during the same GLib event loop, only the first call
-- will be run.
-- All arguments are passed to the queued `update_callback` call.
--
-- @tparam[opt={}] table args A table to pass to the widget update function.
-- @method update
-- @noreturn
function template:update(args)
    if type(args) == "table" then
        self._private.update_args = gtable.crush(
            gtable.clone(self._private.update_args or {}, false),
            args
        )
    end

    if not template.queued_updates[self] then
        gtimer.delayed_call(function()
            self:_do_update_now()
        end)
        template.queued_updates[self] = true
    end
end

--- Change the widget template.
--
-- Note that this will discard the existing widget instance. Thus, any
-- `set_property` will no longer be honored. `bind_property`, on the other hand,
-- will still be honored.
--
-- @property template
-- @tparam[opt=nil] template|nil template The new widget to use as a
--   template.
-- @emits widget::redraw_needed
-- @emits widget::layout_changed
-- @propemits true false

function template:set_template(widget_template)
    if widget_template == self._private.widget_template then return end

    self._private.widget = nil
    self._private.widget_template = widget_template

    self:emit_signal("widget::redraw_needed")
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::template", widget_template)
end

--- Set a property on one or more template sub-widget instances.
--
-- This method allows to set a value at any time on any of the sub widget of
-- the template. To use this, you can set, or document a set of `ids` which
-- your template support. These are usually referred as "roles" across the
-- other APIs.
--
--@DOC_wibox_widget_template_set_property_existing_EXAMPLE@
--
-- It is also possible to take this one step further and apply a property
-- to the entire sub-widget tree. This allows users to implement their own
-- template even if it doesn't use the same "roles" as the default one:
--
--@DOC_wibox_widget_template_set_property_custom_EXAMPLE@
--
-- @method set_property
-- @tparam string property The property name.
-- @param value The property value.
-- @tparam[opt=nil] nil|string|table ids A sub-widget `id` or list of `id`s. Use
--  `nil` for "all sub widgets".
-- @noreturn
-- @see bind_property
function template:set_property(property, value, ids)
    local widgets
    local target = self._private.widget

    -- Lazy load later.
    if not target then
        table.insert(self._private.connections, {
            src_obj  = nil,
            src_prop = nil,
            once     = true,
            apply    = function()
                self:set_property(property, value, ids)
            end,
        })
        return
    end

    if ids then
        widgets = {}
        for _, id in ipairs(type(ids) == "string" and {ids} or ids) do
            for _, widget in ipairs(target:get_children_by_id(id)) do
                table.insert(widgets, widget)
            end
        end
    end

    widgets = widgets or {target}

    for _, widget in ipairs(widgets) do
        if rawget(widget, "set_"..property) then
            widget["set_"..property](widget, value)
        else
            widget[property] = value
        end
    end
end

-- Do not use, backward compatibility only.
function template:_set_property_on_tree(w, property, value)
    local apply_property
    apply_property = function(wdgs)
        for _, widget in ipairs(wdgs) do
            if widget["set_"..property] then
                widget["set_"..property](widget, value)
            end

            if widget.get_children then
                apply_property(widget:get_children())
            end
        end
    end

    apply_property({w})
end

--- Monitor the value of a property on a source object and apply it on a target.
--
-- This is the equivalent of:
--
--    src_obj:connect_signal(
--        "property::"..src_prop,
--        function(v)
--            self:set_property(dest_prop, v, dest_ids)
--        end
--    )
--    self:set_property(dest_prop, v, dest_ids)
--
-- @DOC_sequences_widget_tmpl_bind_property_EXAMPLE@
--
-- @method bind_property
-- @tparam object src_obj The source object (must be derived from `gears.object`).
-- @tparam string src_prop The source object property name.
-- @tparam string dest_prop The destination widget property name.
-- @tparam[opt=nil] table|string|nil dest_ids A sub-widget `id` or list of `id`s.
--  Use `nil` for "all sub widgets".
-- @noreturn
-- @see clear_bindings
-- @see set_property

function template:bind_property(src_obj, src_prop, dest_prop, dest_ids)
    local function apply()
        self:set_property(dest_prop, src_obj[src_prop], dest_ids)
    end

    table.insert(self._private.connections, {
        src_obj  = src_obj,
        src_prop = src_prop,
        apply    = apply,
    })

    src_obj:connect_signal("property::"..src_prop, apply)

    apply()
end


--- Disconnect all signals created in `bind_property`.
-- @method clear_bindings
-- @tparam[opt=nil] widget|nil src_obj Disconnect only for this specific object.
-- @noreturn
-- @see bind_property

function template:clear_bindings(src_obj)
    for _, conn in ipairs(self._private.connections) do
        if conn.src_obj and (conn.src_obj == src_obj or not src_obj) then
            conn.src_obj:disconnect_signal("property::"..conn.src_prop, conn.apply)
        end
    end
    self._private.connections = {}
end

--- Give the internal widget instance.
-- @treturn widget The widget instance.
-- @method get_widget
-- @hidden
function template:get_widget()
    return lazy_load_child(self)
end

function template:get_children()
    return { lazy_load_child(self) }
end

--- Set the update_callback property.
-- @tparam function update_callback The new callback function.
-- @method set_update_callback
-- @hidden
function template:set_update_callback(update_callback)
    assert(type(update_callback) == "function" or update_callback == nil)

    self._private.update_callback = update_callback
end

function template:get_update_callback()
    return self._private.update_callback
end

-- Undocumented, for backward compatibility
function template:get_create_callback()
    return rawget(lazy_load_child(self), "create_callback")
end

--- Hack to allow automatic update of the widget at construction time.
-- This is supposed to be a setter for an `update_now` property. This property
-- however doesn't exist. We use this setter in the scope of the widget
-- construction from wibox.widget internals to offer an easy way for the user to
-- ask for the widget to be update right after its construction.
-- Note : The update is not instantly called, but is registered as a normal
-- update from the `:update()` method.
-- @tparam[opt] boolean update_now Update the widget now.
-- @method set_update_now
-- @hidden
function template:set_update_now(update_now)
    if update_now then
        self:update()
    end
end


--- Create a new `wibox.template` instance using the same template.
--
-- This copy will be blank. Note that `set_property`, `bind_property` or
-- `update_callback` from `self` will **NOT** be transferred over to the copy.
--
-- The following example is a new widget to list a bunch of colors. It uses
-- `wibox.template` to let the module user define their own color
-- widget. It does so by cloning the original template into new instances. This
-- example doesn't handle removing or updating them to keep the size small.
--
--@DOC_wibox_widget_template_clone1_EXAMPLE@
--
-- @method clone
-- @treturn wibox.template The copy.
function template:clone()
    return template(self._private.widget_template)
end

--- Create a new `wibox.template` instance.
-- @tparam[opt] table tmpl
-- @tparam[opt] function tmpl.update_callback The callback function to update
--   the widget.
-- @tparam[opt] boolean tmpl.update_now Update the widget after its
--   construction. This will call the `:update()` method with no parameter.
-- @treturn wibox.template The new instance.
-- @constructorfct wibox.template
function template.new(tmpl)
    tmpl = tmpl or {}

    local ret = wbase.make_widget(nil, nil, { enable_properties = true })

    gtable.crush(ret, template, true)
    ret._private.connections = {}

    ret:set_template(tmpl)
    ret:set_update_callback(tmpl.update_callback)
    ret:set_update_now(tmpl.update_now)

    rawset(ret, "_is_template", true)

    return ret
end

--- Create a `wibox.template` from a table.
--
-- @staticfct wibox.template.make_from_value
-- @tparam[opt=nil] table|wibox.template|nil value A template declaration.
-- @treturn wibox.template The template object.
function template.make_from_value(value)
    if not value then return nil end

    if rawget(value, "_is_template") then return value:clone() end

    assert(
        not rawget(value, "is_widget"),
        "This property requires a widget template, not a widget object.\n"..
        "Use `wibox.template` instead of `wibox.widget`"
    )

    return template.new(value)
end

function template.mt:__call(...)
    return template.new(...)
end

return setmetatable(template, template.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
