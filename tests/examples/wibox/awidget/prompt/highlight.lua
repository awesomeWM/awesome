--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_NO_USAGE --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local awful     = { prompt = require("awful.prompt") }--DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE

    local amp = "&amp"..string.char(0x3B)
    local quot = "&quot"..string.char(0x3B)

    local atextbox = wibox.widget.textbox()

    -- Create a shortcut function
    local function echo_test()
        awful.prompt.run {
            prompt       = "<b>Echo: </b>",
            text         = 'a_very "nice" $SHELL && command', --DOC_HIDE
            bg_cursor    = "#ff0000",
            -- To use the default `rc.lua` prompt:
            --textbox      = mouse.screen.mypromptbox.widget,
            textbox      = atextbox,
            highlighter  = function(b, a)
                -- Add a random marker to delimitate the cursor
                local cmd = b.."ZZZCURSORZZZ"..a

                -- Find shell variables
                local sub = "<span foreground='#CFBA5D'>%1</span>"
                cmd = cmd:gsub("($[A-Za-z][a-zA-Z0-9]*)", sub)

                -- Highlight " && "
                sub = "<span foreground='#159040'>%1</span>"
                cmd = cmd:gsub("( "..amp..amp..")", sub)

                -- Highlight double quotes
                local quote_pos = cmd:find("[^\\]"..quot)
                while quote_pos do
                    local old_pos = quote_pos
                    quote_pos = cmd:find("[^\\]"..quot, old_pos+2)

                    if quote_pos then
                        local content = cmd:sub(old_pos+1, quote_pos+6)
                        cmd = table.concat({
                                cmd:sub(1, old_pos),
                                "<span foreground='#2977CF'>",
                                content,
                                "</span>",
                                cmd:sub(quote_pos+7, #cmd)
                        }, "")
                        quote_pos = cmd:find("[^\\]"..quot, old_pos+38)
                    end
                end

                -- Split the string back to the original content
                -- (ignore the recursive and escaped ones)
                local pos = cmd:find("ZZZCURSORZZZ")
                b,a = cmd:sub(1, pos-1), cmd:sub(pos+12, #cmd)
                return b,a
            end,
        }
    end

echo_test() --DOC_HIDE

parent:add( wibox.widget {    --DOC_HIDE
    atextbox,  --DOC_HIDE
    bg = beautiful.bg_normal,  --DOC_HIDE
    widget = wibox.container.background  --DOC_HIDE
}) --DOC_HIDE
