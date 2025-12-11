#include "fingerprintworker.h"
#include <QtMath>
#include <QThread>

FingerprintWorker::FingerprintWorker(QObject *parent) : QObject(parent) {}
FingerprintWorker::~FingerprintWorker() {}

void FingerprintWorker::processTask(const TaskData params)
{
    QString taskId = params.id;
    QString filePath = params.filePath;

    AVFormatContext *fmtCtx = nullptr;
    // 1. 打开文件
    if (avformat_open_input(&fmtCtx, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit errorOccurred(taskId, "无法打开视频文件");
        return;
    }

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        emit errorOccurred(taskId, "无法读取流信息");
        return;
    }

    // 2. 查找正片视频流 (排除封面)
    int videoStreamIndex = -1;
    for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
        AVStream *st = fmtCtx->streams[i];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && !(st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        // 兜底：随便找一个视频流
        for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
            if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;
            }
        }
    }

    if (videoStreamIndex == -1) {
        avformat_close_input(&fmtCtx);
        emit errorOccurred(taskId, "未找到视频流");
        return;
    }

    AVStream *videoStream = fmtCtx->streams[videoStreamIndex];
    AVCodecParameters *codecPar = videoStream->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        emit errorOccurred(taskId, "无法打开解码器");
        return;
    }

    // --- 准备数据 ---
    int targetH = 600;
    int safeW = 32;
    struct SwsContext *swsCtx = nullptr;

    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pFrameRGB = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, safeW, targetH, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, safeW, targetH, 1);

    int maxStripes = params.sampleCount;
    if (maxStripes <= 0) maxStripes = 1000;

    QImage linearMap(maxStripes, targetH, QImage::Format_RGB888);
    linearMap.fill(Qt::black);

    // 计算时长 (使用流的时长，如果流没有，用容器的)
    int64_t totalDuration = videoStream->duration;
    AVRational timeBase = videoStream->time_base;

    if (totalDuration <= 0) {
        // 回退到容器时长
        if (fmtCtx->duration != AV_NOPTS_VALUE) {
            totalDuration = av_rescale_q(fmtCtx->duration, AV_TIME_BASE_Q, timeBase);
        } else {
            totalDuration = 100 * timeBase.den; // 假数据防止除0
        }
    }

    int64_t step = totalDuration / maxStripes;
    int currentStripe = 0;
    int validFrames = 0;

    // --- 循环开始 ---
    for (int i = 0; i < maxStripes; ++i) {
        if (QThread::currentThread()->isInterruptionRequested()) break;

        int64_t targetTimestamp = i * step;

        av_seek_frame(fmtCtx, videoStreamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codecCtx);

        bool frameDecoded = false;
        int attempts = 0;

        while (av_read_frame(fmtCtx, packet) >= 0 && attempts < 50) {
            if (packet->stream_index == videoStreamIndex) {
                if (avcodec_send_packet(codecCtx, packet) == 0) {
                    if (avcodec_receive_frame(codecCtx, pFrame) == 0) {

                        // 懒加载 swsCtx
                        if (swsCtx == nullptr) {
                            swsCtx = sws_getContext(
                                pFrame->width, pFrame->height, (AVPixelFormat)pFrame->format,
                                safeW, targetH, AV_PIX_FMT_RGB24,
                                SWS_BILINEAR, nullptr, nullptr, nullptr
                            );
                        }

                        // 转换
                        sws_scale(swsCtx, (uint8_t const * const *)pFrame->data,
                                  pFrame->linesize, 0, pFrame->height,
                                  pFrameRGB->data, pFrameRGB->linesize);

                        // 写入 QImage
                        if (currentStripe < linearMap.width()) {
                            // 取色
                            int centerCol = safeW / 2;
                            int stride = pFrameRGB->linesize[0];

                            for (int y = 0; y < targetH; ++y) {
                                int offset = y * stride + centerCol * 3;
                                uint8_t r = pFrameRGB->data[0][offset];
                                uint8_t g = pFrameRGB->data[0][offset + 1];
                                uint8_t b = pFrameRGB->data[0][offset + 2];
                                linearMap.setPixel(currentStripe, y, qRgb(r, g, b));
                            }
                        }
                        frameDecoded = true;
                        validFrames++;
                    }
                }
            }
            av_packet_unref(packet);
            if (frameDecoded) break;
            attempts++;
        }
        currentStripe++;

        if (currentStripe % 20 == 0 || currentStripe == maxStripes) {
            emit progressUpdated(taskId, (int)((float)currentStripe / maxStripes * 100));
        }
    }

    // --- 结果生成 ---
    if (currentStripe > 0 && !QThread::currentThread()->isInterruptionRequested()) {
        QImage validLinear = linearMap.copy(0, 0, qMin(currentStripe, linearMap.width()), targetH);
        QImage finalResult;

        if (params.isIris) {
            finalResult = convertToIris(validLinear, params.outputSize);
        } else {
            finalResult = validLinear.scaled(params.outputSize, params.outputSize / 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        emit finished(taskId, finalResult);
    } else {
        if (!QThread::currentThread()->isInterruptionRequested())
            emit errorOccurred(taskId, "未能提取到有效帧");
    }

    // --- 清理 ---
    av_free(buffer);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameRGB);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    if (swsCtx) sws_freeContext(swsCtx);
}

QImage FingerprintWorker::convertToIris(const QImage &linearImg, int size)
{
    int outputSize = size;
    int holeRadius = outputSize * 0.15;
    QImage iris(outputSize, outputSize, QImage::Format_ARGB32);
    iris.fill(Qt::transparent);

    int cx = outputSize / 2;
    int cy = outputSize / 2;
    int maxRadius = outputSize / 2;

    for (int y = 0; y < outputSize; ++y) {
        for (int x = 0; x < outputSize; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            double dist = std::sqrt(dx*dx + dy*dy);

            if (dist >= holeRadius && dist <= maxRadius) {
                double angle = std::atan2(dy, dx);
                double ratioX = (angle + M_PI) / (2 * M_PI);
                double ratioY = (dist - holeRadius) / (maxRadius - holeRadius);

                int srcX = qBound(0, (int)(ratioX * linearImg.width()), linearImg.width() - 1);
                int srcY = qBound(0, (int)(ratioY * linearImg.height()), linearImg.height() - 1);

                iris.setPixel(x, y, linearImg.pixel(srcX, srcY));
            }
        }
    }
    return iris;
}
