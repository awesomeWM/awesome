local shape,cr,show = ... --DOC_HIDE

shape.rounded_rect(cr, 70, 70, 10)
show(cr) --DOC_HIDE

shape.rounded_rect(cr,20,70, 5)
show(cr) --DOC_HIDE

shape.transform(shape.rounded_rect) : translate(0,25) (cr,70,20, 5)
show(cr) --DOC_HIDE
