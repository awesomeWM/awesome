--DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_HIDE_ALL

screen[1]._resize {x = 0, width = 640, height = 480}


local awful = {
    wibar = require("awful.wibar"),
    tag = require("awful.tag"),
    tag_layout = require("awful.layout.suit.tile")
}

function awful.spawn(_, args)
    local c = client.gen_fake{}
    c:tags({args.tag})
    assert(#c:tags() == 1)
    assert(c:tags()[1] == args.tag)
end

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
    screen              = screen[1],
    selected            = true,
    layout              = awful.tag_layout.right,
    gap                 = 5,
    master_width_factor = 0.6,
    column_count        = 2
})

local clients = {
    ['master #1 (60%)'] = client.gen_fake{},
    ['#1 (20%)'] = client.gen_fake{},
    ['#2 (20%)'] = client.gen_fake{},
    ['#3 (20%)'] = client.gen_fake{},
    ['#4 (20%)'] = client.gen_fake{}
}
for _,c in ipairs(clients) do
    c:tags{"1"}
end

return {
    factor                = 2    ,
    show_boxes            = true,
    draw_wibar            = wibar,
    draw_clients          = clients,
    display_screen_info   = false,
    draw_mwfact           = true,
}
