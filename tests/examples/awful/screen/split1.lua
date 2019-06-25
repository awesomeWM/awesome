--DOC_GEN_IMAGE --DOC_HIDE  --DOC_NO_USAGE

require("awful.screen") --DOC_HIDE

screen[1]._resize {x = 95, width = 256, height = 108} --DOC_HIDE

    screen[1]:split({1/5, 3/5, 1/5})

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
