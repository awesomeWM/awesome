---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
---------------------------------------------------------------------------

local util = require("awful.util")

describe("awful.util", function()
    describe("quote_pattern", function()
        it("text", function()
            assert.is.equal(util.quote_pattern("text"), "text")
        end)

        it("do.t", function()
            assert.is.equal(util.quote_pattern("do.t"), "do%.t")
        end)

        it("per%cen[tage", function()
            assert.is.equal(util.quote_pattern("per%cen[tage"), "per%%cen%[tage")
        end)
    end)

    describe("query_to_pattern", function()
        it("DownLow", function()
            assert.is.equal(string.match("DownLow", util.query_to_pattern("downlow")), "DownLow")
        end)

        it("%word", function()
            assert.is.equal(string.match("%word", util.query_to_pattern("%word")), "%word")
        end)

        it("Substring of DownLow", function()
            assert.is.equal(string.match("DownLow", util.query_to_pattern("ownl")), "ownL")
        end)
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
