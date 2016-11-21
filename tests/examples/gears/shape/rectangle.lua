local shape,cr,show = ... --DOC_HIDE

shape.rectangle(cr, 70, 70)
show(cr) --DOC_HIDE

shape.rectangle(cr,20,70)
show(cr) --DOC_HIDE

shape.transform(shape.rectangle) : scale(0.5, 1)(cr,70,70)
show(cr) --DOC_HIDE
