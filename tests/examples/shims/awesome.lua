local lgi       = require("lgi")
local gears_obj = require("gears.object")

-- Emulate the C API classes. They differ from C API objects as connect_signal
-- doesn't take an object as first argument and they support fallback properties
-- handlers.
local function _shim_fake_class()
    local obj = gears_obj()
    obj.data = {}

    local meta = {
        __index     = function()end,
        __newindex = function()end,
    }

    obj._connect_signal = obj.connect_signal

    function obj.connect_signal(name, func)
        return obj._connect_signal(obj, name, func)
    end

    function obj.set_index_miss_handler(handler)
        meta.__index = handler
    end

    function obj.set_newindex_miss_handler(handler)
        meta.__newindex = handler
    end

    function obj.emit_signal(name, c, ...)
        local conns = obj._signals[name] or {strong={}}
        for func in pairs(conns.strong) do
            func(c, ...)
        end
    end

    return obj, meta
end

local awesome = _shim_fake_class()
awesome._shim_fake_class = _shim_fake_class

-- Avoid c.screen = acreen.focused() to be called, all tests will fail
awesome.startup = true

function awesome.register_xproperty()
end

function awesome.xkb_get_group_names()
    return "pc+us+inet(evdev)"
end

function awesome.xkb_get_layout_group()
    return 0
end

awesome.load_image = lgi.cairo.ImageSurface.create_from_png

function awesome.pixbuf_to_surface(_, path)
    return awesome.load_image(path)
end

-- Always show deprecated messages
awesome.version = "v9999"

-- SVG are composited. Without it we need a root surface
awesome.composite_manager_running = true

return awesome

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
