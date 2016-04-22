---------------------------------------------------------------------------
--- Mouse snapping related functions
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @submodule awful.mouse
---------------------------------------------------------------------------

local aclient = require("awful.client")

local capi = {
    root = root,
    mouse = mouse,
    screen = screen,
    client = client,
    mousegrabber = mousegrabber,
}

local module = {
    default_distance = 8
}

local function snap_outside(g, sg, snap)
    if g.x < snap + sg.x + sg.width and g.x > sg.x + sg.width then
        g.x = sg.x + sg.width
    elseif g.x + g.width < sg.x and g.x + g.width > sg.x - snap then
        g.x = sg.x - g.width
    end
    if g.y < snap + sg.y + sg.height and g.y > sg.y + sg.height then
        g.y = sg.y + sg.height
    elseif g.y + g.height < sg.y and g.y + g.height > sg.y - snap then
        g.y = sg.y - g.height
    end
    return g
end

local function snap_inside(g, sg, snap)
    local edgev = 'none'
    local edgeh = 'none'
    if math.abs(g.x) < snap + sg.x and g.x > sg.x then
        edgev = 'left'
        g.x = sg.x
    elseif math.abs((sg.x + sg.width) - (g.x + g.width)) < snap then
        edgev = 'right'
        g.x = sg.x + sg.width - g.width
    end
    if math.abs(g.y) < snap + sg.y and g.y > sg.y then
        edgeh = 'top'
        g.y = sg.y
    elseif math.abs((sg.y + sg.height) - (g.y + g.height)) < snap then
        edgeh = 'bottom'
        g.y = sg.y + sg.height - g.height
    end

    -- What is the dominant dimension?
    if g.width > g.height then
        return g, edgeh
    else
        return g, edgev
    end
end

--- Snap a client to the closest client or screen edge.
-- @param c The client to snap.
-- @param snap The pixel to snap clients.
-- @param x The client x coordinate.
-- @param y The client y coordinate.
-- @param fixed_x True if the client isn't allowed to move in the x direction.
-- @param fixed_y True if the client isn't allowed to move in the y direction.
function module.snap(c, snap, x, y, fixed_x, fixed_y)
    snap = snap or module.default_distance
    c = c or capi.client.focus
    local cur_geom = c:geometry()
    local geom = c:geometry()
    geom.width = geom.width + (2 * c.border_width)
    geom.height = geom.height + (2 * c.border_width)
    local edge
    geom.x = x or geom.x
    geom.y = y or geom.y

    geom, edge = snap_inside(geom, capi.screen[c.screen].geometry, snap)
    geom = snap_inside(geom, capi.screen[c.screen].workarea, snap)

    -- Allow certain windows to snap to the edge of the workarea.
    -- Only allow docking to workarea for consistency/to avoid problems.
    if c.dockable then
        local struts = c:struts()
        struts['left'] = 0
        struts['right'] = 0
        struts['top'] = 0
        struts['bottom'] = 0
        if edge ~= "none" and c.floating then
            if edge == "left" or edge == "right" then
                struts[edge] = cur_geom.width
            elseif edge == "top" or edge == "bottom" then
                struts[edge] = cur_geom.height
            end
        end
        c:struts(struts)
    end

    geom.x = geom.x - (2 * c.border_width)
    geom.y = geom.y - (2 * c.border_width)

    for _, snapper in ipairs(aclient.visible(c.screen)) do
        if snapper ~= c then
            geom = snap_outside(geom, snapper:geometry(), snap)
        end
    end

    geom.width = geom.width - (2 * c.border_width)
    geom.height = geom.height - (2 * c.border_width)
    geom.x = geom.x + (2 * c.border_width)
    geom.y = geom.y + (2 * c.border_width)

    -- It's easiest to undo changes afterwards if they're not allowed
    if fixed_x then geom.x = cur_geom.x end
    if fixed_y then geom.y = cur_geom.y end

    return geom
end

return setmetatable(module, {__call = function(_, ...) return module.snap(...) end})

