local a_tag    = require("awful.tag") --DOC_HIDE --DOC_NO_USAGE
local a_layout = require("awful.layout") --DOC_HIDE
local wibox = {container={mirror = require("wibox.container.mirror")}} --DOC_HIDE

screen._setup_grid(128, 96, {4}) --DOC_HIDE
local tags = {} --DOC_HIDE

local function add_clients(s) --DOC_HIDE
    local x, y = s.geometry.x, s.geometry.y --DOC_HIDE
    for _=1, 5 do --DOC_HIDE
        client.gen_fake{x = x+45, y = y+35, width=40, height=30, screen=s}:_hide() --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE

for j=1, 4 do --DOC_HIDE
    tags[screen[j]] = a_tag.add("Test"..j, {screen=screen[j]}) --DOC_HIDE
    add_clients(screen[j],j) --DOC_HIDE
end --DOC_HIDE


    local tile = require("awful.layout.dynamic.base_layout")
    local manual = require("awful.layout.dynamic.suit.manual")

    local function gen_with_mirror(hor, ver)
        return manual {
            {
                {
                    reflow       = true,
                    max_elements = 2,
                    priority     = 3,
                    ratio        = 0.20,
                    layout       = tile.vertical
                },
                {
                    {
                        reflow       = true,
                        max_elements = 1,
                        priority     = 2,
                        ratio        = 0.20,
                        layout       = tile.vertical
                    },
                    {
                        reflow   = true,
                        priority = 1,
                        ratio    = 0.20,
                        layout   = tile.horizontal
                    },
                    reflow = true,
                    layout = tile.vertical
                },
                reflow = true,
                layout = tile.horizontal
            },
            reflection = {
                horizontal = hor,
                vertical   = ver,
            },
            layout = wibox.container.mirror
        }
    end

    screen[1].selected_tag.layout = gen_with_mirror(false, false)
    screen[2].selected_tag.layout = gen_with_mirror(false, true )
    screen[3].selected_tag.layout = gen_with_mirror(true , false)
    screen[4].selected_tag.layout = gen_with_mirror(true , true )

for j=1, 4 do --DOC_HIDE
    local t = tags[screen[j]] --DOC_HIDE
    local params = a_layout.parameters(t, screen[j]) --DOC_HIDE
    t.layout.arrange(params) --DOC_HIDE
end
