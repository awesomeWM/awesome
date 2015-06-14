---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.base
---------------------------------------------------------------------------

local xpcall = xpcall
local print = print
local cairo = require("lgi").cairo
local wbase = require("wibox.widget.base")

local base = {}

--- Fit a widget for the given available width and height
-- @param context The context in which we are fit.
-- @param widget The widget to fit (this uses widget:fit(width, height)).
-- @param width The available width for the widget
-- @param height The available height for the widget
-- @return The width and height that the widget wants to use
function base.fit_widget(context, widget, width, height)
    if not widget.visible then
        return 0, 0
    end
    -- Sanitize the input. This also filters out e.g. NaN.
    local width = math.max(0, width)
    local height = math.max(0, height)

    local w, h = widget:fit(context, width, height)

    -- Also sanitize the output.
    w = math.max(0, math.min(w, width))
    h = math.max(0, math.min(h, height))
    return w, h
end

--- Draw a widget via a cairo context
-- @param context The context in which we are drawn.
-- @param cr The cairo context used
-- @param widget The widget to draw (this uses widget:draw(cr, width, height)).
-- @param x The position that the widget should get
-- @param y The position that the widget should get
-- @param width The widget's width
-- @param height The widget's height
function base.draw_widget(context, cr, widget, x, y, width, height)
    if not widget.visible then
        return
    end

    -- Use save() / restore() so that our modifications aren't permanent
    cr:save()

    -- Move (0, 0) to the place where the widget should show up
    cr:translate(x, y)

    -- Make sure the widget cannot draw outside of the allowed area
    cr:rectangle(0, 0, width, height)
    cr:clip()

    if widget.opacity ~= 1 then
        cr:push_group()
    end
    -- Let the widget draw itself
    xpcall(function()
        widget:draw(context, cr, width, height)
    end, function(err)
        print(debug.traceback("Error while drawing widget: "..tostring(err), 2))
    end)
    if widget.opacity ~= 1 then
        cr:pop_group_to_source()
        cr.operator = cairo.Operator.OVER
        cr:paint_with_alpha(widget.opacity)
    end

    -- Register the widget for input handling
    context:widget_at(widget, wbase.rect_to_device_geometry(cr, 0, 0, width, height))

    cr:restore()
end

return base

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
