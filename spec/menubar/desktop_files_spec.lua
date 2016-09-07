local runner = require("_runner")
local menubar = require("menubar")
local gdebug = require("gears.debug")

local assets_dir = (os.getenv("SOURCE_DIRECTORY") or '.') .. "/spec/menubar/desktop_files"
menubar.menu_gen.all_menu_dirs = { assets_dir }

local menu_gen_list = nil

runner.run_steps({
    function()
        menubar.menu_gen.generate(function(menu_entries)
            menu_gen_list = menu_entries
        end)
        return true
    end,
    function(count)
        if menu_gen_list == nil then
            -- Continue waiting
            if count >= 5 then
                return false
            end
            return
        else
            -- Begin the actual test
            local active_entry = menu_gen_list[1] -- Lua array counting starts at 1, right?
            -- Travis doesn't actually have any icons installed so this is all we can test
            -- this seems suboptimal
            assert(active_entry.icon == nil, active_entry.icon)
            assert(active_entry.cmdline == "ls", active_entry.cmdline)
            -- If we want to make things show up in multi categories this should be a full table
            gdebug.dump(active_entry.category, "Category", 4)
            assert(active_entry.category == "games", active_entry.category)
            assert(active_entry.name == "Example", active_entry.name)
            return true
        end
    end,
})
