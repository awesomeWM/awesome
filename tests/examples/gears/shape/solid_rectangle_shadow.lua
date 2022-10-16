--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.solid_rectangle_shadow(cr, 70, 70, 10, 5)
show(cr) --DOC_HIDE

shape.solid_rectangle_shadow(cr, 70, 70, 5, -10)
show(cr) --DOC_HIDE

shape.solid_rectangle_shadow(cr, 70, 70, 30, -30)
show(cr) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
