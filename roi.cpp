#include <iostream>

#include "roi.h"

using namespace std; // for cerr

// Default constructor
Roi::Roi():
    area(0.),
    center(),
    dev(0.),
    indexinImage(0),
    length(0.),
    max(0.),
    mean(0.),
    min(0.),
    name(""),
    nbPts(0),
    pts_mmList(), // Empty lists
    pts_pxList(),
    ptsValList(),
    total(0.),
    type(TypeRoi::UNDEF)
    /*item_p(nullptr)
    scene_p(nullptr),
    selected(false)*/
{
    nbRois++;
}

// Copy constructor
Roi::Roi(const Roi& roi_in):
    area(roi_in.area),
    center(roi_in.center),
    dev(roi_in.dev),
    indexinImage(roi_in.indexinImage),
    length(roi_in.length),
    max(roi_in.max),
    mean(roi_in.mean),
    min(roi_in.min),
    name(roi_in.name),
    nbPts(roi_in.nbPts),
    pts_mmList(roi_in.pts_mmList),
    pts_pxList(roi_in.pts_pxList),
    ptsValList(roi_in.ptsValList),
    total(roi_in.total),
    type(roi_in.type)
    /*item_p(roi_in.item_p)
    scene_p(roi_in.scene_p),
    selected(roi_in.selected)*/
{
    nbRois++;
}

// Move constructor?
/*Roi::Roi(Roi&& roi_in):
    area(roi_in.area),
    center(roi_in.center),
    dev(roi_in.dev),
    indexinImage(roi_in.indexinImage),
    length(roi_in.length),
    max(roi_in.max),
    mean(roi_in.mean),
    min(roi_in.min),
    name(roi_in.name),
    nbPts(roi_in.nbPts),
    pts_mmList(roi_in.pts_mmList),
    pts_pxList(roi_in.pts_pxList),
    ptsValList(roi_in.ptsValList),
    total(roi_in.total),
    type(roi_in.type)
{
    nbRois++;
}*/

Roi::~Roi(){
    pts_mmList.clear(); // Perhaps useless...? (the lists may be automatically deleted)
    pts_pxList.clear();
    ptsValList.clear();
    nbRois--;
}

void RoiCirc::addToScene(MScene *scene_p){
    QVector<QPointF> vector = QVector<QPointF>::fromList(getPts_pxList());
    QPolygonF polygon(vector);
    QPen pen(Qt::red, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    setPen(pen);
    setPolygon(polygon);
    scene_p->addItem(this);
}

void RoiCurve::addToScene(MScene *scene_p){
    QVector<QPointF> vector = QVector<QPointF>::fromList(getPts_pxList());
    qreal a1=0, a2=0, x=0, y=0;
    QPointF ctrlPt;
    // TODO: add Bézier curve for first and last points (instead of lines)
    if (vector.size())
    {
        QPainterPath painterPath(vector[0]); // first point
        if (vector.size() > 1)
        {
            painterPath.lineTo(vector[1]); // second point: line
            if (vector.size() > 3)
            {
                for (int i=1; i < vector.size()-2; i++) // loop to determine the control point of the Bézier curve
                {
                    a1 = (vector[i+1].y()-vector[i-1].y())/(vector[i+1].x()-vector[i-1].x());
                    a2 = (vector[i+2].y()-vector[i].y())/(vector[i+2].x()-vector[i].x());
                    x = (a1*vector[i].x()-a2*vector[i+1].x()+vector[i+1].y()-vector[i].y())/(a1-a2);
                    y = a1*(x-vector[i].x()) + vector[i].y();
                    ctrlPt = QPointF(x,y);
                    painterPath.quadTo(ctrlPt, vector[i+1]);
                }
            }
            painterPath.lineTo(vector.back()); // last point: line
        }
        QPen pen(Qt::blue, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        setPen(pen);
        setPath(painterPath);
        scene_p->addItem(this);
    }
}

void RoiRect::addToScene(MScene *scene_p){
    if (getPts_pxList().size() == 4)
    {
        QRectF rect(getPts_pxList()[0], getPts_pxList()[2]); // top left, bottom right
        QPen pen(Qt::green, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        setPen(pen);
        setRect(rect);
        scene_p->addItem(this);
    }
    else cerr << "Unable to display the \"" << name.toStdString() << "\" ROI: no 4 points" << endl;
}

// Reimplementation
void RoiCirc::mousePressEvent(QGraphicsSceneMouseEvent* event){
    cout << getName().toStdString() << " clicked" << endl;
    QGraphicsPolygonItem::mousePressEvent(event);
}

void RoiCirc::mouseMoveEvent(QGraphicsSceneMouseEvent *event){
    cout << "Mouse moved on " << getName().toStdString() << endl;
    QGraphicsPolygonItem::mouseMoveEvent(event);
    /* Update ROI properties: center, dev, max, mean, min, points lists (mm, px, val), total
     * Maybe compute that in mouseReleaseEvent() */
}
