--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.rounded_rect(cr, 70, 70, 10)
show(cr) --DOC_HIDE

shape.rounded_rect(cr,20,70, 5)
show(cr) --DOC_HIDE

shape.transform(shape.rounded_rect) : translate(0,25) (cr,70,20, 5)
show(cr) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
