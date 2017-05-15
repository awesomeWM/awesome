local lgi       = require("lgi")
local GdkPixbuf = lgi.GdkPixbuf
local Gdk       = lgi.Gdk
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

local init, surfaces = false, {}

function awesome.load_image(file)
    if not init then
        Gdk.init{}
        init = true
    end

    local _, width, height = GdkPixbuf.Pixbuf.get_file_info(file)

    local pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_size(file, width, height)

    if not pixbuf then
        return nil, "Could not load "..file
    end

    local s = Gdk.cairo_surface_create_from_pixbuf( pixbuf, 1, nil )

    table.insert(surfaces, s)

    return s._native, not s and "Could not load surface from "..file or nil, s
end

-- Always show deprecated messages
awesome.version = "v9999"

return awesome

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
