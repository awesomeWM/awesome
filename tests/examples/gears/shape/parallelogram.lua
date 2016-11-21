local shape,cr,show = ... --DOC_HIDE

shape.parallelogram(cr, 70, 70)
show(cr) --DOC_HIDE

shape.parallelogram(cr,70,20)
show(cr) --DOC_HIDE

shape.transform(shape.parallelogram) : scale(0.5, 1)(cr,70,70)
show(cr) --DOC_HIDE
