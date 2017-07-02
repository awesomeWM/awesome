--DOC_HIDE_ALL
local naughty = require("naughty") --DOC_HIDE

for _, pos in ipairs {
    "top_left",
    "top_middle",
    "top_right",
    "bottom_left",
    "bottom_middle",
    "bottom_right",
} do
    naughty.notify {
        title    = pos,
        position = pos,
        text     = "",
    }
end


--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
