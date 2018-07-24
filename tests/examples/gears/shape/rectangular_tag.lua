--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.rectangular_tag(cr, 70, 70)
show(cr) --DOC_HIDE

-- shape.rectangular_tag(cr,20,70) --TODO broken --DOC_HIDE
-- show(cr) --DOC_HIDE

shape.transform(shape.rectangular_tag) : translate(0, 30) (cr, 70, 10,  10)
show(cr) --DOC_HIDE

shape.transform(shape.rectangular_tag) : translate(0, 30) (cr, 70, 10, -10)
show(cr) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
