local shape,cr,show = ... --DOC_HIDE

local original_shape = shape.arrow
local rotated_shape = shape.rotate_180(original_shape)

original_shape(cr, 70, 70)
show(cr) --DOC_HIDE

rotated_shape(cr, 70, 70)
show(cr) --DOC_HIDE
