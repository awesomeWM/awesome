--DOC_HIDE --DOC_NO_USAGE
local gears = require("gears") --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

-- luacheck: globals my_textbox get_children_by_id --DOC_HIDE

   local w = wibox.widget {
       -- Get the current cliently focused name.
       text = gears.connection {
           source_class = client,
           signals      = {"focused", "property::name"},
           initiate     = false,
           callback     = function(source, target, sig_arg1, ...) --luacheck: no unused args
               -- Since the class signal first arg is the source, this works!
               assert(source == sig_arg1)

               --DOC_NEWLINE

               -- All widgets with IDs are visible from this callback!
               assert(target and my_textbox) --DOC_HIDE
               assert(target == my_textbox)

               --DOC_NEWLINE

               -- get_children_by_id can also be used!
               assert(get_children_by_id("my_textbox")[1] == target)

               --DOC_NEWLINE

               if not source then return "Nothing!" end

               --DOC_NEWLINE

               return "Name: " .. source.name .. "!"
           end
       },
       forced_width  = 100, --DOC_HIDE
       forced_height = 20, --DOC_HIDE
       id = "my_textbox",
       widget = wibox.widget.textbox
   }


--DOC_HIDE 1: delayed connection, 2: delayed layout, 3: delayed draw
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
assert(w.text == "Nothing!") --DOC_HIDE

