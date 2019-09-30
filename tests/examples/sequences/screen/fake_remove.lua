 --DOC_GEN_IMAGE --DOC_ASTERISK
local module = ... --DOC_HIDE
screen[1]:fake_resize(0, 0, 800, 600) --DOC_HIDE
screen.fake_add(800,0,800,600) --DOC_HIDE
screen.fake_add(800*2,0,800,600) --DOC_HIDE
assert(screen.count()==3) --DOC_HIDE
assert(screen[1].geometry.width == 800) --DOC_HIDE
assert(screen[1].geometry.height == 600) --DOC_HIDE
assert(screen[3].geometry.x == 800*2)--DOC_HIDE
local rw, rh = root.size() --DOC_HIDE
assert(rw == 3*800) --DOC_HIDE
assert(rh == 600) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.add_event("Calling :fake_remove()", function() --DOC_HIDE
    require("gears.timer").run_delayed_calls_now() --DOC_HIDE
    screen[2]:fake_remove()
end) --DOC_HIDE

module.display_tags() --DOC_HIDE

module.execute {display_screen=true} --DOC_HIDE
