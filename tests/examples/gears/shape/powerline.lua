--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.powerline(cr, 70, 70)
show(cr) --DOC_HIDE

shape.transform(shape.powerline) : translate(0, 25) (cr,70,20)
show(cr) --DOC_HIDE

shape.transform(shape.powerline) : translate(0, 25) (cr,70,20, -20)
show(cr) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
