--DOC_GEN_OUTPUT --DOC_GEN_IMAGE --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
require("awful.tag").add("1", {screen=screen[1], selected=true}) --DOC_HIDE

--DOC_HIDE no_offscreen is auto-called when startup is true, avoid this.
awesome.startup = false -- luacheck: globals awesome.startup --DOC_HIDE

-- using just no_offscreen with honor_workarea:
local c = client.gen_fake {x = 0, y = 0, width= screen[1].geometry.width, height=screen[1].geometry.height} --DOC_HIDE
c.fullscreen = true --DOC_HIDE
print("no_offscreen:") --DOC_HIDE
print("Before:", "x="..c.x..", y="..c.y..", width="..c.width..", height="..c.height) --DOC_HIDE
awful.placement.no_offscreen(c, {honor_workarea=true, })
print("After:", "x="..c.x..", y="..c.y..", width="..c.width..", height="..c.height) --DOC_HIDE
c:kill() --DOC_HIDE

-- using no_offscreen + skip_fullscreen:
c = client.gen_fake {x = 0, y = 0, width= screen[1].geometry.width, height=screen[1].geometry.height} --DOC_HIDE
c.fullscreen = true --DOC_HIDE
print("no_offscreen + skip_fullscreen:") --DOC_HIDE
print("Before:", "x="..c.x..", y="..c.y..", width="..c.width..", height="..c.height) --DOC_HIDE
local f = (awful.placement.no_offscreen + awful.placement.skip_fullscreen)
f(c, {honor_workarea=true, })
print("After:", "x="..c.x..", y="..c.y..", width="..c.width..", height="..c.height) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
