#include "BarcodeGenerator.h"
#include "IrisTransformer.h"
#include <QtMath>
#include <QThread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

BarcodeGenerator::BarcodeGenerator(QObject *parent)
    : QObject(parent) {}

BarcodeGenerator::~BarcodeGenerator() {}

void BarcodeGenerator::requestPause() { m_paused = true; }
void BarcodeGenerator::requestCancel() { m_cancelled = true; }
void BarcodeGenerator::resume() { m_paused = false; }

void BarcodeGenerator::process()
{
    m_paused = false;
    m_cancelled = false;

    QImage barcode = generateBarcode();

    if (m_cancelled) return;

    if (barcode.isNull()) {
        emit errorOccurred("未能提取到有效帧");
        return;
    }

    QImage result;
    if (m_isIris) {
        result = IrisTransformer::transform(barcode, m_outputSize);
    } else {
        result = barcode.scaled(m_outputSize, m_outputSize / 2,
                                 Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    emit finished(result);
}

QImage BarcodeGenerator::generateBarcode()
{
    AVFormatContext *fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, m_videoPath.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit errorOccurred("无法打开视频文件");
        return {};
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        emit errorOccurred("无法读取流信息");
        return {};
    }

    int videoIdx = -1;
    for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
        AVStream *st = fmtCtx->streams[i];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
            !(st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            videoIdx = i; break;
        }
    }
    if (videoIdx < 0) {
        for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
            if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoIdx = i; break;
            }
        }
    }
    if (videoIdx < 0) {
        avformat_close_input(&fmtCtx);
        emit errorOccurred("未找到视频流");
        return {};
    }

    AVStream *videoSt = fmtCtx->streams[videoIdx];
    AVCodecParameters *codecPar = videoSt->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    codecCtx->thread_count = 0; // auto thread count

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        emit errorOccurred("无法打开解码器");
        return {};
    }

    int targetH = 600;
    int cropW = 1; // optimized: only need 1 column
    int srcCenterX = codecPar->width / 2;

    SwsContext *sws = sws_getContext(
        codecPar->width, codecPar->height, (AVPixelFormat)codecPar->format,
        cropW, targetH, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    AVFrame *frame = av_frame_alloc();
    AVFrame *frameRGB = av_frame_alloc();
    AVPacket *pkt = av_packet_alloc();

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, cropW, targetH, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes);
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_RGB24, cropW, targetH, 1);

    int64_t totalDuration = videoSt->duration;
    AVRational timeBase = videoSt->time_base;
    if (totalDuration <= 0) {
        if (fmtCtx->duration != AV_NOPTS_VALUE)
            totalDuration = av_rescale_q(fmtCtx->duration, AV_TIME_BASE_Q, timeBase);
        else
            totalDuration = 100 * timeBase.den;
    }

    int maxStripes = m_sampleCount;
    if (maxStripes <= 0) maxStripes = 1000;

    QImage linearMap(maxStripes, targetH, QImage::Format_RGB888);
    linearMap.fill(Qt::black);

    int64_t step = totalDuration / maxStripes;
    int currentStripe = 0;

    for (int i = 0; i < maxStripes; ++i) {
        // Check pause/cancel
        while (m_paused && !m_cancelled) {
            QThread::msleep(50);
        }
        if (m_cancelled) break;

        int64_t targetTs = i * step;
        av_seek_frame(fmtCtx, videoIdx, targetTs, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codecCtx);

        bool decoded = false;
        int attempts = 0;
        while (av_read_frame(fmtCtx, pkt) >= 0 && attempts < 50) {
            if (pkt->stream_index == videoIdx) {
                if (avcodec_send_packet(codecCtx, pkt) == 0) {
                    if (avcodec_receive_frame(codecCtx, frame) == 0) {
                        // Scale only the center column (srcCenterX, 0, 1, srcH) → 1×targetH
                        sws_scale(sws,
                                  (uint8_t const * const *)frame->data,
                                  frame->linesize,
                                  0, frame->height,
                                  frameRGB->data,
                                  frameRGB->linesize);

                        // Read the single column of pixels
                        if (currentStripe < linearMap.width()) {
                            for (int y = 0; y < targetH; ++y) {
                                int offset = y * frameRGB->linesize[0];
                                uint8_t r = frameRGB->data[0][offset];
                                uint8_t g = frameRGB->data[0][offset + 1];
                                uint8_t b = frameRGB->data[0][offset + 2];
                                linearMap.setPixel(currentStripe, y, qRgb(r, g, b));
                            }
                        }
                        decoded = true;
                    }
                }
            }
            av_packet_unref(pkt);
            if (decoded) break;
            attempts++;
        }
        currentStripe++;

        if (currentStripe % 20 == 0 || currentStripe == maxStripes) {
            emit progressUpdated((int)((float)currentStripe / maxStripes * 100));
        }
    }

    av_free(buffer);
    av_frame_free(&frame);
    av_frame_free(&frameRGB);
    av_packet_free(&pkt);
    sws_freeContext(sws);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);

    if (m_cancelled || currentStripe == 0) return {};
    return linearMap.copy(0, 0, qMin(currentStripe, linearMap.width()), targetH);
}
