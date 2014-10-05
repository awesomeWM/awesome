---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local pairs = pairs
local pcall = pcall
local print = print
local min = math.min
local max = math.max

-- wibox.layout.base
local base = {}

--- Figure out the geometry in device coordinate space. This gives only tight
-- bounds if no rotations by non-multiples of 90° are used.
function base.rect_to_device_geometry(cr, x, y, width, height)
    local x1, y1 = cr:user_to_device(x, y)
    local x2, y2 = cr:user_to_device(x, y + height)
    local x3, y3 = cr:user_to_device(x + width, y + height)
    local x4, y4 = cr:user_to_device(x + width, y)
    local x = min(x1, x2, x3, x4)
    local y = min(y1, y2, y3, y4)
    local width = max(x1, x2, x3, x4) - x
    local height = max(y1, y2, y3, y4) - y

    return x, y, width, height
end

--- Fit a widget for the given available width and height
-- @param widget The widget to fit (this uses widget:fit(width, height)).
-- @param width The available width for the widget
-- @param height The available height for the widget
-- @return The width and height that the widget wants to use
function base.fit_widget(widget, width, height)
    -- Sanitize the input. This also filters out e.g. NaN.
    local width = math.max(0, width)
    local height = math.max(0, height)

    -- Since the geometry cache is a weak table, we have to be careful when
    -- doing lookups. We can't do "if cache[width] ~= nil then"!
    local cache = widget._fit_geometry_cache
    local result = cache[width]
    if not result then
        result = {}
        cache[width] = result
    end
    cache, result = result, result[height]
    if not result then
        local w, h = widget:fit(width, height)
        result = { width = w, height = h }
        cache[height] = result
    end
    return result.width, result.height
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
    wibox:widget_at(widget, base.rect_to_device_geometry(cr, 0, 0, width, height))

    cr:restore()
end

return base

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
