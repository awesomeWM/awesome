--DOC_GEN_IMAGE
--DOC_HIDE_START
local awful     = require("awful")
local wibox     = require("wibox")
local beautiful = require("beautiful")

screen._track_workarea = true
screen[1]._resize {width = 480, height = 128}
screen._add_screen {x = 0, width = 480, height = 128, y = 140}

local t1 = awful.tag.add("1", {
    screen   = screen[1],
    selected = true,
    layout   = awful.layout.suit.tile.right,
    gap      = 4
})

local c1 = client.gen_fake {hide_first=true, screen = screen[2]}
c1:tags{t1}

local t2 = awful.tag.add("1", {
    screen   = screen[2],
    selected = true,
    layout   = awful.layout.suit.tile.right,
    gap      = 4
})

local c2 = client.gen_fake {hide_first=true}
c2:tags{t2}

for _, c in ipairs {c1, c2} do
    local top_titlebar = awful.titlebar(c, {
        size      = 20,
        bg_normal =  beautiful.bg_highligh,
    })
    top_titlebar : setup {
        { -- Left
            awful.titlebar.widget.iconwidget(c),
            layout  = wibox.layout.fixed.horizontal
        },
        { -- Middle
            { -- Title
                halign = "center",
                widget = awful.titlebar.widget.titlewidget(c)
            },
            layout  = wibox.layout.flex.horizontal
        },
        { -- Right
            awful.titlebar.widget.floatingbutton (c),
            awful.titlebar.widget.maximizedbutton(c),
            awful.titlebar.widget.stickybutton   (c),
            awful.titlebar.widget.ontopbutton    (c),
            awful.titlebar.widget.closebutton    (c),
            layout = wibox.layout.fixed.horizontal()
        },
        layout = wibox.layout.align.horizontal
    }
end

--DOC_HIDE_END

awful.wibar {
    position = "top",
    screen   = screen[1],
    stretch  = false,
    width    = 196,
    margins  = 24,
    widget   = {
        halign = "center",
        text   = "unform margins",
        widget = wibox.widget.textbox
    }
}

--DOC_NEWLINE

awful.wibar {
    position = "top",
    screen   = screen[2],
    stretch  = false,
    width    = 196,
    margins = {
        top    = 12,
        bottom = 5
    },
    widget   = {
        halign = "center",
        text   = "non unform margins",
        widget = wibox.widget.textbox
    }

}

awful.layout.arrange(screen[1]) --DOC_HIDE
awful.layout.arrange(screen[2]) --DOC_HIDE
c1:_hide_all() --DOC_HIDE
c2:_hide_all() --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
