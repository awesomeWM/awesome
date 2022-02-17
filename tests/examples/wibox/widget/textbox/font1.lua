--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE --DOC_GEN_OUTPUT
local l = ...
local wibox = require("wibox")
local beautiful = require("beautiful")
--DOC_HIDE_END
   local pango = require("lgi").Pango
--DOC_HIDE_START

local ret = wibox.layout.fixed.vertical()

--DOC_HIDE_END
   local fonts = {
       "sans",
       "Roboto, Bold",
       "DejaVu Sans, Oblique",
       "Noto Mono, Regular"
   }

   --DOC_NEWLINE

   for _, font in ipairs(fonts) do
       local w = wibox.widget {
           font   = font,
           text   = "The quick brown fox jumps over the lazy dog!",
           widget = wibox.widget.textbox,
       }

       --DOC_NEWLINE
       -- Use the low level Pango API to validate the font was parsed properly.
       local desc = pango.FontDescription.from_string(w.font)
       print(
           string.format(
               "%s %d %s %s %s",
               w.font,
               desc:get_size(),
               desc:get_family(),
               desc:get_variant(),
               desc:get_style()
            )
       )

    --DOC_HIDE_START

    assert(w._private.font == font)

    ret:add(
        wibox.widget {
            markup = "Font: <b>" .. font .."</b>",
            widget = wibox.widget.textbox
        },
        wibox.widget{
            w,
            margins =  1,
            color = beautiful.bg_normal,
            forced_width = 500,
            widget = wibox.container.margin,
        },
        {
            forced_height = 10,
            widget = wibox.widget.base.make_widget()
        }
    )
    --DOC_HIDE_END
   end

--DOC_HIDE_START

l:add(ret)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
