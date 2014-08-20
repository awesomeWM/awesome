---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local capi = {
    drawin = drawin,
    root = root,
    awesome = awesome
}
local setmetatable = setmetatable
local pairs = pairs
local type = type
local table = table
local string_format = string.format
local color = require("gears.color")
local object = require("gears.object")
local sort = require("gears.sort")
local beautiful = require("beautiful")
local surface = require("gears.surface")
local cairo = require("lgi").cairo

--- This provides widget box windows. Every wibox can also be used as if it were
-- a drawin. All drawin functions and properties are also available on wiboxes!
-- wibox
local wibox = { mt = {} }
wibox.layout = require("wibox.layout")
wibox.widget = require("wibox.widget")
wibox.drawable = require("wibox.drawable")

--- Set the widget that the wibox displays
function wibox:set_widget(widget)
    self._drawable:set_widget(widget)
end

--- Set the background of the wibox
-- @param c The background to use. This must either be a cairo pattern object,
--          nil or a string that gears.color() understands.
function wibox:set_bg(c)
    self._drawable:set_bg(c)
end

--- Set the foreground of the wibox
-- @param c The foreground to use. This must either be a cairo pattern object,
--          nil or a string that gears.color() understands.
function wibox:set_fg(c)
    self._drawable:set_fg(c)
end

--- Find a widget by a point.
-- The wibox must have drawn itself at least once for this to work.
-- @param x X coordinate of the point
-- @param y Y coordinate of the point
-- @return A sorted table with all widgets that contain the given point. The
--         widgets are sorted by relevance.
function wibox:find_widgets(x, y)
    return self._drawable:find_widgets(x, y)
end

for _, k in pairs{ "buttons", "struts", "geometry", "get_xproperty", "set_xproperty" } do
    wibox[k] = function(self, ...)
        return self.drawin[k](self.drawin, ...)
    end
end

local function setup_signals(_wibox)
    local w = _wibox.drawin

    local function clone_signal(name)
        _wibox:add_signal(name)
        -- When "name" is emitted on wibox.drawin, also emit it on wibox
        w:connect_signal(name, function(_, ...)
            _wibox:emit_signal(name, ...)
        end)
    end
    clone_signal("property::border_color")
    clone_signal("property::border_width")
    clone_signal("property::buttons")
    clone_signal("property::cursor")
    clone_signal("property::height")
    clone_signal("property::ontop")
    clone_signal("property::opacity")
    clone_signal("property::struts")
    clone_signal("property::visible")
    clone_signal("property::width")
    clone_signal("property::x")
    clone_signal("property::y")

    local d = _wibox._drawable
    local function clone_signal(name)
        _wibox:add_signal(name)
        -- When "name" is emitted on wibox.drawin, also emit it on wibox
        d:connect_signal(name, function(_, ...)
            _wibox:emit_signal(name, ...)
        end)
    end
    clone_signal("button::press")
    clone_signal("button::release")
    clone_signal("mouse::enter")
    clone_signal("mouse::leave")
    clone_signal("mouse::move")
    clone_signal("property::surface")
end

local function new(args)
    local ret = object()
    local w = capi.drawin(args)
    ret.drawin = w
    ret._drawable = wibox.drawable(w.drawable, ret)

    for k, v in pairs(wibox) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    setup_signals(ret)
    ret.draw = ret._drawable.draw
    ret.widget_at = function(_, widget, x, y, width, height)
        return ret._drawable:widget_at(widget, x, y, width, height)
    end

    -- Set the default background
    ret:set_bg(args.bg or beautiful.bg_normal)
    ret:set_fg(args.fg or beautiful.fg_normal)

    -- Make sure the wibox is drawn at least once
    ret.draw()

    -- Redirect all non-existing indexes to the "real" drawin
    setmetatable(ret, {
        __index = w,
        __newindex = w
    })

    return ret
end

--- Redraw a wibox. You should never have to call this explicitely because it is
-- automatically called when needed.
-- @param wibox
-- @name draw
-- @class function

--- Widget box object.
-- Every wibox "inherits" from a drawin and you can use all of drawin's
-- functions directly on this as well. When creating a wibox, you can specify a
-- "fg" and a "bg" color as keys in the table that is passed to the constructor.
-- All other arguments will be passed to drawin's constructor.
-- @class table
-- @name drawin

function wibox.mt:__call(...)
    return new(...)
end

return setmetatable(wibox, wibox.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
