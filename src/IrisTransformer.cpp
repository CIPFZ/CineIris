#include "IrisTransformer.h"
#include <QtMath>

QImage IrisTransformer::transform(const QImage &linearSrc, int size)
{
    int holeRadius = size * 0.15;
    QImage iris(size, size, QImage::Format_ARGB32);
    iris.fill(Qt::transparent);

    int cx = size / 2;
    int cy = size / 2;
    int maxRadius = size / 2;
    int srcW = linearSrc.width();
    int srcH = linearSrc.height();

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist >= holeRadius && dist <= maxRadius) {
                double angle = std::atan2(dy, dx);
                double ratioX = (angle + M_PI) / (2 * M_PI);
                double ratioY = (dist - holeRadius) / (maxRadius - holeRadius);

                int srcX = qBound(0, (int)(ratioX * srcW), srcW - 1);
                int srcY = qBound(0, (int)(ratioY * srcH), srcH - 1);
                iris.setPixel(x, y, linearSrc.pixel(srcX, srcY));
            }
        }
    }
    return iris;
}
