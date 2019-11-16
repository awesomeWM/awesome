--DOC_GEN_IMAGE --DOC_NO_USAGE --DOC_HIDE_ALL

screen[1]._resize {x = 0, width = 640, height = 480}

local awful = {wibar = require("awful.wibar")}

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

return {
    factor                = 2    ,
    show_boxes            = false,
    highlight_tiling_area = true ,
    draw_wibar            = wibar,
}
