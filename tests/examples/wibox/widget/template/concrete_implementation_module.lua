--DOC_NO_USAGE

--DOC_HIDE_START
local parent = ...

local gears = require("gears")
local wibox = require("wibox")

local concrete_widget_template_builder
--DOC_HIDE_END

    -- Build the default widget used as a fallback if user doesn't provide a template
    local default_widget = {
        template = wibox.widget.textclock,
        update_callback = function(widget_template, args)
            local text = args and args.text or "???"
            widget_template.widget.text = text
        end,
    }

--DOC_NEWLINE
    function concrete_widget_template_builder(args)
        args = args or {}

--DOC_NEWLINE
        -- Build an instance of the template widget with either, the
        -- user provided parameters or the default
        local ret =  wibox.widget.template(
            args.widget_template and args.widget_template or
            default_widget
        )

--DOC_NEWLINE
            -- Patch the methods and fields the widget instance should have

--DOC_NEWLINE
            -- e.g. Apply the received buttons, visible, forced_width and so on
            gears.table.crush(ret, args)

--DOC_NEWLINE
            -- Optionally, call update once with parameters to prepare the widget
            ret:update {
                text = "default text",
            }

--DOC_NEWLINE
        return ret
    end

--DOC_HIDE_START

local w = concrete_widget_template_builder()
parent:add(w)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

