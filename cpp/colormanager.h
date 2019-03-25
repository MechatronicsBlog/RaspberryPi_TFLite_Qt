#ifndef COLORMANAGER_H
#define COLORMANAGER_H

#include <QColor>
#include <QStringList>
#include <QList>
#include <QImage>
#include <QTransform>

class ColorManager
{
    public:
        QColor getColor(QString element);
        static QImage billinearInterpolation(QImage mask, double newHeight, double newWidth);
        static QImage applyTransformation(QImage image, QTransform painterTransform);
        bool getRgb() const;
        void setRgb(bool value);

private:
        QStringList elements;
        QList<QColor> colors;
        QColor getNewColor();
        bool rgb = true;

        // NOTE: change or add new colors
        const QList<QColor> defColors = {"#f6a625","#99ca53","#2097d2","#b5563d","#7264d6"};
};

#endif // COLORMANAGER_H
