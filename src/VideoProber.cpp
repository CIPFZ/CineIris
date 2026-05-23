#include "VideoProber.h"
#include <QFileInfo>

bool VideoProber::probe(const QString &filePath, VideoMeta &out)
{
    AVFormatContext *fmtCtx = nullptr;
    int ret = avformat_open_input(&fmtCtx, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) return false;

    ret = avformat_find_stream_info(fmtCtx, nullptr);
    if (ret < 0) {
        avformat_close_input(&fmtCtx);
        return false;
    }

    QFileInfo fi(filePath);
    out.fileSize = formatSize(fi.size());

    if (fmtCtx->duration != AV_NOPTS_VALUE) {
        out.durationSec = fmtCtx->duration / AV_TIME_BASE;
        out.durationStr = formatTime(out.durationSec);
    }

    for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
        AVStream *st = fmtCtx->streams[i];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
            !(st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            AVCodecParameters *p = st->codecpar;
            out.width = p->width;
            out.height = p->height;
            out.resolution = QString("%1 × %2").arg(p->width).arg(p->height);

            const AVCodec *codec = avcodec_find_decoder(p->codec_id);
            out.codec = codec ? QString::fromUtf8(codec->name) : "Unknown";

            if (st->avg_frame_rate.den > 0) {
                double fps = av_q2d(st->avg_frame_rate);
                out.frameRate = QString::number(fps, 'f', 0) + " fps";
            }
            break;
        }
    }

    avformat_close_input(&fmtCtx);
    return true;
}

QString VideoProber::formatSize(qint64 bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024LL * 1024 * 1024)
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

QString VideoProber::formatTime(qint64 seconds)
{
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}
