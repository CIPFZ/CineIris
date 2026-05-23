#include "TaskController.h"
#include "BarcodeGenerator.h"
#include "VideoProber.h"
#include <QFileInfo>
#include <QFile>
#include <QStandardPaths>
#include <QDir>

TaskController::TaskController(QObject *parent)
    : QObject(parent) {}

TaskController::~TaskController()
{
    if (m_thread && m_thread->isRunning()) {
        m_cancelRequested = true;
        if (m_generator) m_generator->requestCancel();
        m_thread->quit();
        m_thread->wait();
        m_thread->deleteLater();
    }
}

void TaskController::setOutputSize(int v) {
    v = (v / 2) * 2; // ensure even
    if (v != m_outputSize) { m_outputSize = v; emit outputSizeChanged(); }
}

void TaskController::setSampleCount(int v) {
    if (v != m_sampleCount) { m_sampleCount = v; emit sampleCountChanged(); }
}

void TaskController::setIsIris(bool v) {
    if (v != m_isIris) { m_isIris = v; emit isIrisChanged(); }
}

void TaskController::loadVideo(const QString &path)
{
    cancel(); // stop any running task

    m_videoPath = path;
    m_videoMeta.clear();
    m_resultImage = QImage();
    m_progress = 0;
    emit videoPathChanged();
    emit resultImageChanged();

    VideoMeta meta;
    if (VideoProber::probe(path, meta)) {
        m_rawDurationSec = meta.durationSec;
        m_rawWidth = meta.width;
        m_videoMeta["duration"] = meta.durationStr;
        m_videoMeta["resolution"] = meta.resolution;
        m_videoMeta["codec"] = meta.codec;
        m_videoMeta["frameRate"] = meta.frameRate;
        m_videoMeta["fileSize"] = meta.fileSize;
        emit videoMetaChanged();

        calculateSmartDefaults();
    } else {
        m_videoMeta["duration"] = "-";
        m_videoMeta["resolution"] = "-";
        m_videoMeta["codec"] = "-";
        m_videoMeta["frameRate"] = "-";
        m_videoMeta["fileSize"] = "-";
        emit videoMetaChanged();
        emit errorOccurred("无法读取视频信息");
        setStatus(Error);
    }
}

void TaskController::calculateSmartDefaults()
{
    if (m_rawDurationSec > 0) {
        int smart = m_rawDurationSec * 5;
        smart = qBound(200, smart, 2500);
        setSampleCount(smart);
    }
    if (m_rawWidth > 0) {
        int smart = 2048;
        if (m_rawWidth < smart) smart = m_rawWidth;
        if (smart % 2) smart--;
        setOutputSize(smart);
    }
}

void TaskController::start()
{
    if (m_status == Processing) return;
    if (m_status == Paused) { resume(); return; }
    if (m_videoPath.isEmpty()) return;

    m_cancelRequested = false;
    m_pauseRequested = false;

    // Batch all state resets before emitting any signal,
    // so QML sees progress=0 and resultImage="" atomically.
    bool statusWillChange = (m_status != Processing);
    bool progressWillChange = (m_progress != 0);
    bool resultWillChange = !m_resultImage.isNull() || !m_resultImagePath.isEmpty();

    m_progress = 0;
    m_resultImage = QImage();
    if (!m_resultImagePath.isEmpty()) {
        QFile::remove(m_resultImagePath);
        m_resultImagePath.clear();
    }
    m_status = Processing;

    if (progressWillChange) emit progressChanged();
    if (resultWillChange) emit resultImageChanged();
    if (statusWillChange) emit statusChanged();

    m_thread = new QThread(this);
    m_generator = new BarcodeGenerator;
    m_generator->setVideoPath(m_videoPath);
    m_generator->setSampleCount(m_sampleCount);
    m_generator->setOutputSize(m_outputSize);
    m_generator->setIsIris(m_isIris);
    m_generator->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_generator, &BarcodeGenerator::process);
    connect(m_thread, &QThread::finished, m_generator, &QObject::deleteLater);

    connect(m_generator, &BarcodeGenerator::progressUpdated, this, [this](int p) {
        setProgress(p);
    });
    connect(m_generator, &BarcodeGenerator::finished, this, [this](QImage img) {
        setResultImage(img);
        setStatus(Finished);
        m_thread->quit(); m_thread->wait(); m_thread->deleteLater(); m_thread = nullptr;
    });
    connect(m_generator, &BarcodeGenerator::errorOccurred, this, [this](QString msg) {
        setStatus(Error);
        m_thread->quit(); m_thread->wait(); m_thread->deleteLater(); m_thread = nullptr;
        emit errorOccurred(msg);
    });

    m_thread->start();
}

void TaskController::pause()
{
    if (m_status != Processing) return;
    m_pauseRequested = true;
    if (m_generator) m_generator->requestPause();
    setStatus(Paused);
}

void TaskController::resume()
{
    if (m_status != Paused) return;
    m_pauseRequested = false;
    if (m_generator) m_generator->resume();
    setStatus(Processing);
}

void TaskController::cancel()
{
    if (m_status != Processing && m_status != Paused) return;
    m_cancelRequested = true;
    m_pauseRequested = false;
    if (m_generator) {
        m_generator->resume();  // unpause so it can exit
        m_generator->requestCancel();
    }
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait();
        m_thread->deleteLater();
        m_thread = nullptr;
    }
    setProgress(0);
    setStatus(Idle);
}

void TaskController::saveResult(const QString &path)
{
    if (!m_resultImage.isNull())
        m_resultImage.save(path);
}

void TaskController::resetToDefaults()
{
    if (m_rawDurationSec <= 0 && m_rawWidth <= 0) return;
    calculateSmartDefaults();
}

void TaskController::setStatus(Status s)
{
    if (m_status != s) { m_status = s; emit statusChanged(); }
}

void TaskController::setProgress(int p)
{
    if (m_progress != p) { m_progress = p; emit progressChanged(); }
}

void TaskController::setResultImage(const QImage &img)
{
    m_resultImage = img;
    if (!img.isNull()) {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        m_resultImagePath = dir + "/cineiris_result.png";
        img.save(m_resultImagePath, "PNG");
    } else {
        m_resultImagePath.clear();
        QFile::remove(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/cineiris_result.png");
    }
    emit resultImageChanged();
}
