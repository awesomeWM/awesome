--DOC_NO_USAGE

--DOC_HIDE_START
local wibox = require("wibox")
--DOC_HIDE_END

    local my_template_widget = {
        id = "mytemplatewidget",
        template = wibox.widget.textbox,
        update_callback = function(template_widget, args)
            local text = args.text or "???"
            template_widget.widget.text = text
        end,
        widget = wibox.widget.template,
    }

--DOC_HIDE_START

-- Generate the widget to use the update method in the example.
-- A better approch would have been to use an ID and the `get_children_by_id`
-- on the parent.
local widget = wibox.widget(my_template_widget)
widget:update { text = "Cool text to update the template!" }

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
