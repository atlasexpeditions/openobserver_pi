#pragma once

#include <wx/string.h>

class ooProjectFieldMapper
{
public:
    static wxString SuggestFieldTypeForColumn(const wxString& columnName);
};