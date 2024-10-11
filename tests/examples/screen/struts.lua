--DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_ASTERISK

screen[1]._resize {x = 0, width = 640, height = 480} --DOC_HIDE

local awful = { --DOC_HIDE
    wibar = require("awful.wibar"), --DOC_HIDE
    tag = require("awful.tag"), --DOC_HIDE
    tag_layout = require("awful.layout.suit.tile") --DOC_HIDE
} --DOC_HIDE

    -- Wibars and docked clients are the main users of the struts.
    local wibar = awful.wibar {
        position = "top",
        height   = 24, -- this will set the wibar own :struts() to top=24
    }

awful.tag.add("1", { --DOC_HIDE
    screen   = screen[1], --DOC_HIDE
    selected = true, --DOC_HIDE
    layout   = awful.tag_layout.right, --DOC_HIDE
    gap      = 4 --DOC_HIDE
}) --DOC_HIDE

local c = client.gen_fake{} --DOC_HIDE

    -- This is the client in the bottom left.
    c.name     = "w. struts"
    c.floating = true

--DOC_NEWLINE

    c:geometry {
        x      = 0,
        y      = 380,
        height = 100,
        width  = 100,
    }

--DOC_NEWLINE

    c:struts {
        left   = 100,
        bottom = 100
    }

local clients = { --DOC_HIDE
    ['w. struts'] = c, --DOC_HIDE
    ['client #1'] = client.gen_fake{}, --DOC_HIDE
    ['client #2'] = client.gen_fake{}, --DOC_HIDE
    ['client #3'] = client.gen_fake{}, --DOC_HIDE
} --DOC_HIDE

return { --DOC_HIDE
    factor                = 2, --DOC_HIDE
    show_boxes            = true, --DOC_HIDE
    draw_wibars           = {wibar}, --DOC_HIDE
    draw_clients          = clients, --DOC_HIDE
    display_screen_info   = false, --DOC_HIDE
    draw_struts           = true, --DOC_HIDE
} --DOC_HIDE
