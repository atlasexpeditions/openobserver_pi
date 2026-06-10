#pragma once

#include <wx/arrstr.h>
#include <wx/string.h>

struct ooImportedTable
{
    wxArrayString headers;
    int rowCount = 0;
};

class ooTableImport
{
public:
    static bool ReadCsvHeaders(
        const wxString& filePath,
        ooImportedTable& table,
        wxString& errorMessage);

private:
    static wxArrayString ParseCsvLine(const wxString& line);
};