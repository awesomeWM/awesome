---------------------------------------------------------------------------
--- Utilities related to the keyboard and text entry.
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2018 Emmanuel Lepage Vallee
-- @module awful.keyboard
---------------------------------------------------------------------------
local capi = {root = root, client = client}

local module = {}

-- The hash order isn't consistent when the content changes, but it is
-- constant when the table doesn't change.
local conversion = {
    Super_L   = "Mod4",
    Control_L = "Control",
    Shift_L   = "Shift",
    Alt_L     = "Mod1",
    Super_R   = "Mod4",
    Control_R = "Control",
    Shift_R   = "Shift",
    Alt_R     = "Mod1",
}

--- Send an artificial set of key events to trigger a key combination.
--
-- This can be used to call application actions or Awesome own keybindings.
--
-- @tparam table modifiers The modifiers (like `Control` or `Mod4`).
-- @tparam string|number key The key name or keycode.
-- @function awful.keyboard.execute_keybindings
function module.execute_keybinding(modifiers, key)
    for _, mod in ipairs(modifiers) do
        for real_key, mod_name in pairs(conversion) do
            if mod == mod_name then
                capi.root.fake_inputs("key_press", real_key)
                break
            end
        end
    end

    capi.root.fake_inputs("key_press"  , key)
    capi.root.fake_inputs("key_release", key)

    for _, mod in ipairs(modifiers) do
        for real_key, mod_name in pairs(conversion) do
            if mod == mod_name then
                capi.root.fake_inputs("key_release", real_key)
                break
            end
        end
    end
end

--- Send artificial key events to write a string.
-- @tparam string string The string.
-- @tparam[opt=nil] client c A client to send the string to.
-- @function awful.keyboard.write_string
function module.write_string(string, c)
    local old_c = capi.client.focus

    if c then
        capi.client.focus = c
    end

    for i=1, #string do
        local char = string:sub(i,i)
        capi.root.fake_inputs("key_press"  , char)
        capi.root.fake_inputs("key_release", char)
    end

    if c then
        capi.client.focus = old_c
    end
end

return module
