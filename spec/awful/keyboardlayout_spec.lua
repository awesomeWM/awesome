---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter and Kazunobu Kuriyama
---------------------------------------------------------------------------

local kb = require("awful.widget.keyboardlayout")

describe("awful.widget.keyboardlayout get_groups_from_group_names", function()
    it("nil", function()
        assert.is_nil(kb.get_groups_from_group_names(nil))
    end)

    local tests = {
        -- possible worst cases
        [""] = {
        },
        ["pc+dvorak_alt2+inet(evdev)"] = {
        },
        ["empty"] = {
        },
        ["empty(basic)"] = {
        },
        -- contrived cases for robustness test
        ["pc()+de+jp+group()"] = {
            { file = "de", group_idx = 1 },
            { file = "jp", group_idx = 1 }
        },
        ["us(altgr-intl)"] = {
            { file = "us", group_idx = 1, section = "altgr-intl" }
        },
        -- possible eight variations of a single term
        ["de"] = {
            { file = "de", group_idx = 1 }
        },
        ["de:2" ] = {
            { file = "de", group_idx = 2 }
        },
        ["de(nodeadkeys)"] = {
            { file = "de", group_idx = 1, section = "nodeadkeys" }
        },
        ["de(nodeadkeys):2"] = {
            { file = "de", group_idx = 2, section = "nodeadkeys" }
        },
        ["macintosh_vndr/de"] = {
            { file = "de", group_idx = 1, vendor = "macintosh_vndr" }
        },
        ["macintosh_vndr/de:2"] = {
            { file = "de", group_idx = 2, vendor = "macintosh_vndr" }
        },
        ["macintosh_vndr/de(nodeadkeys)"] = {
            { file = "de", group_idx = 1, vendor = "macintosh_vndr", section = "nodeadkeys" }
        },
        ["macintosh_vndr/de(nodeadkeys):2"] = {
            { file = "de", group_idx = 2, vendor = "macintosh_vndr", section = "nodeadkeys" }
        },
        -- multiple terms
        ["pc+de"] = {
            { file = "de", group_idx = 1 }
        },
        ["pc+us+inet(evdev)+terminate(ctrl_alt_bksp)"] = {
            { file = "us", group_idx = 1 }
        },
        ["pc(pc105)+us+group(caps_toggle)+group(ctrl_ac)"] = {
            { file = "us", group_idx = 1 }
        },
        ["pc+us(intl)+inet(evdev)+group(win_switch)"] = {
            { file = "us", group_idx = 1, section = "intl" }
        },

        ["macintosh_vndr/apple(alukbd)+macintosh_vndr/jp(usmac)"] = {
            { file = "jp", group_idx = 1, vendor = "macintosh_vndr", section = "usmac" },
        },
        -- multiple layouts
        ["pc+jp+us:2+inet(evdev)+capslock(hyper)"] = {
            { file = "jp", group_idx = 1 },
            { file = "us", group_idx = 2 }
        },
        ["pc+us+ru:2+de:3+ba:4+inet"] = {
            { file = "us", group_idx = 1 },
            { file = "ru", group_idx = 2 },
            { file = "de", group_idx = 3 },
            { file = "ba", group_idx = 4 },
        },
        ["macintosh_vndr/apple(alukbd)+macintosh_vndr/jp(usmac)+macintosh_vndr/jp(mac):2+group(shifts_toggle)"] = {
            { file = "jp", group_idx = 1, vendor = "macintosh_vndr", section = "usmac" },
            { file = "jp", group_idx = 2, vendor = "macintosh_vndr", section = "mac" },
        },
    }

    for arg, expected in pairs(tests) do
        it(arg, function()
            assert.is.same(expected, kb.get_groups_from_group_names(arg))
        end)
    end
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
