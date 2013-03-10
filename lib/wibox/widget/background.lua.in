---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local color = require("gears.color")
local layout_base = require("wibox.layout.base")
local surface = require("gears.surface")
local cairo = require("lgi").cairo
local setmetatable = setmetatable
local pairs = pairs
local type = type

-- wibox.widget.background
local background = { mt = {} }

--- Draw this widget
function background:draw(wibox, cr, width, height)
    if not self.widget then
        return
    end

    cr:save()

    if self.background then
        cr:set_source(self.background)
        cr:paint()
    end
    if self.bgimage then
        local pattern = cairo.Pattern.create_for_surface(self.bgimage)
        cr:set_source(pattern)
        cr:paint()
    end

    cr:restore()

    if self.foreground then
        cr:save()
        cr:set_source(self.foreground)
    end
    layout_base.draw_widget(wibox, cr, self.widget, 0, 0, width, height)
    if self.foreground then
        cr:restore()
    end
end

--- Fit this widget into the given area
function background:fit(width, height)
    if not self.widget then
        return 0, 0
    end

    return self.widget:fit(width, height)
end

--- Set the widget that is drawn on top of the background
function background:set_widget(widget)
    if self.widget then
        self.widget:disconnect_signal("widget::updated", self._emit_updated)
    end
    if widget then
        base.check_widget(widget)
        widget:connect_signal("widget::updated", self._emit_updated)
    end
    self.widget = widget
    self._emit_updated()
end

--- Set the background to use
function background:set_bg(bg)
    if bg then
        self.background = color(bg)
    else
        self.background = nil
    end
    self._emit_updated()
end

--- Set the foreground to use
function background:set_fg(fg)
    if fg then
        self.foreground = color(fg)
    else
        self.foreground = nil
    end
    self._emit_updated()
end

--- Set the background image to use
function background:set_bgimage(image)
    self.bgimage = surface.load(image)
    self._emit_updated()
end

--- Returns a new background layout. A background layout applies a background
-- and foreground color to another widget.
-- @param widget The widget to display (optional)
-- @param bg The background to use for that widget (optional)
local function new(widget, bg)
    local ret = base.make_widget()

    for k, v in pairs(background) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret._emit_updated = function()
        ret:emit_signal("widget::updated")
    end

    ret:set_widget(widget)
    ret:set_bg(bg)

    return ret
end

function background.mt:__call(...)
    return new(...)
end

return setmetatable(background, background.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
