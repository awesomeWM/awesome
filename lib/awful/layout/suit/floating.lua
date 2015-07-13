---------------------------------------------------------------------------
--- Dummy function for floating layout
--
-- @author Gregor Best
-- @copyright 2008 Gregor Best
-- @release @AWESOME_VERSION@
-- @module awful.layout.suit.floating
---------------------------------------------------------------------------

-- Grab environment we need
local math = math
local ipairs = ipairs
local capi =
{
    mouse = mouse,
    mousegrabber = mousegrabber
}

local floating = {}

function floating.mouse_resize_handler(c, corner, x, y)
    local g = c:geometry()

    -- Do not allow maximized clients to be resized by mouse
    local fixed_x = c.maximized_horizontal
    local fixed_y = c.maximized_vertical

    -- Warp mouse pointer
    capi.mouse.coords({ x = x, y = y })

    local prev_coords = {}
    capi.mousegrabber.run(function (_mouse)
                              for k, v in ipairs(_mouse.buttons) do
                                  if v then
                                      local ng
                                      prev_coords = { x =_mouse.x, y = _mouse.y }
                                      if corner == "bottom_right" then
                                          ng = { width = _mouse.x - g.x,
                                                 height = _mouse.y - g.y }
                                      elseif corner == "bottom_left" then
                                          ng = { x = _mouse.x,
                                                 width = (g.x + g.width) - _mouse.x,
                                                 height = _mouse.y - g.y }
                                      elseif corner == "top_left" then
                                          ng = { x = _mouse.x,
                                                 width = (g.x + g.width) - _mouse.x,
                                                 y = _mouse.y,
                                                 height = (g.y + g.height) - _mouse.y }
                                      else
                                          ng = { width = _mouse.x - g.x,
                                                 y = _mouse.y,
                                                 height = (g.y + g.height) - _mouse.y }
                                      end
                                      if ng.width <= 0 then ng.width = nil end
                                      if ng.height <= 0 then ng.height = nil end
                                      if fixed_x then ng.width = g.width ng.x = g.x end
                                      if fixed_y then ng.height = g.height ng.y = g.y end
                                      c:geometry(ng)
                                      -- Get real geometry that has been applied
                                      -- in case we honor size hints
                                      -- XXX: This should be rewritten when size
                                      -- hints are available from Lua.
                                      local rg = c:geometry()

                                      if corner == "bottom_right" then
                                          ng = {}
                                      elseif corner == "bottom_left" then
                                          ng = { x = (g.x + g.width) - rg.width  }
                                      elseif corner == "top_left" then
                                          ng = { x = (g.x + g.width) - rg.width,
                                                 y = (g.y + g.height) - rg.height }
                                      else
                                          ng = { y = (g.y + g.height) - rg.height }
                                      end
                                      c:geometry({ x = ng.x, y = ng.y })
                                      return true
                                  end
                              end
                              return prev_coords.x == _mouse.x and prev_coords.y == _mouse.y
                          end, corner .. "_corner")
end

function floating.arrange()
end

floating.name = "floating"

return floating
