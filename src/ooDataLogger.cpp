#include "ooDataLogger.h"

#include <wx/defs.h>

ooDataLogger::ooDataLogger()
    : m_loggerProjectIndex(wxNOT_FOUND),
      m_loggerProjectName(wxEmptyString),
      m_intervalSeconds(1800),
      m_captureDurationSeconds(0),
      m_running(false),
      m_nextTriggerTime(),
      m_hasPositionFix(false),
      m_positionFixTime(0),
      m_positionFixLat(0.0),
      m_positionFixLon(0.0),
      m_lastNmeaSentence(wxEmptyString)
{
}

void ooDataLogger::ClearLoggerProject()
{
    m_loggerProjectIndex = wxNOT_FOUND;
    m_loggerProjectName.Clear();
    m_running = false;
    m_nextTriggerTime = wxDateTime();
}

void ooDataLogger::SetLoggerProject(int projectIndex, const wxString& projectName)
{
    if (m_loggerProjectIndex != projectIndex) {
        m_running = false;
        m_nextTriggerTime = wxDateTime();
    }

    m_loggerProjectIndex = projectIndex;
    m_loggerProjectName = projectName;
}

bool ooDataLogger::HasLoggerProject() const
{
    return m_loggerProjectIndex != wxNOT_FOUND && !m_loggerProjectName.IsEmpty();
}

bool ooDataLogger::IsLoggerProject(int projectIndex) const
{
    return HasLoggerProject() && m_loggerProjectIndex == projectIndex;
}

int ooDataLogger::GetLoggerProjectIndex() const
{
    return m_loggerProjectIndex;
}

const wxString& ooDataLogger::GetLoggerProjectName() const
{
    return m_loggerProjectName;
}

void ooDataLogger::SetIntervalSeconds(long seconds)
{
    m_intervalSeconds = seconds < 0 ? 0 : seconds;

    if (m_running) {
        MarkTriggeredNow(wxDateTime::Now());
    }
}

long ooDataLogger::GetIntervalSeconds() const
{
    return m_intervalSeconds;
}

void ooDataLogger::SetCaptureDurationSeconds(long seconds)
{
    m_captureDurationSeconds = seconds < 0 ? 0 : seconds;
}

long ooDataLogger::GetCaptureDurationSeconds() const
{
    return m_captureDurationSeconds;
}

void ooDataLogger::SetPositionFix(time_t fixTime, double lat, double lon)
{
    m_positionFixTime = fixTime;
    m_positionFixLat = lat;
    m_positionFixLon = lon;
    m_hasPositionFix = true;
}

void ooDataLogger::SetNmeaSentence(const wxString& sentence)
{
    m_lastNmeaSentence = sentence;
}

bool ooDataLogger::HasPositionFix() const
{
    return m_hasPositionFix;
}

time_t ooDataLogger::GetPositionFixTime() const
{
    return m_positionFixTime;
}

double ooDataLogger::GetPositionFixLat() const
{
    return m_positionFixLat;
}

double ooDataLogger::GetPositionFixLon() const
{
    return m_positionFixLon;
}

const wxString& ooDataLogger::GetLastNmeaSentence() const
{
    return m_lastNmeaSentence;
}

void ooDataLogger::Start()
{
    if (HasLoggerProject()) {
        m_running = true;
        MarkTriggeredNow(wxDateTime::Now());
    }
}

void ooDataLogger::Stop()
{
    m_running = false;
    m_nextTriggerTime = wxDateTime();
}

void ooDataLogger::Toggle()
{
    if (m_running) {
        Stop();
    } else {
        Start();
    }
}

bool ooDataLogger::IsRunning() const
{
    return m_running;
}

bool ooDataLogger::ShouldTriggerNow(const wxDateTime& now) const
{
    return m_running &&
           HasLoggerProject() &&
           m_nextTriggerTime.IsValid() &&
           now.IsValid() &&
           now >= m_nextTriggerTime;
}

void ooDataLogger::MarkTriggeredNow(const wxDateTime& now)
{
    if (!m_running || !now.IsValid()) {
        m_nextTriggerTime = wxDateTime();
        return;
    }

    const long safeIntervalSeconds = m_intervalSeconds <= 0 ? 1 : m_intervalSeconds;
    m_nextTriggerTime = now + wxTimeSpan::Seconds(safeIntervalSeconds);
}

wxDateTime ooDataLogger::GetNextTriggerTime() const
{
    return m_nextTriggerTime;
}

wxString ooDataLogger::GetStatusText() const
{
    if (!IsRunning() || !HasLoggerProject()) {
        return wxEmptyString;
    }

    return wxString::Format("Logger ON: %s", m_loggerProjectName);
}