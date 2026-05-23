#ifndef VIDEOPROBER_H
#define VIDEOPROBER_H

#include <QString>
#include <QVariantMap>

extern "C" {
#include <libavformat/avformat.h>
}

struct VideoMeta {
    qint64 durationSec = 0;
    int width = 0;
    int height = 0;
    QString durationStr = "-";
    QString resolution = "-";
    QString codec = "-";
    QString frameRate = "-";
    QString fileSize = "-";
};

class VideoProber
{
public:
    static bool probe(const QString &filePath, VideoMeta &out);
    static QString formatSize(qint64 bytes);
    static QString formatTime(qint64 seconds);
};

#endif // VIDEOPROBER_H
