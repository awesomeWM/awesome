---------------------------------------------------------------------------
-- An abstract widget that handles a preset of concrete widget.
--
-- The `wibox.widget.template` widget is an abstraction layer that contains a
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
-- Alternatively, you can declare the `template` widget instance using the
-- declarative pattern (both variants are strictly equivalent):
--
--@DOC_wibox_widget_template_basic_textbox_declarative_EXAMPLE@
--
-- Usage in libraries
-- ==================
--
-- (this part is to be completed according to awful.widget comming PRs to implement the widget_template usage)
--
-- This widget was designed to be used as a standard way to offer customization
-- over concrete widget implementation. We use the `wibox.widget.template` as a
-- base to implement widgets from the `awful.widget` library. This way, it is
-- easy for the final user to customize the standard widget offered by awesome!
--
-- It is possible to use the template widget as a base for a custom 3rd party
-- widget module to offer more customization to the final user.
--
-- Here is an example of implementation for a custom widget inheriting from
-- `wibox.widget.template` :
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
-- @widgetmod wibox.widget.template
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local wbase = require("wibox.widget.base")
local gtable = require("gears.table")
local gtimer = require("gears.timer")

local template = {
    mt = {},
    queued_updates = {},
}

-- Layout this layout.
-- @method layout
-- @hidden
function template:layout(_, width, height)
    if not self._private.widget then
        return
    end

    return { wbase.place_widget_at(self._private.widget, 0, 0, width, height) }
end

-- Fit this layout into the given area.
-- @method fit
-- @hidden
function template:fit(context, width, height)
    if not self._private.widget then
        return 0, 0
    end

    return wbase.fit_widget(self, context, self._private.widget, width, height)
end

-- Draw the widget if it's actually a widget instance
-- @method draw
-- @hidden
function template:draw(...)
    if type(self._private.widget.draw) == "function" then
        return self._private.widget:draw(...)
    end
end

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
-- This function will call the `update_callback` function at the end of the
-- current GLib event loop. Updates are batched by event loop, it means that the
-- widget can only be update once by event loop. If the `template:update` method
-- is called multiple times during the same GLib event loop, only the first call
-- will be run.
-- All arguments are passed to the queued `update_callback` call.
-- @tparam[opt] table args A table to pass to the widget update function.
-- @method update
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
-- @tparam table|widget|function widget_template The new widget to use as a
--   template.
-- @method set_template
-- @emits widget::redraw_needed
-- @hidden
function template:set_template(widget_template)
    local widget = type(widget_template) == "function" and widget_template()
        or widget_template
        or wbase.empty_widget()

    local widget_instance = wbase.make_widget_from_value(widget)

    if widget_instance then
        wbase.check_widget(widget_instance)
    end

    self._private.template = widget
    self._private.widget = widget_instance

    -- We need to connect to these signals to actually redraw the template
    -- widget when its child needs to.
    local signals = {
        "widget::redraw_needed",
        "widget::layout_changed",
    }
    for _, signal in pairs(signals) do
        self._private.widget:connect_signal(signal, function(...)
            self:emit_signal(signal, ...)
        end)
    end

    self:emit_signal("widget::redraw_needed")
end

--- Give the internal widget instance.
-- @treturn widget The widget instance.
-- @method get_widget
-- @hidden
function template:get_widget()
    return self._private.widget
end

--- Set the update_callback property.
-- @tparam function update_callback The new callback function.
-- @method set_update_callback
-- @hidden
function template:set_update_callback(update_callback)
    assert(type(update_callback) == "function" or update_callback == nil)

    self._private.update_callback = update_callback
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

--- Create a new `wibox.widget.template` instance.
-- @tparam[opt] table args
-- @tparam[opt] table|widget|function args.template The widget template to use.
-- @tparam[opt] function args.update_callback The callback function to update
--   the widget.
-- @tparam[opt] boolean args.update_now Update the widget after its
--   construction. This will call the `:update()` method with no parameter.
-- @treturn wibox.widget.template The new instance.
-- @constructorfct wibox.widget.template
function template.new(args)
    args = args or {}

    local ret = wbase.make_widget(nil, nil, { enable_properties = true })

    gtable.crush(ret, template, true)

    ret:set_template(args.template)
    ret:set_update_callback(args.update_callback)
    ret:set_update_now(args.update_now)

    -- Apply the received buttons, visible, forced_width and so on
    gtable.crush(ret, args)

    return ret
end

function template.mt:__call(...)
    return template.new(...)
end

return setmetatable(template, template.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
