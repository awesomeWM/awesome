
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
        assert.is_false(gstring.startswith(nil, "anything"))
    end)

    describe("endswith", function()
        assert.is_true(gstring.endswith("something", ""))
        assert.is_true(gstring.endswith("something", "thing"))
        assert.is_false(gstring.endswith("something", "that"))
        assert.is_false(gstring.endswith(nil, "anything"))
    end)

    describe("split", function()
        assert.is_same(gstring.split("", "\n"), {""})
        assert.is_same(gstring.split("\n", "\n"), {"", ""})
        assert.is_same(gstring.split("foo", "\n"), {"foo"})
        assert.is_same(gstring.split("foo\n", "\n"), {"foo", ""})
        assert.is_same(gstring.split("foo\nbar", "\n"), {"foo", "bar"})

        assert.is_same(gstring.split("", "."), {""})
        assert.is_same(gstring.split(".", "."), {"", ""})
        assert.is_same(gstring.split("foo", "."), {"foo"})
        assert.is_same(gstring.split("foo.", "."), {"foo", ""})
        assert.is_same(gstring.split("foo.bar", "."), {"foo", "bar"})
    end)

    describe("psplit", function()
        assert.is_same(gstring.psplit("", ""), {})
        assert.is_same(gstring.psplit(".", ""), {"."})
        assert.is_same(gstring.psplit("foo", ""), {"f", "o", "o"})
        assert.is_same(gstring.psplit("foo.", ""), {"f", "o", "o", "."})
        assert.is_same(gstring.psplit("foo.bar", "%."), {"foo", "bar"})

        assert.is_same(gstring.psplit("", "."), {""})
        assert.is_same(gstring.psplit("a", "."), {"", ""})
        assert.is_same(gstring.psplit("foo", "."), {"", "", "", ""})
        assert.is_same(gstring.psplit("foo.", "%W"), {"foo", ""})
        assert.is_same(gstring.psplit(".foo.2.5.bar.73", "%.%d"), {".foo", "", ".bar", "3"})
    end)
end)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
