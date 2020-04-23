--DOC_GEN_OUTPUT --DOC_GEN_IMAGE --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE

--DOC_HIDE no_offscreen is auto-called when startup is true, avoid this.
awesome.startup = false -- luacheck: globals awesome.startup --DOC_HIDE

local c = client.gen_fake {x = 0, y = 0, width= screen[1].geometry.width, height=screen[1].geometry.height} --DOC_HIDE
c.fullscreen = true --DOC_HIDE
print("Before:", "x="..c.x..", y="..c.y..", width="..c.width..", height="..c.height) --DOC_HIDE
awful.placement.no_offscreen(c, {honor_workarea=true, })
print("After:", "x="..c.x..", y="..c.y..", width="..c.width..", height="..c.height) --DOC_HIDE
c:kill() --DOC_HIDE

c = client.gen_fake {x = 0, y = 0, width= screen[1].geometry.width, height=screen[1].geometry.height} --DOC_HIDE
c.fullscreen = true --DOC_HIDE
print("Before:", "x="..c.x..", y="..c.y..", width="..c.width..", height="..c.height) --DOC_HIDE
local f = (awful.placement.no_offscreen + awful.placement.skip_fullscreen)
f(c, {honor_workarea=true, })
print("After:", "x="..c.x..", y="..c.y..", width="..c.width..", height="..c.height) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
