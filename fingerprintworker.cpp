#include "fingerprintworker.h"
#include <QtMath>
#include <QDebug>

FingerprintWorker::FingerprintWorker(QObject *parent) : QObject(parent) {}
FingerprintWorker::~FingerprintWorker() {}

void FingerprintWorker::processTask(const TaskData params)
{
    QString taskId = params.id;
    QString filePath = params.filePath;

    // --- 1. FFmpeg 初始化 ---
    AVFormatContext *fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, filePath.toLocal8Bit().data(), nullptr, nullptr) < 0) {
        emit errorOccurred(taskId, "无法打开视频文件");
        return;
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        emit errorOccurred(taskId, "无法读取流信息");
        return;
    }

    int videoStreamIndex = -1;
    for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1) {
        avformat_close_input(&fmtCtx);
        emit errorOccurred(taskId, "未找到视频流");
        return;
    }

    AVCodecParameters *codecPar = fmtCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        emit errorOccurred(taskId, "无法打开解码器");
        return;
    }

    // --- 2. 图像转换准备 (采样高度固定600) ---
    int targetH = 600;
    struct SwsContext *swsCtx = sws_getContext(
        codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
        1, targetH, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pFrameRGB = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, 1, targetH, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, 1, targetH, 1);

    // --- 3. 抽帧循环 ---
    int maxStripes = params.sampleCount;
    if (maxStripes <= 0) maxStripes = 1000;

    QImage linearMap(maxStripes, targetH, QImage::Format_RGB888);
    linearMap.fill(Qt::black);

    int64_t totalDuration = fmtCtx->duration;
    if (totalDuration <= 0) totalDuration = 10 * AV_TIME_BASE;

    AVRational timeBase = fmtCtx->streams[videoStreamIndex]->time_base;
    int64_t streamDuration = av_rescale_q(totalDuration, AV_TIME_BASE_Q, timeBase);
    int64_t step = streamDuration / maxStripes;

    int currentStripe = 0;
    for (int i = 0; i < maxStripes; ++i) {
        // [新增] 响应中断请求 (用于暂停或删除)
        if (QThread::currentThread()->isInterruptionRequested()) {
            // 清理并退出
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            av_free(buffer);
            av_frame_free(&pFrame);
            av_frame_free(&pFrameRGB);
            av_packet_free(&packet);
            sws_freeContext(swsCtx);
            return; // 直接结束
        }

        int64_t targetTimestamp = i * step;
        av_seek_frame(fmtCtx, videoStreamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codecCtx);

        bool frameDecoded = false;
        int attempts = 0;
        while (av_read_frame(fmtCtx, packet) >= 0 && attempts < 15) {
            if (packet->stream_index == videoStreamIndex) {
                if (avcodec_send_packet(codecCtx, packet) == 0) {
                    if (avcodec_receive_frame(codecCtx, pFrame) == 0) {
                        sws_scale(swsCtx, (uint8_t const * const *)pFrame->data,
                                  pFrame->linesize, 0, codecCtx->height,
                                  pFrameRGB->data, pFrameRGB->linesize);

                        // [修复] 边界检查，防止崩溃
                        if (currentStripe < linearMap.width()) {
                            for (int y = 0; y < targetH; ++y) {
                                uint8_t r = pFrameRGB->data[0][y * pFrameRGB->linesize[0]];
                                uint8_t g = pFrameRGB->data[0][y * pFrameRGB->linesize[0] + 1];
                                uint8_t b = pFrameRGB->data[0][y * pFrameRGB->linesize[0] + 2];
                                linearMap.setPixel(currentStripe, y, qRgb(r, g, b));
                            }
                        }
                        frameDecoded = true;
                    }
                }
            }
            av_packet_unref(packet);
            if (frameDecoded) break;
            attempts++;
        }
        currentStripe++;
        if (currentStripe % 10 == 0) emit progressUpdated(taskId, currentStripe * 100 / maxStripes);
    }

    // --- 4. 结果生成 ---
    if (currentStripe > 0) {
        QImage validLinear = linearMap.copy(0, 0, qMin(currentStripe, linearMap.width()), targetH);
        QImage finalResult;

        if (params.isIris) {
            finalResult = convertToIris(validLinear, params.outputSize);
        } else {
            finalResult = validLinear.scaled(params.outputSize, params.outputSize / 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        emit finished(taskId, finalResult);
    } else {
        emit errorOccurred(taskId, "未提取到任何帧");
    }

    // --- 5. 清理 ---
    av_free(buffer);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameRGB);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    sws_freeContext(swsCtx);
}

QImage FingerprintWorker::convertToIris(const QImage &linearImg, int size)
{
    int outputSize = size;
    int holeRadius = outputSize * 0.2;
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
