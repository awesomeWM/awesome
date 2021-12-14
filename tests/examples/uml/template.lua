local file_path, image_path = ...
require("_common_template")(...)
local cairo = require("lgi").cairo
local shape = require("gears.shape")
local wibox = require("wibox")
local beautiful = require("beautiful")

-- Make the path relative.
local path = image_path:match("/(images/[^/]+)$")
if not path then
    path = image_path:match("/(raw_images/[^/]+)$"):gsub("raw_", "")
end
local relative_image_path = "../" .. path

-- This template generates an HTML table with how other classes are associated
-- with a given class.

local function gen_class(name)
    return wibox.widget {
        {
            {
                {
                    valign = "center",
                    align  = "center",
                    markup = "<b>"..name.."</b>",
                    widget = wibox.widget.textbox
                },
                fg            = "#ffffff",
                bg            = beautiful.bg_normal,
                widget        = wibox.container.background,
            },
            nil,
            nil,
            widget = wibox.layout.align.vertical,
        },
        shape = function(cr,w,h)
            shape.partially_rounded_rect(cr,w,h, true,true,false,false,5)
        end,
        forced_width = 75,
        border_width = 1,
        border_strategy = "inner",
        border_color = beautiful.border_color,
        bg           = "#00000000",
        widget       = wibox.container.background
    }
end

local function gen_arrow(association)
    local w = wibox.widget.base.make_widget()

    function w:fit(_, width, _)
        return width, 15
    end

    function w:draw(_, cr, width, height)
        cr:set_line_width(1.5)
        cr:move_to(0, height/2)
        cr:line_to(width-(association == "association" and 0 or 40), height/2)
        cr:stroke()

        if association ~= "association" then
            cr:set_antialias(cairo.ANTIALIAS_SUBPIXEL)
            cr:set_line_width(1)
            cr:translate(0,2)
            width, height = width-2, height-4
            cr:move_to(width-40, height/2)
            cr:line_to(width-20, 0  )
            cr:line_to(width   , height/2)
            cr:line_to(width-20, height  )
            cr:line_to(width-40, height/2)
            cr:close_path()

            if association == "composition" then
                cr:fill()
            else
                cr:stroke()
            end
        end
    end

    w.forced_height = 15

    return w
end

local function arrow_widget(dir)
    local w = wibox.widget.base.make_widget()

    function w:fit(_, width, _)
        return width, 5
    end

    function w:draw(_, cr, width, height)

        cr:translate(10, 0)
        width = width - 20

        cr:set_source_rgba(0,0,0,0.2)
        cr:set_line_width(1)
        cr:move_to(0, height/2)
        cr:line_to(width,height/2)
        cr:stroke()

        if dir == "left" then
            cr:move_to(height/2, 0)
            cr:line_to(0, height/2)
            cr:line_to(height/2, height)
            cr:stroke()
        else
            cr:move_to(width-height/2, 0)
            cr:line_to(width, height/2)
            cr:line_to(width-height/2, height)
            cr:stroke()
        end
    end

    return w
end

local function gen_table_uml(entry, class1, class2, revert)
    local left  = revert and entry.right or entry.left
    local right = revert and entry.left  or entry.right

    return wibox.widget {
        gen_class(class2),
        {
            {
                {
                    text   = " "..left.card,
                    widget = wibox.widget.textbox
                },
                arrow_widget "left",
                {
                    wrap   = "char",
                    markup = left.msg.."<span alpha='1'> .</span>",
                    widget = wibox.widget.textbox
                },
                spacing = 5,
                layout  = wibox.layout.align.horizontal
            },
            gen_arrow(entry.composition),
            {
                {
                    wrap   = "char",
                    markup = right.msg.."<span alpha='1'> .</span>",
                    widget = wibox.widget.textbox
                },
                arrow_widget "right",
                {
                    text   = right.card.." ",
                    widget = wibox.widget.textbox
                },
                spacing = 5,
                layout  = wibox.layout.align.horizontal
            },
            layout = wibox.layout.fixed.vertical,
        },
        gen_class(class1),
        layout = wibox.layout.align.horizontal
    }
end

local map = {
    to   = function(o) return "Acquire other objects from a "..o end,
    from = function(o) return "Acquire a "..o.." from other objects" end
}

local function gen_table_header(title, o)
    print([[<div class="components-relationship--diagram"><table class='widget_list' border=1><colgroup span="3"></colgroup><tr><th align='center' colspan=3 scope=colgroup>]]..map[title](o)..[[</th></tr><tr style='font-weight: bold;'><th align='center'>Class</th><th align='center'></th><th align='center'>Property</th></tr>]])

end

local function get_table_row(path, class, prop)
    print([[<tr><td>]].. class ..[[</td><td><img src="]]..path..[["></td><td>]].. prop ..[[</td></tr>]])
end

local function get_table_footer()
    print '</table></div>'
end

local module = {}

local counter = 1

function module.generate_nav_table(t)
    assert(t.content and t.class)

    print("\n\nCore components relationship\n===\n")
    print('<div class="components-relationship"><div class="components-relationship--diagrams">')

    -- Validate early to avoid debugging cryptic backtraces.
    for _, tab in ipairs {"to", "from"} do
        gen_table_header(tab, t.class)
        for _, entry in ipairs(t.content) do
            if entry[tab.."_property"] then
                assert(entry.left and entry.right and entry.association)
                assert(entry.class)
                assert(entry.left.msg and entry.left.card)
                assert(entry.right.msg and entry.right.card)
                local path = relative_image_path..counter..".svg"
                local fpath = image_path..counter..".svg"
                local widget = gen_table_uml(entry, t.class, entry.class, false)
                wibox.widget.draw_to_svg_file(widget, fpath, 320, 50)
                get_table_row(path, entry.class, entry[tab.."_property"])
                counter = counter + 1
            end
        end
        get_table_footer()
    end

    -- End the last section and add a footer
    print([[</div><div class="components-relationship--legend"><b>Legend:</b> <i>c</i>: a client object, <i>t</i>: a tag object, <i>s</i>: a screen object, <i>k</i>: an awful.key object, <i>b</i>: a awful.button object, <i>n</i>: a naughty.notification object</div></div> <!-- .components-relationship -->]])
end

loadfile(file_path)(module)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
