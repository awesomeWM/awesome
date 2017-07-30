local menu_gen = require("menubar.menu_gen")
local gdebug = require("gears.debug")
local glib = require("lgi").GLib

local assets_dir = (os.getenv("SOURCE_DIRECTORY") or '.') .. "/spec/menubar/desktop_files"
menu_gen.all_menu_dirs = { assets_dir }


describe("menubar.menu_gen Entry Generation", function()
    local menu_entries
    setup(function()
        local main_loop = glib.MainLoop.new()
        local success = false
        menu_gen.generate(function(menu_table)
            menu_entries = menu_table
            success = true
            main_loop:quit()
        end)
        -- In case something goes very very wrong
        glib.timeout_add(glib.PRIORITY_DEFAULT, 1000, function()
            main_loop:quit()
        end)
        main_loop:run()
        assert.is_true(success)
    end)
    it("With a Standard example", function()
        assert.are.same(
        {
            name = "Example",
            -- Travis doesn't actually have any icons installed so this is all we can test
            -- this seems suboptimal
            icon = nil,
            cmdline = "ls",
            category = "games",
            foobaz = nil
        }, menu_entries[2])
    end)
    it("With No Category", function()
        assert.are.same(
        {
            name = "Signal Private Messenger",
            cmdline = "/usr/bin/chromium --profile-directory=Default --app-id=bikioccmkafdpakkkcpdbppfkghcmihk",
            icon = nil,
            category = "other"
        }, menu_entries[1])
    end)
end)
