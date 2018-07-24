--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_NO_USAGE --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local awful     = { prompt = require("awful.prompt") }--DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local naughty   = {} --DOC_HIDE

    local atextbox = wibox.widget.textbox()

    -- Create a shortcut function
    local function echo_test()
        awful.prompt.run {
            prompt       = "<b>Echo: </b>",
            text         = "default command",
            bg_cursor    = "#ff0000",
            -- To use the default `rc.lua` prompt:
            --textbox      = mouse.screen.mypromptbox.widget,
            textbox      = atextbox,
            exe_callback = function(input)
                if not input or #input == 0 then return end
                naughty.notify{ text = "The input was: "..input }
            end
        }
    end

echo_test() --DOC_HIDE

parent:add( wibox.widget {    --DOC_HIDE
    atextbox,  --DOC_HIDE
    bg = beautiful.bg_normal,  --DOC_HIDE
    widget = wibox.container.background  --DOC_HIDE
}) --DOC_HIDE
