--DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_HIDE_ALL

screen[1]._resize {x = 0, width = 640, height = 480}

local awful = {
    wibar = require("awful.wibar"),
    tag = require("awful.tag"),
    tag_layout = require("awful.layout.suit.tile")
}

screen[1].padding = {
    left   = 40,
    right  = 40,
    top    = 20,
    bottom = 20,
}

local wibar = awful.wibar {
    position = "top",
    height   = 24,
}

awful.tag.add("1", {
    screen   = screen[1],
    selected = true,
    layout   = awful.tag_layout.right,
    gap      = 50
})

local clients = {
    ['client #1'] = client.gen_fake{},
    ['client #2'] = client.gen_fake{},
    ['client #3'] = client.gen_fake{}
}

return {
    factor                = 2    ,
    show_boxes            = true,
    draw_wibar            = wibar,
    draw_clients          = clients,
    display_screen_info   = false,
    draw_gaps             = true,
}
