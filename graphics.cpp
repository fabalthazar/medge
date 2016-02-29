#include "mainwindow.h"
#include "graphics.h"

#include <QGraphicsView>

void MScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent){
    cout << "Mouse pressed on scene" << endl;
    QGraphicsScene::mousePressEvent(mouseEvent); // allow propagation to items
    /*if (!views().isEmpty())
    {
        cout << "translate 10,10" << endl;
        views()[0]->translate(10, 10);
    }*/
    QGraphicsItem* itemSelected = itemAt(mouseEvent->scenePos(), QTransform());
    if (itemSelected)
    {
        if (itemSelected->isSelected()) cout << "Item selected";
    }
    if (itemSelected == mouseGrabberItem())
    {
        cout << " and grabber!";
    }
    cout << endl;
}

void MScene::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent){
    cout << "Mouse moved on scene" << endl;
    QGraphicsScene::mouseMoveEvent(mouseEvent); // propagation to items
    if (MainWindow::viewMode() == ViewMode::MOVE)
    {
        // Move the scene
        if (!views().isEmpty())
        {
            QPointF pointTmp = mouseEvent->scenePos() - mouseEvent->lastScenePos();
            cout << "Delta: " << pointTmp.x() << " " << pointTmp.y() << endl;
            //views()[0]->translate(delta.x(), delta.y()); // FIXME
        }
    }
}

void MPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event){
    cout << "Mouse pressed on pixmap" << endl;
    QGraphicsPixmapItem::mousePressEvent(event);
}
