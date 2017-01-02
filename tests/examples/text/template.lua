local file_path = ...
require("_common_template")(...)

-- Execute the test
loadfile(file_path)()

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
