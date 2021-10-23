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
    queued_updates = {}
}

function template:_do_update_now()
    if type(self.update_callback) == "function" then
        self:update_callback(self._private.update_args)
    end
    self._private.update_args = nil
    template.queued_updates[self] = false
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
        self._private.update_args = gtable.crush(gtable.clone(self._private.update_args or {}, false), args)
    end

    if not template.queued_updates[self] then
        gtimer.delayed_call(
            function()
                self:_do_update_now()
            end
        )
        template.queued_updates[self] = true
    end
end

--- Create a new `wibox.widget.template` instance.
-- @tparam[opt] table args
-- @tparam[opt] table|widget|function args.template The widget template to use.
-- @tparam[opt] function args.update_callback The callback function to update the widget.
-- @treturn wibox.widget.template The new instance.
function template.new(args)
    args = args or {}

    local widget_template = type(args.template) == "function" and
        args.template() or
        args.template or
        wbase.empty_widget()

    local widget = wbase.make_widget_from_value(widget_template)

    widget.update_callback = args.update_callback

    gtable.crush(widget, template, true)

    return widget
end

function template.mt:__call(...) --luacheck: ignore unused variable self
    return template.new(...)
end

return setmetatable(template, template.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
