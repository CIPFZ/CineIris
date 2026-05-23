#ifndef IRISTRANSFORMER_H
#define IRISTRANSFORMER_H

#include <QImage>
#include <QVector>

class IrisTransformer
{
public:
    static QImage transform(const QImage &linearSrc, int outputSize);
};

#endif // IRISTRANSFORMER_H
