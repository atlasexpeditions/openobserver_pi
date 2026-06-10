#pragma once

#include <wx/arrstr.h>
#include <wx/string.h>

class ooProjectFieldMapper
{
public:
    static wxString SuggestFieldTypeForColumn(
        const wxString& columnName,
        const wxArrayString& availableFieldTypes);
};
