-- Stretch the drawable to the top of the parent area. --DOC_HEADER
-- @tparam drawable d A drawable (like `client` or `wibox`) --DOC_HEADER
-- @tparam[opt={}] table args Other arguments --DOC_HEADER
-- @name stretch_up --DOC_HEADER
-- @class function --DOC_HEADER

screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local placement = require("awful.placement") --DOC_HIDE

local c = client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE
placement.stretch_up(client.focus)

assert(c.y==c.border_width) --DOC_HIDE
assert(c.x==45) --DOC_HIDE
assert(c.width == 40) --DOC_HIDE
print(c.height)
assert(c.height == 35+30) --DOC_HIDE
