--DOC_HIDE --DOC_GEN_IMAGE --DOC_NO_USAGE
local naughty = require("naughty") --DOC_HIDE

screen[1]._resize {width = 640, height = 480} --DOC_HIDE
require("_date") --DOC_HIDE
require("_default_look") --DOC_HIDE

local function ever_longer_messages(iter) --DOC_HIDE
    local ret = "content! " --DOC_HIDE
    for _=1, iter do --DOC_HIDE
        ret = ret.."more! " --DOC_HIDE
    end --DOC_HIDE
    return ret --DOC_HIDE
end --DOC_HIDE

--DOC_NEWLINE

naughty.connect_signal("request::display", function(n) --DOC_HIDE
    naughty.layout.box {notification = n} --DOC_HIDE
end) --DOC_HIDE

--DOC_NEWLINE

   for _, pos in ipairs {
       "top_left"   , "top_middle"   , "top_right",
       "bottom_left", "bottom_middle", "bottom_right",
   } do
       for i=1, 3 do
           naughty.notification {
               position = pos,
               title    = pos .. " " .. i,
               message  = ever_longer_messages(i)
           }
       end
   end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
