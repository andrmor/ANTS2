#ifndef GRAPHWINDOWCLASS_H
#define GRAPHWINDOWCLASS_H

#include "aguiwindow.h"
#include "adrawobject.h"
#include "abasketitem.h"
#include "adrawtemplate.h"

#include <QVector>
#include <QVariantList>

#include "TMathBase.h"

class MainWindow;
class RasterWindowGraphClass;
class TH2;
class TGraph;
class TGraph2D;
class QGraphicsView;
class AToolboxScene;
class QListWidgetItem;
class TObject;
class TTree;
class ABasketManager;
class ADrawExplorerWidget;
class ABasketListWidget;
class TLegend;

namespace Ui {
class GraphWindowClass;
}

class GraphWindowClass : public AGuiWindow
{
    Q_OBJECT

public:
    explicit GraphWindowClass(QWidget *parent, MainWindow *mw);
    ~GraphWindowClass();
    friend class ADrawExplorerWidget;

    QString LastDistributionShown;

    //Drawing
    void Draw(TObject* obj, const char* options = "", bool DoUpdate = true, bool TransferOwnership = true);  //registration should be skipped only for scripts!
    void DrawWithoutFocus(TObject* obj, const char* options = "", bool DoUpdate = true, bool TransferOwnership = true);  //registration should be skipped only for scripts!
    void RedrawAll();

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

    //use this to only construct! *** to separate file+namespace
    TGraph* ConstructTGraph(const QVector<double>& x, const QVector<double>& y) const;
    TGraph* ConstructTGraph(const std::vector<float>& x, const std::vector<float>& y) const;
    TGraph* ConstructTGraph(const QVector<double>& x, const QVector<double>& y,
                            const char *Title, const char *XTitle, const char *YTitle,
                            Color_t MarkerColor=2, int MarkerStyle=20, int MarkerSize=1,
                            Color_t LineColor=2,   int LineStyle=1,    int LineWidth=2) const;
    TGraph* ConstructTGraph(const QVector<double>& x, const QVector<double>& y,
                            const QString & Title, const QString & XTitle, const QString & YTitle,
                            Color_t MarkerColor=2, int MarkerStyle=20, int MarkerSize=1,
                            Color_t LineColor=2,   int LineStyle=1,    int LineWidth=2) const;
    TGraph* ConstructTGraph(const std::vector<float>& x, const std::vector<float>& y,
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

    void ClearBasket();
    TObject *GetMainPlottedObject();
    void SaveGraph(QString fileName);    
    void EnforceOverlayOff();    
    void ClearDrawObjects_OnShutDown(); //precvents crash on shut down
    void RegisterTObject(TObject* obj);
    void ShowTextPanel(const QString Text, bool bShowFrame=true, int AlignLeftCenterRight=0);

    void SetStatPanelVisible(bool flag);
    void TriggerGlobalBusy(bool flag);

    void MakeCopyOfDrawObjects();
    void ClearCopyOfDrawObjects();

    void ClearBasketActiveId();
    void MakeCopyOfActiveBasketId();
    void RestoreBasketActiveId();
    void ClearCopyOfActiveBasketId();
    QString & getLastOpendDir();    
    void ShowProjectionTool();
    TLegend * AddLegend();
    void HighlightUpdateBasketButton(bool flag);

protected:
    void mouseMoveEvent(QMouseEvent *event);
    bool event(QEvent *event);
    void closeEvent(QCloseEvent *event);

public slots:
    void UpdateControls(); //updates visualisation of the current master graph parameters
    void DoSaveGraph(QString name);
    void AddCurrentToBasket(const QString &name);
    void AddLegend(double x1, double y1, double x2, double y2, QString title);
    void SetLegendBorder(int color, int style, int size);
    void AddText(QString text, bool bShowFrame, int Alignment_0Left1Center2Right);
    void on_pbAddLegend_clicked();
    void ExportTH2AsText(QString fileName); //for temporary script command

    QVector<double> Get2DArray(); //for temporary script command

    void DrawStrOpt(TObject* obj, QString options = "", bool DoUpdate = true);
    void onDrawRequest(TObject* obj, const QString options, bool transferOwnership, bool focusWindow);
    bool DrawTree(TTree* tree, const QString& what, const QString& cond, const QString& how,
                  const QVariantList binsAndRanges = QVariantList(), const QVariantList markersAndLines = QVariantList(),
                  QString *result = 0);

private slots:
    void Reshape();
    void BasketCustomContextMenuRequested(const QPoint &pos);
    void onBasketItemDoubleClicked(QListWidgetItem *item);
    void BasketReorderRequested(const QVector<int> & indexes, int toRow);
    void deletePressed();

    void on_pbToolboxDragMode_clicked();

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
    void on_actionSave_image_triggered();
    void on_actionBasic_ROOT_triggered();
    void on_actionDeep_sea_triggered();
    void on_actionGrey_scale_triggered();
    void on_actionDark_body_radiator_triggered();
    void on_actionTwo_color_hue_triggered();
    void on_actionRainbow_triggered();
    void on_actionInverted_dark_body_triggered();
    void on_pbToolboxDragMode_2_clicked();
    void on_actionTop_triggered();
    void on_actionSide_triggered();
    void on_actionFront_triggered();
    void on_pbDensityDistribution_clicked();
    void on_actionEqualize_scale_XY_triggered();
    void on_ledRulerDX_editingFinished();
    void on_ledRulerDY_editingFinished();
    void on_cbShowFitParameters_toggled(bool checked);
    void on_pbXaveraged_clicked();
    void on_pbYaveraged_clicked();    
    void on_pbAddText_clicked();
    void on_pbRemoveLegend_clicked();
    void on_ledAngle_customContextMenuRequested(const QPoint &pos);
    void on_actionToggle_toolbar_triggered(bool checked);
    void on_pbBackToLast_clicked();
    void on_actionToggle_Explorer_Basket_toggled(bool arg1);
    void on_pbUpdateInBasket_clicked();
    void on_actionShow_ROOT_attribute_panel_triggered();
    void on_pbShowRuler_clicked();
    void on_pbExitToolMode_clicked();

    void on_actionSet_width_triggered();

    void on_actionSet_height_triggered();

    void on_actionMake_square_triggered();

    void on_actionCreate_template_triggered();

    void on_actionApply_template_triggered();

private:
    MainWindow *MW;
    Ui::GraphWindowClass *ui;
    RasterWindowGraphClass * RasterWindow = nullptr; //owns
    ADrawExplorerWidget * Explorer = nullptr; //owns
    bool ExtractionCanceled = false;
    int LastOptStat = 1111;

    QVector<ADrawObject> DrawObjects;  //always local objects -> can have a copy from the Basket
    QVector<ADrawObject> PreviousDrawObjects; //last draw made from outside of the graph window
    ABasketManager * Basket = nullptr;
    ABasketListWidget * lwBasket = nullptr;
    int ActiveBasketItem = -1; //-1 - Basket is off; 0+ -> basket loaded, can be updated
    int PreviousActiveBasketItem = -1; //-1 - Basket is off; 0+ -> basket loaded, can be updated

    QVector<TObject*> tmpTObjects;

    QGraphicsView* gvOver = 0;
    AToolboxScene* scene = 0;

    ADrawTemplate DrawTemplate;

    void doDraw(TObject *obj, const char *opt, bool DoUpdate); //actual drawing, does not have window focussing - done to avoid refocussing issues leading to bugs

    bool TMPignore = false; //temporarily forbid updates - need for bulk update to avoid cross-modification
    bool ColdStart = true;

    double xmin, xmax, ymin, ymax, zmin, zmax;

    void clearTmpTObjects();   //enable qDebugs inside for diagnostics of cleanup!

    void changeOverlayMode(bool bOn);

    void switchToBasket(int index);
    void UpdateBasketGUI();    
    void Basket_DrawOnTop(int row);

    void ShowProjection(QString type);
    void UpdateGuiControlsForMainObject(const QString &ClassName, const QString & options);
    void contextMenuForBasketMultipleSelection(const QPoint &pos);
    void removeAllSelectedBasketItems();
};

#endif // GRAPHWINDOWCLASS_H
