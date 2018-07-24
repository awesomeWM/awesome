--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.partially_rounded_rect(cr, 70, 70)
show(cr) --DOC_HIDE

shape.partially_rounded_rect(cr, 70, 70, true)
show(cr) --DOC_HIDE

shape.partially_rounded_rect(cr, 70, 70, true, true, false, true, 30)
show(cr) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
