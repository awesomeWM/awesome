local awful = require("awful")
local cairo = require("lgi").cairo
local wibox = require("wibox")

return { create_wibox = function()
    local img = cairo.ImageSurface(cairo.Format.ARGB32, 20, 20)

    -- Widgets that are aligned to the left
    local left_layout = wibox.layout.fixed.horizontal()
    left_layout:add(awful.widget.launcher({ image = img, command = "bash" }))
    left_layout:add(awful.widget.taglist {
        screen = 1,
        filter = awful.widget.taglist.filter.all
    })
    left_layout:add(awful.widget.prompt())

    -- Widgets that are aligned to the right
    local right_layout = wibox.layout.fixed.horizontal()
    local textclock = wibox.widget.textclock()
    right_layout:add(textclock)
    right_layout:add(awful.widget.layoutbox(1))

    -- Now bring it all together (with the tasklist in the middle)
    local layout = wibox.layout.align.horizontal()
    layout:set_left(left_layout)
    layout:set_middle(awful.widget.tasklist {
        screen = 1,
        filter = awful.widget.tasklist.filter.currenttags
    })
    layout:set_right(right_layout)

    -- Create wibox
    local wb = wibox({ width = 1024, height = 20, screen = 1 })
    --wb.visible = true
    wb:set_widget(layout)

    return wb, textclock, img, left_layout, right_layout, layout
end }

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
