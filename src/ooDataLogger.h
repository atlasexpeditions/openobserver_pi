#pragma once

#include <ctime>

#include <wx/datetime.h>
#include <wx/string.h>

class ooDataLogger
{
public:
    ooDataLogger();

    void ClearLoggerProject();
    void SetLoggerProject(int projectIndex, const wxString& projectName);

    bool HasLoggerProject() const;
    bool IsLoggerProject(int projectIndex) const;

    int GetLoggerProjectIndex() const;
    const wxString& GetLoggerProjectName() const;

    void SetIntervalSeconds(long seconds);
    long GetIntervalSeconds() const;

    void SetCaptureDurationSeconds(long seconds);
    long GetCaptureDurationSeconds() const;

    void SetPositionFix(time_t fixTime, double lat, double lon);
    void SetNmeaSentence(const wxString& sentence);

    bool HasPositionFix() const;
    time_t GetPositionFixTime() const;
    double GetPositionFixLat() const;
    double GetPositionFixLon() const;
    const wxString& GetLastNmeaSentence() const;

    void Start();
    void Stop();
    void Toggle();

    bool IsRunning() const;
    bool ShouldTriggerNow(const wxDateTime& now) const;
    void MarkTriggeredNow(const wxDateTime& now);

    wxDateTime GetNextTriggerTime() const;
    wxString GetStatusText() const;

private:
    int m_loggerProjectIndex;
    wxString m_loggerProjectName;
    long m_intervalSeconds;
    long m_captureDurationSeconds;
    bool m_running;
    wxDateTime m_nextTriggerTime;

    bool m_hasPositionFix;
    time_t m_positionFixTime;
    double m_positionFixLat;
    double m_positionFixLon;
    wxString m_lastNmeaSentence;
};
