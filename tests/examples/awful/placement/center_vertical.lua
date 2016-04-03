-- Align a client to the vertical center of the parent area. --DOC_HEADER
-- @tparam drawable d A drawable (like `client`, `mouse` or `wibox`) --DOC_HEADER
-- @tparam[opt={}] table args Other arguments") --DOC_HEADER
-- @name center_vertical --DOC_HEADER
-- @class function --DOC_HEADER

screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
--[[local c = ]]client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE

awful.placement.center_vertical(client.focus)

-- print("\n\n\n\n", c.y, c.height, screen[1].geometry.height/2-30/2)--FIXME
-- assert(c.x == 45 and c.y==screen[1].geometry.height/2-30/2--DOC_HIDE
--     and c.width==40 and c,height==30)--DOC_HIDE
