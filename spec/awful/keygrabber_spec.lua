local gtable = require "gears.table"

describe("awful.keygrabber", function()
   package.loaded["gears.timer"] = {}
   package.loaded["awful.keyboard"] = {
      append_global_keybinding = function() end,
   }
   _G.awesome = gtable.join(_G.awesome, { connect_signal = function() end })
   _G.key = {
      set_index_miss_handler = function() end,
      set_newindex_miss_handler = function() end,
   }

   local akeygrabber = require "awful.keygrabber"

   local kg = nil
   local fake_key = {
      _is_awful_key = true,
      key = {},
   }

   before_each(function()
      kg = akeygrabber {
         keybindings = {},
         stop_key = "Escape",
         stop_callback = function() end,
         export_keybindings = true,
      }
   end)

   -- issue #3567: add_keybinding fail when called with an `awful.key` instance
   it("awful.keygrabber:add_keybinding() doesn't throw error", function()
      kg:add_keybinding(fake_key)

      -- dummy test that should be trusty if add_keybinding doesn't thow an exception
      -- (if add_keybinding fails, we will not reach this line anyway...)
      assert(kg, "kg is nil")
   end)
end)
