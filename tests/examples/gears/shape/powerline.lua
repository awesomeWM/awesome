local shape,cr,show = ... --DOC_HIDE

shape.powerline(cr, 70, 70)
show(cr) --DOC_HIDE

shape.transform(shape.powerline) : translate(0, 25) (cr,70,20)
show(cr) --DOC_HIDE

shape.transform(shape.powerline) : translate(0, 25) (cr,70,20, -20)
show(cr) --DOC_HIDE
