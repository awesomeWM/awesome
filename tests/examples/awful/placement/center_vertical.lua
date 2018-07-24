--DOC_GEN_IMAGE --DOC_HIDE
-- Align a client to the vertical center of the parent area. --DOC_HEADER
-- @tparam drawable d A drawable (like `client`, `mouse` or `wibox`) --DOC_HEADER
-- @tparam[opt={}] table args Other arguments") --DOC_HEADER
-- @name center_vertical --DOC_HEADER
-- @class function --DOC_HEADER

screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
--[[local c = ]]client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE

awful.placement.center_vertical(client.focus)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
