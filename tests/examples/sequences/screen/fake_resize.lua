 --DOC_GEN_IMAGE --DOC_ASTERISK
local module = ... --DOC_HIDE
screen[1]:fake_resize(0, 0, 1280, 720) --DOC_HIDE
assert(screen[1].geometry.width == 1280) --DOC_HIDE
assert(screen[1].geometry.height == 720) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.add_event("Calling :fake_resize()", function() --DOC_HIDE
    screen[1]:fake_resize(100, 0, 1024, 768)
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute {display_screen=true, show_code_pointer=false} --DOC_HIDE
