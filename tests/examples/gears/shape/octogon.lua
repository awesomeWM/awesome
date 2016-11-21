local shape,cr,show = ... --DOC_HIDE

shape.octogon(cr, 70, 70)
show(cr) --DOC_HIDE

shape.octogon(cr,70,70,70/2.5)
show(cr) --DOC_HIDE

shape.transform(shape.octogon) : translate(0, 25) (cr,70,20)
show(cr) --DOC_HIDE
