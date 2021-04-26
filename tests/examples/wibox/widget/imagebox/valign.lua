--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local beautiful = require( "beautiful" )  --DOC_HIDE

local l = wibox.layout { --DOC_HIDE
    forced_width  = 340, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.vertical --DOC_HIDE
} --DOC_HIDE

for _, resize in ipairs {true, false} do
    local row = wibox.layout { --DOC_HIDE
        spacing = 5, --DOC_HIDE
        layout = wibox.layout.fixed.horizontal --DOC_HIDE
    } --DOC_HIDE

    row:add(wibox.widget { --DOC_HIDE
        markup = "<b>resize = "..(resize and "true" or "false").."</b>", --DOC_HIDE
        forced_width = 80, --DOC_HIDE
        widget = wibox.widget.textbox --DOC_HIDE
    }) --DOC_HIDE

    for _, valign in ipairs {"top", "center", "bottom"} do
        local w = wibox.widget {
            {
                {
                    image         = beautiful.awesome_icon,
                    forced_height = 32,
                    forced_width  = 32,
                    valign        = valign,
                    resize        = resize,
                    widget        = wibox.widget.imagebox
                },
                bg     = beautiful.bg_normal,
                forced_height = 80, --DOC_HIDE
                forced_width = 32, --DOC_HIDE
                widget = wibox.container.background
            },
            widget = wibox.container.place
        }

        row:add(wibox.widget {--DOC_HIDE
            {--DOC_HIDE
                markup = "<b>`valign` = "..valign.."</b>",--DOC_HIDE
                widget = wibox.widget.textbox,--DOC_HIDE
            },--DOC_HIDE
            w,--DOC_HIDE
            layout = wibox.layout.fixed.vertical,--DOC_HIDE
        }) --DOC_HIDE
    end

    l:add(row) --DOC_HIDE
end

parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
