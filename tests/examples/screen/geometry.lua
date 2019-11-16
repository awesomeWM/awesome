--DOC_GEN_IMAGE --DOC_ASTERISK --DOC_NO_USAGE --DOC_HIDE_ALL

screen[1]._resize {x = 0, width = 640, height = 480} --DOC_HIDE

local awful = {wibar = require("awful.wibar")} --DOC_HIDE

function awful.spawn(_, args) --DOC_HIDE
    local c = client.gen_fake{} --DOC_HIDE
    c:tags({args.tag}) --DOC_HIDE
    assert(#c:tags() == 1) --DOC_HIDE
    assert(c:tags()[1] == args.tag) --DOC_HIDE
end --DOC_HIDE

    -- With a padding, the tiled clients wont use 20px at the top and bottom
    -- and 40px on the left and right.
    screen[1].padding = {
        left   = 40,
        right  = 40,
        top    = 20,
        bottom = 20,
    }

    --DOC_NEWLINE

    -- This will shift the workarea by 24px at the top.
    local wibar = awful.wibar {
        position = "top",
        height   = 24,
    }

return { --DOC_HIDE
    factor             = 2    , --DOC_HIDE
    show_boxes         = false, --DOC_HIDE
    highlight_geometry = true , --DOC_HIDE
    draw_wibar         = wibar, --DOC_HIDE
} --DOC_HIDE
