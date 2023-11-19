---------------------------------------------------------------------------
-- A simple container that will hide its child widget based a
-- boolean state variable.
--
--@DOC_wibox_container_conditional_comparison_EXAMPLE@
--
-- @author Lucas Schwiderski
-- @copyright 2022 Lucas Schwiderski
-- @containermod wibox.container.conditional
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local gtable = require("gears.table")
local gobject = require("gears.object")

local conditional = { mt = {} }


function conditional:fit(context, width, height)
    if not self._private.state then
        return 0, 0
    end

    return base.fit_widget(self, context, self._private.widget, width, height)
end


function conditional:layout(_, width, height)
    if not self._private.state then
        return {}
    end

    return { base.place_widget_at(self._private.widget, 0, 0, width, height) }
end


--- The conditional state.
--
-- The child widget is shown or hidden based on this state.
--
-- @property state
-- @tparam boolean state
-- @propemits true false

function conditional:set_state(state)
    state = not (not state)

    if self._private.state == state then
        return
    end

    self._private.state = state

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::state", state)
end

function conditional:get_state()
    return self._private.state
end


--- The widget to be displayed based on the state.
-- @property widget
-- @tparam widget widget
-- @propemits true false
-- @interface container

conditional.set_widget = base.set_widget_common
function conditional:get_widget()
    return self._private.widget
end


function conditional:set_children(widgets)
    self:set_widget(widgets[1])
end

function conditional:get_children()
    return { self._private.widget }
end


local function new(widget, state)
    local ret = base.make_widget(nil, nil, { enable_properties = true})

    gtable.crush(ret, conditional, true)
    ret.widget_name = gobject.modulename(2)

    ret._private.widget = widget

    if type(state) == "boolean" then
        ret._private.state = state
    else
        ret._private.state = true
    end

    return ret
end


function conditional.mt:__call(...)
    return new(...)
end

return setmetatable(conditional, conditional.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
