--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )

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

local function build_ib(size, resize)
    return cell_centered_widget(wibox.widget {
        {
            resize = resize,
            forced_height = size,
            forced_width = size,
            image  = beautiful.awesome_icon,
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

l:add_widget_at(cell_centered_widget(wibox.widget.textbox('resize = true')), 1, 1)
l:add_widget_at(cell_centered_widget(wibox.widget.textbox('resize = false')), 2, 1)
l:add_widget_at(cell_centered_widget(wibox.widget.textbox('imagebox size')), 3, 1)

for i,size in ipairs({16, 32, 64}) do
    l:add_widget_at(build_ib(size, true), 1, i + 1)
    l:add_widget_at(build_ib(size, false), 2, i + 1)
    l:add_widget_at(cell_centered_widget(wibox.widget.textbox(size..'x'..size)), 3, i + 1)
end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
