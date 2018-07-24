--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_NO_USAGE --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local awful     = { prompt = require("awful.prompt"),--DOC_HIDE
                    util   = require("awful.util"),--DOC_HIDE
                    screen = require("awful.screen")}--DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local gfs       = require("gears.filesystem") --DOC_HIDE
local naughty   = {} --DOC_HIDE

    local atextbox = wibox.widget.textbox()

    local notif = nil
    awful.prompt.run {
        prompt               = "<b>Run: </b>",
        keypressed_callback  = function(mod, key, cmd) --luacheck: no unused args
            if key == "Shift_L" then
                notif = naughty.notify { text = "Shift pressed" }
            end
        end,
        keyreleased_callback = function(mod, key, cmd) --luacheck: no unused args
            if notif then
                naughty.destroy(notif)
                notif = nil
            end
        end,
        textbox              = atextbox,
        history_path         = gfs.get_cache_dir() .. "/history",
    }

parent:add( wibox.widget {    --DOC_HIDE
    atextbox,  --DOC_HIDE
    bg = beautiful.bg_normal,  --DOC_HIDE
    widget = wibox.container.background  --DOC_HIDE
}) --DOC_HIDE
