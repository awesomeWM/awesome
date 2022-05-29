--DOC_NO_USAGE

--DOC_HIDE_START
local parent = ...

local wibox = require("wibox")

local function concrete_widget_template_builder(args)
    return wibox.widget.template(args)
end
--DOC_HIDE_END

    -- Instanciate the widget with the default template
    local default_widget = concrete_widget_template_builder()

--DOC_NEWLINE
    -- Instanciate the widget with a custom template
    local custom_widget = concrete_widget_template_builder {
        widget_template = {
            template = wibox.widget.imagebox,
            update_callback = function (template, args)
                if args and args.text == "default text" then
                    template.widget.image = "/path/to/image.png"
                else
                    template.widget.image = "/path/to/another-image.png"
                end
            end

        }
    }

--DOC_HIDE_START

parent:add(default_widget)
parent:add(custom_widget)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

