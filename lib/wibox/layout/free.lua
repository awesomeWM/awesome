---------------------------------------------------------------------------
-- A layout with widgets added at specific positions.
--
-- Use cases include desktop icons, complex custom composed widgets, a floating
-- client layout and fine grained control over the output.
--
--@DOC_wibox_layout_defaults_free_EXAMPLE@
-- @author Emmanuel Lepage Vallee
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.free
---------------------------------------------------------------------------
local util = require("awful.util")
local base = require("wibox.widget.base")

local free_layout = {}

function free_layout:fit(_, width, height)
    return width, height
end

function free_layout:layout(context, width, height)
    local res = {}

    for k, v in ipairs(self._private.widgets) do
        local pt = self._private.pos[k] or {0,0}
        local w, h = base.fit_widget(self, context, v, width, height)

        if type(pt) == "function" then
            pt = pt(width, height, w, h)
        end

        table.insert(res, base.place_widget_at(v, pt[1], pt[2], w, h))
    end

    return res
end

function free_layout:add(...)
    local wdgs = {...}
    util.table.merge(self._private.widgets, {...})

    -- Add the points
    for k, v in ipairs(wdgs) do
        if v.point then
            self._private.pos[k] = v.point
        end
    end

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::redraw_needed")
end

function free_layout:add_at(widget, point)
    self._private.pos[#self._private.widgets+1] = point
    self:add(widget)
end

function free_layout:get_children()
    return self._private.widgets
end

function free_layout:set_children(children)
    self:reset()

    -- It is async, so this is ok
    self._private.widgets = children

    -- To make the declarative syntax work
    for k, v in ipairs(self._private.widgets) do
        if v.point then
            self._private.pos[k] = v.point
        end
    end
end

function free_layout:reset()
    self._private.widgets = {}
    self._private.pos     = {}
    self:emit_signal( "widget::layout_changed" )
    self:emit_signal( "widget::redraw_needed"  )
end

local function new_free(...)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    util.table.crush(ret, free_layout, true)
    ret._private.widgets = {}
    ret._private.pos = {}

    return ret
end

return setmetatable(free_layout, {__call=function(_,...) return new_free(...) end})
