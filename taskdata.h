#ifndef TASKDATA_H
#define TASKDATA_H

#include <QString>
#include <QImage>
#include <QJsonObject>
#include <QVariant> // <--- [关键修复] 必须添加这一行，否则无法转换类型

enum TaskStatus {
    Status_Draft,     // 📝 草稿
    Status_Pending,   // ⏳ 排队中
    Status_Running,   // 🚀 运行中
    Status_Finished,  // ✅ 完成
    Status_Error,     // ❌ 错误
    Status_Paused     // ⏸️ 暂停
};

struct TaskData {
    QString id;
    QString filePath;
    QString fileName;

    // --- 元数据 (Metadata) ---
    QString durationStr = "-";
    QString resolution = "-";
    QString codec = "-";
    QString frameRate = "-";
    QString fileSize = "-";

    // 用于智能计算的原始数值
    int64_t rawDurationSec = 0;
    int rawWidth = 0;
    int rawHeight = 0;

    // --- 任务参数 ---
    int sampleCount = 1000;
    int outputSize = 1024;
    bool isIris = true;

    // --- 运行状态 ---
    TaskStatus status = Status_Draft;
    int progress = 0;
    QImage resultImage;

    // 序列化为 JSON
    QJsonObject toJson() const {
        QJsonObject json;
        json["id"] = id;
        json["filePath"] = filePath;
        json["fileName"] = fileName;
        json["sampleCount"] = sampleCount;
        json["outputSize"] = outputSize;
        json["isIris"] = isIris;
        json["status"] = (int)status;

        // 存元数据
        json["meta_dur"] = durationStr;
        json["meta_res"] = resolution;
        json["meta_codec"] = codec;
        json["meta_fps"] = frameRate;
        json["meta_size"] = fileSize;
        json["raw_dur"] = (qint64)rawDurationSec;
        json["raw_w"] = rawWidth;

        return json;
    }

    // 从 JSON 恢复
    static TaskData fromJson(const QJsonObject &json) {
        TaskData t;
        t.id = json["id"].toString();
        t.filePath = json["filePath"].toString();
        t.fileName = json["fileName"].toString();
        t.sampleCount = json["sampleCount"].toInt(1000);
        t.outputSize = json["outputSize"].toInt(1024);
        t.isIris = json["isIris"].toBool(true);

        int s = json["status"].toInt();
        if (s == Status_Running) s = Status_Paused; // 恢复时如果是运行中，改为暂停
        t.status = (TaskStatus)s;

        t.durationStr = json["meta_dur"].toString("-");
        t.resolution = json["meta_res"].toString("-");
        t.codec = json["meta_codec"].toString("-");
        t.frameRate = json["meta_fps"].toString("-");
        t.fileSize = json["meta_size"].toString("-");

        // 这里的 .toVariant() 需要 <QVariant> 头文件支持
        t.rawDurationSec = json["raw_dur"].toVariant().toLongLong();
        t.rawWidth = json["raw_w"].toInt();

        return t;
    }
};

#endif // TASKDATA_H
