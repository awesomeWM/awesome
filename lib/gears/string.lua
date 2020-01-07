---------------------------------------------------------------------------
--- String module for gears.
--
-- @utillib gears.string
---------------------------------------------------------------------------

local gstring = {}

local xml_entity_names = { ["'"] = "&apos;", ["\""] = "&quot;", ["<"] = "&lt;", [">"] = "&gt;", ["&"] = "&amp;" };

--- Escape a string from XML char.
-- Useful to set raw text in textbox.
-- @param text Text to escape.
-- @treturn string Escaped text.
-- @staticfct gears.string.xml_escape
function gstring.xml_escape(text)
    return text and text:gsub("['&<>\"]", xml_entity_names) or nil
end

local xml_entity_chars = { lt = "<", gt = ">", nbsp = " ", quot = "\"", apos = "'", ndash = "-", mdash = "-",
                           amp = "&" };

--- Unescape a string from entities.
-- @param text Text to unescape.
-- @treturn string Unescaped text.
-- @staticfct gears.string.xml_unescape
function gstring.xml_unescape(text)
    return text and text:gsub("&(%a+);", xml_entity_chars) or nil
end

--- Count number of lines in a string.
-- @DOC_text_gears_string_linecount_EXAMPLE@
-- @tparam string text Input string.
-- @treturn int Number of lines.
-- @staticfct gears.string.linecount
function gstring.linecount(text)
    return select(2, text:gsub('\n', '\n')) + 1
end

--- Split a string into multiple lines.
-- @DOC_text_gears_string_linewrap_EXAMPLE@
-- @tparam string text String to wrap.
-- @tparam number width Maximum length of each line. Default: 72.
-- @tparam number indent Number of spaces added before each wrapped line. Default: 0.
-- @treturn string The string with lines wrapped to width.
-- @staticfct gears.string.linewrap
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
-- @DOC_text_gears_string_quote_pattern_EXAMPLE@
-- @tparam string s String to generate pattern for
-- @treturn string string with escaped characters
-- @staticfct gears.string.quote_pattern
function gstring.quote_pattern(s)
    -- All special characters escaped in a string: %%, %^, %$, ...
    local patternchars = '['..("%^$().[]*+-?"):gsub("(.)", "%%%1")..']'
    return string.gsub(s, patternchars, "%%%1")
end

--- Generate a pattern matching expression that ignores case.
-- @tparam string q Original pattern matching expression.
-- @staticfct gears.string.query_to_pattern
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
-- @tparam string str String to be splitted
-- @tparam string delimiter Character where the string will be splitted
-- @treturn table list of the substrings
-- @staticfct gears.string.split
function gstring.split(str, delimiter)
    delimiter = delimiter or "\n"
    local result = {}
    if gstring.startswith(str, delimiter) then
        result[#result+1] = ""
    end
    local pattern = string.format("([^%s]+)", delimiter)
    str:gsub(pattern, function(c) result[#result+1] = c end)
    if gstring.endswith(str, delimiter) then
        result[#result+1] = ""
    end
    if #result == 0 then
        result[#result+1] = str
    end
    return result
end

--- Check if a string starts with another string.
-- @DOC_text_gears_string_startswith_EXAMPLE@
-- @tparam string str String to search
-- @treturn boolean `true` if string starts with specified string
-- @tparam string sub String to check for.
-- @staticfct gears.string.startswith
function gstring.startswith(str, sub)
    return string.sub(str, 1, string.len(sub)) == sub
end

--- Check if a string ends with another string.
-- @DOC_text_gears_string_endswith_EXAMPLE@
-- @tparam string str String to search
-- @tparam string sub String to check for.
-- @treturn boolean `true` if string ends with specified string
-- @staticfct gears.string.endswith
function gstring.endswith(str, sub)
    return sub == "" or string.sub(str,-string.len(sub)) == sub
end

return gstring
