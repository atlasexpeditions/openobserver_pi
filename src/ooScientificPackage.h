/**************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Open Observer Scientific Package Export
 *
 **************************************************************************/

#ifndef _OO_SCIENTIFIC_PACKAGE_H_
#define _OO_SCIENTIFIC_PACKAGE_H_

#include <wx/string.h>
#include <wx/arrstr.h>

class ooObservations;

class ooScientificPackage
{
public:
    struct RunSummary
    {
        RunSummary()
            : foldersTouched(0),
              nmeaRecordingsCopied(0),
              exportFilesRefreshed(0)
        {
        }

        int foldersTouched;
        int nmeaRecordingsCopied;
        int exportFilesRefreshed;
        wxString logPath;
        wxArrayString logLines;
    };
    static bool Create(
        ooObservations* observations,
        const wxString& rootDir,
        wxString& createdPackagePath,
        wxString& errorMessage,
        const wxArrayString& dailyFolders,
        const wxArrayString& workingFolders,
        const wxArrayString& rawDataFolders,
        RunSummary& runSummary);

    static bool Update(
        ooObservations* observations,
        const wxString& packageDir,
        wxString& errorMessage,
        const wxArrayString& dailyFolders,
        const wxArrayString& workingFolders,
        const wxArrayString& rawDataFolders,
        RunSummary& runSummary);

private:
    static wxString SanitizeFileName(const wxString& value);
    static wxString GetCurrentUtcDateString();
    static wxString GetCurrentUtcTimestampString();

    static wxString GetProjectName(ooObservations* observations);
    static wxArrayString GetObservationDates(ooObservations* observations);
    static wxArrayString GetObservationIdsOrFallbacks(ooObservations* observations);

    static wxArrayString GetDefaultDailyFolders();
    static wxArrayString GetDefaultWorkingFolders();
    static wxArrayString GetDefaultRawDataFolders();
    static wxArrayString ReadDailyFoldersFromTemplate(const wxString& templatePath);
    static wxArrayString ReadWorkingFoldersFromTemplate(const wxString& templatePath);
    static wxArrayString ReadRawDataFoldersFromTemplate(const wxString& templatePath);
    static wxString GetDefaultTemplatePath();
    static bool CopyTemplateUsed(const wxString& packageDir, wxString& errorMessage);

    static bool EnsureDirectory(const wxString& path, wxString& errorMessage);
    static bool IsScientificPackageFolder(ooObservations* observations,const wxString& packageDir);
    
    static bool CreateDailyFolders(
        const wxString& packageDir,
        const wxArrayString& dates,
        const wxArrayString& dailyFolders,
        wxString& errorMessage,
        RunSummary& runSummary);

    static bool CreateStaticFolders(
        const wxString& packageDir,
        const wxArrayString& folderPaths,
        wxString& errorMessage,
        RunSummary& runSummary);

    static bool ExportObservations(
        ooObservations* observations,
        const wxString& packageDir,
        wxString& errorMessage,
        RunSummary& runSummary);

    static bool CopyNmeaRecordings(
        ooObservations* observations,
        const wxString& packageDir,
        wxString& errorMessage,
        RunSummary& runSummary);

    static bool WriteReadme(
        ooObservations* observations,
        const wxString& packageDir,
        wxString& errorMessage);

    static bool WriteObservationIds(
        ooObservations* observations,
        const wxString& packageDir,
        wxString& errorMessage);

    static bool WritePackageMetadata(
        ooObservations* observations,
        const wxString& packageDir,
        wxString& errorMessage);

    static bool WriteDataPackageSettings(
        const wxString& packageDir,
        const wxArrayString& dailyFolders,
        const wxArrayString& workingFolders,
        const wxArrayString& rawDataFolders,
        wxString& errorMessage);

    static bool WriteRunLog(
        const wxString& packageDir,
        const wxString& actionLabel,
        RunSummary& runSummary,
        wxString& errorMessage);        

    static bool WriteGeneratedFilesWarning(
        const wxString& packageDir,
        wxString& errorMessage);
};

#endif
