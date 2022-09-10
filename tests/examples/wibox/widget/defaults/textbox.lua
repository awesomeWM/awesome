--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

parent:add( --DOC_HIDE

wibox.widget{
    markup = "This <i>is</i> a <b>textbox</b>!!!",
    halign = "center",
    valign = "center",
    widget = wibox.widget.textbox
}

) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
