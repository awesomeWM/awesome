--DOC_GEN_IMAGE --DOC_HIDE
local shape,cr,show = ... --DOC_HIDE

shape.infobubble(cr, 70, 70)
show(cr) --DOC_HIDE

shape.transform(shape.infobubble) : translate(0, 20)
: rotate_at(35,35,math.pi) (cr,70,20,10, 5, 35 - 5)
show(cr) --DOC_HIDE

shape.transform(shape.infobubble)
: rotate_at(35,35,3*math.pi/2) (cr,70,70, nil, nil, 40)
show(cr) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
