--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local parent = ...
local wibox  = require("wibox")
local beautiful = require( "beautiful" )

-- luacheck: push no max line length

local image_path = '<?xml version="1.0"?>'..
    '<svg xmlns:xlink="http://www.w3.org/1999/xlink" width="30" height="30" viewBox="0 0 7.937 7.937">'..
    '  <defs>'..
    '    <linearGradient id="a">'..
    '      <stop style="stop-opacity:1;stop-color:magenta;" offset="0" id="first"/>'..
    '      <stop offset=".5" style="stop-opacity:1;stop-color:cyan;" id="second"/>'..
    '      <stop style="stop-opacity:1;stop-color:yellow;" offset="1" id="third"/>'..
    '    </linearGradient>' ..
    '    <linearGradient xlink:href="#a" id="b" gradientUnits="userSpaceOnUse" x1="28.726" y1="64.923" x2="182.185" y2="201.75" gradientTransform="matrix(.04726 0 0 .053 83.075 141.528)"/>'..
    '  </defs>'..
    '  <path d="M84.732 144.627c-.372 0-.642.329-.642.679v6.58c0 .35.27.678.642.678h6.653c.372 0 .642-.328.642-.679v-6.579c0-.35-.27-.68-.642-.68zm.043.685h6.567v6.568h-6.567z" style="fill:url(#b);" transform="translate(-84.09 -144.627)"/>'..
    '</svg>'

--luacheck: pop

local function generic_widget(text)
    return wibox.widget {
        {
            {
                {
                    id     = "text",
                    align  = "center",
                    valign = "center",
                    text   = text,
                    widget = wibox.widget.textbox
                },
                margins = 10,
                widget  = wibox.container.margin,
            },
            border_width = 3,
            border_color = "transparent",
            bg           = beautiful.bg_normal,
            widget       = wibox.container.background
        },
        opacity = 0.5,
        widget  = wibox.container.margin,
    }
end

--DOC_HIDE_END
    local w = wibox.widget {
        -- This is the background border.
        {
            paddings      = {
                left   = 5,
                top    = 3,
                right  = 10,
                bottom = 10,
            },
            borders       = 20,
            border_image  = image_path,
            honor_borders = false,
            widget        = wibox.container.border
        },
        -- This border container is used top place widgets.
        {
            {
                text          = "Center widget",
                valign        = "center",
                align         = "center",
                widget        = wibox.widget.textbox
            },
            border_widgets = {
                top_left     = generic_widget(""),
                top          = generic_widget("top"),
                top_right    = generic_widget(""),
                right        = generic_widget("right"),
                bottom_right = generic_widget(""),
                bottom       = generic_widget("bottom"),
                bottom_left  = generic_widget(""),
                left         = generic_widget("left"),
            },
            widget = wibox.container.border
        },
        forced_width  = 200, --DOC_HIDE
        forced_height = 200, --DOC_HIDE
        layout = wibox.layout.stack
    }

--DOC_HIDE_START
parent:add(w)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
