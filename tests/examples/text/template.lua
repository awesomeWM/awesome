local file_path = ...
require("_common_template")(...)

-- Execute the test
loadfile(file_path)()
