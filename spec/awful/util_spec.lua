---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
---------------------------------------------------------------------------

local util = require("awful.util")
local say = require("say")

describe("awful.util", function()
    it("table.keys_filter", function()
        local t = { "a", 1, function() end, false}
        assert.is.same(util.table.keys_filter(t, "number", "function"), { 2, 3 })
    end)

    it("table.reverse", function()
        local t = { "a", "b", c = "c", "d" }
        assert.is.same(util.table.reverse(t), { "d", "b", "a", c = "c" })
    end)

    describe("table.iterate", function()
        it("no filter", function()
            local t = { "a", "b", c = "c", "d" }
            local f = util.table.iterate(t, function() return true end)
            assert.is.equal(f(), "a")
            assert.is.equal(f(), "b")
            assert.is.equal(f(), "d")
            assert.is.equal(f(), nil)
            assert.is.equal(f(), nil)
        end)

        it("b filter", function()
            local t = { "a", "b", c = "c", "d" }
            local f = util.table.iterate(t, function(i) return i == "b" end)
            assert.is.equal(f(), "b")
            assert.is.equal(f(), nil)
            assert.is.equal(f(), nil)
        end)

        it("with offset", function()
            local t = { "a", "b", c = "c", "d" }
            local f = util.table.iterate(t, function() return true end, 2)
            assert.is.equal(f(), "b")
            assert.is.equal(f(), "d")
            assert.is.equal(f(), "a")
            assert.is.equal(f(), nil)
            assert.is.equal(f(), nil)
        end)
    end)

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
