local root = {_tags={}}

local gtable = require("gears.table")
local cairo  = require( "lgi" ).cairo


function root:tags()
    return root._tags
end

function root.size()
    local geo = {x1 = math.huge, y1 = math.huge, x2 = 0, y2 = 0}

    for s in screen do
        geo.x1 = math.min( geo.x1, s.geometry.x                   )
        geo.y1 = math.min( geo.y1, s.geometry.y                   )
        geo.x2 = math.max( geo.x2, s.geometry.x+s.geometry.width  )
        geo.y2 = math.max( geo.y2, s.geometry.y+s.geometry.height )
    end

    return math.max(0, geo.x2-geo.x1), math.max(0, geo.y2 - geo.y1)
end

function root.size_mm()
    local w, h = root.size()
    return (w/96)*25.4, (h/96)*25.4
end

function root.cursor() end

-- GLOBAL KEYBINDINGS --

local keys = {}

function root._keys(k)
    keys = k or keys
    return keys
end

-- FAKE INPUTS --

-- Turn keysym into modkey names
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

-- The currently pressed modkeys.
local mods = {}
local function get_mods()
    local ret = {}

    for mod in pairs(mods) do
        table.insert(ret, mod)
    end

    return ret
end

local function add_modkey(key)
    if not conversion[key] then return end
    mods[conversion[key]] = true
end

local function remove_modkey(key)
    if not conversion[key] then return end
    mods[conversion[key]] = nil
end

local function match_modifiers(mods1, mods2)
    if #mods1 ~= #mods2 then return false end

    for _, mod1 in ipairs(mods1) do
        if not gtable.hasitem(mods2, mod1) then
            return false
        end
    end

    return true
end

local function execute_keybinding(key, event)
    for _, v in ipairs(keys) do
        if key == v.key and match_modifiers(v.modifiers, get_mods()) then
            v:emit_signal(event)
            return
        end
    end
end

local fake_input_handlers = {
    key_press      = function(key)
        add_modkey(key)
        if keygrabber._current_grabber then
            keygrabber._current_grabber(get_mods(), key, "press")
        else
            execute_keybinding(key, "press")
        end
    end,
    key_release    = function(key)
        remove_modkey(key)
        if keygrabber._current_grabber then
            keygrabber._current_grabber(get_mods(), key, "release")
        else
            execute_keybinding(key, "release")
        end
    end,
    button_press   = function() --[[TODO]] end,
    button_release = function() --[[TODO]] end,
    motion_notify  = function() --[[TODO]] end,
}

function root.fake_input(event_type, detail, x, y)
    assert(fake_input_handlers[event_type], "Unknown event_type")

    fake_input_handlers[event_type](detail, x, y)
end

function root._buttons()
    return {}
end

-- Send an artificial set of key events to trigger a key combination.
-- It only works in the shims and should not be used with UTF-8 chars.
function root._execute_keybinding(modifiers, key)
    for _, mod in ipairs(modifiers) do
        for real_key, mod_name in pairs(conversion) do
            if mod == mod_name then
                root.fake_input("key_press", real_key)
                break
            end
        end
    end

    root.fake_input("key_press"  , key)
    root.fake_input("key_release", key)

    for _, mod in ipairs(modifiers) do
        for real_key, mod_name in pairs(conversion) do
            if mod == mod_name then
                root.fake_input("key_release", real_key)
                break
            end
        end
    end
end

-- Send artificial key events to write a string.
-- It only works in the shims and should not be used with UTF-8 strings.
function root._write_string(string, c)
    local old_c = client.focus

    if c then
        client.focus = c
    end

    for i=1, #string do
        local char = string:sub(i,i)
        root.fake_input("key_press"  , char)
        root.fake_input("key_release", char)
    end

    if c then
        client.focus = old_c
    end
end

function root._wallpaper(pattern)
    if not pattern then return root._wallpaper_surface end

    -- Make a copy because `:finish()` is called by `root.wallpaper` to avoid
    -- a memory leak in the "real" backend.
    local target = cairo.ImageSurface(cairo.Format.RGB32, root.size())
    local cr     = cairo.Context(target)

    cr:set_source(pattern)
    cr:rectangle(0, 0, root.size())
    cr:fill()

    root._wallpaper_pattern = cairo.Pattern.create_for_surface(target)
    root._wallpaper_surface = target

    return target
end


function root.set_newindex_miss_handler(h)
    rawset(root, "_ni_handler", h)
end

function root.set_index_miss_handler(h)
    rawset(root, "_i_handler", h)
end

return setmetatable(root, {
    __index = function(self, key)
        if key == "screen" then
            return screen[1]
        end
        local h = rawget(root,"_i_handler")
        if h then
            return h(self, key)
        end
    end,
    __newindex = function(...)
        local h = rawget(root,"_ni_handler")
        if h then
            h(...)
        end
    end,
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
