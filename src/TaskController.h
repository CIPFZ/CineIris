#ifndef TASKCONTROLLER_H
#define TASKCONTROLLER_H

#include <QObject>
#include <QImage>
#include <QThread>
#include <QString>
#include <QVariantMap>

class BarcodeGenerator;

class TaskController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString videoPath READ videoPath NOTIFY videoPathChanged)
    Q_PROPERTY(QVariantMap videoMeta READ videoMeta NOTIFY videoMetaChanged)
    Q_PROPERTY(QString resultImagePath READ resultImagePath NOTIFY resultImageChanged)
    Q_PROPERTY(int outputSize READ outputSize WRITE setOutputSize NOTIFY outputSizeChanged)
    Q_PROPERTY(int sampleCount READ sampleCount WRITE setSampleCount NOTIFY sampleCountChanged)
    Q_PROPERTY(bool isIris READ isIris WRITE setIsIris NOTIFY isIrisChanged)
    Q_PROPERTY(int maxSize READ maxSize CONSTANT)
    Q_PROPERTY(int maxSamples READ maxSamples CONSTANT)

public:
    enum Status { Idle, Processing, Paused, Finished, Error };
    Q_ENUM(Status)

    explicit TaskController(QObject *parent = nullptr);
    ~TaskController();

    Status status() const { return m_status; }
    int progress() const { return m_progress; }
    QString videoPath() const { return m_videoPath; }
    QVariantMap videoMeta() const { return m_videoMeta; }
    QString resultImagePath() const { return m_resultImagePath; }
    int outputSize() const { return m_outputSize; }
    int sampleCount() const { return m_sampleCount; }
    bool isIris() const { return m_isIris; }
    int maxSize() const { return 4096; }
    int maxSamples() const { return 5000; }

    void setOutputSize(int v);
    void setSampleCount(int v);
    void setIsIris(bool v);

public slots:
    void loadVideo(const QString &path);
    void start();
    void pause();
    void resume();
    void cancel();
    void saveResult(const QString &path);
    void resetToDefaults();

signals:
    void statusChanged();
    void progressChanged();
    void videoPathChanged();
    void videoMetaChanged();
    void resultImageChanged();
    void outputSizeChanged();
    void sampleCountChanged();
    void isIrisChanged();
    void errorOccurred(const QString &message);

private:
    void setStatus(Status s);
    void setProgress(int p);
    void setResultImage(const QImage &img);
    void calculateSmartDefaults();

    Status m_status = Idle;
    int m_progress = 0;
    QString m_videoPath;
    QVariantMap m_videoMeta;
    QImage m_resultImage;
    QString m_resultImagePath;
    int m_outputSize = 1024;
    int m_sampleCount = 1000;
    bool m_isIris = true;

    QThread *m_thread = nullptr;
    BarcodeGenerator *m_generator = nullptr;
    bool m_pauseRequested = false;
    bool m_cancelRequested = false;

    // raw metadata for smart defaults
    int64_t m_rawDurationSec = 0;
    int m_rawWidth = 0;
};

#endif // TASKCONTROLLER_H
