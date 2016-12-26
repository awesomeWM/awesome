local runner = require("_runner")
local wibox = require("wibox")
local beautiful = require("beautiful")
local cairo = require("lgi").cairo

local w = wibox {
    x = 10,
    y = 10,
    width = 20,
    height = 20,
    visible = true,
    bg = "#ff0000",
}

local callback_called
local function callback(context, cr, width, height, arg)
    assert(type(context) == "table", type(context))
    assert(type(context.dpi) == "number", context.dpi)
    assert(cairo.Context:is_type_of(cr), cr)
    assert(width == 20, width)
    assert(height == 20, height)
    assert(arg == "argument: 42", arg)

    callback_called = true
end

runner.run_steps({
                 -- Set some bg image
                 function()
                     local img = assert(beautiful.titlebar_close_button_normal)
                     w:set_bgimage(img)
                     return true
                 end,

                 -- Do nothing for a while iteration to give the repaint some time to happen
                 function(arg)
                     if arg == 3 then
                         return true
                     end
                 end,

                 -- Set some bg image function
                 function()
                     w:set_bgimage(callback, "argument: 42")
                     return true
                 end,

                 -- Wait for the function to be done
                 function()
                     if callback_called then
                         return true
                     end
                 end,
             })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
