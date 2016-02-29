#ifndef GRAPHICS
#define GRAPHICS

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

using namespace std;

class MScene: public QGraphicsScene{
public:
    MScene(QObject* parent = 0): QGraphicsScene(parent){}

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent); // TODO
};

class MPixmapItem: public QGraphicsPixmapItem{
public:
    MPixmapItem(QGraphicsItem* parent = 0): QGraphicsPixmapItem(parent){}
    MPixmapItem(const QPixmap& pixmap, QGraphicsItem* parent = 0): QGraphicsPixmapItem(pixmap, parent){}

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
};

#endif // GRAPHICS

