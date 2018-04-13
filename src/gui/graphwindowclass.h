#ifndef GRAPHWINDOWCLASS_H
#define GRAPHWINDOWCLASS_H

#include <QMainWindow>

#include "TMathBase.h"

class MainWindow;
class RasterWindowGraphClass;
class TH2;
class TH1D;
class TGraph;
class TGraph2D;
class QGraphicsView;
class AToolboxScene;
class QListWidgetItem;
class TObject;
class TTree;

class DrawObjectStructure
{
public:
    void setPointer(TObject* p) {Pointer = p;}
    TObject* getPointer() {return Pointer;}

    void setOptions(QString str) {Options = str;}
    void setOptions(const char* chars) {Options = chars;}
    QString getOptions() {return Options;}

    DrawObjectStructure() {Pointer=0;}
    DrawObjectStructure(TObject* pointer, const char* options) {Pointer = pointer; Options = options;}
    DrawObjectStructure(TObject* pointer, QString options) {Pointer = pointer; Options = options;}

private:
    TObject* Pointer;
    QString Options;
};

class BasketItemClass
{
public:
    QString Name;
    QString Type;
    QVector<DrawObjectStructure> DrawObjects;

    BasketItemClass(QString name, QVector<DrawObjectStructure>* drawObjects);
    BasketItemClass(){}
    ~BasketItemClass();

    void clearObjects();
};

namespace Ui {
class GraphWindowClass;
}

class GraphWindowClass : public QMainWindow
{
    Q_OBJECT

public:
    explicit GraphWindowClass(QWidget *parent, MainWindow *mw);
    ~GraphWindowClass();

    QString LastDistributionShown;

    //Drawing
    void Draw(TObject* obj, const char* options = "", bool DoUpdate = true, bool TransferOwnership = true);  //registration should be skipped only for scripts!
    void DrawWithoutFocus(TObject* obj, const char* options = "", bool DoUpdate = true, bool TransferOwnership = true);  //registration should be skipped only for scripts!

    //canvas control
    void ShowAndFocus();
    void SetAsActiveRootWindow();
    void ClearRootCanvas();
    void UpdateRootCanvas();
    void SetModifiedFlag();
    void SetLog(bool X, bool Y);

    //Canvas size in actual coordinates of plotted data
    double getCanvasMinX();
    double getCanvasMaxX();
    double getCanvasMinY();
    double getCanvasMaxY();

    //Values presented to user in Range boxes
    double getMinX(bool *ok);
    double getMaxX(bool *ok);
    double getMinY(bool *ok);
    double getMaxY(bool *ok);
    double getMinZ(bool *ok);
    double getMaxZ(bool *ok);

    //extraction of coordinates from graph
    bool IsExtractionComplete();
    bool IsExtractionCanceled() {return ExtractionCanceled;}

    //commands to start extraction of shapes on canvas
    void ExtractX();  //start extraction of X coordinate from a 1D graph/histogram using mouse
    void Extract2DLine();  //start extraction of ABC line coordinate from a 2D graph/histogram using mouse
    void Extract2DEllipse();  //start extraction of (T)ellipse from a 2D graph/histogram using mouse
    void Extract2DBox();  //start extraction of 2D box (2 opposite corners)
    void Extract2DPolygon();  //start extraction of 2D polygon, extraction ends by right click (or doubleclick?)

    //commands to get results of extraction
    double extractedX();
    double extracted2DLineA();
    double extracted2DLineB();
    double extracted2DLineC();
    double extracted2DLineXstart();
    double extracted2DLineXstop();
    double extracted2DEllipseX();
    double extracted2DEllipseY();
    double extracted2DEllipseR1();
    double extracted2DEllipseR2();
    double extracted2DEllipseTheta();
    double extractedX1();
    double extractedY1();
    double extractedX2();
    double extractedY2();
    QList<double> extractedPolygon();

    //deprecated!
    TGraph* MakeGraph(const QVector<double> *x, const QVector<double> *y,
                      Color_t color, const char *XTitle, const char *YTitle,
                      int MarkerStyle=20, int MarkerSize=1, int LineStyle=1, int LineWidth = 2,
                      const char* options = "",
                      bool OnlyBuild = false);

    //use this to only construct!
    TGraph* ConstructTGraph(const QVector<double>& x, const QVector<double>& y) const;
    TGraph* ConstructTGraph(const QVector<double>& x, const QVector<double>& y,
                            const char *Title, const char *XTitle, const char *YTitle,
                            Color_t MarkerColor=2, int MarkerStyle=20, int MarkerSize=1,
                            Color_t LineColor=2,   int LineStyle=1,    int LineWidth=2) const;
    TGraph2D* ConstructTGraph2D(const QVector<double>& x, const QVector<double>& y, const QVector<double>& z) const;
    TGraph2D* ConstructTGraph2D(const QVector<double>& x, const QVector<double>& y, const QVector<double>& z,
                              const char *Title, const char *XTitle, const char *YTitle, const char *ZTitle,
                              Color_t MarkerColor=2, int MarkerStyle=20, int MarkerSize=1,
                              Color_t LineColor=2,   int LineStyle=1,    int LineWidth=2);

    void AddLine(double x1, double y1, double x2, double y2, int color, int width, int style);

    void OnBusyOn();
    void OnBusyOff();

    bool Extraction();

    void switchOffBasket();
    void ClearBasket();
    TObject *GetMainPlottedObject();
    void SaveGraph(QString fileName);    
    void EnforceOverlayOff();    
    void ClearDrawObjects_OnShutDown(); //precvents crash on shut down
    void RegisterTObject(TObject* obj); //ONLY use to register objects which were NOT drawn using Draw method of this window (but root Draw method instead)    
    void ShowTextPanel(const QString Text, bool bShowFrame=true, int AlignLeftCenterRight=0);

protected:
    void resizeEvent(QResizeEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    bool event(QEvent *event);
    void closeEvent(QCloseEvent *event);

public slots:
    // if the first object (no "same" option) was drawn without update, call this function after the manual update
    void UpdateControls(); //updates visualisation of the current master graph parameters
    void DoSaveGraph(QString name);
    void AddCurrentToBasket(QString name);
    void AddLegend(double x1, double y1, double x2, double y2, QString title);
    void AddText(QString text, bool bShowFrame, int Alignment_0Left1Center2Right);
    void on_pbAddLegend_clicked();
    void ExportTH2AsText(QString fileName); //for temporary script command

    QVector<double> Get2DArray(); //for temporary script command

    void DrawStrOpt(TObject* obj, QString options = "", bool DoUpdate = true);
    bool DrawTree(TTree* tree, const QString& what, const QString& cond, const QString& how, const QVariantList& binsAndRanges);

private slots:
    void Reshape();
    void on_lwBasket_customContextMenuRequested(const QPoint &pos);
    void on_lwBasket_itemDoubleClicked(QListWidgetItem *item);

    void on_cbToolBox_toggled(bool checked);
    void on_cobToolBox_currentIndexChanged(int index);
    void on_pbToolboxDragMode_clicked();

    //selBox
    void selBoxGeometryChanged();
    void selBoxResetGeometry(double halfW, double halfH);
    void selBoxControlsUpdated();

    void on_pbSelBoxToCenter_clicked();
    void on_pbSelBoxFGColor_clicked();
    void on_pbSelBoxBGColor_clicked();
    void rulerGeometryChanged();
    void rulerControlsP1Updated();
    void rulerControlsP2Updated();
    void rulerControlsLenAngleUpdated();
    void on_ledRulerTicksLength_editingFinished();
    void on_pbRulerFGColor_clicked();
    void on_pbRulerBGColor_clicked();
    void on_pbResetRuler_clicked();
    void on_pbAddToBasket_clicked();
    void on_cbGridX_toggled(bool checked);
    void on_cbGridY_toggled(bool checked);
    void on_cbLogX_toggled(bool checked);
    void on_cbLogY_toggled(bool checked);
    void on_ledXfrom_editingFinished();
    void on_ledXto_editingFinished();
    void on_ledYfrom_editingFinished();
    void on_ledYto_editingFinished();
    void on_ledZfrom_editingFinished();
    void on_ledZto_editingFinished();
    void on_cbShowLegend_toggled(bool checked);
    void on_pbZoom_clicked();
    void on_pbUnzoom_clicked();
    void on_leOptions_editingFinished();
    void on_pbXprojection_clicked();
    void on_pbYprojection_clicked();
    void on_cbShowBasket_toggled(bool checked);
    void on_pbBasketBackToLast_clicked();    
    void on_actionSave_image_triggered();
    void on_actionExport_data_as_text_triggered();
    void on_actionExport_data_using_bin_start_positions_TH1_triggered();
    void on_actionBasic_ROOT_triggered();
    void on_actionDeep_sea_triggered();
    void on_actionGrey_scale_triggered();
    void on_actionDark_body_radiator_triggered();
    void on_actionTwo_color_hue_triggered();
    void on_actionRainbow_triggered();
    void on_actionInverted_dark_body_triggered();
    void on_pbToolboxDragMode_2_clicked();
    void on_actionSave_root_object_triggered();
    void on_pbSmooth_clicked();
    void on_actionTop_triggered();
    void on_actionSide_triggered();
    void on_actionFront_triggered();
    void on_pbAttributes_clicked();
    void on_actionToggle_toolbar_toggled(bool arg1);
    void on_pbDensityDistribution_clicked();
    void on_actionEqualize_scale_XY_triggered();
    void on_ledRulerDX_editingFinished();
    void on_ledRulerDY_editingFinished();
    void on_cbShowFitParameters_toggled(bool checked);
    void on_pbXaveraged_clicked();
    void on_pbYaveraged_clicked();    
    void on_pbAddText_clicked();
    void on_pbRemoveLegend_clicked();
    void on_pbRemoveText_clicked();
    void on_pbFWHM_clicked();

    void on_ledAngle_customContextMenuRequested(const QPoint &pos);

private:
    Ui::GraphWindowClass *ui;
    MainWindow *MW;
    RasterWindowGraphClass *RasterWindow;
    QWidget *QWinContainer;
    bool ExtractionCanceled;
    int LastOptStat;

    QVector<DrawObjectStructure> DrawObjects;  //currently drawn
    QVector<DrawObjectStructure> MasterDrawObjects; //last draw made from outse of the graph window
    QVector< BasketItemClass > Basket; //container with user selected "drawings"
    int CurrentBasketItem;
    int BasketMode;

    QList<TObject*> tmpTObjects;
    TH1D* hProjection;  //for toolbox
    double TG_X0, TG_Y0;

    QGraphicsView* gvOver;
    AToolboxScene* scene;

    void doDraw(TObject *obj, const char *options, bool DoUpdate); //actual drawing, does not have window focussing - done to avoid refocussing issues leading to bugs

    //flags
    bool TMPignore; //temporarily forbid updates - need for bulk update to avoid cross-modification
    bool BarShown;
    bool ColdStart;
    bool fFirstTime; //signals that "UnZoom" range values (xmin0, etc...) have to be stored

    double xmin, xmax, ymin, ymax, zmin, zmax;
    double xmin0, xmax0, ymin0, ymax0, zmin0, zmax0; //start values - filled on first draw, can be used to reset view with "Unzoom"
    QString old_option;

    void RedrawAll();    
    void clearTmpTObjects();   //enable qDebugs inside for diagnostics of cleanup!
    void updateLegendVisibility();
    void startOverlayMode();
    void endOverlayMode();
    void UpdateBasketGUI();    
    void ExportData(bool fUseBinCenters=true);
    void exportTextForTH2(TH2 *h);
    void SaveBasket();
    void AppendBasket();
    void AppendRootHistsOrGraphs();
    QVector<DrawObjectStructure> *getCurrentDrawObjects();
    void ShowProjection(QString type);
    double runScaleDialog();
};

#endif // GRAPHWINDOWCLASS_H
