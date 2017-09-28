---------------------------------------------------------------------------
--- Magnifier layout module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @module awful.layout
---------------------------------------------------------------------------

-- Grab environment we need
local ipairs = ipairs
local math = math
local capi =
{
    client = client,
    screen = screen,
    mouse = mouse,
    mousegrabber = mousegrabber
}

--- The magnifier layout layoutbox icon.
-- @beautiful beautiful.layout_magnifier
-- @param surface
-- @see gears.surface

local magnifier = {}

function magnifier.mouse_resize_handler(c, corner, x, y)
    capi.mouse.coords({ x = x, y = y })

    local wa = c.screen.workarea
    local center_x = wa.x + wa.width / 2
    local center_y = wa.y + wa.height / 2
    local maxdist_pow = (wa.width^2 + wa.height^2) / 4

    local prev_coords = {}
    capi.mousegrabber.run(function (_mouse)
                              if not c.valid then return false end

                              for _, v in ipairs(_mouse.buttons) do
                                  if v then
                                      prev_coords = { x =_mouse.x, y = _mouse.y }
                                      local dx = center_x - _mouse.x
                                      local dy = center_y - _mouse.y
                                      local dist = dx^2 + dy^2

                                      -- New master width factor
                                      local mwfact = dist / maxdist_pow
                                      c.screen.selected_tag.master_width_factor
                                        = math.min(math.max(0.01, mwfact), 0.99)
                                      return true
                                  end
                              end
                              return prev_coords.x == _mouse.x and prev_coords.y == _mouse.y
                          end, corner .. "_corner")
end

local function get_screen(s)
    return s and capi.screen[s]
end

function magnifier.arrange(p)
    -- Fullscreen?
    local area = p.workarea
    local cls = p.clients
    local focus = p.focus or capi.client.focus
    local t = p.tag or capi.screen[p.screen].selected_tag
    local mwfact = t.master_width_factor
    local fidx

    -- Check that the focused window is on the right screen
    if focus and focus.screen ~= get_screen(p.screen) then focus = nil end

    -- If no window is focused or focused window is not tiled, take the first tiled one.
    if not focus or focus.floating then
        focus = cls[1]
        fidx = 1
    end

    -- Abort if no clients are present
    if not focus then return end

    local geometry = {}
    if #cls > 1 then
        geometry.width = area.width * math.sqrt(mwfact)
        geometry.height = area.height * math.sqrt(mwfact)
        geometry.x = area.x + (area.width - geometry.width) / 2
        geometry.y = area.y + (area.height - geometry.height) /2
    else
        geometry.x = area.x
        geometry.y = area.y
        geometry.width = area.width
        geometry.height = area.height
    end

    local g = {
        x = geometry.x,
        y = geometry.y,
        width = geometry.width,
        height = geometry.height
    }
    p.geometries[focus] = g

    if #cls > 1 then
        geometry.x = area.x
        geometry.y = area.y
        geometry.height = area.height / (#cls - 1)
        geometry.width = area.width

        -- We don't know the focus window index. Try to find it.
        if not fidx then
            for k, c in ipairs(cls) do
                if c == focus then
                    fidx = k
                    break
                end
            end
        end

        -- First move clients that are before focused client.
        for k = fidx + 1, #cls do
            p.geometries[cls[k]] = {
                x = geometry.x,
                y = geometry.y,
                width = geometry.width,
                height = geometry.height
            }
            geometry.y = geometry.y + geometry.height
        end

        -- Then move clients that are after focused client.
        -- So the next focused window will be the one at the top of the screen.
        for k = 1, fidx - 1 do
            p.geometries[cls[k]] = {
                x = geometry.x,
                y = geometry.y,
                width = geometry.width,
                height = geometry.height
            }
            geometry.y = geometry.y + geometry.height
        end
    end
end

--- The magnifier layout.
-- @clientlayout awful.layout.suit.magnifier

magnifier.name = "magnifier"

-- This layout handles the currently focused client specially and needs to be
-- called again when the focus changes.
magnifier.need_focus_update = true

return magnifier

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
