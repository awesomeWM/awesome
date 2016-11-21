local shape,cr,show = ... --DOC_HIDE

shape.partially_rounded_rect(cr, 70, 70)
show(cr) --DOC_HIDE

shape.partially_rounded_rect(cr, 70, 70, true)
show(cr) --DOC_HIDE

shape.partially_rounded_rect(cr, 70, 70, true, true, false, true, 30)
show(cr) --DOC_HIDE
