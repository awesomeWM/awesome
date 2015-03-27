---------------------------------------------------------------------------
--- Mouse module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release @AWESOME_VERSION@
-- @module awful.mouse
---------------------------------------------------------------------------

-- Grab environment we need
local layout = require("awful.layout")
local tag = require("awful.tag")
local aclient = require("awful.client")
local widget = require("awful.widget")
local awibox = require("awful.wibox")
local util = require("awful.util")
local type = type
local math = math
local ipairs = ipairs
local capi =
{
    root = root,
    mouse = mouse,
    screen = screen,
    client = client,
    mousegrabber = mousegrabber,
}

local finder = require("awful.mouse.finder")

local mouse = {}

mouse.client = {}
mouse.wibox = {}

--- Get the client object under the pointer.
-- @return The client object under the pointer, if one can be found.
function mouse.client_under_pointer()
    local obj = capi.mouse.object_under_pointer()
    if type(obj) == "client" then
        return obj
    end
end

--- Get the drawin object under the pointer.
-- @return The drawin object under the pointer, if one can be found.
function mouse.drawin_under_pointer()
    local obj = capi.mouse.object_under_pointer()
    if type(obj) == "drawin" then
        return obj
    end
end

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
function mouse.client.snap(c, snap, x, y, fixed_x, fixed_y)
    local snap = snap or 8
    local c = c or mouse.client.focus
    local cur_geom = c:geometry()
    local geom = c:geometry()
    geom.width = geom.width + (2 * c.border_width)
    geom.height = geom.height + (2 * c.border_width)
    local edge = "none"
    local edge2 = "none"
    geom.x = x or geom.x
    geom.y = y or geom.y

    geom, edge = snap_inside(geom, capi.screen[c.screen].geometry, snap)
    geom = snap_inside(geom, capi.screen[c.screen].workarea, snap)

    -- Allow certain windows to snap to the edge of the workarea.
    -- Only allow docking to workarea for consistency/to avoid problems.
    if aclient.dockable.get(c) then
        local struts = c:struts()
        struts['left'] = 0
        struts['right'] = 0
        struts['top'] = 0
        struts['bottom'] = 0
        if edge ~= "none" and aclient.floating.get(c) then
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

    for k, snapper in ipairs(aclient.visible(c.screen)) do
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

--- Move a client.
-- @param c The client to move, or the focused one if nil.
-- @param snap The pixel to snap clients.
function mouse.client.move(c, snap)
    local c = c or capi.client.focus

    if not c
        or c.fullscreen
        or c.type == "desktop"
        or c.type == "splash"
        or c.type == "dock" then
        return
    end

    local orig = c:geometry()
    local m_c = capi.mouse.coords()
    local dist_x = m_c.x - orig.x
    local dist_y = m_c.y - orig.y
    -- Only allow moving in the non-maximized directions
    local fixed_x = c.maximized_horizontal
    local fixed_y = c.maximized_vertical

    capi.mousegrabber.run(function (_mouse)
                              for k, v in ipairs(_mouse.buttons) do
                                  if v then
                                      local lay = layout.get(c.screen)
                                      if lay == layout.suit.floating or aclient.floating.get(c) then
                                          local x = _mouse.x - dist_x
                                          local y = _mouse.y - dist_y
                                          c:geometry(mouse.client.snap(c, snap, x, y, fixed_x, fixed_y))
                                      elseif lay ~= layout.suit.magnifier then
                                          -- Only move the client to the mouse
                                          -- screen if the target screen is not
                                          -- floating.
                                          -- Otherwise, we move if via geometry.
                                          if layout.get(capi.mouse.screen) == layout.suit.floating then
                                              local x = _mouse.x - dist_x
                                              local y = _mouse.y - dist_y
                                              c:geometry(mouse.client.snap(c, snap, x, y, fixed_x, fixed_y))
                                          else
                                              c.screen = capi.mouse.screen
                                          end
                                          if layout.get(c.screen) ~= layout.suit.floating then
                                              local c_u_m = mouse.client_under_pointer()
                                              if c_u_m and not aclient.floating.get(c_u_m) then
                                                  if c_u_m ~= c then
                                                      c:swap(c_u_m)
                                                  end
                                              end
                                          end
                                      end
                                      return true
                                  end
                              end
                              return false
                          end, "fleur")
end

mouse.client.dragtotag = { }

--- Move a client to a tag by dragging it onto the left / right side of the screen
-- @param c The client to move
function mouse.client.dragtotag.border(c)
    capi.mousegrabber.run(function (_mouse)
                                local button_down = false
                                for _, v in ipairs(_mouse.buttons) do
                                    if v then button_down = true end
                                end
                                local wa = capi.screen[c.screen].workarea
                                if _mouse.x >= wa.x + wa.width then
                                    capi.mouse.coords({ x = wa.x + wa.width - 1 })
                                elseif _mouse.x <= wa.x then
                                    capi.mouse.coords({ x = wa.x + 1 })
                                end
                                if not button_down then
                                    local tags = tag.gettags(c.screen)
                                    local t = tag.selected()
                                    local idx
                                    for i, v in ipairs(tags) do
                                        if v == t then
                                            idx = i
                                        end
                                    end
                                    if _mouse.x > wa.x + wa.width - 10 then
                                        local newtag = tags[util.cycle(#tags, idx + 1)]
                                        aclient.movetotag(newtag, c)
                                        tag.viewnext()
                                    elseif _mouse.x < wa.x + 10 then
                                        local newtag = tags[util.cycle(#tags, idx - 1)]
                                        aclient.movetotag(newtag, c)
                                        tag.viewprev()
                                    end
                                    return false
                                end
                                return true
                            end, "fleur")
end

--- Move the wibox under the cursor
--@param w The wibox to move, or none to use that under the pointer
function mouse.wibox.move(w)
    local w = w or mouse.wibox_under_pointer()
    if not w then return end

    local offset = {
        x = w.x - capi.mouse.coords().x,
        y = w.y - capi.mouse.coords().y
    }

    capi.mousegrabber.run(function (_mouse)
        local button_down = false
        if awibox.get_position(w) == "floating" then
            w.x = capi.mouse.coords().x + offset.x
            w.y = capi.mouse.coords().y + offset.y
        else
            local wa = capi.screen[capi.mouse.screen].workarea

            if capi.mouse.coords()["y"] > wa.y + wa.height - 10 then
                awibox.set_position(w, "bottom", w.screen)
            elseif capi.mouse.coords()["y"] < wa.y + 10 then
                awibox.set_position(w, "top", w.screen)
            elseif capi.mouse.coords()["x"] > wa.x + wa.width - 10 then
                awibox.set_position(w, "right", w.screen)
            elseif capi.mouse.coords()["x"] < wa.x + 10 then
                awibox.set_position(w, "left", w.screen)
            end
            w.screen = capi.mouse.screen
        end
        for k, v in ipairs(_mouse.buttons) do
            if v then button_down = true end
        end
        if not button_down then
            return false
        end
        return true
    end, "fleur")
end

--- Get a client corner coordinates.
-- @param c The client to get corner from, focused one by default.
-- @param corner The corner to use: auto, top_left, top_right, bottom_left,
-- bottom_right. Default is auto, and auto find the nearest corner.
-- @return Actual used corner and x and y coordinates.
function mouse.client.corner(c, corner)
    local c = c or capi.client.focus
    if not c then return end

    local g = c:geometry()

    if not corner or corner == "auto" then
        local m_c = capi.mouse.coords()
        if math.abs(g.y - m_c.y) < math.abs(g.y + g.height - m_c.y) then
            if math.abs(g.x - m_c.x) < math.abs(g.x + g.width - m_c.x) then
                corner = "top_left"
            else
                corner = "top_right"
            end
        else
            if math.abs(g.x - m_c.x) < math.abs(g.x + g.width - m_c.x) then
                corner = "bottom_left"
            else
                corner = "bottom_right"
            end
        end
    end

    local x, y
    if corner == "top_right" then
        x = g.x + g.width
        y = g.y
    elseif corner == "top_left" then
        x = g.x
        y = g.y
    elseif corner == "bottom_left" then
        x = g.x
        y = g.y + g.height
    else
        x = g.x + g.width
        y = g.y + g.height
    end

    return corner, x, y
end

--- Resize a client.
-- @param c The client to resize, or the focused one by default.
-- @param corner The corner to grab on resize. Auto detected by default.
function mouse.client.resize(c, corner)
    local c = c or capi.client.focus

    if not c then return end

    if c.fullscreen
        or c.type == "desktop"
        or c.type == "splash"
        or c.type == "dock" then
        return
    end

    local lay = layout.get(c.screen)
    local corner, x, y = mouse.client.corner(c, corner)

    if lay == layout.suit.floating or aclient.floating.get(c) then
        return layout.suit.floating.mouse_resize_handler(c, corner, x, y)
    elseif lay.mouse_resize_handler then
        return lay.mouse_resize_handler(c, corner, x, y)
    end
end

-- Set the cursor at startup
capi.root.cursor("left_ptr")

return mouse

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
