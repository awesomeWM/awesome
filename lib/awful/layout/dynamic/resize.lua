---------------------------------------------------------------------------
--- Utilities required to resize a client wrapped in ratio layouts
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @module awful.layout.dynamic.resize
---------------------------------------------------------------------------
local util   = require( "awful.util"      )
local cairo  = require( "lgi"             ).cairo
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

-- Loop the path between the client widget and the layout to find nodes
-- capable of resizing in both directions
local function ratio_lookup(handler, wrapper)
    local _, parent, path = handler.widget:index(wrapper, true)
    local res = {}

    local full_path = util.table.join({parent}, path)

    for i=#full_path, 1, -1 do
        local w = full_path[i]
        if w.inc_ratio then
            res[w._private.dir] = res[w._private.dir] or {
                layout = w,
                widget = full_path[i-1] or wrapper
            }
        end
    end

    return res
end

-- Compute the new ratio before, for and after geo
local function compute_ratio(workarea, geo)
    local x_before = (geo.x - workarea.x) / workarea.width
    local x_self   = (geo.width         ) / workarea.width
    local x_after  = 1 - x_before - x_self
    local y_before = (geo.y - workarea.y) / workarea.height
    local y_self   = (geo.height        ) / workarea.height
    local y_after  = 1 - y_before - y_self

    return {
        x = { math.max(0, x_before), math.max(0, x_self), math.max(0, x_after) },
        y = { math.max(0, y_before), math.max(0, y_self), math.max(0, y_after) },
    }
end

-- If there is a ratio based layout somewhere, try to get all geometry updated
-- This method is mostly for internal purpose, use `ajust_geometry`
-- @param handler The layout handler
-- @tparam client c The client
-- @param widget The client wrapper
-- @param geo The new geometry
local function update_ratio(handler, c, widget, hints) --luacheck: no unused args
    local ratio_wdgs = ratio_lookup(handler, widget)

    -- Apply the size hints
    if c.apply_size_hints then
        local w, h = c:apply_size_hints(
            hints.width  - 2*hints.gap - 2*c.border_width,
            hints.height - 2*hints.gap - 2*c.border_width
        )
        hints = {
            x      = hints.x,
            y      = hints.y,
            width  = w + 2*hints.gap + 2*c.border_width,
            height = h + 2*hints.gap + 2*c.border_width,
        }
    end

    -- Cairo doesn't like tables with extra keys
    local geo_rect = cairo.RectangleInt {
        x      = hints.x,
        y      = hints.y,
        width  = hints.width,
        height = hints.height,
    }

    -- Layouts only span one screen, fit `geo` into that screen
    local region = cairo.Region.create_rectangle(geo_rect)
    region:intersect(cairo.Region.create_rectangle(
        cairo.RectangleInt(handler.param.workarea)
    ))
    hints = region:get_rectangle(0)

    -- Don't waste time, it will go wrong anyway
    if hints.width == 0 or hints.height == 0 then return end

    local ratio = compute_ratio(handler.param.workarea, hints)

    if ratio_wdgs.x then
        ratio_wdgs.x.layout:ajust_widget_ratio(ratio_wdgs.x.widget, unpack(ratio.x))
    end
    if ratio_wdgs.y then
        ratio_wdgs.y.layout:ajust_widget_ratio(ratio_wdgs.y.widget, unpack(ratio.y))
    end

    handler.widget:emit_signal("widget::redraw_needed")
end

return update_ratio
