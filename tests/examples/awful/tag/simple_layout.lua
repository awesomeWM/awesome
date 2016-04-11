local dynamic     = require( "awful.layout.dynamic.base"        ) --DOC_HIDE
local wibox       = require( "wibox"                            ) --DOC_HIDE
local a_tag = require("awful.tag") --DOC_HIDE
local a_layout = require("awful.layout") --DOC_HIDE
local awful = { layout = { dynamic = dynamic } } --DOC_HIDE
--DOC_NO_USAGE --DOC_HIDE

screen._setup_grid(128, 96, {1}) --DOC_HIDE
local t_real = a_tag.add("Test", {screen=screen[1]}) --DOC_HIDE

for _=1, 3 do --DOC_HIDE
    client.gen_fake {x = 45, y = 35, width=40, height=30}:_hide() --DOC_HIDE
end --DOC_HIDE

    local t = mouse.screen.selected_tag --luacheck: no unused
    t = t_real --DOC_HIDE

    -- The first argument (_) is a tag, it is here unused.
    t.layout = awful.layout.dynamic("vertical_fair", function(_)
        return wibox.layout.flex.horizontal()
    end)

local params = a_layout.parameters(t) --DOC_HIDE
t.layout.arrange(params) --DOC_HIDE

return {hide_lines=true} --DOC_HIDE
