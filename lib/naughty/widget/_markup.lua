local beautiful = require("beautiful")

local module = {}

-- Since some escaping needs to be undone, we have to escape the escaped <>.
local pre_escape = {["&lt;"] = "&zzlt;", ["&gt;"] = "&zzgt;"}

local escape_pattern = "[<>&]"
local escape_subs    = { ['<'] = "&lt;", ['>'] = "&gt;", ['&'] = "&amp;" }

-- Also reverse escaping some allowed tags because people actually use them.
local escape_undo = {['&lt;span '] = "<span "}

for _, allowed in ipairs {'b', 'i', 'u', 'span'} do
    escape_undo['&lt;' ..allowed..'&gt;'] = "<" ..allowed..">"
    escape_undo['&lt;/'..allowed..'&gt;'] = "</"..allowed..">"
end

-- Best effort attempt to allow a subset of pango markup in the text while
-- removing invalid content. If invalid content is present, nothing is
-- displayed.
local function escape_text(text)
    -- Take care of the already escaped content.
    for pattern, subs in pairs(pre_escape) do
        text = text:gsub(pattern, subs)
    end

    -- Try to set the text while only interpreting <br>.
    text = text:gsub("<br[ /]*>", "\n")

    -- Since the title cannot contain markup, it must be escaped first so that
    -- it is not interpreted by Pango later.
    text = text:gsub(escape_pattern, escape_subs)

    -- Restore a subset of markup tags.
    for pattern, subs in pairs(escape_undo) do
        text = text:gsub(pattern, subs)
    end

    -- Restore pre-escaped content.
    for subs, pattern in pairs(pre_escape) do
        text = text:gsub(pattern, subs)
    end

    return text
end

function module.set_markup(wdg, text, fg, font)
    local ret = escape_text(text or "")
    fg = fg or beautiful.notification_fg

    wdg:set_font(font or beautiful.notification_font)

    if fg then
        ret = "<span color='" .. fg .. "'>" .. ret .. "</span>"
    end

    wdg:set_markup_silently(ret)
    return ret
end

return module
