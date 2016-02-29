#ifndef IMAGE_H
#define IMAGE_H

#include <QGraphicsScene>

#include <dcmtk/dcmdata/dctk.h>

#include "roi.h"
#include "graphics.h"

// Here are specific definitions for images

struct resolution_s{
    unsigned int width;
    unsigned int height;
};

struct metaInfoGlobal_s{
    OFString patientName;
    OFString modality;
    OFString seriesDescription;
    resolution_s resolution;
    unsigned int nbSlices;
};

struct pixelSpacing_s{
    Float64 verticalSpacing;
    Float64 horizontalSpacing;
};

struct slice_s{
    QString fileName;
    DcmFileFormat dcmFileFormat;
    Float64 sliceLocation;
    pixelSpacing_s pixelSpacing;
    MScene* scene_p;
    //QGraphicsPixmapItem* pixmap_p;
    MPixmapItem* pixmap_p;
    QList<Roi*> roiList;
};

#endif // IMAGE_H

