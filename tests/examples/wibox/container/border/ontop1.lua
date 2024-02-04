--DOC_GEN_IMAGE --DOC_HIDE_START
local parent = ...
local wibox  = require("wibox")
local color = require("gears.color")
local beautiful = require( "beautiful" )
local cairo = require("lgi").cairo

-- luacheck: push no max string line length
local image_path = [[
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="67.006" height="65.26">
   <defs>
      <filter height="1.408" y="-.204" width="1.408" x="-.204" id="a" style="color-interpolation-filters:sRGB">
         <feGaussianBlur stdDeviation="2.079" />
      </filter>
      <filter height="1.158" y="-.079" width="1.158" x="-.079" id="b" style="color-interpolation-filters:sRGB">
         <feGaussianBlur stdDeviation=".919" />
      </filter>
      <clipPath id="c" clipPathUnits="userSpaceOnUse">
         <rect style="opacity:1;fill:#FF0000;fill-opacity:1;" width="12.7" height="12.7" x="56.318" y="122.526" rx="2.266" ry="2.266" />
      </clipPath>
   </defs>
   <g transform="matrix(3.77953 0 0 3.77953 -205.339 -465.345)">
      <path style="opacity:1;fill:#000;fill-opacity:1;filter:url(#a)" d="M341.607 504.254v40.498H326.25v-.09h-28.72a8.542 8.542 0 0 0 8.154 5.942h30.873a8.545 8.545 0 0 0 8.562-8.565v-30.873a8.527 8.527 0 0 0-3.512-6.912z" transform="matrix(.26458 0 0 .26458 -21.823 -7.793)" />
      <rect ry="2.266" rx="2.266" y="122.526" x="56.318" height="12.7" width="12.7" style="opacity:1;fill:none;fill-opacity:1;filter:url(#b)" clip-path="url(#c)" transform="translate(-.267 1.837)" />
      <path style="color:#000;dominant-baseline:auto;baseline-shift:baseline;white-space:normal;shape-padding:0;clip-rule:nonzero;display:inline;overflow:visible;visibility:visible;opacity:1;isolation:auto;mix-blend-mode:normal;color-interpolation:sRGB;color-interpolation-filters:linearRGB;solid-color:#000;solid-opacity:1;vector-effect:none;fill:#FF7700;fill-opacity:1;fill-rule:nonzero;color-rendering:auto;image-rendering:auto;shape-rendering:auto;enable-background:accumulate" d="M58.384 123.88a2.617 2.617 0 0 0-2.616 2.615v8.17a2.617 2.617 0 0 0 2.616 2.615h8.168a2.619 2.619 0 0 0 2.617-2.615v-8.17a2.619 2.619 0 0 0-2.617-2.615zm0 .7h8.168c1.067 0 1.916.847 1.916 1.915v8.17a1.904 1.904 0 0 1-1.917 1.916h-8.167a1.904 1.904 0 0 1-1.916-1.916v-8.17c0-1.068.848-1.914 1.916-1.914z" />
      <path style="color:#000;dominant-baseline:auto;baseline-shift:baseline;white-space:normal;shape-padding:0;clip-rule:nonzero;display:inline;overflow:visible;visibility:visible;opacity:1;isolation:auto;mix-blend-mode:normal;color-interpolation:sRGB;color-interpolation-filters:linearRGB;solid-color:#000;solid-opacity:1;vector-effect:none;fill:#FF0000;fill-opacity:1;fill-rule:nonzero;color-rendering:auto;image-rendering:auto;shape-rendering:auto;enable-background:accumulate" d="M58.383 123.73a2.766 2.766 0 0 0-2.764 2.764v8.17a2.768 2.768 0 0 0 2.764 2.766h8.168a2.771 2.771 0 0 0 2.767-2.766v-8.17a2.77 2.77 0 0 0-2.767-2.764zm0 .3h8.168a2.468 2.468 0 0 1 2.469 2.464v8.17a2.47 2.47 0 0 1-2.47 2.467h-8.167a2.466 2.466 0 0 1-2.465-2.467v-8.17a2.464 2.464 0 0 1 2.465-2.465zm0 .402a2.054 2.054 0 0 0-2.065 2.062v8.17c0 1.147.918 2.066 2.065 2.066h8.168a2.06 2.06 0 0 0 2.066-2.066v-8.17c0-1.147-.92-2.062-2.066-2.062zm0 .298h8.168a1.75 1.75 0 0 1 1.767 1.764v8.17c0 .988-.78 1.768-1.767 1.768h-8.168c-.988 0-1.766-.78-1.766-1.768v-8.17c0-.988.778-1.764 1.766-1.764z" />
   </g>
</svg>
]]
--luacheck: pop

local function sur_to_pat(img2)
    local pat = cairo.Pattern.create_for_surface(img2)
    pat:set_extend(cairo.Extend.REPEAT)
    return pat
end

-- Imported for elv13/blind/pattern.lua
local function stripe_pat(col, angle, line_width, spacing)
    col        = color(col)
    angle      = angle or math.pi/4
    line_width = line_width or 2
    spacing    = spacing or 2

    local hy = line_width + 2*spacing

    -- Get the necessary width and height so the line repeat itself correctly
    local a, o = math.cos(angle)*hy, math.sin(angle)*hy

    local w, h = math.ceil(a + (line_width - 1)), math.ceil(o + (line_width - 1))

    -- Create the pattern
    local img2 = cairo.SvgSurface.create(nil, w, h)
    local cr2  = cairo.Context(img2)
    cr2:set_antialias(cairo.ANTIALIAS_NONE)

    -- Avoid artefacts caused by anti-aliasing
    local offset = line_width

    -- Setup
    cr2:set_source(color(col))
    cr2:set_line_width(line_width)

    -- The central line
    cr2:move_to(-offset, -offset)
    cr2:line_to(w+offset, h+offset)
    cr2:stroke()

    -- Top right
    cr2:move_to(-offset + w - spacing/2+line_width, -offset)
    cr2:line_to(2*w+offset - spacing/2+line_width, h+offset)
    cr2:stroke()

    -- Bottom left
    cr2:move_to(-offset + spacing/2-line_width, -offset + h)
    cr2:line_to(w+offset + spacing/2-line_width, 2*h+offset)
    cr2:stroke()

    return sur_to_pat(img2)
end

local stripe_pattern = stripe_pat(beautiful.bg_normal)

local l = wibox.layout {
    forced_width  = 240,
    spacing       = 5,
    layout        = wibox.layout.fixed.vertical
}

for _, ontop in ipairs {true, false} do
    --DOC_HIDE_END
    local w = wibox.widget {
        {
            {
                markup        = "<b>Central widget</b>",
                valign        = "center",
                align         = "center",
                forced_height = 30,
                forced_width  = 30,
                widget        = wibox.widget.textbox
            },
            bg     = stripe_pattern,
            widget = wibox.container.background
        },
        paddings      = {
            left   = 5,
            top    = 3,
            right  = 10,
            bottom = 10,
        },
        borders       = 20,
        border_image  = image_path,
        ontop         = ontop,
        honor_borders = false,
        widget        = wibox.container.border
    }

    --DOC_HIDE_START
    l:add(wibox.widget {
        {
            markup = "<b>ontop = "..(ontop and "true" or "false").."</b>",
            widget = wibox.widget.textbox,
        },
        w,
        layout = wibox.layout.fixed.vertical,
    })
end

parent:add(l)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
