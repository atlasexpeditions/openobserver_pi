#include "ooTableImport.h"

#include <wx/textfile.h>

bool ooTableImport::ReadCsvHeaders(
    const wxString& filePath,
    ooImportedTable& table,
    wxString& errorMessage)
{
    table.headers.Clear();
    table.rowCount = 0;

    if (filePath.IsEmpty()) {
        errorMessage = "No CSV file selected.";
        return false;
    }

    wxTextFile file(filePath);
    if (!file.Open()) {
        errorMessage = "Unable to open CSV file.";
        return false;
    }

    if (file.GetLineCount() == 0) {
        errorMessage = "CSV file is empty.";
        file.Close();
        return false;
    }

    table.headers = ParseCsvLine(file.GetLine(0));
    table.rowCount = static_cast<int>(file.GetLineCount()) - 1;

    file.Close();

    if (table.headers.IsEmpty()) {
        errorMessage = "CSV file does not contain headers.";
        return false;
    }

    return true;
}

wxArrayString ooTableImport::ParseCsvLine(const wxString& line)
{
    wxArrayString cells;
    wxString current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.Length(); ++i) {
        const wxChar ch = line[i];

        if (ch == '"') {
            if (inQuotes && i + 1 < line.Length() && line[i + 1] == '"') {
                current += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            current.Trim(true);
            current.Trim(false);
            cells.Add(current);
            current.Clear();
        } else {
            current += ch;
        }
    }

    current.Trim(true);
    current.Trim(false);
    cells.Add(current);

    return cells;
}