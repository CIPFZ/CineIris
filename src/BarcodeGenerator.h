#ifndef BARCODEGENERATOR_H
#define BARCODEGENERATOR_H

#include <QObject>
#include <QImage>
#include <QString>

class BarcodeGenerator : public QObject
{
    Q_OBJECT
public:
    explicit BarcodeGenerator(QObject *parent = nullptr);
    ~BarcodeGenerator();

    void setVideoPath(const QString &path) { m_videoPath = path; }
    void setSampleCount(int n) { m_sampleCount = n; }
    void setOutputSize(int n) { m_outputSize = n; }
    void setIsIris(bool v) { m_isIris = v; }

public slots:
    void process();

    // Thread-safe pause/cancel
    void requestPause();
    void requestCancel();
    void resume();

signals:
    void progressUpdated(int percent);
    void finished(QImage image);
    void errorOccurred(QString message);

private:
    QImage generateBarcode();
    QImage convertToIris(const QImage &linear);

    QString m_videoPath;
    int m_sampleCount = 1000;
    int m_outputSize = 1024;
    bool m_isIris = true;

    // LUT for iris transform (computed once)
    struct Coord { int x, y; };
    QVector<Coord> m_irisLut;
    int m_lutSize = 0;
    void buildIrisLut(int size, int holeRadius, int maxRadius, int srcW, int srcH);

    // Thread control
    volatile bool m_paused = false;
    volatile bool m_cancelled = false;
};

#endif // BARCODEGENERATOR_H
