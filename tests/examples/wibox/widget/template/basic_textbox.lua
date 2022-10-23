--DOC_GEN_IMAGE --DOC_NO_USAGE

--DOC_HIDE_START
local parent = ...
local wibox = require("wibox")
--DOC_HIDE_END

    local my_template_widget = wibox.widget.template {
        widget = wibox.widget.textbox,
        update_callback = function(template_widget, args)
            local text = args.text or "???"
            template_widget.widget.text = text
        end,
    }

--DOC_NEWLINE
    -- Later in the code

--DOC_NEWLINE
    -- With no arguments, the update_callback will fallback to "???"
    my_template_widget:update()

--DOC_NEWLINE
    -- With custom arguments, the update_callback will changes the displayed text
    my_template_widget:update { text = "Cool text to update the template!" }

--DOC_HIDE_START

parent:add(my_template_widget)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
