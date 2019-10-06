---------------------------------------------------------------------------
--
-- A container that makes a widget display only on a specified screen.
--
-- @author Uli Schlachter
-- @copyright 2017 Uli Schlachter
-- @containermod awful.widget.only_on_screen
---------------------------------------------------------------------------

local type = type
local pairs = pairs
local setmetatable = setmetatable
local base = require("wibox.widget.base")
local gtable = require("gears.table")
local capi = {
    screen = screen,
    awesome = awesome
}

local only_on_screen = { mt = {} }

local instances = setmetatable({}, { __mode = "k" })

local function should_display_on(self, s)
    if not self._private.widget then
        return false
    end
    local own_s = self._private.screen
    if type(own_s) == "number" and (own_s < 1 or own_s > capi.screen.count()) then
        -- Invalid screen number
        return false
    end
    return capi.screen[self._private.screen] == s
end

-- Layout this layout
function only_on_screen:layout(context, ...)
    if not should_display_on(self, context.screen) then return end
    return { base.place_widget_at(self._private.widget, 0, 0, ...) }
end

-- Fit this layout into the given area
function only_on_screen:fit(context, ...)
    if not should_display_on(self, context.screen) then
        return 0, 0
    end
    return base.fit_widget(self, context, self._private.widget, ...)
end

--- The widget to be displayed
-- @property widget
-- @tparam widget widget The widget

only_on_screen.set_widget = base.set_widget_common

function only_on_screen:get_widget()
    return self._private.widget
end

function only_on_screen:get_children()
    return {self._private.widget}
end

function only_on_screen:set_children(children)
    self:set_widget(children[1])
end

--- The screen to display on. Can be a screen object, a screen index, a screen
-- name ("VGA1") or the string "primary" for the primary screen.
-- @property screen
-- @tparam screen|string|integer screen The screen.

function only_on_screen:set_screen(s)
    self._private.screen = s
    self:emit_signal("widget::layout_changed")
end

function only_on_screen:get_screen()
    return self._private.screen
end

--- Returns a new only_on_screen container.
-- This widget makes some other widget visible on just some screens. Use
-- `:set_widget()` to set the widget and `:set_screen()` to set the screen.
-- @param[opt] widget The widget to display.
-- @param[opt] s The screen to display on.
-- @treturn table A new only_on_screen container
-- @constructorfct awful.widget.only_on_screen
local function new(widget, s)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, only_on_screen, true)

    ret:set_widget(widget)
    ret:set_screen(s or "primary")

    instances[ret] = true

    return ret
end

function only_on_screen.mt:__call(...)
    return new(...)
end

-- Handle lots of cases where a screen changes and thus this widget jumps around

capi.screen.connect_signal("primary_changed", function()
    for widget in pairs(instances) do
        if widget._private.widget and widget._private.screen == "primary" then
            widget:emit_signal("widget::layout_changed")
        end
    end
end)

capi.screen.connect_signal("list", function()
    for widget in pairs(instances) do
        if widget._private.widget and type(widget._private.screen) == "number" then
            widget:emit_signal("widget::layout_changed")
        end
    end
end)

capi.screen.connect_signal("property::outputs", function()
    for widget in pairs(instances) do
        if widget._private.widget and type(widget._private.screen) == "string" then
            widget:emit_signal("widget::layout_changed")
        end
    end
end)

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(only_on_screen, only_on_screen.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
