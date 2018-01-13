---------------------------------------------------------------------------
--- Widget module for awful
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
-- @classmod awful.widget
---------------------------------------------------------------------------

return
{
    taglist = require("awful.widget.taglist");
    tasklist = require("awful.widget.tasklist");
    layoutlist = require("awful.widget.layoutlist");
    button = require("awful.widget.button");
    launcher = require("awful.widget.launcher");
    prompt = require("awful.widget.prompt");
    progressbar = require("awful.widget.progressbar");
    graph = require("awful.widget.graph");
    layoutbox = require("awful.widget.layoutbox");
    textclock = require("awful.widget.textclock");
    keyboardlayout = require("awful.widget.keyboardlayout");
    watch = require("awful.widget.watch");
    only_on_screen = require("awful.widget.only_on_screen");
    clienticon = require("awful.widget.clienticon");
    calendar_popup = require("awful.widget.calendar_popup");
    common = require("awful.widget.common");
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
