--DOC_GEN_OUTPUT --DOC_GEN_IMAGE --DOC_HIDE
require("awful.tag").add("1", {screen=screen[1], selected=true}) --DOC_HIDE
screen[1]._resize {x = 175, width = 128, height = 96} --DOC_HIDE
mouse.coords {x=175+60,y=60} --DOC_HIDE

-- Get the position
print(mouse.coords().x)

-- Change the position
mouse.coords {
    x = 185,
    y = 10
}

mouse.push_history() --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
