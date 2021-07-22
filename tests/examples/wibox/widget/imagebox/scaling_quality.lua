--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local lgi = require("lgi")--DOC_HIDE
local cairo = lgi.cairo --DOC_HIDE

--DOC_HIDE A simple Awesome logo
local function demo()--DOC_HIDE
    local img = cairo.ImageSurface.create(cairo.Format.ARGB32, 32, 22)--DOC_HIDE
    local cr = cairo.Context(img)--DOC_HIDE
    cr:set_antialias(cairo.Antialias.NONE) --DOC_HIDE

    -- Awesome default #555555--DOC_HIDE
    cr:set_source_rgb(0,0,1)--DOC_HIDE
    cr:paint() --DOC_HIDE

    cr:set_source_rgb(1,0,0)--DOC_HIDE

    cr:rectangle(0, 10, 32, 2)--DOC_HIDE
    cr:rectangle(15, 0, 2, 22)--DOC_HIDE
    cr:fill()--DOC_HIDE

    cr:set_source_rgb(0,1,0)--DOC_HIDE

    cr:arc(16, 11, 8, 0, 2*math.pi)--DOC_HIDE
    cr:fill()--DOC_HIDE

    return img--DOC_HIDE
end--DOC_HIDE

local img = demo () --DOC_HIDE

local l = wibox.layout { --DOC_HIDE
    -- forced_width  = 720, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.vertical --DOC_HIDE
} --DOC_HIDE

 for _, quality in ipairs {"fast", "good", "best", "nearest", "bilinear"} do
    local w = wibox.widget {
        {
            {
                image           = img,
                forced_height   = 64,
                forced_width    = 96,
                scaling_quality = quality,
                widget          = wibox.widget.imagebox
            },
            widget = wibox.container.place
        },
        -- forced_height = 96, --DOC_HIDE
        -- forced_width = 96, --DOC_HIDE
        widget = wibox.container.background
    }

    --DOC_HIDE SVG doesn't support those mode, so rasterize everything.
    local raster = wibox.widget.draw_to_image_surface(w, 96, 64) --DOC_HIDE

    l:add(wibox.widget {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>`scaling_quality` = "..quality.."</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        { --DOC_HIDE
            image = raster, --DOC_HIDE
            forced_height   = 64, --DOC_HIDE
            forced_width    = 96, --DOC_HIDE
            widget = wibox.widget.imagebox --DOC_HIDE
        },--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    }) --DOC_HIDE
 end


parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
