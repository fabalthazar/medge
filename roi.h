#ifndef ROI_H
#define ROI_H

#include <QString>
#include <QList>
#include <QPointF>
#include <QGraphicsScene>
#include <QGraphicsPolygonItem>

#include "graphics.h"

enum class FsmXmlImage : char {KEY, IMAGE_INDEX, NUMBER_ROIS, ROIS};
enum class FsmXmlRoi : char {KEY, AREA, CENTER, DEV, INDEX, LENGTH, MAX, MEAN, MIN, NAME, NUMBER_PTS, POINT_MM, POINT_PX, POINT_VAL, TOTAL, TYPE};
enum class TypeRoi: unsigned char{UNDEF, LIN=10, CIRC=15, RECT=6}; // Types of ROIs: 10 (linear); 15 (circular); 6 (rectangle)

class Roi;

struct roiImport_s{
    unsigned int imageIndex;
    //unsigned int nbRoisInImage;
    QList<Roi*> roiList;
};

class Roi{
protected:
    static unsigned int nbRois; // total number of ROIs allocated.
    // Attributes from XML
    float area;
    QPointF center; // in mm!
    float dev;
    unsigned int indexinImage; // starts from 0: first image (lowest Slice Location)
    float length;
    float max;
    float mean;
    float min;
    QString name; // TODO: perhaps redefine
    unsigned int nbPts;
    QList<QPointF> pts_mmList; // in mm
    QList<QPointF> pts_pxList; // in px
    QList<float> ptsValList;
    float total;
    TypeRoi type;
    // End: attributes from XML

    // Scene-related
    //QGraphicsItem* item_p;
    //QGraphicsScene* scene_p; // not needed anymore: method scene() gives the scene
    //bool selected;

public:
    Roi(); // default constructor
    Roi(const Roi& roi_in); // copy constructor
    //Roi(Roi&& roi_in);
    virtual ~Roi();

    void setArea(float a){area = a;}
    void setCenter(QPointF pt){center = pt;} // in mm
    void setDev(float d){dev = d;}
    void setIndexinImage(unsigned int i){indexinImage = i;}
    void setLength(float l){length = l;}
    void setMax(float m){max = m;}
    void setMean(float m){mean = m;}
    void setMin(float m){min = m;}
    void setName(QString n){name = n;}
    void setNumberPts(unsigned int n){nbPts = n;}
    void addPoint_mm(QPointF pt){pts_mmList.append(pt);} // in mm
    void addPoint_px(QPointF pt){pts_pxList.append(pt);} // in px
    void addPoint_val(float v){ptsValList.append(v);}
    void setTotal(float t){total = t;}
    void setType(TypeRoi t){type = t;}

    static unsigned int getNbRois(){return nbRois;}

    float getArea(){return area;}
    QPointF getCenter(){return center;}
    float getDev(){return dev;}
    unsigned int getIndexinImage(){return indexinImage;}
    float getLength(){return length;}
    float getMax(){return max;}
    float getMean(){return mean;}
    float getMin(){return min;}
    QString getName(){return name;}
    unsigned int getNumberPts(){return nbPts;}
    // lists?...
    const QList<QPointF>& getPts_mmList(){return pts_mmList;}
    const QList<QPointF>& getPts_pxList(){return pts_pxList;}
    const QList<float>& getPtsValList(){return ptsValList;}
    float getTotal(){return total;}
    TypeRoi getType(){return type;}

    virtual void setFlags(QGraphicsItem::GraphicsItemFlags){}
    virtual void addToScene(MScene*){}
    virtual void removeFromScene(){}
};

class RoiCirc: public QGraphicsPolygonItem, public Roi{
public:
    RoiCirc(Roi roi): Roi(roi){} // default constructor removed
    ~RoiCirc(){}

    void setFlags(GraphicsItemFlags flags){QGraphicsPolygonItem::setFlags(flags);}

    void addToScene(MScene* scene_p);
    void removeFromScene(){if (scene()) scene()->removeItem(this);}
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
};

class RoiCurve: public QGraphicsPathItem, public Roi{
public:
    RoiCurve(Roi roi): Roi(roi){}
    ~RoiCurve(){}

    void setFlags(GraphicsItemFlags flags){QGraphicsPathItem::setFlags(flags);}

    void addToScene(MScene *scene_p);
    void removeFromScene(){if (scene()) scene()->removeItem(this);}
};

class RoiRect: public QGraphicsRectItem, public Roi{
public:
    RoiRect(Roi roi): Roi(roi){}
    ~RoiRect(){}

    void setFlags(GraphicsItemFlags flags){QGraphicsRectItem::setFlags(flags);}

    void addToScene(MScene* scene_p);
    void removeFromScene(){if (scene()) scene()->removeItem(this);}
};

#endif // ROI_H

