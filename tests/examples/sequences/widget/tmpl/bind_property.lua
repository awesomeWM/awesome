 --DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local module = ...
local wibox = require("wibox")
local beautiful = require("beautiful")
local gears = {surface = require("gears.surface")}

client.focus = client.gen_fake{
    class = "client",
    name  = "A client!",
    icon  = beautiful.awesome_icon,
}

    --DOC_HIDE_END

    local my_template_widget = wibox.template {
        {
            {
                set_icon = function(self, icon)
                    self.image = gears.surface(icon)
                end,
                id     = "icon_role",
                widget = wibox.widget.imagebox
            },
            {
                id     = "title_role",
                widget = wibox.widget.textbox
            },
            widget = wibox.layout.fixed.horizontal,
        },
        widget     = wibox.container.background,
        id         = "background_role",
        set_urgent = function(self, status)
            self.bg = status and "#ff0000" or nil
        end,
        forced_width  = 200, --DOC_HIDE
        forced_height = 24, --DOC_HIDE
    }

--DOC_NEWLINE
--DOC_HIDE_START
module.display_widget(my_template_widget, 200, 24)

module.add_event("Original state", function()
    --DOC_HIDE_END

    -- Use the normal widget properties.
    my_template_widget:bind_property(client.focus, "name", "text", "title_role")
    my_template_widget:bind_property(client.focus, "icon", "icon", "icon_role")
    --DOC_NEWLINE
    -- This one uses an inline setter method.
    my_template_widget:bind_property(client.focus, "urgent", "urgent", "background_role")

    --DOC_HIDE_START
end)

--DOC_NEWLINE
module.display_widget(my_template_widget, 200, 24)

module.add_event("Change the client name.", function()
    --DOC_HIDE_END
    --DOC_NEWLINE
    -- Change the client name.
    client.focus.name = "New name!"
    --DOC_HIDE_START
end)

--DOC_NEWLINE
module.display_widget(my_template_widget, 200, 24)

module.add_event("Make urgent", function()
    --DOC_HIDE_END
    --DOC_NEWLINE
    -- Make urgent.
    client.focus.urgent = true
    --DOC_HIDE_START
end)

module.display_widget(my_template_widget, 200, 24)

module.add_event("Make not urgent", function()
    --DOC_HIDE_END
    --DOC_NEWLINE
    -- "Make not urgent.
    client.focus.urgent = false
    --DOC_HIDE_START
end)

module.display_widget(my_template_widget, 200, 24)


module.execute { display_screen = false, display_clients     = true,
                 display_label  = false, display_client_name = true,
                 display_mouse  = true ,
}
