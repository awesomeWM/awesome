local shape,cr,show = ... --DOC_HIDE

shape.circle(cr, 70, 70)
show(cr) --DOC_HIDE

shape.circle(cr,20,70)
show(cr) --DOC_HIDE

shape.transform(shape.circle) : scale(0.5, 1)(cr,70,70)
show(cr) --DOC_HIDE
