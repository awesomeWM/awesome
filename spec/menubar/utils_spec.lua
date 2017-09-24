---------------------------------------------------------------------------
-- @author Zach Peltzer
-- @copyright 2017 Zach Peltzer
---------------------------------------------------------------------------

local utils = require("menubar.utils")

describe("menubar.utils unescape", function()
    local single_strings = {
        [ [[\n:\r:\s:\t:\\]] ] = "\n:\r: :\t:\\",
        -- Make sure escapes are not read recursively
        [ [[\\s]] ] = [[\s]],
        -- Make sure ';' is not escaped for non-list strings and other
        -- characters are not escaped
        [ [[ab\c\;1\23]] ] = [[ab\c\;1\23]],
    }

    for escaped, unescaped in pairs(single_strings) do
        it(escaped, function()
            assert.is.equal(unescaped, utils.unescape(escaped, false))
        end)
    end

    local list_strings = {
        -- Normal list
        [ [[abc;123;xyz]] ] = { "abc", "123", "xyz" },
        -- Optional terminating semicolon
        [ [[abc;123;xyz;]] ] = { "abc", "123", "xyz" },
        -- Blank item
        [ [[abc;;123]] ] = { "abc", "", "123" },
        -- Escape semicolon
        [ [[abc\;;12\;3;\;xyz]] ] = { "abc;", "12;3", ";xyz" },
        -- Normal escapes are parsed like normal
        [ [[ab\c;1\s23;x\\yz]] ] = { "ab\\c", "1 23", "x\\yz" },
        -- Escaped backslashes before semicolon
        [ [[abc\\\;;12\\;3;xyz]] ] = { "abc\\;", "12\\", "3", "xyz" },
    }

    for escaped, unescaped in pairs(list_strings) do
        it(escaped, function()
            local returned = utils.unescape(escaped, true)
            assert.is.equal(#unescaped, #returned)

            for i = 1, #unescaped do
                assert.is.equal(unescaped[i], returned[i])
            end
        end)
    end
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
