---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.widget.base
---------------------------------------------------------------------------

local debug = require("gears.debug")
local object = require("gears.object")
local cache = require("gears.cache")
local setmetatable = setmetatable
local pairs = pairs
local type = type
local table = table

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
-- @tparam[opt] string widget_name Name of the widget.  If not set, it will be
--   set automatically via `gears.object.modulename`.
function base.make_widget(proxy, widget_name)
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
    local function cb(...)
        return ret:fit(...)
    end
    ret._fit_geometry_cache = cache.new(cb)
    ret:connect_signal("widget::updated", function()
        ret._fit_geometry_cache = cache.new(cb)
    end)

    -- Add visible property and setter.
    ret.visible = true
    function ret:set_visible(b)
        if b ~= self.visible then
            self.visible = b
            self:emit_signal("widget::updated")
        end
    end

    -- Add opacity property and setter.
    ret.opacity = 1
    function ret:set_opacity(b)
        if b ~= self.opacity then
            self.opacity = b
            self:emit_signal("widget::updated")
        end
    end

    -- Add __tostring method to metatable.
    ret.widget_name = widget_name or object.modulename(3)
    local mt = {}
    local orig_string = tostring(ret)
    mt.__tostring = function(o)
        return string.format("%s (%s)", ret.widget_name, orig_string)
    end
    return setmetatable(ret, mt)
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

    local width, height = widget:fit({}, 0, 0)
    debug.assert(type(width) == "number")
    debug.assert(type(height) == "number")
end

return base

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
