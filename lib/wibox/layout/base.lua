---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.base
---------------------------------------------------------------------------

local pcall = pcall
local print = print
local wbase = require("wibox.widget.base")

local base = {}

--- Fit a widget for the given available width and height
-- @param widget The widget to fit (this uses widget:fit(width, height)).
-- @param width The available width for the widget
-- @param height The available height for the widget
-- @return The width and height that the widget wants to use
function base.fit_widget(widget, width, height)
    if not widget.visible then
        return 0, 0
    end
    -- Sanitize the input. This also filters out e.g. NaN.
    local width = math.max(0, width)
    local height = math.max(0, height)

    return widget._fit_geometry_cache:get(width, height)
end

--- Draw a widget via a cairo context
-- @param wibox The wibox on which we are drawing
-- @param cr The cairo context used
-- @param widget The widget to draw (this uses widget:draw(cr, width, height)).
-- @param x The position that the widget should get
-- @param y The position that the widget should get
-- @param width The widget's width
-- @param height The widget's height
function base.draw_widget(wibox, cr, widget, x, y, width, height)
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

    -- Let the widget draw itself
    local success, msg = pcall(widget.draw, widget, wibox, cr, width, height)
    if not success then
        print("Error while drawing widget: " .. msg)
    end

    -- Register the widget for input handling
    wibox:widget_at(widget, wbase.rect_to_device_geometry(cr, 0, 0, width, height))

    cr:restore()
end

return base

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
