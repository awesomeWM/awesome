---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.widget
---------------------------------------------------------------------------

local cairo = require("lgi").cairo
local hierarchy = require("wibox.hierarchy")

local widget = {
    base = require("wibox.widget.base");
    textbox = require("wibox.widget.textbox");
    imagebox = require("wibox.widget.imagebox");
    background = require("wibox.widget.background");
    systray = require("wibox.widget.systray");
    textclock = require("wibox.widget.textclock");
    progressbar = require("wibox.widget.progressbar");
    graph = require("wibox.widget.graph");
    checkbox = require("wibox.widget.checkbox");
    piechart = require("wibox.widget.piechart");
    slider = require("wibox.widget.slider");
    calendar = require("wibox.widget.calendar");
    separator = require("wibox.widget.separator");
}

setmetatable(widget, {
    __call = function(_, args)
        return widget.base.make_widget_declarative(args)
    end
})

--- Draw a widget directly to a given cairo context.
-- This function creates a temporary `wibox.hierarchy` instance and uses that to
-- draw the given widget once to the given cairo context.
-- @tparam widget wdg A widget to draw
-- @tparam cairo_context cr The cairo context to draw the widget on
-- @tparam number width The width of the widget
-- @tparam number height The height of the widget
-- @tparam[opt={dpi=96}] table context The context information to give to the widget.
function widget.draw_to_cairo_context(wdg, cr, width, height, context)
    local function no_op() end
    context = context or {dpi=96}
    local h = hierarchy.new(context, wdg, width, height, no_op, no_op, {})
    h:draw(context, cr)
end

--- Create an SVG file showing this widget.
-- @tparam widget wdg A widget
-- @tparam string path The output file path
-- @tparam number width The surface width
-- @tparam number height The surface height
-- @tparam[opt={dpi=96}] table context The context information to give to the widget.
function widget.draw_to_svg_file(wdg, path, width, height, context)
    local img = cairo.SvgSurface.create(path, width, height)
    local cr = cairo.Context(img)
    widget.draw_to_cairo_context(wdg, cr, width, height, context)
    img:finish()
end

--- Create a cairo image surface showing this widget.
-- @tparam widget wdg A widget
-- @tparam number width The surface width
-- @tparam number height The surface height
-- @param[opt=cairo.Format.ARGB32] format The surface format
-- @tparam[opt={dpi=96}] table context The context information to give to the widget.
-- @return The cairo surface
function widget.draw_to_image_surface(wdg, width, height, format, context)
    local img = cairo.ImageSurface(format or cairo.Format.ARGB32, width, height)
    local cr = cairo.Context(img)
    widget.draw_to_cairo_context(wdg, cr, width, height, context)
    return img
end

return widget

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
