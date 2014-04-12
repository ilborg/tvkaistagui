#include <QDebug>
#include "programmefeedparser.h"

ProgrammeFeedParser::ProgrammeFeedParser() : m_dateTimeRegexp("(\\d{1,2}) (\\w{3}) (\\d+) (\\d{2}):(\\d{2}):(\\d{2})"),
    m_timeRegexp("^(\\d{1,2}):(\\d{1,2}):(\\d{1,2})")
{
}

bool ProgrammeFeedParser::parse(QIODevice *device)
{
    m_reader.setDevice(device);
    m_programmes.clear();

    if (!m_reader.readNextStartElement()) {
        m_error = "Invalid programme feed";
        return false;
    }

    if (m_reader.name() != "rss") {
        m_error = "Channel feed does not contain rss element";
        return false;
    }

    while (m_reader.readNextStartElement()) {
        if (m_reader.name() == "channel") {
            parseChannelElement();
        }
        else {
            m_reader.skipCurrentElement();
        }
    }

    return true;
}

QString ProgrammeFeedParser::lastError() const
{
    return m_error;
}

QList<Programme> ProgrammeFeedParser::programmes() const
{
    return m_programmes;
}

QList<Thumbnail> ProgrammeFeedParser::thumbnails() const
{
    return m_thumbnails;
}

void ProgrammeFeedParser::parseChannelElement()
{
    while (m_reader.readNextStartElement()) {
        if (m_reader.name() == "item") {
            parseItemElement();
        }
        else {
            m_reader.skipCurrentElement();
        }
    }
}

void ProgrammeFeedParser::parseItemElement()
{
    Programme programme;

    while (m_reader.readNextStartElement()) {
        if (m_reader.name() == "title") {
            programme.title = m_reader.readElementText();
        }
        else if (m_reader.name() == "description") {
            programme.description = m_reader.readElementText();
        }
        else if (m_reader.qualifiedName() == "link") {
            programme.id = parseProgrammeId(m_reader.readElementText());
        }
        else if (m_reader.name() == "source") {
            programme.channelId = parseChannelId(m_reader.attributes().value("url").toString());
            m_reader.skipCurrentElement();
        }
        else if (m_reader.name() == "pubDate") {
            programme.startDateTime = parseDateTime(m_reader.readElementText());
        }
        else if (m_reader.qualifiedName() == "media:group") {
            parseMediaGroupElement(programme);
        }
        else {
            m_reader.skipCurrentElement();
        }
    }

    m_programmes.append(programme);
}

void ProgrammeFeedParser::parseMediaGroupElement(Programme &programme)
{
    while (m_reader.readNextStartElement()) {
        if (m_reader.qualifiedName() == "media:content") {
            bool ok;
            int duration = m_reader.attributes().value("duration").toString().toInt(&ok);

            if (ok) {
                programme.duration = duration;
            }
        }
        else if (m_reader.qualifiedName() == "media:thumbnail") {
            Thumbnail thumbnail(QUrl(m_reader.attributes().value("url").toString()),
                                parseTime(m_reader.attributes().value("time").toString()));

            if (!thumbnail.time.isNull()) {
                m_thumbnails.append(thumbnail);
            }
        }

        m_reader.skipCurrentElement();
    }
}

int ProgrammeFeedParser::parseProgrammeId(const QString &s)
{
    /* "http://tvkaista.com/search/?findid=8155949" -> 8155949 */
    /* "http://services.tvkaista.com/feedbeta/seasonpasses/852238" -> 852238 */
    int pos = s.lastIndexOf('=');

    if (pos < 0) {
        pos = s.lastIndexOf('/');
    }

    bool ok;
    int programmeId = s.mid(pos + 1).toInt(&ok);

    if (!ok) {
        return -1;
    }

    return programmeId;
}

int ProgrammeFeedParser::parseChannelId(const QString &s)
{
    /* "http://tvkaista.com/feed/channels/1855486/flv.mediarss" -> 1855486 */
    int pos = s.indexOf("channels/");

    if (pos < 0) {
        return -1;
    }

    pos += 9;
    int pos2 = s.indexOf('/', pos);

    if (pos2 < 0) {
        return -1;
    }

    bool ok;
    int channelId = s.mid(pos, pos2 - pos).toInt(&ok);

    if (!ok) {
        return -1;
    }

    return channelId;
}

QDateTime ProgrammeFeedParser::parseDateTime(const QString &s)
{
    if (m_dateTimeRegexp.indexIn(s) >= 0) {
        QString monthName = m_dateTimeRegexp.cap(2);
        int month;

        if (monthName == "Jan") month = 1;
        else if (monthName == "Feb") month = 2;
        else if (monthName == "Mar") month = 3;
        else if (monthName == "Apr") month = 4;
        else if (monthName == "May") month = 5;
        else if (monthName == "Jun") month = 6;
        else if (monthName == "Jul") month = 7;
        else if (monthName == "Aug") month = 8;
        else if (monthName == "Sep") month = 9;
        else if (monthName == "Oct") month = 10;
        else if (monthName == "Nov") month = 11;
        else if (monthName == "Dec") month = 12;
        else return QDateTime();

        int day = m_dateTimeRegexp.cap(1).toInt();
        int year = m_dateTimeRegexp.cap(3).toInt();
        int hour = m_dateTimeRegexp.cap(4).toInt();
        int min = m_dateTimeRegexp.cap(5).toInt();
        int sec = m_dateTimeRegexp.cap(6).toInt();
        return QDateTime(QDate(year, month, day), QTime(hour, min, sec), Qt::UTC).toLocalTime();
    }

    return QDateTime();
}

QTime ProgrammeFeedParser::parseTime(const QString &s)
{
    if (m_timeRegexp.indexIn(s) >= 0) {
        int hour = m_timeRegexp.cap(1).toInt();
        int min = m_timeRegexp.cap(2).toInt();
        int sec = m_timeRegexp.cap(3).toInt();
        return QTime(hour, min, sec);
    }

    return QTime();
}
