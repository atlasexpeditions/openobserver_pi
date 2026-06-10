#include "ooProjectFieldMapper.h"

wxString ooProjectFieldMapper::SuggestFieldTypeForColumn(const wxString& columnName)
{
    wxString name = columnName.Lower();
    name.Trim(true);
    name.Trim(false);

    if (name.Contains("timestamp")) return "Start Timestamp UTC";
    if (name == "date" || name.Contains("start date")) return "Start Date";
    if (name == "time" || name.Contains("start time")) return "Start Time";
    if (name == "lat" || name.Contains("latitude")) return "Start Latitude";
    if (name == "lon" || name.Contains("longitude")) return "Start Longitude";
    if (name.Contains("yes") || name.Contains("no") || name.Contains("true") || name.Contains("false")) return "Checkbox";
    if (name.Contains("check") || name.Contains("bool")) return "Checkbox";

    return "Text";
}