--DOC_GEN_IMAGE --DOC_HIDE_START
local parent = ...
local wibox  = require("wibox")
local cairo  = require("lgi").cairo

-- luacheck: push no max string line length

local svg_image_path = [[
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="48" height="48" viewBox="0 0 12.7 12.7">
   <defs>
      <linearGradient id="a">
         <stop style="stop-color:#ff2121;stop-opacity:1" offset="0" />
         <stop style="stop-color:#2c21ff;stop-opacity:1" offset="1" />
      </linearGradient>
      <linearGradient xlink:href="#a" id="b" x1="37.798" y1="89.869" x2="148.167" y2="200.238" gradientUnits="userSpaceOnUse" gradientTransform="translate(31.412 180.42) scale(.11008)" />
      <linearGradient xlink:href="#a" id="c" gradientUnits="userSpaceOnUse" gradientTransform="translate(31.412 180.42) scale(.11008)" x1="148.167" y1="200.238" x2="37.798" y2="89.869" />
   </defs>
   <g transform="translate(-35.298 -190.038)">
      <rect style="fill:url(#b);stroke-width:.55040765;stroke-miterlimit:4;;" width="12.15" height="12.15" x="35.573" y="190.313" rx="2.371" ry="2.371" />
      <path style="color:#000;dominant-baseline:auto;baseline-shift:baseline;white-space:normal;shape-padding:0;clip-rule:nonzero;display:inline;overflow:visible;visibility:visible;isolation:auto;mix-blend-mode:normal;color-interpolation:sRGB;color-interpolation-filters:linearRGB;solid-color:#000;solid-vector-effect:none;fill:url(#c);fill-fill-rule:nonzero;stroke:none;stroke-width:.55040765;stroke-linecap:butt;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dashoffset:0;stroke-color-rendering:auto;image-rendering:auto;shape-rendering:auto;enable-background:accumulate" d="M37.943 190.037a2.647 2.647 0 0 0-2.646 2.647v7.408a2.647 2.647 0 0 0 2.646 2.646h7.409a2.647 2.647 0 0 0 2.646-2.646v-7.408a2.647 2.647 0 0 0-2.646-2.647zm0 .55h7.409c1.165 0 2.095.931 2.095 2.097v7.408c0 1.165-.93 2.095-2.095 2.095h-7.409a2.085 2.085 0 0 1-2.095-2.095v-7.408c0-1.166.93-2.096 2.095-2.096z" />
   </g>
</svg>
]]

--luacheck: pop

-- There is no path there, but that's just for the doc.
local handle = wibox.widget.imagebox._load_rsvg_handle(svg_image_path)
local png_image_path = cairo.ImageSurface(cairo.Format.ARGB32, 48, 48)
local cr = cairo.Context(png_image_path)
handle:render_cairo(cr)

local l = wibox.layout {
    forced_width    = 440,
    spacing         = 5,
    forced_num_cols = 2,
    homogeneous     = false,
    expand          = false,
    layout          = wibox.layout.grid.vertical
}

l:add_widget_at(wibox.widget {
    markup = "<b>SVG image (34x34 pt):</b>",
    widget = wibox.widget.textbox,
},1,1)

l:add_widget_at(wibox.widget {
    markup = "<b>PNG image (48x48 px):</b>",
    widget = wibox.widget.textbox,
},1,2)

l:add_widget_at(wibox.widget {
    image         = svg_image_path,
    forced_height = 48,
    forced_width  = 200,
    halign        = "center",
    widget        = wibox.widget.imagebox,
}, 2, 1, 1, 1)

l:add_widget_at(wibox.widget {
    image         = png_image_path,
    forced_height = 48,
    forced_width  = 200,
    halign        = "center",
    widget        = wibox.widget.imagebox,
}, 2, 2, 1, 1)

--DOC_HIDE_END
for k, dpi in ipairs {72, 96, 220} do
    --DOC_HIDE_START
    local r = 1 + k*2

    l:add_widget_at(wibox.widget {
        markup = "<b>border_image_dpi = ".. dpi .."</b>",
        widget = wibox.widget.textbox,
    }, r, 1, 1, 2)
    --DOC_HIDE_END

    for idx, image in ipairs {svg_image_path, png_image_path} do
        local w = wibox.widget {
            {
                text          = "Central widget",
                valign        = "center",
                align         = "center",
                forced_height = 30,
                forced_width  = 30,
                widget        = wibox.widget.textbox
            },
            fill               = false,
            border_image_dpi   = dpi,
            borders            = 10,
            border_image       = image,
            forced_width       = 200, --DOC_HIDE
            widget             = wibox.container.border
        }
        l:add_widget_at(w, r+1, idx, 1, 1)
    end
end
--DOC_HIDE_START

parent:add(l)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
