--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.pie(cr, 70, 70)
show(cr) --DOC_HIDE

shape.pie(cr,70,70, 1.0471975511966,   4.1887902047864)
show(cr) --DOC_HIDE

shape.pie(cr,70,70, 0, 2*math.pi, 10)
show(cr) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
