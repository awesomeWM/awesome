-- Align a client to the top of the parent area. --DOC_HEADER
-- @tparam drawable d A drawable (like `client`, `mouse` or `wibox`) --DOC_HEADER
-- @tparam[opt={}] table args Other arguments") --DOC_HEADER
-- @name top --DOC_HEADER
-- @class function --DOC_HEADER

screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
local c = client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE

awful.placement.top(client.focus)

assert(c.x == screen[1].geometry.width/2-40/2 and c.y==c.border_width--DOC_HIDE
    and c.width==40 and c.height==30)--DOC_HIDE
