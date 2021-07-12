local source_date_epoch = os.getenv("SOURCE_DATE_EPOCH")
if source_date_epoch and source_date_epoch ~= '' then
    local old_osdate = os.date
    os.date = function(format, timestamp) -- luacheck: ignore
        if timestamp then
            return old_osdate(format, timestamp)
        end
        format = "!" .. format
        return old_osdate(format, source_date_epoch)
    end
end
