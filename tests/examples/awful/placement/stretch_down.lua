-- Stretch the drawable to the bottom of the parent area. --DOC_HEADER
-- @tparam drawable d A drawable (like `client` or `wibox`) --DOC_HEADER
-- @tparam[opt={}] table args Other arguments --DOC_HEADER
-- @treturn table The new geometry --DOC_HEADER
-- @name stretch_down --DOC_HEADER
-- @class function --DOC_HEADER

screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local placement = require("awful.placement") --DOC_HIDE

local c = client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE
placement.stretch_down(client.focus)

assert(c.x==45) --DOC_HIDE
assert(c.y==35) --DOC_HIDE
assert(c.width == 40) --DOC_HIDE
assert(c.y+c.height+2*c.border_width == --DOC_HIDE
       screen[1].geometry.y + screen[1].geometry.height) --DOC_HIDE
