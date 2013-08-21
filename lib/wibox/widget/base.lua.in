---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local debug = require("gears.debug")
local object = require("gears.object")
local setmetatable = setmetatable
local pairs = pairs
local type = type
local table = table

-- wibox.widget.base
local base = {}

--- Set/get a widget's buttons
function base:buttons(_buttons)
    if _buttons then
        self.widget_buttons = _buttons
    end

    return self.widget_buttons
end

--- Handle a button event on a widget. This is used internally.
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

--- Create a new widget. All widgets have to be generated via this function so
-- that the needed signals are added and mouse input handling is set up.
-- @param proxy If this is set, the returned widget will be a proxy for this
--              widget. It will be equivalent to this widget.
function base.make_widget(proxy)
    local ret = object()

    -- This signal is used by layouts to find out when they have to update.
    ret:add_signal("widget::updated")
    -- Mouse input, oh noes!
    ret:add_signal("button::press")
    ret:add_signal("button::release")
    ret:add_signal("mouse::enter")
    ret:add_signal("mouse::leave")

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
        ret.draw = function(_, ...) return proxy:draw(...) end
        ret.fit = function(_, ...) return proxy:fit(...) end
        proxy:connect_signal("widget::updated", function()
            ret:emit_signal("widget::updated")
        end)
    end

    -- Add a geometry for base.fit_widget() that is cleared when necessary
    ret._fit_geometry_cache = setmetatable({}, { __mode = 'v' })
    ret:connect_signal("widget::updated", function()
        ret._fit_geometry_cache = setmetatable({}, { __mode = 'v' })
    end)

    return ret
end

--- Generate an empty widget which takes no space and displays nothing
function base.empty_widget()
    local widget = base.make_widget()
    widget.draw = function() end
    widget.fit = function() return 0, 0 end
    return widget
end

--- Do some sanity checking on widget. This function raises a lua error if
-- widget is not a valid widget.
function base.check_widget(widget)
    debug.assert(type(widget) == "table")
    for k, func in pairs({ "draw", "fit", "add_signal", "connect_signal", "disconnect_signal" }) do
        debug.assert(type(widget[func]) == "function", func .. " is not a function")
    end

    local width, height = widget:fit(0, 0)
    debug.assert(type(width) == "number")
    debug.assert(type(height) == "number")
end

return base

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
