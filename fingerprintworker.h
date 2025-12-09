#ifndef FINGERPRINTWORKER_H
#define FINGERPRINTWORKER_H

#include <QObject>
#include <QImage>
#include <QThread>
#include "taskdata.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class FingerprintWorker : public QObject
{
    Q_OBJECT
public:
    explicit FingerprintWorker(QObject *parent = nullptr);
    ~FingerprintWorker();

    void processTask(const TaskData params);

signals:
    void progressUpdated(QString taskId, int percent);
    void finished(QString taskId, QImage image);
    void errorOccurred(QString taskId, QString message);

private:
    QImage convertToIris(const QImage &linearImg, int size);
};

#endif // FINGERPRINTWORKER_H
