#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsSimpleTextItem>

#include "image.h"
#include "roi.h"

enum class ViewMode: char{MOVE, SELECT, EDIT};

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static ViewMode viewMode(){return viewmode;}
    static bool isAnonymized(){return anonymized;}

private slots:
    void on_actionOpen_triggered();
    void on_sliderImSelector_valueChanged(int newNumSlice);

    void on_actionImport_ROI_s_triggered();

    void on_actionDelete_all_ROIs_triggered();

    void on_action_Select_triggered(bool checked);

    void on_action_Move_triggered(bool checked);

    void on_action_Edit_triggered(bool checked);

    void on_action_Anonymize_triggered(bool checked);

private:
    Ui::MainWindow *ui;

    static ViewMode viewmode;
    static bool anonymized;

    metaInfoGlobal_s metaInfoGlobal;
    QList<slice_s> slicesList;

    //QList<roiImage_s> roiImageList;

    QGraphicsScene* sceneWelcomeText_p; // stay with QGraphics scene to avoid mouse effects
    QGraphicsSimpleTextItem* welcomeTextItem_p;

    void setViewMode(ViewMode newViewMode){viewmode = newViewMode;}

    void openImages();
    bool parseSlices(metaInfoGlobal_s& metaInfoGlobalTmp, std::list<slice_s>& slicesListTmp);
    bool loadImages(metaInfoGlobal_s& metaInfoGlobalTmp, std::list<slice_s>& slicesListTmp);

    void importROIs();
    void deleteImportingROIs(QList<roiImport_s>& roiImageListTmp);
    void displayAllROIs();
    void deleteAllROIs();
};

#endif // MAINWINDOW_H
