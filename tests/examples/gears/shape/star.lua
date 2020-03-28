--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.star(cr, 70, 70, 4)
show(cr) --DOC_HIDE

shape.star(cr, 70, 70, 9)
show(cr) --DOC_HIDE

shape.transform(shape.star) : translate(70/2, 70/2)
: rotate(math.pi) : scale(0.5, 0.75)
: translate(-70/2, -70/2) (cr, 70, 70)
show(cr) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
