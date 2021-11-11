---------------------------------------------------------------------------
-- An abstract widget that handles a preset of concrete widget.
--
-- The `wibox.widget.template` widget is an abstraction layer that contains a
-- concrete widget definition. The template widget can be used to build widgets
-- that the user can customize at their will, thanks to the template mechanism.
--
-- @author Aire-One
-- @copyright 2021 Aire-One <aireone@aireone.xyz>
--
-- @classmod wibox.widget.template
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local wbase = require("wibox.widget.base")
local gtable = require("gears.table")
local gtimer = require("gears.timer")

local template = {
    mt = {},
    queued_updates = {},
}

-- Layout this layout
function template:layout(_, width, height)
    if not self._private.widget then
        return
    end

    return { wbase.place_widget_at(self._private.widget, 0, 0, width, height) }
end

-- Fit this layout into the given area.
function template:fit(context, width, height)
    if not self._private.widget then
        return 0, 0
    end

    return wbase.fit_widget(self, context, self._private.widget, width, height)
end

function template:draw(...)
    if type(self._private.widget.draw) == "function" then
        return self._private.widget:draw(...)
    end
end

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

function template:set_template(widget_template)
    local widget = type(widget_template) == "function" and widget_template()
        or widget_template
        or wbase.empty_widget()

    self._private.template = widget
    self._private.widget = wbase.make_widget_from_value(widget)

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

function template:get_widget()
    return self._private.widget
end

function template:set_update_callback(update_callback)
    assert(type(update_callback) == "function" or update_callback == nil)

    self._private.update_callback = update_callback
end

-- @hidden
function template:set_update_now(update_now)
    if update_now then
        self:update()
    end
end

--- Create a new `wibox.widget.template` instance.
-- @tparam[opt] table args
-- @tparam[opt] table|widget|function args.template The widget template to use.
-- @tparam[opt] function args.update_callback The callback function to update the widget.
-- @treturn wibox.widget.template The new instance.
function template.new(args)
    args = args or {}

    local ret = wbase.make_widget(nil, nil, { enable_properties = true })

    gtable.crush(ret, template, true)

    ret:set_template(args.template)
    ret:set_update_callback(args.update_callback)
    ret:set_update_now(args.update_now)

    if args.buttons then
        ret:set_buttons(args.buttons)
    end

    return ret
end

function template.mt:__call(...)
    return template.new(...)
end

return setmetatable(template, template.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
