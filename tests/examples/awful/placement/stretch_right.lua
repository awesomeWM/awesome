--DOC_GEN_IMAGE --DOC_HIDE
-- Stretch the drawable to the right of the parent area. --DOC_HEADER
-- @tparam drawable d A drawable (like `client` or `wibox`) --DOC_HEADER
-- @tparam[opt={}] table args Other arguments --DOC_HEADER
-- @treturn table The new geometry --DOC_HEADER
-- @name stretch_right --DOC_HEADER
-- @class function --DOC_HEADER

screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local placement = require("awful.placement") --DOC_HIDE

local c = client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE
placement.stretch_right(client.focus)

local right = screen[1].geometry.x + screen[1].geometry.width --DOC_HIDE
assert(c.height == 30 and c.x == 45 and c.x+c.width+2*c.border_width == right) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
