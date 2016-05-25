local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

parent:add( --DOC_HIDE

wibox.widget{
    markup = "This <i>is</i> a <b>textbox</b>!!!",
    align  = "center",
    valign = "center",
    widget = wibox.widget.textbox
}

) --DOC_HIDE
