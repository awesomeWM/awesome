--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_NO_USAGE --DOC_HIDE
local awful     = { widget = {  --DOC_HIDE
                    prompt = require("awful.widget.prompt")}}--DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE

-- Fake a theme --DOC_HIDE
beautiful.prompt_fg        = "#F6F2E8" --DOC_HIDE
beautiful.prompt_bg        = "#404040" --DOC_HIDE
beautiful.prompt_fg_cursor = "#000000" --DOC_HIDE
beautiful.prompt_bg_cursor = "#898941" --DOC_HIDE
beautiful.prompt_font      = "DejaVu Sans Mono 8" --DOC_HIDE


    local myprompt = awful.widget.prompt {
        prompt = "Execute: "
    }

    myprompt:run()

parent:add(myprompt) --DOC_HIDE
