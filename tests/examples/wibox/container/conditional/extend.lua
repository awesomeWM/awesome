local parent = ... --DOC_HIDE --DOC_NO_USAGE
local wibox  = require("wibox") --DOC_HIDE
local awful = {screen = require("awful.screen")} --DOC_HIDE

--DOC_HIDE Add a client and a tag
local c = client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE
client.focus = c --DOC_HIDE
local t = require("awful.tag").add("Test", {screen=screen[1]}) --DOC_HIDE
c:tags{t} --DOC_HIDE

    local function extended_conditional()
        return wibox.container.conditional {
            argument_callback = function()
                return client.focus, awful.screen.focused().selected_tag
            end,
            connected_global_signals = {
                {client , "focus"               },
                {tag    , "property::selected"  },
                {tag    , "property::activated" },
            }
        }
    end

    local widget = wibox.widget {
        {
            text   = "There is no tag!",
            when   = function(focused_client, current_tag) -- luacheck: no unused args
                return not current_tag
            end,
            widget = wibox.widget.textbox
        },
        {
            text   = "There is no client!",
            when   = function(focused_client, current_tag) -- luacheck: no unused args
                return not focused_client
            end,
            widget = wibox.widget.textbox
        },
        {
            text   = "There is a client!",
            when   = function(focused_client, current_tag) -- luacheck: no unused args
                assert(focused_client and focused_client.name) --DOC_HIDE
                assert(current_tag and current_tag.name) --DOC_HIDE
                return true
            end,

            widget = wibox.widget.textbox
        },
        layout = extended_conditional
    }

parent:add(widget) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
