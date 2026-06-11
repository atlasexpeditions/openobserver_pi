#pragma once

#include <wx/arrstr.h>
#include <wx/string.h>

class ooObservations;

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

    static bool ImportCsvRowsByHeaderMapping(
        const wxString& filePath,
        ooObservations* observations,
        const wxArrayString& availableFieldTypes,
        int& rowsImported,
        wxString& errorMessage);

private:
    static wxArrayString ParseCsvLine(const wxString& line);
};
