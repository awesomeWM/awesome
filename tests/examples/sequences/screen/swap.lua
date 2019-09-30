 --DOC_GEN_IMAGE --DOC_ASTERISK
local module = ... --DOC_HIDE
screen[1]:fake_resize(0, 0, 1280, 720) --DOC_HIDE
screen.fake_add(1280,0,1280,720) --DOC_HIDE
assert(screen.count()==2) --DOC_HIDE
assert(screen[1].geometry.width == 1280) --DOC_HIDE
assert(screen[1].geometry.height == 720) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.add_event("Calling :swap()", function() --DOC_HIDE
    screen[2]:swap(screen[1])
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute {display_screen=true, show_code_pointer=false} --DOC_HIDE
