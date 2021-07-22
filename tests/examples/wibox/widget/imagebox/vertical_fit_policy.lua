--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )
local lgi = require("lgi")
local cairo = lgi.cairo

-- A simple Awesome logo
local function demo()
    local img = cairo.ImageSurface.create(cairo.Format.ARGB32, 32, 22)
    local cr = cairo.Context(img)

    -- Awesome default #555555
    cr:set_source_rgb(0,0,1)
    cr:paint()

    cr:set_source_rgb(1,0,0)

    cr:rectangle(0, 10, 32, 2)
    cr:rectangle(15, 0, 2, 22)
    cr:fill()

    cr:set_source_rgb(0,1,0)

    cr:arc(16, 11, 8, 0, 2*math.pi)
    cr:fill()

    return img
end


local function cell_centered_widget(widget)
    return wibox.widget {
        widget,
        valign = 'center',
        halign = 'center',
        content_fill_vertical = false,
        content_fill_horizontal = false,
        widget = wibox.container.place
    }
end

local function build_ib(size, policy)
    return cell_centered_widget(wibox.widget {
        {
            vertical_fit_policy = policy,
            forced_height = size,
            forced_width = size,
            image  = demo(),
            widget = wibox.widget.imagebox
        },
        forced_width  = size + 2,
        forced_height = size + 2,
        color         = beautiful.border_color,
        margins       = 1,
        widget        = wibox.container.margin
    })
end


local l = wibox.widget {
    homogeneous   = false,
    spacing       = 5,
    layout        = wibox.layout.grid,
}
parent:add(l)

l:add_widget_at(wibox.widget.textbox('vertical_fit_policy = "auto"'), 1, 1)
l:add_widget_at(wibox.widget.textbox('versical_fit_policy = "none"'), 2, 1)
l:add_widget_at(wibox.widget.textbox('vertical_fit_policy = "fit"'), 3, 1)
l:add_widget_at(wibox.widget.textbox('imagebox size'), 4, 1)

for i,size in ipairs({16, 32, 64}) do
    l:add_widget_at(build_ib(size, "auto"), 1, i + 1)
    l:add_widget_at(build_ib(size, "none"), 2, i + 1)
    l:add_widget_at(build_ib(size, "fit" ), 3, i + 1)
    l:add_widget_at(cell_centered_widget(wibox.widget.textbox(size..'x'..size)), 4, i + 1)
end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
