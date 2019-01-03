
local gstring = require("gears.string")

describe("gears.string", function()
    describe("quote_pattern", function()
        it("text", function()
            assert.is.equal(gstring.quote_pattern("text"), "text")
        end)

        it("do.t", function()
            assert.is.equal(gstring.quote_pattern("do.t"), "do%.t")
        end)

        it("per%cen[tage", function()
            assert.is.equal(gstring.quote_pattern("per%cen[tage"), "per%%cen%[tage")
        end)
    end)

    describe("query_to_pattern", function()
        it("DownLow", function()
            assert.is.equal(string.match("DownLow", gstring.query_to_pattern("downlow")), "DownLow")
        end)

        it("%word", function()
            assert.is.equal(string.match("%word", gstring.query_to_pattern("%word")), "%word")
        end)

        it("Substring of DownLow", function()
            assert.is.equal(string.match("DownLow", gstring.query_to_pattern("ownl")), "ownL")
        end)
    end)

    describe("startswith", function()
        assert.is_true(gstring.startswith("something", ""))
        assert.is_true(gstring.startswith("something", "some"))
        assert.is_false(gstring.startswith("something", "none"))
    end)

    describe("endswith", function()
        assert.is_true(gstring.endswith("something", ""))
        assert.is_true(gstring.endswith("something", "thing"))
        assert.is_false(gstring.endswith("something", "that"))
    end)

    describe("split", function()
        assert.is_same(gstring.split("", "\n"), {""})
        assert.is_same(gstring.split("\n", "\n"), {"", ""})
        assert.is_same(gstring.split("foo", "\n"), {"foo"})
        assert.is_same(gstring.split("foo\n", "\n"), {"foo", ""})
        assert.is_same(gstring.split("foo\nbar", "\n"), {"foo", "bar"})
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
