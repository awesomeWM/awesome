--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.partial_squircle(cr, 70, 70, false, true)
show(cr) --DOC_HIDE

shape.partial_squircle(cr, 70, 70, true, false, true)
show(cr) --DOC_HIDE

shape.partial_squircle(cr, 70, 70, true, false, true, true)
show(cr) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
