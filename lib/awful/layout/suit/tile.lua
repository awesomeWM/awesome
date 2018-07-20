---------------------------------------------------------------------------
--- Tiled layouts module for awful
--
-- @author Donald Ephraim Curtis &lt;dcurtis@cs.uiowa.edu&gt;
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Donald Ephraim Curtis
-- @copyright 2008 Julien Danjou
-- @module awful.layout
---------------------------------------------------------------------------

-- Grab environment we need
local tag = require("awful.tag")
local client = require("awful.client")
local ipairs = ipairs
local math = math
local capi =
{
    mouse = mouse,
    screen = screen,
    mousegrabber = mousegrabber
}

local tile = {}

--- The tile layout layoutbox icon.
-- @beautiful beautiful.layout_tile
-- @param surface
-- @see gears.surface

--- The tile top layout layoutbox icon.
-- @beautiful beautiful.layout_tiletop
-- @param surface
-- @see gears.surface

--- The tile bottom layout layoutbox icon.
-- @beautiful beautiful.layout_tilebottom
-- @param surface
-- @see gears.surface

--- The tile left layout layoutbox icon.
-- @beautiful beautiful.layout_tileleft
-- @param surface
-- @see gears.surface

--- Jump mouse cursor to the client's corner when resizing it.
tile.resize_jump_to_corner = true

local function mouse_resize_handler(c, _, _, _, orientation)
    orientation = orientation or "tile"
    local wa = c.screen.workarea
    local mwfact = c.screen.selected_tag.master_width_factor
    local cursor
    local g = c:geometry()
    local offset = 0
    local corner_coords
    local coordinates_delta = {x=0,y=0}

    if orientation == "tile" then
        cursor = "cross"
        if g.height+15 > wa.height then
            offset = g.height * .5
            cursor = "sb_h_double_arrow"
        elseif not (g.y+g.height+15 > wa.y+wa.height) then
            offset = g.height
        end
        corner_coords = { x = wa.x + wa.width * mwfact, y = g.y + offset }
    elseif orientation == "left" then
        cursor = "cross"
        if g.height+15 >= wa.height then
            offset = g.height * .5
            cursor = "sb_h_double_arrow"
        elseif not (g.y+g.height+15 > wa.y+wa.height) then
            offset = g.height
        end
        corner_coords = { x = wa.x + wa.width * (1 - mwfact), y = g.y + offset }
    elseif orientation == "bottom" then
        cursor = "cross"
        if g.width+15 >= wa.width then
            offset = g.width * .5
            cursor = "sb_v_double_arrow"
        elseif not (g.x+g.width+15 > wa.x+wa.width) then
            offset = g.width
        end
        corner_coords = { y = wa.y + wa.height * mwfact, x = g.x + offset}
    else
        cursor = "cross"
        if g.width+15 >= wa.width then
            offset = g.width * .5
            cursor = "sb_v_double_arrow"
        elseif not (g.x+g.width+15 > wa.x+wa.width) then
            offset = g.width
        end
        corner_coords = { y = wa.y + wa.height * (1 - mwfact), x= g.x + offset }
    end
    if tile.resize_jump_to_corner then
        capi.mouse.coords(corner_coords)
    else
        local mouse_coords = capi.mouse.coords()
        coordinates_delta = {
          x = corner_coords.x - mouse_coords.x,
          y = corner_coords.y - mouse_coords.y,
        }
    end

    local prev_coords = {}
    capi.mousegrabber.run(function (_mouse)
                              if not c.valid then return false end

                              _mouse.x = _mouse.x + coordinates_delta.x
                              _mouse.y = _mouse.y + coordinates_delta.y
                              for _, v in ipairs(_mouse.buttons) do
                                  if v then
                                      prev_coords = { x =_mouse.x, y = _mouse.y }
                                      local fact_x = (_mouse.x - wa.x) / wa.width
                                      local fact_y = (_mouse.y - wa.y) / wa.height
                                      local new_mwfact

                                      local geom = c:geometry()

                                      -- we have to make sure we're not on the last visible
                                      -- client where we have to use different settings.
                                      local wfact
                                      local wfact_x, wfact_y
                                      if (geom.y+geom.height+15) > (wa.y+wa.height) then
                                          wfact_y = (geom.y + geom.height - _mouse.y) / wa.height
                                      else
                                          wfact_y = (_mouse.y - geom.y) / wa.height
                                      end

                                      if (geom.x+geom.width+15) > (wa.x+wa.width) then
                                          wfact_x = (geom.x + geom.width - _mouse.x) / wa.width
                                      else
                                          wfact_x = (_mouse.x - geom.x) / wa.width
                                      end


                                      if orientation == "tile" then
                                          new_mwfact = fact_x
                                          wfact = wfact_y
                                      elseif orientation == "left" then
                                          new_mwfact = 1 - fact_x
                                          wfact = wfact_y
                                      elseif orientation == "bottom" then
                                          new_mwfact = fact_y
                                          wfact = wfact_x
                                      else
                                          new_mwfact = 1 - fact_y
                                          wfact = wfact_x
                                      end

                                      c.screen.selected_tag.master_width_factor
                                        = math.min(math.max(new_mwfact, 0.01), 0.99)
                                      client.setwfact(math.min(math.max(wfact,0.01), 0.99), c)
                                      return true
                                  end
                              end
                              return prev_coords.x == _mouse.x and prev_coords.y == _mouse.y
                          end, cursor)
end

local function apply_size_hints(c, width, height, useless_gap)
    local bw = c.border_width
    width, height = width - 2 * bw - useless_gap, height - 2 * bw - useless_gap
    width, height = c:apply_size_hints(math.max(1, width), math.max(1, height))
    return width + 2 * bw + useless_gap, height + 2 * bw + useless_gap
end

local function tile_group(gs, cls, wa, orientation, fact, group, useless_gap)
    -- get our orientation right
    local height = "height"
    local width = "width"
    local x = "x"
    local y = "y"
    if orientation == "top" or orientation == "bottom" then
        height = "width"
        width = "height"
        x = "y"
        y = "x"
    end

    -- make this more generic (not just width)
    local available = wa[width] - (group.coord - wa[x])

    -- find our total values
    local total_fact = 0
    local min_fact = 1
    local size = group.size
    for c = group.first,group.last do
        -- determine the width/height based on the size_hint
        local i = c - group.first +1
        local size_hints = cls[c].size_hints
        local size_hint = size_hints["min_"..width] or size_hints["base_"..width] or 0
        size = math.max(size_hint, size)

        -- calculate the height
        if not fact[i] then
            fact[i] = min_fact
        else
            min_fact = math.min(fact[i],min_fact)
        end
        total_fact = total_fact + fact[i]
    end
    size = math.max(1, math.min(size, available))

    local coord = wa[y]
    local used_size = 0
    local unused = wa[height]
    for c = group.first,group.last do
        local geom = {}
        local hints = {}
        local i = c - group.first +1
        geom[width] = size
        geom[height] = math.max(1, math.floor(unused * fact[i] / total_fact))
        geom[x] = group.coord
        geom[y] = coord
        gs[cls[c]] = geom
        hints.width, hints.height = apply_size_hints(cls[c], geom.width, geom.height, useless_gap)
        coord = coord + hints[height]
        unused = unused - hints[height]
        total_fact = total_fact - fact[i]
        used_size = math.max(used_size, hints[width])
    end

    return used_size
end

local function do_tile(param, orientation)
    local t = param.tag or capi.screen[param.screen].selected_tag
    orientation = orientation or "right"

    -- This handles all different orientations.
    local width = "width"
    local x = "x"
    if orientation == "top" or orientation == "bottom" then
        width = "height"
        x = "y"
    end

    local gs = param.geometries
    local cls = param.clients
    local useless_gap = param.useless_gap
    local nmaster = math.min(t.master_count, #cls)
    local nother = math.max(#cls - nmaster,0)

    local mwfact = t.master_width_factor
    local wa = param.workarea
    local ncol = t.column_count

    local data = tag.getdata(t).windowfact

    if not data then
        data = {}
        tag.getdata(t).windowfact = data
    end

    local coord = wa[x]
    local place_master = true
    if orientation == "left" or orientation == "top" then
        -- if we are on the left or top we need to render the other windows first
        place_master = false
    end

    local grow_master = t.master_fill_policy == "expand"
    -- this was easier than writing functions because there is a lot of data we need
    for _ = 1,2 do
        if place_master and nmaster > 0 then
            local size = wa[width]
            if nother > 0 or not grow_master then
                size = math.min(wa[width] * mwfact, wa[width] - (coord - wa[x]))
            end
            if nother == 0 and not grow_master then
              coord = coord + (wa[width] - size)/2
            end
            if not data[0] then
                data[0] = {}
            end
            coord = coord + tile_group(gs, cls, wa, orientation, data[0],
                                       {first=1, last=nmaster, coord = coord, size = size}, useless_gap)
        end

        if not place_master and nother > 0 then
            local last = nmaster

            -- we have to modify the work area size to consider left and top views
            local wasize = wa[width]
            if nmaster > 0 and (orientation == "left" or orientation == "top") then
                wasize = wa[width] - wa[width]*mwfact
            end
            for i = 1,ncol do
                -- Try to get equal width among remaining columns
                local size = math.min( (wasize - (coord - wa[x])) / (ncol - i + 1) )
                local first = last + 1
                last = last + math.floor((#cls - last)/(ncol - i + 1))
                -- tile the column and update our current x coordinate
                if not data[i] then
                    data[i] = {}
                end
                coord = coord + tile_group(gs, cls, wa, orientation, data[i],
                                           { first = first, last = last, coord = coord, size = size }, useless_gap)
            end
        end
        place_master = not place_master
    end

end

function tile.skip_gap(nclients, t)
    return nclients == 1 and t.master_fill_policy == "expand"
end

--- The main tile algo, on the right.
-- @param screen The screen number to tile.
-- @clientlayout awful.layout.suit.tile.right
tile.right = {}
tile.right.name = "tile"
tile.right.arrange = do_tile
tile.right.skip_gap = tile.skip_gap
function tile.right.mouse_resize_handler(c, corner, x, y)
    return mouse_resize_handler(c, corner, x, y)
end

--- The main tile algo, on the left.
-- @param screen The screen number to tile.
-- @clientlayout awful.layout.suit.tile.left
tile.left = {}
tile.left.name = "tileleft"
tile.left.skip_gap = tile.skip_gap
function tile.left.arrange(p)
    return do_tile(p, "left")
end
function tile.left.mouse_resize_handler(c, corner, x, y)
    return mouse_resize_handler(c, corner, x, y, "left")
end

--- The main tile algo, on the bottom.
-- @param screen The screen number to tile.
-- @clientlayout awful.layout.suit.tile.bottom
tile.bottom = {}
tile.bottom.name = "tilebottom"
tile.bottom.skip_gap = tile.skip_gap
function tile.bottom.arrange(p)
    return do_tile(p, "bottom")
end
function tile.bottom.mouse_resize_handler(c, corner, x, y)
    return mouse_resize_handler(c, corner, x, y, "bottom")
end

--- The main tile algo, on the top.
-- @param screen The screen number to tile.
-- @clientlayout awful.layout.suit.tile.top
tile.top = {}
tile.top.name = "tiletop"
tile.top.skip_gap = tile.skip_gap
function tile.top.arrange(p)
    return do_tile(p, "top")
end
function tile.top.mouse_resize_handler(c, corner, x, y)
    return mouse_resize_handler(c, corner, x, y, "top")
end

tile.arrange = tile.right.arrange
tile.mouse_resize_handler = tile.right.mouse_resize_handler
tile.name = tile.right.name

return tile

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
