local shape,cr,show = ... --DOC_HIDE

shape.cross(cr, 70, 70)
show(cr) --DOC_HIDE

shape.cross(cr,20,70)
show(cr) --DOC_HIDE

shape.transform(shape.cross) : scale(0.5, 1)(cr,70,70)
show(cr) --DOC_HIDE
