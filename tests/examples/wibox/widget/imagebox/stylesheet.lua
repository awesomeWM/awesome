--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

   local image = '<?xml version="1.0" encoding="UTF-8" standalone="no"?>'..
       '<svg width="190" height="60">'..
           '<rect x="10"  y="10" width="50" height="50" />'..
           '<rect x="70"  y="10" width="50" height="50" class="my_class" />'..
           '<rect x="130" y="10" width="50" height="50" id="my_id" />'..
       '</svg>'

   --DOC_NEWLINE

   local stylesheet = "" ..
        "rect { fill: #ffff00; } "..
        ".my_class { fill: #00ff00; } "..
        "#my_id { fill: #0000ff; }"

   --DOC_NEWLINE

   local w = wibox.widget {
       forced_height = 60, --DOC_HIDE
       forced_width  = 190, --DOC_HIDE
       stylesheet = stylesheet,
       image      = image,
       widget     = wibox.widget.imagebox
   }

parent:add(w) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
