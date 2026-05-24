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
    static bool Create(
        ooObservations* observations,
        const wxString& rootDir,
        wxString& createdPackagePath,
        wxString& errorMessage);

    static bool Update(
        ooObservations* observations,
        const wxString& packageDir,
        wxString& errorMessage);

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
        wxString& errorMessage);

    static bool CreateStaticFolders(
        const wxString& packageDir,
        const wxArrayString& folderPaths,
        wxString& errorMessage);

    static bool ExportObservations(
        ooObservations* observations,
        const wxString& packageDir,
        wxString& errorMessage);

    static bool CopyNmeaRecordings(
        ooObservations* observations,
        const wxString& packageDir,
        wxString& errorMessage);

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

    static bool WriteGeneratedFilesWarning(
        const wxString& packageDir,
        wxString& errorMessage);
};

#endif
