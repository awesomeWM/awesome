local shape,cr,show = ... --DOC_HIDE

shape.arc(cr,70,70, 10)
show(cr) --DOC_HIDE

shape.arc(cr,70,70, 10, nil, nil, true, true)
show(cr) --DOC_HIDE

shape.arc(cr,70,70, nil, 0, 2*math.pi)
show(cr) --DOC_HIDE
