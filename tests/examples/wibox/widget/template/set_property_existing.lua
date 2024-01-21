--DOC_GEN_IMAGE --DOC_NO_USAGE

--DOC_HIDE_START
local parent = ...
local wibox = require("wibox")
local gears = {share = require("gears.shape"), surface = require("gears.surface")}
local beautiful = require("beautiful")

client.focus = client.gen_fake{
    class = "client",
    name  = "A client!",
    icon  = beautiful.awesome_icon,
}

--DOC_HIDE_END

    local my_template_widget = wibox.template {
        {
            {
                id     = "icon_role",
                widget = wibox.widget.imagebox
            },
            {
                id     = "title_role",
                widget = wibox.widget.textbox
            },
            widget = wibox.layout.fixed.horizontal,
        },
        id     = "background_role",
        widget = wibox.container.background,
        forced_width  = 200, --DOC_HIDE
        forced_height = 24, --DOC_HIDE
    }

--DOC_NEWLINE
    -- Later in the code
    local c = client.focus
    my_template_widget:set_property("image", gears.surface(c.icon),"icon_role")
    my_template_widget:set_property("text", c.name, "title_role")
    my_template_widget:set_property("bg", "#0000ff", "background_role")
    my_template_widget:set_property("fg", "#ffffff", "background_role")
    my_template_widget:set_property(
        "shape", gears.share.rounded_rect, "background_role"
    )

--DOC_HIDE_START

parent:add(my_template_widget)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
