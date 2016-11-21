local shape,cr,show = ... --DOC_HIDE

shape.pie(cr, 70, 70)
show(cr) --DOC_HIDE

shape.pie(cr,70,70, 1.0471975511966,   4.1887902047864)
show(cr) --DOC_HIDE

shape.pie(cr,70,70, 0, 2*math.pi, 10)
show(cr) --DOC_HIDE
