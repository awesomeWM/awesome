--DOC_GEN_IMAGE --DOC_HIDE

--DOC_HIDE_START
local parent = ...
local wibox  = require("wibox")

local widget = function(inner)
    return wibox.widget {
        {
            {
                inner,
                margins = 5,
                widget = wibox.container.margin,
            },
            border_width = 2,
            widget = wibox.container.background,
        },
        strategy = "max",
        height = 25,
        widget = wibox.container.constraint,
    }
end

local widgets = {
--DOC_HIDE_END

widget{
    text = "This is a very long text, that cannot be displayed fully.",
    ellipsize = "start",
    widget = wibox.widget.textbox,
},

widget{
    text = "This is a very long text, that cannot be displayed fully.",
    ellipsize = "end",
    widget = wibox.widget.textbox,
},

widget{
    text = "This is a very long text, that cannot be displayed fully.",
    ellipsize = "middle",
    widget = wibox.widget.textbox,
},

widget{
    text = "This is a very long text, that cannot be displayed fully.",
    ellipsize = "none",
    valign = "top",
    widget = wibox.widget.textbox,
}

--DOC_HIDE_START
}

parent:set_children(widgets)

return parent:fit({ dpi = 96 }, 150, 200)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
