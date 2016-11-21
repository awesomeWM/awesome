local shape,cr,show = ... --DOC_HIDE

shape.arrow(cr, 70, 70)
show(cr) --DOC_HIDE

shape.arrow(cr,70,70, 30, 10, 60)
show(cr) --DOC_HIDE

shape.transform(shape.arrow) : rotate_at(35,35,math.pi/2)(cr,70,70)
show(cr) --DOC_HIDE
