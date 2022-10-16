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

    local my_template_widget = wibox.widget.template {
        template = {
            {
                {
                    id         = "client_role",
                    set_client = function(self, c)
                        self.image = gears.surface(c.icon)
                    end,
                    widget = wibox.widget.imagebox
                },
                {
                    id         = "client_role",
                    set_client = function(self, c)
                        -- If the value can change, don't forget to connect
                        -- some signals:
                        local function update()
                            local txt = "<b>Name: </b>"..c.name
                            if c.minimized then
                                txt = txt .. "<i> (minimized)</i>"
                            end
                            self.markup = txt
                        end

                        update()
                        c:connect_signal("property::name", update)
                        c:connect_signal("property::minimized", update)
                    end,
                    widget = wibox.widget.textbox
                },
                widget = wibox.layout.fixed.horizontal,
            },
            bg     = "#0000ff",
            fg     = "#ffffff",
            shape  = gears.share.rounded_rect,
            widget = wibox.container.background,
            forced_width  = 200, --DOC_HIDE
            forced_height = 24, --DOC_HIDE
        }
    }

--DOC_NEWLINE
    -- Later in the code
    local c = client.focus
    my_template_widget:set_property("client", c, "client_role")
    c.minimized = true

--DOC_HIDE_START

parent:add(my_template_widget)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
