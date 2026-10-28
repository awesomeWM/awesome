--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.individually_rounded_rect(cr, 70, 70, 0, 10, 20, 30)
show(cr) --DOC_HIDE

shape.individually_rounded_rect(cr, 70, 70, 10, 0, 35, 5)
show(cr) --DOC_HIDE

shape.individually_rounded_rect(cr, 70, 70, 30, 20, 10, 1)
show(cr) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
