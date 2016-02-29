#include <sstream>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "roi.h"
#include "graphics.h"

// dcmtk
//#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dctk.h>
//#include <dcmtk/dcmdata/dcdict.h>
//#include <dcmtk/dcmdata/dcfilefo.h>
//#include <dcmtk/dcmdata/dcdeftag.h>

// libxml2
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <QFileDialog>

using namespace std;

bool compare_SL(const slice_s& first, const slice_s& second){
    return (first.sliceLocation < second.sliceLocation);
}

// Constructor
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    metaInfoGlobal(),
    slicesList()
{
    ui->setupUi(this);
    GlobalDcmDataDictionary dcmdatadict;
    if (!dcmdatadict.isDictionaryLoaded())
        cerr << tr("DICOM dictionary not loaded. Unexpected behaviour should happen").toStdString() << endl;

    sceneWelcomeText_p = new QGraphicsScene();
    welcomeTextItem_p = sceneWelcomeText_p->addSimpleText("Load an image first");
    welcomeTextItem_p->setBrush(QBrush(Qt::white));
    ui->imageView->setScene(sceneWelcomeText_p);
    ui->statusBar->showMessage("Please open a file");
}

// Destructor
MainWindow::~MainWindow()
{
    delete sceneWelcomeText_p;
    if (!slicesList.isEmpty()) for (int i = 0; i < slicesList.size(); i++) delete slicesList[i].scene_p;
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    if (Roi::getNbRois())
    {
        // TODO: ask for removing the rois if not saved
        deleteAllROIs();
    }
    openImages();
}

void MainWindow::on_actionImport_ROI_s_triggered()
{
    importROIs();
    displayAllROIs();
}

void MainWindow::on_actionDelete_all_ROIs_triggered()
{
    deleteAllROIs();
}

void MainWindow::openImages()
{
    QStringList fileNamesTmp = QFileDialog::getOpenFileNames(this, tr("Open image(s)"), ".", tr("DICOM files (*.dcm);;All files (*)"));
    if (!fileNamesTmp.isEmpty())
    {
        // Building temporary data
        metaInfoGlobal_s metaInfoGlobalTmp;
        list<slice_s> slicesListTmp;

        metaInfoGlobalTmp.nbSlices = fileNamesTmp.size();
        unsigned int i;
        for (i = 0; i < metaInfoGlobalTmp.nbSlices; i++)
        {
            slicesListTmp.push_back(slice_s());
            slicesListTmp.back().fileName = fileNamesTmp[i];
            if (slicesListTmp.back().dcmFileFormat.loadFile(slicesListTmp.back().fileName.toStdString().c_str()).bad())
            {
                cerr << (tr("Error while loading the file ") + slicesListTmp.back().fileName).toStdString() << endl;
                ui->statusBar->showMessage(tr("Failed to open ") + slicesListTmp.back().fileName);
                return;
            }
        }
        // All things went well so far

        // Verify multislice & display
        if (parseSlices(metaInfoGlobalTmp, slicesListTmp))
        {
            // Starting from here, the slices are sorted in ascending slice location order!
            if (loadImages(metaInfoGlobalTmp, slicesListTmp))
            {
                // OK. We can clear old data
                if (!slicesList.isEmpty()) for (i = 0; (int)i < slicesList.size(); i++) delete slicesList[i].scene_p;
                // Things went well, let's import new data
                metaInfoGlobal = metaInfoGlobalTmp;
                slicesList = QList<slice_s>::fromStdList(slicesListTmp); // convert to QList which allows indexed access
                // Display new images
                if (welcomeTextItem_p)
                {
                    sceneWelcomeText_p->removeItem(welcomeTextItem_p); //clearing scene
                    welcomeTextItem_p = nullptr;
                    delete sceneWelcomeText_p;
                    sceneWelcomeText_p = nullptr;
                }
                ui->imageView->resetTransform();
                ui->imageView->setScene(slicesList[0].scene_p);
                ui->imageView->scale(slicesList[0].pixelSpacing.horizontalSpacing/slicesList[0].pixelSpacing.verticalSpacing, 1.); // x = horizontal. Displaying vertical in real scale (dilating horizontal)

                ui->imageView->setMinimumSize(ui->imageView->sizeHint()); // comment if too big

                ui->sliderImSelector->setValue(1);
                ui->sliderImSelector->setMaximum(metaInfoGlobal.nbSlices);
                ui->labelImSelCur->setNum(1);
                ui->labelImSelLast->setNum((int)metaInfoGlobal.nbSlices);

                // Display metaInfo
                //ui->labelPatientName->setText("<Anonymized>");
                if (!anonymized) ui->labelPatientName->setText(QString(metaInfoGlobal.patientName.c_str()));
                ui->labelModalityValue->setText(QString(metaInfoGlobal.modality.c_str()));
                ui->labelSeriesDescValue->setText(QString(metaInfoGlobal.seriesDescription.c_str()));
                ui->labelImageRes->setText((std::to_string(metaInfoGlobal.resolution.width) + " x " + std::to_string(metaInfoGlobal.resolution.height)).c_str());
                ui->labelSliceLocationValue->setText(QString(std::to_string(slicesList[0].sliceLocation).c_str()));

                // Activate the ROI import action menu
                ui->actionImport_ROI_s->setEnabled(true);
            }
        }
    }
    else
    {
        ui->statusBar->showMessage(tr("No file selected"));
    }
}

bool MainWindow::parseSlices(metaInfoGlobal_s& metaInfoGlobalTmp, list<slice_s>& slicesListTmp)
{
    list<slice_s>::iterator it = slicesListTmp.begin();
    // Check the consistency of the multislice sequence
    if (metaInfoGlobalTmp.nbSlices > 1)
    {
        const char* seriesInstanceUIDChar = nullptr;
        if (it->dcmFileFormat.getDataset()->findAndGetString(DCM_SeriesInstanceUID, seriesInstanceUIDChar).good())
        {
            QString seriesInstanceUIDString(seriesInstanceUIDChar);
            const char* seriesInstanceUIDCharTmp = nullptr;
            for (++it; it != slicesListTmp.end(); it++)
            {
                it->dcmFileFormat.getDataset()->findAndGetString(DCM_SeriesInstanceUID, seriesInstanceUIDCharTmp);
                if (seriesInstanceUIDString != QString(seriesInstanceUIDCharTmp))
                {
                    cerr << tr("Not the same Series Instance UID in the whole sequence").toStdString() << endl;
                    ui->statusBar->showMessage(tr("Incoherent sequence (Series Instance UID)"));
                    return false;
                }
            }
        }
        else
        {
            cerr << tr("Unable to read the Series Instance UID and to verify the multislice consistency in consequence").toStdString() << endl;
            return false;
        }
    }
    // Defines the slice location method (Slice Location or Image Position Patient)
    Float64 sliceLocation = 0.;
    Float64 verticalSpacing = 1.;
    Float64 horizontalSpacing = 1.;
    bool sliceLocMethod = true;
    DcmStack resultStack;
    if (slicesListTmp.front().dcmFileFormat.getDataset()->search(DCM_SliceLocation, resultStack).condition() == EC_TagNotFound)
        sliceLocMethod = false; // Image Position Patient
    // Gets all slice dependant attributes
    for (it = slicesListTmp.begin(); it != slicesListTmp.end(); it++)
    {
        // Slice Location
        if (sliceLocMethod)
        {
            if (it->dcmFileFormat.getDataset()->findAndGetFloat64(DCM_SliceLocation, sliceLocation).good())
            {
                it->sliceLocation = sliceLocation;
            }
            else
            {
                cerr << (tr("Unable to read the Slice Location, file ") + it->fileName).toStdString() << ". Images order is not guaranteed" << endl;
                ui->statusBar->showMessage(tr("Unable to read the slice location, file ") + it->fileName);
                it->sliceLocation = 0.;
            }
        }
        else // Image Position Patient [Z]
        {
            if (it->dcmFileFormat.getDataset()->findAndGetFloat64(DCM_ImagePositionPatient, sliceLocation, 2).good())
            {
                it->sliceLocation = sliceLocation;
            }
            else
            {
                cerr << (tr("Unable to read the Image Position Patient, file ") + it->fileName).toStdString() << ". Images order is not guaranteed" << endl;
                ui->statusBar->showMessage(tr("Unable to read the slice location, file ") + it->fileName);
                it->sliceLocation = 0.;
            }
        }
        // Pixel Spacing
        if (it->dcmFileFormat.getDataset()->findAndGetFloat64(DCM_PixelSpacing, verticalSpacing, 0).good())
        {
            it->pixelSpacing.verticalSpacing = verticalSpacing;
        }
        else
        {
            cerr << (tr("Unable to read the vertical Pixel Spacing, file ") + it->fileName + ". Assuming 1.0").toStdString() << endl;
            ui->statusBar->showMessage(tr("Unable to read the vertical Pixel Spacing, file ") + it->fileName + ". Assuming 1.0");
            it->pixelSpacing.verticalSpacing = 1.;
        }
        if (it->dcmFileFormat.getDataset()->findAndGetFloat64(DCM_PixelSpacing, horizontalSpacing, 1).good())
        {
            it->pixelSpacing.horizontalSpacing = horizontalSpacing;
        }
        else
        {
            cerr << (tr("Unable to read the horizontal Pixel Spacing, file ") + it->fileName + ". Assuming 1.0").toStdString() << endl;
            ui->statusBar->showMessage(tr("Unable to read the horizontal Pixel Spacing, file ") + it->fileName + ". Assuming 1.0");
            it->pixelSpacing.horizontalSpacing = 1.;
        }
    }
    // Global meta data (slice independant)
    OFString patientName;
    OFString modality;
    OFString seriesDescription;
    Uint16 rows, columns;
    // Patient name
    if (slicesListTmp.front().dcmFileFormat.getDataset()->findAndGetOFString(DCM_PatientName, patientName).good())
    {
        patientName.replace(patientName.find("^"), 1, " ");
        metaInfoGlobalTmp.patientName = patientName;
    }
    else
    {
        metaInfoGlobalTmp.patientName = "ERROR";
        cerr << tr("Unable to read the Patient Name").toStdString() << endl;
    }
    // Modality
    if (slicesListTmp.front().dcmFileFormat.getDataset()->findAndGetOFString(DCM_Modality, modality).good())
        metaInfoGlobalTmp.modality = modality;
    else
    {
        metaInfoGlobalTmp.modality = "ERROR";
        cerr << tr("Unable to read the Modality").toStdString() << endl;
    }
    // Series Description
    if (slicesListTmp.front().dcmFileFormat.getDataset()->findAndGetOFString(DCM_SeriesDescription, seriesDescription).good())
        metaInfoGlobalTmp.seriesDescription = seriesDescription;
    else
    {
        metaInfoGlobalTmp.seriesDescription = "ERROR";
        cerr << tr("Unable to read the Series Description").toStdString() << endl;
    }
    // Resolution
    if (slicesListTmp.front().dcmFileFormat.getDataset()->findAndGetUint16(DCM_Columns, columns).good())
    {
        metaInfoGlobalTmp.resolution.width = columns;
    }
    else
    {
        cerr << tr("Unable to read the image width").toStdString() << endl;
        ui->statusBar->showMessage(tr("Unable to read the image width"));
        return false;
    }
    if (slicesListTmp.front().dcmFileFormat.getDataset()->findAndGetUint16(DCM_Rows, rows).good())
    {
        metaInfoGlobalTmp.resolution.height = rows;
    }
    else
    {
        cerr << tr("Unable to read the image height").toStdString() << endl;
        ui->statusBar->showMessage(tr("Unable to read the image height"));
        return false;
    }
    // Re-order with respect to slice locations ascending order
    slicesListTmp.sort(compare_SL); // gives compare_SL comparing function
    return true;
}

bool MainWindow::loadImages(metaInfoGlobal_s& metaInfoGlobalTmp, list<slice_s>& slicesListTmp)
{
    Uint16 pixelLength = 0;
    list<slice_s>::iterator it = slicesListTmp.begin();
    if (it->dcmFileFormat.getDataset()->findAndGetUint16(DCM_BitsAllocated, pixelLength).good())
    {
        OFCondition status;
        const void* imageData = nullptr;
        uchar* imchararray = nullptr;
        int nbpixtmp = metaInfoGlobalTmp.resolution.width * metaInfoGlobalTmp.resolution.height;
        Uint16 maxPixel = 0;
        float reducRatio = 0.;
        QImage myImage;
        QPixmap myPixmap;
        for (; it != slicesListTmp.end(); it++)
        {
            if (pixelLength == 8)
            {
                //status = it->dcmFileFormat.getDataset()->findAndGetUint8Array(DCM_PixelData, (const Uint8*&)imageData);
                cerr << tr("8 Bits Allocated attribute not supported yet").toStdString() << endl;
                ui->statusBar->showMessage(tr("8 Bits Allocated attribute not yet supported"));
                return false;
            }
            else if (pixelLength == 16)
            {
                status = it->dcmFileFormat.getDataset()->findAndGetUint16Array(DCM_PixelData, (const Uint16*&)imageData);
            }
            else
            {
                cerr << tr("Invalid Bits Allocated attribute").toStdString() << endl;
                return false;
            }
            if (status.good())
            {
                imchararray = new uchar[nbpixtmp];
                int j;
                if (it->dcmFileFormat.getDataset()->findAndGetUint16(DCM_LargestImagePixelValue, maxPixel).bad())
                {
                    cerr << tr("Unable to read the Largest Image Pixel Value. Searching it").toStdString() << endl;
                    for (j = 0; j < nbpixtmp; j++)
                    {
                        if ((int)(((const Uint16*)imageData)[j]) > maxPixel) maxPixel = (int)(((const Uint16*)imageData)[j]);
                    }
                }
                reducRatio = ((1<<8)-1.0)/maxPixel;
                // Convert to uchar array for QImage creation
                for (j = 0; j < nbpixtmp; j++)
                {
                    imchararray[j] = (uchar)(reducRatio * (float)(((const Uint16*)imageData)[j]));
                }

                // Building the scene
                myImage = QImage(imchararray, metaInfoGlobalTmp.resolution.width, metaInfoGlobalTmp.resolution.height, QImage::Format_Indexed8);
                myPixmap = QPixmap::fromImage(myImage);
                delete[] imchararray; // freeing memory
                it->scene_p = new MScene(this); // here can happen problem (this)
                //it->scene_p = new Scene(this);
                it->pixmap_p = new MPixmapItem(myPixmap); // no QGraphicsItem parent
                //it->pixmap_p = (PixmapItem*)it->scene_p->addPixmap(myPixmap);
                it->scene_p->addItem(it->pixmap_p);
            }
            else
            {
                cerr << (tr("Unable to read the pixels, file ") + it->fileName).toStdString() << OFendl;
                ui->statusBar->showMessage(tr("Unable to read the pixels, file ") + it->fileName);
                return false;
            }
        }
    }
    else
    {
        cerr << tr("Unable to read the Bits Allocated attribute").toStdString() << endl;
        ui->statusBar->showMessage(tr("Unable to read the pixel length"));
        return false;
    }
    return true;
}

void MainWindow::on_sliderImSelector_valueChanged(int newNumSlice)
{
    ui->imageView->resetTransform();
    ui->imageView->setScene(slicesList[newNumSlice-1].scene_p);
    ui->imageView->scale(slicesList[newNumSlice-1].pixelSpacing.horizontalSpacing/slicesList[newNumSlice-1].pixelSpacing.verticalSpacing, 1.); // x = horizontal
    ui->labelImSelCur->setNum(newNumSlice);
    ui->labelSliceLocationValue->setText(QString(to_string(slicesList[newNumSlice-1].sliceLocation).c_str()));
}

void MainWindow::importROIs()
{
    QString fileName = QFileDialog::getOpenFileName(this->parentWidget(), tr("Open ROI(s)"), ".", tr("XML files (*.xml);;All files (*)"));
    if (!fileName.isEmpty())
    {
        xmlDocPtr doc = xmlParseFile(fileName.toStdString().c_str());
        if (doc == NULL)
        {
            cerr << tr("XML file not parsed successfully").toStdString() << endl;
            ui->statusBar->showMessage(tr("XML file not parsed successfully"));
            return;
        }
        xmlNodePtr curNode = xmlDocGetRootElement(doc); // pointer to "plist" element
        if (curNode == NULL)
        {
            cerr << tr("Empty ROI file").toStdString() << endl;
            ui->statusBar->showMessage(tr("Empty ROI file"));
            xmlFreeDoc(doc);
            return;
        }
        curNode = curNode->children; // ptr to (before?) "dict"
        while ((curNode != NULL) && xmlStrcmp(curNode->name, (const xmlChar*)"dict")){ // find "dict"
            curNode = curNode->next;
        }
        if (curNode == NULL)
        {
            cerr << tr("Invalid ROI XML file: no first \"dict\" node").toStdString() << endl;
            ui->statusBar->showMessage("Invalid ROI XML file");
            xmlFreeDoc(doc);
            return;
        }
        curNode = curNode->children;
        while ((curNode != NULL) && xmlStrcmp(curNode->name, (const xmlChar*)"array")){ // find "array"
            curNode = curNode->next;
        }
        if (curNode == NULL)
        {
            cerr << tr("Invalid ROI XML file: no first \"array\" node").toStdString() << endl;
            ui->statusBar->showMessage("Invalid ROI XML file");
            xmlFreeDoc(doc);
            return;
        }
        curNode = curNode->children; // first child of first "array"
        // Strating from here, one "dict" per image
        xmlNodePtr curNodeDictImage = curNode;
        xmlNodePtr curNodeKeyImage = NULL;
        xmlNodePtr curNodeDictRoi = NULL;
        xmlNodePtr curNodeKeyRoi = NULL;
        xmlNodePtr curNodePoint = NULL;
        FsmXmlImage stateImage;
        FsmXmlRoi stateRoi;
        xmlChar* content = NULL;
        unsigned int uintTmp;
        float realTmp;
        QPointF pointTmp;
        QList<roiImport_s> roiImageListTmp;
        while (curNodeDictImage != NULL) // one "dict"/image
        {
            if (!xmlStrcmp(curNodeDictImage->name, (const xmlChar*)"dict"))
            {
                curNodeKeyImage = curNodeDictImage->children;
                stateImage = FsmXmlImage::KEY;
                roiImport_s roiImageTmp;
                while (curNodeKeyImage != NULL)
                {
                    switch (stateImage) {
                    case FsmXmlImage::KEY:
                        if (!xmlStrcmp(curNodeKeyImage->name, (const xmlChar*)"key")) // find "key" for key decrypting
                        {
                            content = xmlNodeListGetString(doc, curNodeKeyImage->children, 1);
                            if (!xmlStrcmp(content, (const xmlChar*)"ImageIndex"))
                                stateImage = FsmXmlImage::IMAGE_INDEX;
                            else if (!xmlStrcmp(content, (const xmlChar*)"NumberOfROIs"))
                                stateImage = FsmXmlImage::NUMBER_ROIS;
                            else if (!xmlStrcmp(content, (const xmlChar*)"ROIs"))
                                stateImage = FsmXmlImage::ROIS;
                            else
                                cerr << tr("Unrecognized key: ").toStdString() << content << endl;
                            xmlFree(content);
                        }
                        break; // if not "key", may be "text", so let's play another "while" loop
                    case FsmXmlImage::IMAGE_INDEX:
                        if (!xmlStrcmp(curNodeKeyImage->name, (const xmlChar*)"integer")) // find "integer" for "ImageIndex"
                        {
                            content = xmlNodeListGetString(doc, curNodeKeyImage->children, 1);
                            stringstream(string((const char*)content)) >> uintTmp;
                            xmlFree(content);
                            if (uintTmp >= (unsigned int)slicesList.size())
                            {
                                cerr << tr("Roi outside of the image range").toStdString() << " (ImageIndex = " << uintTmp << ")" << endl;
                                xmlFreeDoc(doc);
                                deleteImportingROIs(roiImageListTmp);
                                return;
                            }
                            roiImageTmp.imageIndex = uintTmp;
                            stateImage = FsmXmlImage::KEY;
                        }
                        break;
                    case FsmXmlImage::NUMBER_ROIS:
                        // Not used anymore
                        /*if (!xmlStrcmp(curNodeKeyImage->name, (const xmlChar*)"integer"))
                        {
                            content = xmlNodeListGetString(doc, curNodeKeyImage->children, 1);
                            stringstream(string((const char*)content)) >> uintTmp;
                            roiImageTmp.nbRoisInImage = uintTmp;
                            xmlFree(content);
                            stateImage = FsmXmlImage::KEY;
                        }*/
                        stateImage = FsmXmlImage::KEY;
                        break;
                    case FsmXmlImage::ROIS:
                        if (!xmlStrcmp(curNodeKeyImage->name, (const xmlChar*)"array"))
                        {
                            curNodeDictRoi = curNodeKeyImage->children;
                            while (curNodeDictRoi != NULL)
                            {
                                if (!xmlStrcmp(curNodeDictRoi->name, (const xmlChar*)"dict")) // should happen NumberOfROIs
                                {
                                    curNodeKeyRoi = curNodeDictRoi->children;
                                    stateRoi = FsmXmlRoi::KEY;
                                    Roi roiTmp;
                                    while (curNodeKeyRoi != NULL)
                                    {
                                        switch (stateRoi){
                                        case FsmXmlRoi::KEY:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"key"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                if (!xmlStrcmp(content, (const xmlChar*)"Area"))
                                                    stateRoi = FsmXmlRoi::AREA;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Center"))
                                                    stateRoi = FsmXmlRoi::CENTER;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Dev"))
                                                    stateRoi = FsmXmlRoi::DEV;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"IndexInImage"))
                                                    stateRoi = FsmXmlRoi::INDEX;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Length"))
                                                    stateRoi = FsmXmlRoi::LENGTH;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Max"))
                                                    stateRoi = FsmXmlRoi::MAX;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Mean"))
                                                    stateRoi = FsmXmlRoi::MEAN;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Min"))
                                                    stateRoi = FsmXmlRoi::MIN;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Name"))
                                                    stateRoi = FsmXmlRoi::NAME;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"NumberOfPoints"))
                                                    stateRoi = FsmXmlRoi::NUMBER_PTS;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Point_mm"))
                                                    stateRoi = FsmXmlRoi::POINT_MM;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Point_px"))
                                                    stateRoi = FsmXmlRoi::POINT_PX;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Point_value"))
                                                    stateRoi = FsmXmlRoi::POINT_VAL;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Total"))
                                                    stateRoi = FsmXmlRoi::TOTAL;
                                                else if (!xmlStrcmp(content, (const xmlChar*)"Type"))
                                                    stateRoi = FsmXmlRoi::TYPE;
                                                else
                                                    cerr << tr("Unrecognized key: ").toStdString() << content << endl;
                                                xmlFree(content);
                                            }
                                            break;
                                        case FsmXmlRoi::AREA:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"real")) // find "real" for "Area"
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                stringstream(string((const char*)content)) >> realTmp;
                                                roiTmp.setArea(realTmp);
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::CENTER:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"string"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                istringstream issTmp(string((const char*)content+1)); // skip '('
                                                string token;
                                                getline(issTmp, token, ',');
                                                stringstream(token) >> realTmp;
                                                pointTmp.setX(realTmp);
                                                getline(issTmp, token, ',');
                                                stringstream(token) >> realTmp;
                                                pointTmp.setY(realTmp);
                                                roiTmp.setCenter(pointTmp);
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::DEV:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"real"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                stringstream(string((const char*)content)) >> realTmp;
                                                roiTmp.setDev(realTmp);
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::INDEX:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"integer"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                stringstream(string((const char*)content)) >> uintTmp;
                                                roiTmp.setIndexinImage(uintTmp);
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::LENGTH:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"real"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                stringstream(string((const char*)content)) >> realTmp;
                                                roiTmp.setLength(realTmp);
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::MAX:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"real"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                stringstream(string((const char*)content)) >> realTmp;
                                                roiTmp.setMax(realTmp);
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::MEAN:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"real"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                stringstream(string((const char*)content)) >> realTmp;
                                                roiTmp.setMean(realTmp);
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::MIN:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"real"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                stringstream(string((const char*)content)) >> realTmp;
                                                roiTmp.setMin(realTmp);
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::NAME:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"string"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                roiTmp.setName((const char*)content);
                                                //cout << roiTmp.getName().toStdString(); // TEMP
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::NUMBER_PTS:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"integer"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                stringstream(string((const char*)content)) >> uintTmp;
                                                roiTmp.setNumberPts(uintTmp);
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::POINT_MM:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"array"))
                                            {
                                                curNodePoint = curNodeKeyRoi->children;
                                                while (curNodePoint != NULL)
                                                {
                                                    if (!xmlStrcmp(curNodePoint->name, (const xmlChar*)"string"))
                                                    {
                                                        content = xmlNodeListGetString(doc, curNodePoint->children, 1);
                                                        istringstream issTmp(string((const char*)content+1)); // skip '('
                                                        string token;
                                                        getline(issTmp, token, ',');
                                                        stringstream(token) >> realTmp;
                                                        pointTmp.setX(realTmp);
                                                        getline(issTmp, token, ',');
                                                        stringstream(token) >> realTmp;
                                                        pointTmp.setY(realTmp);
                                                        roiTmp.addPoint_mm(pointTmp);
                                                        xmlFree(content);
                                                    }
                                                    curNodePoint = curNodePoint->next;
                                                }
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::POINT_PX:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"array"))
                                            {
                                                curNodePoint = curNodeKeyRoi->children;
                                                while (curNodePoint != NULL)
                                                {
                                                    if (!xmlStrcmp(curNodePoint->name, (const xmlChar*)"string"))
                                                    {
                                                        content = xmlNodeListGetString(doc, curNodePoint->children, 1);
                                                        istringstream issTmp(string((const char*)content+1)); // skip '('
                                                        string token;
                                                        getline(issTmp, token, ',');
                                                        stringstream(token) >> realTmp;
                                                        pointTmp.setX(realTmp);
                                                        getline(issTmp, token, ')');
                                                        stringstream(token) >> realTmp;
                                                        pointTmp.setY(realTmp);
                                                        roiTmp.addPoint_px(pointTmp);
                                                        xmlFree(content);
                                                    }
                                                    curNodePoint = curNodePoint->next;
                                                }
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::POINT_VAL:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"array"))
                                            {
                                                curNodePoint = curNodeKeyRoi->children;
                                                while (curNodePoint != NULL)
                                                {
                                                    if (!xmlStrcmp(curNodePoint->name, (const xmlChar*)"real"))
                                                    {
                                                        content = xmlNodeListGetString(doc, curNodePoint->children, 1);
                                                        stringstream(string((const char*)content)) >> realTmp;
                                                        roiTmp.addPoint_val(realTmp);
                                                        xmlFree(content);
                                                    }
                                                    curNodePoint = curNodePoint->next;
                                                }
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::TOTAL:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"real"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                stringstream(string((const char*)content)) >> realTmp;
                                                roiTmp.setTotal(realTmp);
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        case FsmXmlRoi::TYPE:
                                            if (!xmlStrcmp(curNodeKeyRoi->name, (const xmlChar*)"integer"))
                                            {
                                                content = xmlNodeListGetString(doc, curNodeKeyRoi->children, 1);
                                                stringstream(string((const char*)content)) >> uintTmp;
                                                roiTmp.setType(TypeRoi(uintTmp));
                                                xmlFree(content);
                                                stateRoi = FsmXmlRoi::KEY;
                                            }
                                            break;
                                        default:
                                            break;
                                        }
                                        curNodeKeyRoi = curNodeKeyRoi->next; // next key in ROI
                                    }
                                    // roiTmp complete
                                    switch (roiTmp.getType()) {
                                    case TypeRoi::CIRC:
                                        roiImageTmp.roiList.append(new RoiCirc(roiTmp));
                                        break;
                                    case TypeRoi::LIN:
                                        roiImageTmp.roiList.append(new RoiCurve(roiTmp));
                                        break;
                                    case TypeRoi::RECT:
                                        roiImageTmp.roiList.append(new RoiRect(roiTmp));
                                        break;
                                    default:
                                        cerr << tr("Unrecognized ROI type: ").toStdString() << (unsigned char)roiTmp.getType() << ". ROI ignored" << endl;
                                        break;
                                    }
                                    if (!roiImageTmp.roiList.isEmpty())
                                    {
                                        // If ROI not added, flags are overwritten: OK
                                        roiImageTmp.roiList.last()->setFlags(QGraphicsItem::ItemIsMovable |
                                                                             QGraphicsItem::ItemIsSelectable |
                                                                             QGraphicsItem::ItemIsFocusable);
                                    }
                                }
                                curNodeDictRoi = curNodeDictRoi->next; // next ROI
                            }
                            stateImage = FsmXmlImage::KEY;
                        }
                        break;
                    default:
                        break;
                    }
                    curNodeKeyImage = curNodeKeyImage->next; // next key in image
                }
                // roiImageTmp complete
                roiImageListTmp.append(roiImageTmp);
            }
            curNodeDictImage = curNodeDictImage->next; // next image
        }
        // Things went well: merging the new rois
        for (int i=0; i < roiImageListTmp.size(); i++)
        {
            slicesList[roiImageListTmp[i].imageIndex].roiList << roiImageListTmp[i].roiList;
        }
        ui->actionDelete_all_ROIs->setEnabled(true); // Enable the menu "Delete all ROIs"
    }
    else
    {
        ui->statusBar->showMessage(tr("No file selected"));
    }
}

void MainWindow::deleteImportingROIs(QList<roiImport_s>& roiImageListTmp)
{
    for (int i=0; i < roiImageListTmp.size(); i++)
    {
        for (int j=0; j < roiImageListTmp[i].roiList.size(); j++)
        {
            delete roiImageListTmp[i].roiList[j];
        }
    }
}

void MainWindow::displayAllROIs()
{
    for (int i=0; i < slicesList.size(); i++)
    {
        if (!slicesList[i].roiList.isEmpty())
        {
            for (int j=0; j < slicesList[i].roiList.size(); j++)
            {
                slicesList[i].roiList[j]->addToScene(slicesList[i].scene_p);
                //slicesList[i].scene_p->addItem(slicesList[i].roiList[j]);
            }
        }
    }
}

void MainWindow::deleteAllROIs()
{
    int i;
    // Remove from scene part. Could be in a different function
    for (i=0; i < slicesList.size(); i++)
    {
        for (int j=0; j < slicesList[i].roiList.size(); j++)
        {
            slicesList[i].roiList[j]->removeFromScene();
        }
    }
    for (i=0; i < slicesList.size(); i++)
    {
        if (!slicesList[i].roiList.isEmpty())
        {
            for (int j=0; j < slicesList[i].roiList.size(); j++) delete slicesList[i].roiList[j];
            slicesList[i].roiList.clear();
        }
    }
    ui->actionDelete_all_ROIs->setDisabled(true);
}

void MainWindow::on_action_Select_triggered(bool checked)
{
    if (!checked && (viewMode() == ViewMode::SELECT))
    {
        // Stay in SELECT mode
        ui->action_Select->setChecked(true);
    }
    else // checked && (viewMode != ViewMode::SELECT), other cases should not happen
    {
        setViewMode(ViewMode::SELECT);
        ui->action_Move->setChecked(false);
        ui->action_Edit->setChecked(false);
    }
}

void MainWindow::on_action_Move_triggered(bool checked)
{
    if (!checked && (viewMode() == ViewMode::MOVE))
    {
        ui->action_Move->setChecked(true);
    }
    else
    {
        setViewMode(ViewMode::MOVE);
        ui->action_Select->setChecked(false);
        ui->action_Edit->setChecked(false);
    }
}

void MainWindow::on_action_Edit_triggered(bool checked)
{
    if (!checked && (viewMode() == ViewMode::EDIT))
    {
        ui->action_Edit->setChecked(true);
    }
    else
    {
        setViewMode(ViewMode::EDIT);
        ui->action_Select->setChecked(false);
        ui->action_Move->setChecked(false);
    }
}

void MainWindow::on_action_Anonymize_triggered(bool checked)
{
    anonymized = checked;
    if (anonymized)
    {
        ui->labelPatientName->setText(QString("<Anonymized>"));
    }
    else
    {
        ui->labelPatientName->setText(QString(metaInfoGlobal.patientName.c_str()));
    }
}
