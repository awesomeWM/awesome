--DOC_GEN_IMAGE --DOC_HIDE_START
local parent = ...
local wibox  = require("wibox")

--luacheck: push no max line length

--DOC_HIDE_END

local image_path = '<?xml version="1.0"?>'..
    '<svg xmlns:xlink="http://www.w3.org/1999/xlink" width="30" height="30" viewBox="0 0 7.937 7.937">'..
    '  <defs>'..
    '    <linearGradient id="a">'..
    '      <stop style="stop-opacity:1" offset="0" id="first"/>'..
    '      <stop offset=".5" style="stop-opacity:1" id="second"/>'..
    '      <stop style="stop-opacity:1" offset="1" id="third"/>'..
    '    </linearGradient>' ..
    '    <linearGradient xlink:href="#a" id="b" gradientUnits="userSpaceOnUse" x1="28.726" y1="64.923" x2="182.185" y2="201.75" gradientTransform="matrix(.04726 0 0 .053 83.075 141.528)"/>'..
    '  </defs>'..
    '  <path d="M84.732 144.627c-.372 0-.642.329-.642.679v6.58c0 .35.27.678.642.678h6.653c.372 0 .642-.328.642-.679v-6.579c0-.35-.27-.68-.642-.68zm.043.685h6.567v6.568h-6.567z" style="fill:url(#b);" transform="translate(-84.09 -144.627)"/>'..
    '</svg>'

--DOC_NEWLINE

local style = ""..
    "#first  {stop-color: magenta;}" ..
    "#second {stop-color: cyan;}" ..
    "#third  {stop-color: yellow;}"

--DOC_NEWLINE

local w = wibox.widget {
    {
        text   = "Center widget",
        valign = "center",
        align  = "center",
        widget = wibox.widget.textbox
    },
    borders                 = 50,
    border_image_stylesheet = style,
    border_image            = image_path,
    honor_borders           = false,
    forced_width            = 100, --DOC_HIDE
    forced_height           = 100, --DOC_HIDE
    widget                  = wibox.container.border
}

--DOC_HIDE_START

--luacheck: pop

parent:add(w)
