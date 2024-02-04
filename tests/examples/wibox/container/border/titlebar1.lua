--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local parent = ...
local wibox  = require("wibox")
local gears = { shape = require("gears.shape")}
local beautiful = require( "beautiful" )

-- luacheck: push no max line length
local bg = [[
<?xml version="1.0"?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="105.827" height="23.532" viewBox="0 0 28 6.226">
  <defs>
    <linearGradient gradientTransform="matrix(.22077 0 0 .2208 -204.378 31.642)" xlink:href="#a" id="d" x1="-264.083" y1="45.26" x2="-264.083" y2="17.29" gradientUnits="userSpaceOnUse"/>
    <linearGradient id="a">
      <stop style="stop-color:#777;stop-opacity:1" offset="0"/>
      <stop style="stop-color:#dadada;stop-opacity:1" offset="1"/>
    </linearGradient>
    <linearGradient gradientTransform="matrix(.22077 0 0 .2208 -204.378 31.642)" xlink:href="#b" id="c" x1="-254.596" y1="18.068" x2="-254.596" y2="44.26" gradientUnits="userSpaceOnUse" spreadMethod="pad"/>
    <linearGradient id="b">
      <stop style="stop-color:#ececec;stop-opacity:1" offset="0"/>
      <stop offset=".963" style="stop-color:#cbcbcb;stop-opacity:1"/>
      <stop style="stop-color:#8d8d8d;stop-opacity:1" offset="1"/>
    </linearGradient>
  </defs>
  <path d="M-250.12 35.462h23.877c1.111 0 2.006.894 2.006 2.006v3.999h-27.89v-3.999c0-1.112.895-2.006 2.006-2.006z" style="opacity:1;fill:url(#c);fill-opacity:1;stroke:none;stroke-width:.06623417;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1" transform="translate(252.127 -35.351)"/>
  <path style="opacity:1;fill:none;fill-opacity:1;stroke:url(#d);stroke-width:.22078058;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1" d="M-249.988 35.462h23.745c1.111 0 2.006.894 2.006 2.006v3.999h-27.757v-3.999c0-1.112.895-2.006 2.006-2.006z" transform="translate(252.127 -35.351)"/>
</svg>
]]

local btn = [[
<?xml version="1.0"?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="45.773" height="45.773" viewBox="0 0 12.111 12.111">
  <defs>
    <linearGradient id="a">
      <stop style="stop-opacity:1" offset="0" id="bg2"/>
      <stop style="stop-opacity:.88627452" offset="1" id="bg1"/>
    </linearGradient>
    <linearGradient xlink:href="#b" id="g" gradientUnits="userSpaceOnUse" x1="-453.433" y1="105.448" x2="-453.433" y2="95.432"/>
    <linearGradient id="b">
      <stop style="stop-color:#d8eaff;stop-opacity:1" offset="0"/>
      <stop style="stop-color:#d8eaff;stop-opacity:0" offset="1"/>
    </linearGradient>
    <linearGradient id="c">
      <stop offset="0" style="stop-color:#ececec;stop-opacity:1"/>
      <stop offset="1" style="stop-color:#8d8d8d;stop-opacity:1"/>
    </linearGradient>
    <linearGradient xlink:href="#d" id="i" x1="-453.111" y1="95.539" x2="-453.111" y2="105.508" gradientUnits="userSpaceOnUse"/>
    <linearGradient id="d">
      <stop style="stop-color:#000;stop-opacity:1" offset="0"/>
      <stop style="stop-color:#000;stop-opacity:0" offset="1"/>
    </linearGradient>
    <linearGradient id="e">
      <stop style="stop-color:#101010;stop-opacity:.02531646" offset="0"/>
      <stop offset=".788" style="stop-color:#000;stop-opacity:0"/>
      <stop style="stop-color:#272727;stop-opacity:.68776369" offset=".875"/>
      <stop style="stop-color:#ededed;stop-opacity:.40784314" offset="1"/>
    </linearGradient>
    <radialGradient xlink:href="#a" id="f" gradientUnits="userSpaceOnUse" cx="-453.052" cy="104.365" fx="-453.052" fy="104.365" r="5.292"/>
    <radialGradient xlink:href="#e" id="j" gradientUnits="userSpaceOnUse" gradientTransform="matrix(1.14291 -.0086 .00853 1.13429 51.276 -28.422)" cx="-442.185" cy="110.611" fx="-442.185" fy="110.611" r="5.292"/>
    <clipPath clipPathUnits="userSpaceOnUse" id="h">
      <circle r="5.292" cy="100.83" cx="-453.193" style="opacity:1;fill:url(#radialGradient1250);fill-opacity:1;stroke:none;stroke-width:1;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"/>
    </clipPath>
  </defs>
  <g transform="translate(459.213 -94.787)">
    <circle r="5.292" cy="100.83" cx="-453.193" style="opacity:1;fill:url(#f);fill-opacity:1;stroke:none;stroke-width:1;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"/>
    <path d="M-448.374 98.657c.13 3.044-9.622 2.906-9.661.094-.017-.466 1.585-3.179 4.842-3.212 2.922 0 4.668 2.334 4.819 3.118z" style="opacity:1;fill:url(#g);fill-opacity:1;stroke:url(#linearGradient1378);stroke-width:1;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:0"/>
    <circle clip-path="url(#h)" style="opacity:1;fill:none;fill-opacity:1;stroke:url(#i);stroke-width:1;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1;filter:url(#filter1216)" cx="-453.193" cy="100.83" r="5.292"/>
    <circle style="opacity:1;fill:none;fill-opacity:1;stroke:url(#j);stroke-width:1.92490542;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1" cx="-453.158" cy="100.842" r="5.093"/>
  </g>
</svg>
]]

--luacheck: pop

local function generic_widget(text, margins)
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
            border_color = beautiful.border_color,
            bg = beautiful.bg_normal,
            widget = wibox.container.background
        },
        margins = margins or 5,
        widget  = wibox.container.margin,
    }
end

--DOC_HIDE_END

    local w = wibox.widget {
        {
            {
                text    = "Content",
                align   = "center",
                valign  = "center",
                widget  = wibox.widget.textbox
            },
            border_widgets = {
                top = {
                    {
                        {
                            {
                                stylesheet = "#bg1 {stop-color:#ca2b2b;} #bg2 {stop-color:#f8b9b9;}",
                                image      = btn,
                                widget     = wibox.widget.imagebox
                            },
                            {
                                stylesheet = "#bg1 {stop-color:#ec9527;} #bg2 {stop-color:#ffff9c;}",
                                image      = btn,
                                widget     = wibox.widget.imagebox
                            },
                            {
                                stylesheet = "#bg1 {stop-color:#75b525;} #bg2 {stop-color:#e0fda9;}",
                                image      = btn,
                                widget     = wibox.widget.imagebox
                            },
                            spacing = 3,
                            layout  = wibox.layout.fixed.horizontal
                        },
                        {
                            align  = "center",
                            text   = "Shameless macOS ripoff",
                            widget = wibox.widget.textbox
                        },
                        layout = wibox.layout.align.horizontal
                    },
                    paddings      = 6,
                    borders       = 14,
                    border_image  = bg,
                    honor_borders = false,
                    fill          = true,
                    forced_height = 28, --DOC_HIDE
                    widget        = wibox.container.border
                },
                right        = generic_widget(""),
                bottom_right = generic_widget(""),
                bottom       = generic_widget(""),
                bottom_left  = generic_widget(""),
                left         = generic_widget(""),
            },
            borders = {
                top    = 28,
                left   = 22,
                right  = 22,
                bottom = 22,
            },
            border_merging = {
                top = true
            },
            forced_width  = 300, --DOC_HIDE
            forced_height = 100, --DOC_HIDE
            widget = wibox.container.border
        },
        bg     = "#d9d9d9",
        shape  = gears.shape.rounded_rect,
        widget = wibox.container.background
    }

--DOC_HIDE_START
parent:add(w)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
