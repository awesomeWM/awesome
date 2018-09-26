---------------------------------------------------------------------------
--- String module for gears
--
-- @module gears.string
---------------------------------------------------------------------------

local gstring = {}

local xml_entity_names = { ["'"] = "&apos;", ["\""] = "&quot;", ["<"] = "&lt;", [">"] = "&gt;", ["&"] = "&amp;" };
--- Escape a string from XML char.
-- Useful to set raw text in textbox.
-- @class function
-- @name xml_escape
-- @param text Text to escape.
-- @return Escape text.
function gstring.xml_escape(text)
    return text and text:gsub("['&<>\"]", xml_entity_names) or nil
end

local xml_entity_chars = { lt = "<", gt = ">", nbsp = " ", quot = "\"", apos = "'", ndash = "-", mdash = "-",
						   amp = "&" };
--- Unescape a string from entities.
-- @class function
-- @name xml_unescape
-- @param text Text to unescape.
-- @return Unescaped text.
function gstring.xml_unescape(text)
    return text and text:gsub("&(%a+);", xml_entity_chars) or nil
end

--- Count number of lines in a string
-- @class function
-- @name linecount
-- @tparam string text Input string.
-- @treturn int Number of lines.
function gstring.linecount(text)
    return select(2, text:gsub('\n', '\n')) + 1
end

--- Split a string into multiple lines
-- @class function
-- @name linewrap
-- @param text String to wrap.
-- @param width Maximum length of each line. Default: 72.
-- @param indent Number of spaces added before each wrapped line. Default: 0.
-- @return The string with lines wrapped to width.
function gstring.linewrap(text, width, indent)
    text = text or ""
    width = width or 72
    indent = indent or 0

    local pos = 1
    return text:gsub("(%s+)()(%S+)()",
        function(_, st, word, fi)
            if fi - pos > width then
                pos = st
                return "\n" .. string.rep(" ", indent) .. word
            end
        end)
end

--- Escape all special pattern-matching characters so that lua interprets them
-- literally instead of as a character class.
-- Source: http://stackoverflow.com/a/20778724/15690
-- @class function
-- @name quote_pattern
function gstring.quote_pattern(s)
    -- All special characters escaped in a string: %%, %^, %$, ...
    local patternchars = '['..("%^$().[]*+-?"):gsub("(.)", "%%%1")..']'
    return string.gsub(s, patternchars, "%%%1")
end

--- Generate a pattern matching expression that ignores case.
-- @param s Original pattern matching expression.
-- @class function
-- @name query_to_pattern
function gstring.query_to_pattern(q)
    local s = gstring.quote_pattern(q)
    -- Poor man's case-insensitive character matching.
    s = string.gsub(s, "%a",
                    function (c)
                        return string.format("[%s%s]", string.lower(c),
                                             string.upper(c))
                    end)
    return s
end

--- Split separates a string containing a delimiter into the list of
-- substrings between that delimiter.
-- @class function
-- @name split
-- @tparam string str String to be splitted
-- @tparam string delimiter Character where the string will be splitted
-- @treturn table list of the substrings
function gstring.split(str, delimiter)
    local pattern = "(.-)" .. delimiter .. "()"
    local result = {}
    local n = 0
    local lastPos = 0
    for part, pos in string.gmatch(str, pattern) do
        n = n + 1
        result[n] = part
        lastPos = pos
    end
    result[n + 1] = string.sub(str, lastPos)
    return result
end

--- Check if a string starts with another string
-- @class function
-- @name startswith
-- @tparam string str String to search
-- @tparam string sub String to check for
function gstring.startswith(str, sub)
	return string.sub(str, 1, string.len(sub)) == sub
end

--- Check if a string ends with another string
-- @class function
-- @name endswith
-- @tparam string str String to search
-- @tparam string sub String to check for
function gstring.endswith(str, sub)
	return sub == "" or string.sub(str,-string.len(sub)) == sub
end

return gstring
