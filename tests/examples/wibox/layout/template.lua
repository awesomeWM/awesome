local file_path, image_path = ...
require("_common_template")(...)

local wibox     = require( "wibox"         )
local color     = require( "gears.color"   )
local beautiful = require( "beautiful"     )
local unpack    = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)

-- Create a generic rectangle widget to show layout disposition
local function generic_widget(text, col)
    return {
        {
            {
                draw = function(_, _, cr, width, height)
                    cr:set_source(color(col or beautiful.bg_normal))
                    cr:set_line_width(3)
                    cr:rectangle(0, 0, width, height)
                    cr:fill_preserve()
                    cr:set_source(color(beautiful.border_color))
                    cr:stroke()
                end,
                widget = wibox.widget.base.make_widget
            },
            text and {
                align  = "center",
                valign = "center",
                text   = text,
                widget = wibox.widget.textbox
            } or nil,
            widget = wibox.layout.stack
        },
        margins = 5,
        widget  = wibox.container.margin,
    }
end

local names = {
    "first"  ,
    "second" ,
    "third"  ,
    "fourth" ,
    "fifth"  ,
    "sixth"  ,
    "seventh",
    "eighth" ,
    "ninth"  ,
}

-- Generic template to create "before and after" for layout mutators
local function generic_before_after(layout, layout_args, count, method, method_args)
    local ls = {}

    -- Create the layouts
    for i=1, 2 do
        local args = {layout = layout}

        -- In case the layout change the array (it is technically not forbidden)
        for k,v in pairs(layout_args) do
            args[k] = v
        end

        local l = wibox.layout(args)
        for j=1, count or 3 do
            l:add(wibox.widget(generic_widget(names[j] or "N/A")))
        end

        ls[i] = l
    end

    -- Mutate
    if type(method) == "function" then
        method(ls[2], unpack(method_args or {}))
    else
        ls[2][method](ls[2], unpack(method_args or {}))
    end

    return wibox.layout {
        {
            markup = "<b>Before:</b>",
            widget = wibox.widget.textbox,
        },
        ls[1],
        {
            markup = "<b>After:</b>",
            widget = wibox.widget.textbox,
        },
        ls[2],
        layout = wibox.layout.fixed.vertical
    }
end

-- Let the test request a size and file format
local widget, w, h = loadfile(file_path)(generic_widget, generic_before_after)

-- Emulate the event loop for 10 iterations
for _ = 1, 10 do
    awesome:emit_signal("refresh")
end

-- Save to the output file
wibox.widget.draw_to_svg_file(widget, image_path..".svg", w or 200, h or 30)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
