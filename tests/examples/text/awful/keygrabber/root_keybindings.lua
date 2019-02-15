--DOC_GEN_OUTPUT --DOC_HIDE
local awful = { keygrabber = require("awful.keygrabber") } --DOC_HIDE

local keybinding_works = {} --DOC_HIDE

local g = --DOC_HIDE
awful.keygrabber {
    mask_modkeys = true,
    root_keybindings = {
        {{"Mod4"}, "i", function(self)
            print("Is now active!", self)
            keybinding_works[1] = true --DOC_HIDE
        end},
    },
    keybindings = {
        {{"Mod4", "Shift"}, "i", function(self)
            print("Called again!")
            keybinding_works[3] = true --DOC_HIDE
            self:stop()
        end},
    },
    keypressed_callback  = function(_, modifiers, key)
        print("A key was pressed:", key, "with", #modifiers, "modifier!")
        keybinding_works[2] = keybinding_works[2] and keybinding_works[2] + 1 or 1 --DOC_HIDE
    end,
}
--DOC_NEWLINE
-- The following will **NOT** trigger the keygrabbing because it isn't exported
-- to the root (global) keys. Adding `export_keybindings` would solve that
require("gears.delayed_call").run_now() --DOC_HIDE `root_keybindings` is async
root._execute_keybinding({"Mod4", "Shift"}, "i")
assert(#keybinding_works == 0)

--DOC_NEWLINE
-- But this will start the keygrabber because it is part of the root_keybindings
root._execute_keybinding({"Mod4"}, "i")
assert(keybinding_works[1]) --DOC_HIDE
assert(not keybinding_works[2]) --DOC_HIDE

--DOC_NEWLINE
-- Note that that keygrabber is running, all callbacks should work:
root.fake_input("key_press"  , "a")
root.fake_input("key_release"  , "a")
assert(keybinding_works[2] == 1) --DOC_HIDE

--DOC_NEWLINE
-- Calling the root keybindings now wont work because they are not part of
-- the keygrabber internal (own) keybindings, so `keypressed_callback` will
-- be called.
root._execute_keybinding({"Mod4"}, "i")
assert(keybinding_works[2] == 2) --DOC_HIDE because mask_modkeys is set
assert(g == awful.keygrabber.current_instance) --DOC_HIDE
assert(not keybinding_works[3])--DOC_HIDE


--DOC_NEWLINE
-- Now the keygrabber own keybindings will work
root._execute_keybinding({"Mod4", "Shift"}, "i")
assert(keybinding_works[3])--DOC_HIDE
keybinding_works[2] = 0--DOC_HIDE
assert(not awful.keygrabber.current_instance) --DOC_HIDE
root.fake_input("key_press"  , "a") --DOC_HIDE
root.fake_input("key_release"  , "a") --DOC_HIDE
assert(keybinding_works[2] == 0) --DOC_HIDE
