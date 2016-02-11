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
            n="next word",
            [',']="reverse t/T/f/F",
            ['/']=". find",
            ['~']="toggle case",
            ["#"]='prev indent',
            ["$"]='EOL',
            ["%"]='goto match bracket',
            ["^"]='"soft" BOL',
            ["*"]='next indent',
            ["("]='begin sentence',
            [")"]='end sentence',
            ["_"]='"soft" BOL down',
            ["+"]='next line',
            W='next WORD',
            E='end WORD',
            T=". back 'till",
            ['{']="begin parag.",
            ['}']="end parag.",
            F='. "back" find char',
            G='EOF/goto line',
            H='screen top',
            L='screen bottom',
            B='prev WORD',
            N='prev (find)',
            M='screen middle',
            ['?']='. find(rev.)',
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
            g="gg: top of file, gf: open file here",
            z="zt: cursor to top, zb: bottom, zz: center",
            x="delete char",
            v="visual mode",
            m=". set mark",
            ['.']="repeat command",
            ["@"]='. play macro',
            ["&amp;"]='repeat :s',
            Q='ex mode',
            Y='yank line',
            U='undo line',
            P='paste before',
            D='delete to EOL',
            J='join lines',
            K='help',
            [':']='ex cmd line',
            ['"']='. register spec',
            ["|"]='BOL/goto col',
            Z='quit and ZZ:save or ZQ:not',
            X='back-delete',
            V='visual lines',
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
