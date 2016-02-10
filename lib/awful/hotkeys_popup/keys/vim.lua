---------------------------------------------------------------------------
--- VIM hotkeys for awful.hotkeys_widget
--
-- @author Yauheni Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2014-2015 Yauheni Kirylau
-- @release @AWESOME_VERSION@
-- @module awful.hotkeys_popup.keys.vim
---------------------------------------------------------------------------

local hotkeys_popup = require("awful.hotkeys_popup.widget")

local vim_rule_any = {name={"vim", "VIM"}}
for group_name, group_data in pairs({
    vim_motion=                 { color="#009F00", rule_any=vim_rule_any },
    vim_command=                { color="#aFaF00", rule_any=vim_rule_any },
    vim_command_insert= { color="#cF4F40", rule_any=vim_rule_any },
    vim_operator=             { color="#aF6F00", rule_any=vim_rule_any },
}) do
    hotkeys_popup.group_rules[group_name] = group_data
end


local vim_keys = {

    vim_motion={{
        modifiers = {},
        keys = {
            ['`']="goto mark",
            ['0']='"hard" BOL',
            ['-']="prev line",
            w="next word",
            e="end word",
            t=". 'till",
            ['[']=". misc",
            [']']=". misc",
            f=". find char",
            [';']="repeat t/T/f/F",
            ["'"]=". goto mk. BOL",
            b="prev word",
            n="next search match",
            [',']="reverse t/T/f/F",
            ['/']=". find",
            ['~']="toggle case",
            ["#"]='prev indent',
            ["$"]='EOL',
            ["%"]='goto matching bracket',
            ["^"]='"soft" BOL',
            ["*"]='search word under cursor',
            ["("]='sentence begin',
            [")"]='sentence end',
            ["_"]='"soft" BOL down',
            ["+"]='next line',
            W='next WORD',
            E='end WORD',
            T=". \"back\" 'till",
            ['{']="paragraph begin",
            ['}']="paragraph end",
            F='. "back" find char',
            G='EOF/goto line',
            H='move cursor to screen top',
            M='move cursor to screen middle',
            L='move cursor to screen bottom',
            B='prev WORD',
            N='prev search match',
            ['?']='. "back" find',
        }
    }, {
        modifiers = {"Ctrl"},
        keys = {
            e="scroll line up",
            y="scroll line down",
            u="half page up",
            d="half page down",
            b="page up",
            f="page down",
            o="prev mark",
        }
    }},

    vim_operator={{
        modifiers = {},
        keys = {
            ['=']="auto format",
            y="yank",
            d="delete",
            c="change",
            ["!"]='external filter',
            ['&lt;']='unindent',
            ['&gt;']='indent',
        }
    }},

    vim_command={{
        modifiers = {},
        keys = {
            q=". record macro",
            r=". replace char",
            u="undo",
            p="paste after",
            gg="go to the top of file",
            gf="open file under cursor",
            zt="scroll cursor to the top",
            zz="scroll cursor to the center",
            zb="scroll cursor to the bottom",
            x="delete char",
            v="visual mode",
            m=". set mark",
            ['.']="repeat command",
            ["@"]='. play macro',
            ["&amp;"]='repeat :s',
            Q='ex mode',
            Y='yank line',
            U='undo line',
            P='paste before cursor',
            D='delete to EOL',
            J='join lines',
            K='help',
            [':']='ex cmd line',
            ['"']='. register spec',
            ["|"]='BOL/goto col',
            ZZ='quit and save',
            ZQ='quit discarding changes',
            X='back-delete',
            V='visual lines selection',
        }
    }, {
        modifiers = {"Ctrl"},
        keys = {
            w=". window operations",
            r="redo",
            ["["]="normal mode",
            a="increase number",
            x="decrease number",
            g="file/cursor info",
            z="suspend",
            c="cancel/normal mode",
            v="visual block selection",
        }
    }},

    vim_command_insert={{
        modifiers = {},
        keys = {
            i="insert mode",
            o="open below",
            a="append",
            s="subst char",
            R='replace mode',
            I='insert at BOL',
            O='open above',
            A='append at EOL',
            S='subst line',
            C='change to EOL',
        }
    }},
}

hotkeys_popup.add_hotkeys(vim_keys)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
