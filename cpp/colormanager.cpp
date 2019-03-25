#include "colormanager.h"

#include <QPainter>

QColor ColorManager::getColor(QString element)
{
    int index = elements.indexOf(element);

    if (index>=0) return colors.at(index);

    QColor newColor = getNewColor();
    elements.append(element);
    colors.append(newColor);
    return newColor;
}

QColor ColorManager::getNewColor()
{
    QColor color = defColors.at(elements.count()%defColors.count()).toRgb();

    if (!rgb)
    {
        int r = color.red();
        int b = color.blue();

        color.setRed(b);
        color.setBlue(r);
    }

    return color;
}

bool ColorManager::getRgb() const
{
    return rgb;
}

void ColorManager::setRgb(bool value)
{
    rgb = value;
}

int getColor(QImage mask, QColor color, int x, int y)
{
    return color == Qt::red  ? qRed(mask.pixel(x,y))  :
           color == Qt::blue ? qBlue(mask.pixel(x,y)) :
                               qGreen(mask.pixel(x,y));
}

int billinerColor(QImage mask, QColor color, int xa, int xb, int xc, int xd, int ya, int yb, int yc, int yd, double alpha, double beta)
{
    int pa,pb,pc,pd;

    pa = getColor(mask,color,xa,ya);
    pb = getColor(mask,color,xb,yb);
    pc = getColor(mask,color,xc,yc);
    pd = getColor(mask,color,xd,yd);
    return (1-alpha)*(1-beta)*pa+alpha*(1-beta)*pb+
           (1-alpha)*beta*pc + alpha*beta*pd + 0.5;
}

uint billinearPixel(QImage mask, double sx, double sy, int k, int j)
{
    double alpha,beta;
    int xa,xb,xc,xd,ya,yb,yc,yd;

    xa = k/sx; ya = j/sy;
    xb = xa+1; yb = ya;
    xc = xa;   yc = ya+1;
    xd = xa+1; yd = ya+1;
    if (xb>=mask.width())  xb--;
    if (xd>=mask.width())  xd--;
    if (yc>=mask.height()) yc--;
    if (yd>=mask.height()) yd--;
    alpha = k/sx - xa;
    beta  = j/sy - ya;

    int red   = billinerColor(mask,Qt::red,xa,xb,xc,xd,ya,yb,yc,yd,alpha,beta);
    int green = billinerColor(mask,Qt::green,xa,xb,xc,xd,ya,yb,yc,yd,alpha,beta);
    int blue  = billinerColor(mask,Qt::blue,xa,xb,xc,xd,ya,yb,yc,yd,alpha,beta);

    return qRgb(red,green,blue);
}

QImage ColorManager::billinearInterpolation(QImage mask, double newHeight, double newWidth)
{
    const double sy = newHeight/mask.height();
    const double sx = newWidth/mask.width();

    // Resize mask to box size
    QImage maskScaled(mask.width()*sx,mask.height()*sy,QImage::Format_ARGB32_Premultiplied);
    maskScaled.fill(Qt::transparent);

    // Billinear interpolation
    // https://chu24688.tian.yam.com/posts/44797337
    for(int j=0;j<maskScaled.height();j++)
        for(int k=0;k<maskScaled.width();k++)
            maskScaled.setPixel(k,j,billinearPixel(mask,sx,sy,k,j));

    return maskScaled;
}

QImage ColorManager::applyTransformation(QImage image, QTransform painterTransform)
{
    QPainter painter;

    if (painter.begin(&image))
    {
        painter.setTransform(painterTransform);
        painter.end();
    }

    return image;
}
