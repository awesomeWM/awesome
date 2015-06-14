---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.widget.background
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local color = require("gears.color")
local layout_base = require("wibox.layout.base")
local surface = require("gears.surface")
local cairo = require("lgi").cairo
local setmetatable = setmetatable
local pairs = pairs
local type = type

local background = { mt = {} }

--- Draw this widget
function background:draw(context, cr, width, height)
    if not self.widget or not self.widget.visible then
        return
    end

    if self.background then
        cr:set_source(self.background)
        cr:paint()
    end
    if self.bgimage then
        local pattern = cairo.Pattern.create_for_surface(self.bgimage)
        cr:set_source(pattern)
        cr:paint()
    end
end

--- Prepare drawing the children of this widget
function background:before_draw_children(wibox, cr, width, height)
    if self.foreground then
        cr:set_source(self.foreground)
    end
end

--- Layout this widget
function background:layout(context, width, height)
    if self.widget then
        return { base.place_widget_at(self.widget, 0, 0, width, height) }
    end
end

--- Fit this widget into the given area
function background:fit(context, width, height)
    if not self.widget then
        return 0, 0
    end

    return layout_base.fit_widget(context, self.widget, width, height)
end

--- Set the widget that is drawn on top of the background
function background:set_widget(widget)
    if widget then
        base.check_widget(widget)
    end
    self.widget = widget
    self:emit_signal("widget::layout_changed")
end

--- Set the background to use
function background:set_bg(bg)
    if bg then
        self.background = color(bg)
    else
        self.background = nil
    end
    self:emit_signal("widget::redraw_needed")
end

--- Set the foreground to use
function background:set_fg(fg)
    if fg then
        self.foreground = color(fg)
    else
        self.foreground = nil
    end
    self:emit_signal("widget::redraw_needed")
end

--- Set the background image to use
function background:set_bgimage(image)
    self.bgimage = surface.load(image)
    self:emit_signal("widget::redraw_needed")
end

--- Returns a new background layout. A background layout applies a background
-- and foreground color to another widget.
-- @param[opt] widget The widget to display.
-- @param[opt] bg The background to use for that widget.
local function new(widget, bg)
    local ret = base.make_widget()

    for k, v in pairs(background) do
        if type(v) == "function" then
            ret[k] = v
        end
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
