---------------------------------------------------------------------------
-- @author Aleksey Fedotov &lt;lexa@cfotr.com&gt;
-- @copyright 2015 Aleksey Fedotov
-- @release @AWESOME_VERSION@
-- @classmod wibox.widget.keyboardlayout
---------------------------------------------------------------------------

local capi = {awesome = awesome}
local setmetatable = setmetatable
local os = os
local textbox = require("wibox.widget.textbox")
local button = require("awful.button")
local util = require("awful.util")
local widget_base = require("wibox.widget.base")

--- Keyboard Layout widget.
-- awful.widget.keyboardlayout
local keyboardlayout = { mt = {} }

-- Callback for updaing current layout
local function update_status (keyboardlayout)
   keyboardlayout.current = awesome.xkb_get_layout_group();
   local text = ""
   if (#keyboardlayout.layout > 0) then
      text = (" " .. keyboardlayout.layout[keyboardlayout.current] .. " ")
   end
   keyboardlayout.widget:set_text(text)
end

-- Callback for updating list of layouts
local function update_layout(keyboardlayout)
   keyboardlayout.layout = {};
   local group_names = awesome.xkb_get_group_names();

-- A typical layout string looks like "pc+us+ru:2+de:3+ba:4+inet",
-- and we want to get only three matches: "us", "ru:2", "de:3" "ba:4".
-- Please note that numbers of groups reported by xkb_get_group_names
-- is greater by one than the real group number.
   local first_group = string.match(group_names, "+(%a+)");
   if (not first_group) then
      error ("Failed to get list of keyboard groups");
      return;
   end
   keyboardlayout.layout[0] = first_group;

   for name, number_str in string.gmatch(group_names, "+(%a+):(%d)") do
      group = tonumber(number_str);
      keyboardlayout.layout[group - 1] = name;
   end
   update_status(keyboardlayout)
end

--- Create a keyboard layout widget. It shows current keyboard layout name in a textbox.
-- @return A keyboard layout widget.
function keyboardlayout.new()
   local widget = textbox()
   local keyboardlayout = widget_base.make_widget(widget)

   keyboardlayout.widget = widget

   update_layout(keyboardlayout);

   keyboardlayout.next_layout = function()
      new_layout = (keyboardlayout.current + 1) % (#keyboardlayout.layout + 1)
      keyboardlayout.set_layout(new_layout)
   end

   keyboardlayout.set_layout = function(group_number)
      if (0 > group_number) or (group_number > #keyboardlayout.layout) then
         error("Invalid group number: " .. group_number ..
                  "expected number from 0 to " .. #keyboardlayout.layout)
         return;
      end
      awesome.xkb_set_layout_group(group_number);
   end

   -- callback for processing layout changes
   capi.awesome.connect_signal("xkb::map_changed",
                               function () update_layout(keyboardlayout) end)
   capi.awesome.connect_signal("xkb::group_changed",
                               function () update_status(keyboardlayout) end);

   -- Mouse bindings
   keyboardlayout:buttons(
      util.table.join(button({ }, 1, keyboardlayout.next_layout))
   )

   return keyboardlayout
end

local _instance = nil;

function keyboardlayout.mt:__call(...)
   if _instance == nil then
      _instance = keyboardlayout.new(...)
   end
   return _instance
end

return setmetatable(keyboardlayout, keyboardlayout.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
