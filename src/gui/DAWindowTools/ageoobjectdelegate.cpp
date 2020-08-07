#include "ageoobjectdelegate.h"
#include "ageoobject.h"
#include "atypegeoobject.h"
#include "ageoshape.h"
#include "ageoconsts.h"
#include "amessage.h"

#include <QDebug>
#include <QWidget>
#include <QFrame>
#include <QStringList>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLayout>
#include <QDialog>
#include <QListWidget>
#include <QTableWidget>
#include <QMessageBox>
#include <QFont>
#include <QTableWidget>
#include <QHeaderView>

#include "TVector3.h"

AGeoObjectDelegate::AGeoObjectDelegate(const QStringList & materials, QWidget * ParentWidget) :
    AGeoBaseDelegate(ParentWidget)
{
  QFrame * frMainFrame = new QFrame();
  frMainFrame->setFrameShape(QFrame::Box);

  Widget = frMainFrame;
  Widget->setContextMenuPolicy(Qt::CustomContextMenu);

  QPalette palette = frMainFrame->palette();
  palette.setColor( Widget->backgroundRole(), QColor( 255, 255, 255 ) );
  frMainFrame->setPalette( palette );
  frMainFrame->setAutoFillBackground( true );
  lMF = new QVBoxLayout();
    lMF->setContentsMargins(5,5,5,2);

    //object type
    labType = new QLabel("");
    labType->setAlignment(Qt::AlignCenter);
    QFont font = labType->font();
    font.setBold(true);
    labType->setFont(font);
    lMF->addWidget(labType);

    //name and material line
    QHBoxLayout* hl = new QHBoxLayout();
      hl->setContentsMargins(2,0,2,0);
      QLabel* lname = new QLabel();
      lname->setText("Name:");
      lname->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      lname->setMaximumWidth(50);
      hl->addWidget(lname);
      leName = new QLineEdit();
      connect(leName, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onContentChanged);
      leName->setMaximumWidth(200);
      leName->setContextMenuPolicy(Qt::NoContextMenu);
      hl->addWidget(leName);
      lMat = new QLabel();
      lMat->setText("Material:");
      lMat->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      lname->setMaximumWidth(60);
      hl->addWidget(lMat);
      cobMat = new QComboBox();
      cobMat->setContextMenuPolicy(Qt::NoContextMenu);
      cobMat->addItems(materials);
      connect(cobMat, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
      //cobMat->setMaximumWidth(150);
      cobMat->setMinimumWidth(120);
      hl->addWidget(cobMat);
    lMF->addLayout(hl);

    //Shape box
    QHBoxLayout* h2 = new QHBoxLayout();
      h2->setContentsMargins(2,0,2,0);

      pteShape = new QPlainTextEdit();
      pteShape->setContextMenuPolicy(Qt::NoContextMenu);
      connect(pteShape, SIGNAL(textChanged()), this, SLOT(onContentChanged()));
      pteShape->setMaximumHeight(50);
      new AShapeHighlighter(pteShape->document());
      h2->addWidget(pteShape);
    lMF->addLayout(h2);

    //scale widget
    QHBoxLayout * hbs = new QHBoxLayout();
    hbs->setContentsMargins(2,0,2,0);
        hbs->addStretch();
        cbScale = new QCheckBox("Apply scaling factors");
        cbScale->setToolTip("Use scaling only if it is the only choice, e.g. to make ellipsoid from a sphere");
        connect(cbScale, &QCheckBox::clicked, this, &AGeoObjectDelegate::onLocalShapeParameterChange);  // !*! OLD SYSTEM
        connect(cbScale, &QCheckBox::clicked, this, &AGeoObjectDelegate::onScaleToggled); // new system
        connect(cbScale, &QCheckBox::clicked, this, &AGeoObjectDelegate::onContentChanged);
        hbs->addWidget(cbScale);
    scaleWidget = new QWidget();
        QHBoxLayout * hbsw = new QHBoxLayout(scaleWidget);
        hbsw->setContentsMargins(2,0,2,0);
        hbsw->addWidget(new QLabel("in X:"));
        ledScaleX = new QLineEdit("1.0"); hbsw->addWidget(ledScaleX);
        connect(ledScaleX, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onLocalShapeParameterChange); // !*! obsolete?
        connect(ledScaleX, &QLineEdit::editingFinished, this, &AGeoObjectDelegate::updateScalingFactors); // new
        connect(ledScaleX, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onContentChanged);
        hbsw->addWidget(new QLabel("in Y:"));
        ledScaleY = new QLineEdit("1.0"); hbsw->addWidget(ledScaleY);
        connect(ledScaleY, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onLocalShapeParameterChange);
        connect(ledScaleY, &QLineEdit::editingFinished, this, &AGeoObjectDelegate::updateScalingFactors); // new
        connect(ledScaleY, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onContentChanged);
        hbsw->addWidget(new QLabel("in Z:"));
        ledScaleZ = new QLineEdit("1.0"); hbsw->addWidget(ledScaleZ);
        connect(ledScaleZ, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onLocalShapeParameterChange);
        connect(ledScaleZ, &QLineEdit::editingFinished, this, &AGeoObjectDelegate::updateScalingFactors); // new
        connect(ledScaleZ, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onContentChanged);
    hbs->addWidget(scaleWidget);
    hbs->addStretch();
    lMF->addLayout(hbs);
    scaleWidget->setVisible(false);
    QObject::connect(cbScale, &QCheckBox::toggled, scaleWidget, &QWidget::setVisible);

    // Transform block
    QHBoxLayout * lht = new QHBoxLayout();
        pbTransform = new QPushButton("Transform to ...");
        lht->addWidget(pbTransform);
        connect(pbTransform, &QPushButton::pressed, this, &AGeoObjectDelegate::onChangeShapePressed);
        pbShapeInfo = new QPushButton("Info on this shape");
        connect(pbShapeInfo, &QPushButton::pressed, this, &AGeoObjectDelegate::onHelpRequested);
        lht->addWidget(pbShapeInfo);
    lMF->addLayout(lht);

    // Position and orientation block
    PosOrient = new QWidget();
    PosOrient->setContentsMargins(0,0,0,0);
    PosOrient->setMaximumHeight(80);
    QGridLayout *gr = new QGridLayout();
    gr->setContentsMargins(10, 0, 10, 3);
    gr->setVerticalSpacing(1);
      ledX = new QLineEdit();
      ledX->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledX, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledX, 0, 1);
      ledY = new QLineEdit();
      ledY->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledY, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledY, 1, 1);
      ledZ = new QLineEdit();
      ledZ->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledZ, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledZ, 2, 1);

      ledPhi = new QLineEdit();
      ledPhi->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledPhi, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledPhi, 0, 3);
      ledTheta = new QLineEdit();
      ledTheta->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledTheta, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledTheta, 1, 3);
      ledPsi = new QLineEdit();
      ledPsi->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledPsi, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledPsi, 2, 3);

      QLabel *l = new QLabel("X:");
      gr->addWidget(l, 0, 0);
      l = new QLabel("Y:");
      gr->addWidget(l, 1, 0);
      l = new QLabel("Z:");
      gr->addWidget(l, 2, 0);

      l = new QLabel("mm    Phi:");
      gr->addWidget(l, 0, 2);
      l = new QLabel("mm  Theta:");
      gr->addWidget(l, 1, 2);
      l = new QLabel("mm    Psi:");
      gr->addWidget(l, 2, 2);

      l = new QLabel("°");
      gr->addWidget(l, 0, 4);
      l = new QLabel("°");
      gr->addWidget(l, 1, 4);
      l = new QLabel("°");
      gr->addWidget(l, 2, 4);

    PosOrient->setLayout(gr);
    lMF->addWidget(PosOrient);

    // bottom line buttons
    QHBoxLayout * abl = createBottomButtons();
    lMF->addLayout(abl);

  frMainFrame->setLayout(lMF);

  //installing double validators for edit boxes
  /*
  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  ledX->setValidator(dv);
  ledY->setValidator(dv);
  ledZ->setValidator(dv);
  ledPhi->setValidator(dv);
  ledTheta->setValidator(dv);
  ledPsi->setValidator(dv);*/
}

AGeoObjectDelegate::~AGeoObjectDelegate()
{
    delete ShapeCopy;
}

const QString AGeoObjectDelegate::getName() const
{
    return leName->text();
}

bool AGeoObjectDelegate::isValid(AGeoObject * obj)
{
    if ( obj->ObjectType->isHandlingSet())
    {
        //for Set object there is no shape to check
    }
    else
    {
        // !*! temporary!!! to avoid old system of pteEdit control!
        /*AGeoBox * box = dynamic_cast<AGeoBox*>(ShapeCopy);
        if (!box)
        {
            AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(ShapeCopy);
            box = dynamic_cast<AGeoBox*>(scaled->BaseShape);
        }
        if (box) return true;

        AGeoTube * tube = dynamic_cast<AGeoTube*> (ShapeCopy);
        if (!tube)
        {
            AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*> (ShapeCopy);
            tube = dynamic_cast<AGeoTube*> (scaled->BaseShape);
        }
        if (tube) return true;
        */
        AGeoBox * box =dynamic_cast<AGeoBox*> (ShapeCopy);
        AGeoTube * tube =dynamic_cast<AGeoTube*> (ShapeCopy);
        if (!box && !tube)
        {
            qDebug()<< "then its scaled";
            AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*> (ShapeCopy);
            tube =dynamic_cast<AGeoTube*> (scaled->BaseShape);
            qDebug()<< "its a scled tube!";

            if (!tube)
            {
                box =dynamic_cast<AGeoBox*> (scaled->BaseShape);
                qDebug()<< "nope its a scaled box!";

            }
        }
        if (box) return true;
        if (tube) return true;


        // this is normal or composite object then
        //if composite, first check all members
        QString newShape = pteShape->document()->toPlainText();
        //qDebug() << "--> attempt to set shape using string:"<< newShape;

        bool fValid = obj->readShapeFromString(newShape, true); //only checks, no change!
        if (!fValid)
        {
            message(newShape.simplified().startsWith("TGeoArb8") ?
                        "Error parsing shape!\nIt could be non-clockwise order of defined nodes!" :
                        "Error parsing shape!", ParentWidget);
            return false;
        }
    }
    return true;
}

bool processEditBox(QLineEdit* lineEdit, double & val, QString & str, QWidget * parent)
{
    str = lineEdit->text();
    const AGeoConsts & gConsts = AGeoConsts::getConstInstance();
    bool ok = gConsts.evaluateFormula(str, val);
    if (ok) return true;

    qWarning () << "Bad format:" << str;
    QMessageBox::warning(parent, "", QString("Bad format: %1").arg(str));
    return false;
}

bool AGeoObjectDelegate::updateObject(AGeoObject * obj) const  //react to false in void AGeoWidget::onConfirmPressed()
{
    const QString oldName = obj->Name;
    const QString newName = leName->text();
    obj->Name = newName;

    if ( obj->ObjectType->isHandlingSet() )
    {
        //set container object does not have material and shape
    }
    else
    {
        obj->Material = cobMat->currentIndex();
        if (obj->Material == -1) obj->Material = 0; //protection

        //inherit materials for composite members
        if (obj->isCompositeMemeber())
        {
            if (obj->Container && obj->Container->Container)
                obj->Material = obj->Container->Container->Material;
        }

        AGeoShape * shape = ShapeCopy;
        AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(ShapeCopy);
        if (scaled) shape = scaled->BaseShape;
        if (shape)
        {
            QString errorStr = shape->updateShape();
            if (!errorStr.isEmpty())
            {
                QMessageBox::warning(this->ParentWidget,"", errorStr);
                return false;
            }
            delete obj->Shape;
            obj->Shape = ShapeCopy->clone();
        }
        else
        {
            qWarning() << "Something went very wrong, ShapeCopy not found";
            return false;
        }

        /*
        else
        {
            // old system
            QString newShape = pteShape->document()->toPlainText();
            obj->readShapeFromString(newShape);
        }*/


        //if it is a set member, need old values of position and angle
        QVector<double> old;
        old << obj->Position[0]    << obj->Position[1]    << obj->Position[2]
            << obj->Orientation[0] << obj->Orientation[1] << obj->Orientation[2];

        bool ok = true;
        ok = ok && processEditBox(ledX, obj->Position[0], obj->PositionStr[0], ParentWidget);
        ok = ok && processEditBox(ledY, obj->Position[1], obj->PositionStr[1], ParentWidget);
        ok = ok && processEditBox(ledZ, obj->Position[2], obj->PositionStr[2], ParentWidget);
        ok = ok && processEditBox(ledPhi,   obj->Orientation[0], obj->OrientationStr[0], ParentWidget);
        ok = ok && processEditBox(ledTheta, obj->Orientation[1], obj->OrientationStr[1], ParentWidget);
        ok = ok && processEditBox(ledPsi,   obj->Orientation[2], obj->OrientationStr[2], ParentWidget);
        if (!ok) return false;

        // checking was there a rotation of the main object
        bool fWasRotated = false;
        for (int i=0; i<3; i++)
            if (obj->Orientation[i] != old[3+i])
            {
                fWasRotated =true;
                break;
            }
        //qDebug() << "--Was rotated?"<< fWasRotated;

        //for grouped object, taking into accound the shift
        if (obj->Container && obj->Container->ObjectType->isGroup())
        {
            for (int iObj = 0; iObj < obj->Container->HostedObjects.size(); iObj++)
            {
                AGeoObject* hostedObj = obj->Container->HostedObjects[iObj];
                if (hostedObj == obj) continue;

                //center vector for rotation
                //in TGeoRotation, first rotation iz around Z, then new X(manual is wrong!) and finally around new Z
                TVector3 v(hostedObj->Position[0]-old[0], hostedObj->Position[1]-old[1], hostedObj->Position[2]-old[2]);

                //first rotate back to origin in rotation
                rotate(v, -old[3+0], 0, 0);
                rotate(v, 0, -old[3+1], 0);
                rotate(v, 0, 0, -old[3+2]);
                rotate(v, obj->Orientation[0], obj->Orientation[1], obj->Orientation[2]);

                for (int i=0; i<3; i++)
                {
                    double delta = obj->Position[i] - old[i]; //shift in position

                    if (fWasRotated)
                    {
                        //shift due to rotation  +  global shift
                        hostedObj->Position[i] = old[i]+v[i] + delta;
                        //rotation of the object
                        double deltaAng = obj->Orientation[i] - old[3+i];
                        hostedObj->Orientation[i] += deltaAng;
                    }
                    else
                        hostedObj->Position[i] += delta;
                }
            }
        }
        //for stack:
        if (obj->Container && obj->Container->ObjectType->isStack())
            obj->updateStack();
    }

    //additional post-processing
    if ( obj->ObjectType->isArray() )
    {
        //additional properties for array
        ATypeArrayObject* array = static_cast<ATypeArrayObject*>(obj->ObjectType);
        array->numX = sbNumX->value();
        array->numY = sbNumY->value();
        array->numZ = sbNumZ->value();
        array->stepX = ledStepX->text().toDouble();
        array->stepY = ledStepY->text().toDouble();
        array->stepZ = ledStepZ->text().toDouble();
    }
    else if (obj->ObjectType->isComposite())
    {
        AGeoObject* logicals = obj->getContainerWithLogical();
        if (logicals)
            logicals->Name = "CompositeSet_"+obj->Name;
    }
    else if (obj->ObjectType->isGrid())
    {
        AGeoObject* GE = obj->getGridElement();
        if (GE)
        {
            GE->Name = "GridElement_"+obj->Name;
            GE->Material = obj->Material;
        }
    }
    else if (obj->isCompositeMemeber())
    {
        AGeoObject* cont = obj->Container;
        if (cont)
        {
            if (cont->ObjectType->isCompositeContainer())
            {
                AGeoObject* Composite = cont->Container;
                if (Composite)
                { //updating Shape
                    AGeoComposite* cs = dynamic_cast<AGeoComposite*>(Composite->Shape);
                    if (cs)
                    {
                        cs->members.replaceInStrings(oldName, newName);
                        cs->GenerationString.replace(oldName, newName);
                    }
                }
            }
        }
    }
    return true;
}

void AGeoObjectDelegate::onChangeShapePressed()
{
    QDialog * d = new QDialog(ParentWidget);
    d->setWindowTitle("Select new shape");

    QStringList list;
    list << "Box"
         << "Tube" << "Tube segment" << "Tube segment cut" << "Tube elliptical"
         << "Trapezoid simplified" << "Trapezoid"
         << "Polycone"
         << "Polygon simplified" << "Polygon"
         << "Parallelepiped"
         << "Sphere"
         << "Cone" << "Cone segment"
         << "Torus"
         << "Paraboloid"
         << "Arb8";

    QVBoxLayout * l = new QVBoxLayout(d);
        QListWidget * w = new QListWidget();
        w->addItems(list);
    l->addWidget(w);
    d->resize(d->width(), 400);

        QHBoxLayout * h = new QHBoxLayout();
            QPushButton * pbAccept = new QPushButton("Change shape");
            pbAccept->setEnabled(false);
            QObject::connect(pbAccept, &QPushButton::clicked, [this, d, w](){this->onShapeDialogActivated(d, w);});
            QObject::connect(w, &QListWidget::activated, [this, d, w](){this->onShapeDialogActivated(d, w);});
            h->addWidget(pbAccept);
            QPushButton * pbCancel = new QPushButton("Cancel");
            QObject::connect(pbCancel, &QPushButton::clicked, d, &QDialog::reject);
            h->addWidget(pbCancel);
    l->addLayout(h);

    l->addWidget(new QLabel("Warning! There is no undo after change!"));

    QObject::connect(w, &QListWidget::itemSelectionChanged, [pbAccept, w]()
    {
        pbAccept->setEnabled(w->currentRow() != -1);
    });

    d->exec();
    delete d;
}

void AGeoObjectDelegate::onScaleToggled()
{
    AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(ShapeCopy);
    if (scaled)
    {
        qDebug() << "Convering scaled to base shape!";
        /*
        AGeoShape * tmp = ShapeCopy;
        ShapeCopy = scaled->BaseShape;
        delete tmp;
        */
        ShapeCopy = scaled->BaseShape;
        scaled->BaseShape = nullptr;
        delete scaled;
    }
    else
    {
        qDebug() << "Converting shape to scaled!";
        scaled = new AGeoScaledShape();
        scaled->BaseShape = ShapeCopy;
        ShapeCopy = scaled;

        // !*!  TFormula too?
        scaled->scaleX = ledScaleX->text().toDouble();
        scaled->scaleY = ledScaleY->text().toDouble();
        scaled->scaleZ = ledScaleZ->text().toDouble();
    }
}

void AGeoObjectDelegate::addLocalLayout(QLayout * lay)
{
    lMF->insertLayout(3, lay);
}

void AGeoObjectDelegate::updatePteShape(const QString & text)
{
    pteShape->clear();

    QString str = text;
    if (cbScale->isChecked())
        str = QString("TGeoScaledShape( %1, %2, %3, %4 )").arg(text).arg(ledScaleX->text()).arg(ledScaleY->text()).arg(ledScaleZ->text());

    pteShape->appendPlainText(str);
    pbShapeInfo->setToolTip(pteShape->document()->toPlainText());
}

void AGeoObjectDelegate::updateScalingFactors()
{
    AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(ShapeCopy);
    if (scaled)
    {
        // !*! TFormula?   common method with combine with toggle on check box scale?
        scaled->scaleX = ledScaleX->text().toDouble();
        scaled->scaleY = ledScaleY->text().toDouble();
        scaled->scaleZ = ledScaleZ->text().toDouble();
    }
}

const AGeoShape * AGeoObjectDelegate::getBaseShapeOfObject(const AGeoObject * obj)
{
    if (!obj && !obj->Shape) return nullptr;
    AGeoScaledShape * scaledShape = dynamic_cast<AGeoScaledShape*>(obj->Shape);
    if (!scaledShape) return nullptr;

    AGeoShape * baseShape = AGeoShape::GeoShapeFactory(scaledShape->getBaseShapeType());
    bool bOK = baseShape->readFromString( scaledShape->BaseShapeGenerationString );
    if (!bOK) qDebug() << "Failed to read shape properties:" << scaledShape->BaseShapeGenerationString;
    return baseShape;
}

void AGeoObjectDelegate::updateTypeLabel()
{
    if (CurrentObject->ObjectType->isGrid())
        DelegateTypeName = "Grid bulk, " + DelegateTypeName;

    if (CurrentObject->isCompositeMemeber())
        DelegateTypeName += " (logical)";
    else if (CurrentObject->Container)
    {
        if (CurrentObject->Container->ObjectType->isHandlingSet())
        {
            if (CurrentObject->Container->ObjectType->isGroup())
                DelegateTypeName += ",   groupped";
            else
                DelegateTypeName += ",   stacked";
        }
    }

    labType->setText(DelegateTypeName);
}

void AGeoObjectDelegate::updateControlUI()
{
    pteShape->setVisible(false);

    if (CurrentObject->ObjectType->isHandlingSet())
    {
        pteShape->setVisible(false);
        lMat->setVisible(false);
        cobMat->setVisible(false);
        PosOrient->setVisible(false);
    }

    if (CurrentObject->Container && CurrentObject->Container->ObjectType->isStack())
    {
        ledPhi->setEnabled(false);
        ledTheta->setEnabled(false);
        ledPhi->setText("0");
        ledTheta->setText("0");
    }

    if (CurrentObject->isCompositeMemeber())
    {
        cobMat->setEnabled(false);
        cobMat->setCurrentIndex(-1);
    }

    if (CurrentObject->isCompositeMemeber())
    {
        pbShow->setVisible(false);
        pbChangeAtt->setVisible(false);
        pbScriptLine->setVisible(false);
    }
}

void AGeoObjectDelegate::rotate(TVector3 & v, double dPhi, double dTheta, double dPsi) const
{
    v.RotateZ( TMath::Pi()/180.0* dPhi );
    TVector3 X(1,0,0);
    X.RotateZ( TMath::Pi()/180.0* dPhi );
    //v.RotateX( TMath::Pi()/180.0* Theta);
    v.Rotate( TMath::Pi()/180.0* dTheta, X);
    TVector3 Z(0,0,1);
    Z.Rotate( TMath::Pi()/180.0* dTheta, X);
    // v.RotateZ( TMath::Pi()/180.0* Psi );
    v.Rotate( TMath::Pi()/180.0* dPsi, Z );
}

void AGeoObjectDelegate::onShapeDialogActivated(QDialog * d, QListWidget * w)
{
    const QString sel = w->currentItem()->text();
    if      (sel == "Box")                  emit RequestChangeShape(new AGeoBox());
    else if (sel == "Parallelepiped")       emit RequestChangeShape(new AGeoPara());
    else if (sel == "Tube")                 emit RequestChangeShape(new AGeoTube());
    else if (sel == "Tube segment")         emit RequestChangeShape(new AGeoTubeSeg());
    else if (sel == "Tube segment cut")     emit RequestChangeShape(new AGeoCtub());
    else if (sel == "Tube elliptical")      emit RequestChangeShape(new AGeoEltu());
    else if (sel == "Sphere")               emit RequestChangeShape(new AGeoSphere());
    else if (sel == "Trapezoid simplified") emit RequestChangeShape(new AGeoTrd1());
    else if (sel == "Trapezoid")            emit RequestChangeShape(new AGeoTrd2());
    else if (sel == "Cone")                 emit RequestChangeShape(new AGeoCone());
    else if (sel == "Cone segment")         emit RequestChangeShape(new AGeoConeSeg());
    else if (sel == "Paraboloid")           emit RequestChangeShape(new AGeoParaboloid());
    else if (sel == "Torus")                emit RequestChangeShape(new AGeoTorus());
    else if (sel == "Polycone")             emit RequestChangeShape(new AGeoPcon());
    else if (sel == "Polygon simplified")   emit RequestChangeShape(new AGeoPolygon());
    else if (sel == "Polygon")              emit RequestChangeShape(new AGeoPgon());
    else if (sel == "Arb8")                 emit RequestChangeShape(new AGeoArb8());
    else
    {
        //nothing selected or unknown shape
        return;
    }
    d->accept();
}

void AGeoObjectDelegate::onHelpRequested()
{
    message(ShapeHelp, Widget);
}

void AGeoObjectDelegate::Update(const AGeoObject *obj)
{
    CurrentObject = obj;
    leName->setText(obj->Name);

    delete ShapeCopy;
    ShapeCopy = obj->Shape->clone();

    //qDebug() << "--genstring:original/copy->"<<obj->Shape->getGenerationString() << ShapeCopy->getGenerationString();

    int imat = obj->Material;
    if (imat < 0 || imat >= cobMat->count())
    {
        qWarning() << "Material index out of bounds!";
        imat = -1;
    }
    cobMat->setCurrentIndex(imat);

    pteShape->setPlainText(obj->Shape->getGenerationString());

    ledX->setText(obj->PositionStr[0].isEmpty() ? QString::number(obj->Position[0]) : obj->PositionStr[0]);
    ledY->setText(obj->PositionStr[1].isEmpty() ? QString::number(obj->Position[1]) : obj->PositionStr[1]);
    ledZ->setText(obj->PositionStr[2].isEmpty() ? QString::number(obj->Position[2]) : obj->PositionStr[2]);

    ledPhi->  setText(obj->OrientationStr[0].isEmpty() ? QString::number(obj->Orientation[0]) : obj->OrientationStr[0]);
    ledTheta->setText(obj->OrientationStr[1].isEmpty() ? QString::number(obj->Orientation[1]) : obj->OrientationStr[1]);
    ledPsi->  setText(obj->OrientationStr[2].isEmpty() ? QString::number(obj->Orientation[2]) : obj->OrientationStr[2]);

    updateTypeLabel();
    updateControlUI();

    AGeoScaledShape * scaledShape = dynamic_cast<AGeoScaledShape*>(CurrentObject->Shape);
    cbScale->setChecked(scaledShape);
    if (scaledShape)
    {
        ledScaleX->setText(QString::number(scaledShape->scaleX));
        ledScaleY->setText(QString::number(scaledShape->scaleY));
        ledScaleZ->setText(QString::number(scaledShape->scaleZ));
    }
}

void AGeoObjectDelegate::onContentChanged()
{
    pbShapeInfo->setToolTip(pteShape->document()->toPlainText());
    emit ContentChanged();
    //qDebug() <<pteShape->document()->toPlainText();
}

AShapeHighlighter::AShapeHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
  HighlightingRule rule;
  ShapeFormat.setForeground(Qt::blue);
  ShapeFormat.setFontWeight(QFont::Bold);

  QList<AGeoShape*> AvailableShapes = AGeoShape::GetAvailableShapes();
  QStringList ShapePatterns;
  while (!AvailableShapes.isEmpty())
  {
      ShapePatterns << AvailableShapes.first()->getShapeType();
      delete AvailableShapes.first();
      AvailableShapes.removeFirst();
  }

  foreach (const QString &pattern, ShapePatterns)
  {
      rule.pattern = QRegExp(pattern);
      rule.format = ShapeFormat;
      highlightingRules.append(rule);
  }
}

void AShapeHighlighter::highlightBlock(const QString &text)
{
  foreach (const HighlightingRule &rule, highlightingRules)
    {
      QRegExp expression(rule.pattern);
      int index = expression.indexIn(text);
      while (index >= 0) {
          int length = expression.matchedLength();
          setFormat(index, length, rule.format);
          index = expression.indexIn(text, index + length);
        }
    }
}

//---------------

AGeoBoxDelegate::AGeoBoxDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Box";

    ShapeHelp = "A box shape\n"
            "\n"
            "Parameters are the full sizes in X, Y and Z direction\n"
            "\n"
            "The XYZ position is given for the center point\n"
            "\n"
            "Implemented using TGeoBBox(0.5*X_size, 0.5*Y_size, 0.5*Z_size)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("X full size:"), 0, 0);
    gr->addWidget(new QLabel("Y full size:"), 1, 0);
    gr->addWidget(new QLabel("Z full size:"), 2, 0);

    ex = new QLineEdit(); gr->addWidget(ex, 0, 1);
    ey = new QLineEdit(); gr->addWidget(ey, 1, 1);
    ez = new QLineEdit(); gr->addWidget(ez, 2, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {ex, ey, ez};
    for (QLineEdit * le : l)
    {
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoBoxDelegate::ContentChanged);
        //QObject::connect(le, &QLineEdit::editingFinished, this, &AGeoBoxDelegate::onLocalShapeParameterChange);
    }
}

void AGeoBoxDelegate::finalizeLocalParameters()
{
    AGeoBox * box = dynamic_cast<AGeoBox*>(ShapeCopy);
    if (!box)
    {
        AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(ShapeCopy);
        box = dynamic_cast<AGeoBox*>(scaled->BaseShape);
    }

    if (box)
    {
        box->str2dx = ex->text();
        box->str2dy = ey->text();
        box->str2dz = ez->text();
        //emit ContentChanged();
    }
    else qWarning() << "Read delegate: Box shape not found!";
}

void AGeoBoxDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    //old system
    /*
    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoBox * box = dynamic_cast<const AGeoBox *>(tmpShape ? tmpShape : obj->Shape);
    if (box)
    {
        ex->setText(QString::number(box->dx*2.0));
        ey->setText(QString::number(box->dy*2.0));
        ez->setText(QString::number(box->dz*2.0));
    }
    delete tmpShape;
    */

    //new system
    AGeoBox * box = dynamic_cast<AGeoBox*>(ShapeCopy);
    if (!box)
    {
        AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(ShapeCopy);
        box = dynamic_cast<AGeoBox*>(scaled->BaseShape);
    }
    if (box)
    {
        ex->setText(box->str2dx.isEmpty() ? QString::number(box->dx*2.0) : box->str2dx);
        ey->setText(box->str2dy.isEmpty() ? QString::number(box->dy*2.0) : box->str2dy);
        ez->setText(box->str2dz.isEmpty() ? QString::number(box->dz*2.0) : box->str2dz);
    }
    else qWarning() << "Update delegate: Box shape not found!";
}

void AGeoBoxDelegate::onLocalShapeParameterChange() //now performed by "finalizelocalparameters"
{
    //old
    //updatePteShape(QString("TGeoBBox( %1, %2, %3 )").arg(0.5*ex->text().toDouble()).arg(0.5*ey->text().toDouble()).arg(0.5*ez->text().toDouble()));

    //new
    AGeoBox * box = dynamic_cast<AGeoBox*>(ShapeCopy);
    if (!box)
    {
        AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(ShapeCopy);
        box = dynamic_cast<AGeoBox*>(scaled->BaseShape);
    }

    if (box)
    {
        box->str2dx = ex->text();
        box->str2dy = ey->text();
        box->str2dz = ez->text();
        //emit ContentChanged();
    }
    else qWarning() << "Read delegate: Box shape not found!";
}

AGeoTubeDelegate::AGeoTubeDelegate(const QStringList & materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Tube";

    ShapeHelp = "A cylinderical shape\n"
            "\n"
            "Parameters are the outer and inner diameters and the full height\n"
            "\n"
            "The XYZ position is given for the center point\n"
            "\n"
            "Implemented using TGeoTube(0.5*Inner_diameter, 0.5*Outer_diameter, 0.5*Height)";

    gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Outer diameter:"), 0, 0);
    gr->addWidget(new QLabel("Inner diameter:"), 1, 0);
    gr->addWidget(new QLabel("Height:"), 2,0);

    eo = new QLineEdit(); gr->addWidget(eo, 0, 1);
    ei = new QLineEdit(); gr->addWidget(ei, 1, 1);
    ez = new QLineEdit(); gr->addWidget(ez, 2, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {eo, ei, ez};
    for (QLineEdit * le : l)
    {
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTubeDelegate::ContentChanged);
        //QObject::connect(le, &QLineEdit::editingFinished, this, &AGeoTubeDelegate::onLocalShapeParameterChange);
    }

}

void AGeoTubeDelegate::finalizeLocalParameters()
{
    AGeoTube * tube = dynamic_cast<AGeoTube*>(ShapeCopy);
    if (!tube)
    {
        AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*> (ShapeCopy);
        tube = dynamic_cast<AGeoTube*> (scaled->BaseShape);
    }
    if (tube)
    {
        tube->str2rmax = eo->text();
        tube->str2rmin = ei->text();
        tube->str2dz   = ez->text();

        //emit ContentChanged();
    }
    else qWarning() << "Read delegate: Tube shape not found!";
}

void AGeoTubeDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);
    /*
    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoTube * tube = dynamic_cast<const AGeoTube*>(tmpShape ? tmpShape : obj->Shape);
    if (tube)
    {
        eo->setText(QString::number(tube->rmax*2.0));
        ei->setText(QString::number(tube->rmin*2.0));
        ez->setText(QString::number(tube->dz*2.0));
    }
    delete tmpShape;
    */

    AGeoTube * tube = dynamic_cast<AGeoTube*>(ShapeCopy);
    if (!tube)
    {
        AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(ShapeCopy);
        tube = dynamic_cast<AGeoTube*>(scaled->BaseShape);
    }
    if (tube)
    {
        eo->setText(tube->str2rmax.isEmpty() ? QString::number(tube->rmax*2.0) : tube->str2rmax);
        ei->setText(tube->str2rmin.isEmpty() ? QString::number(tube->rmin*2.0) : tube->str2rmin);
        ez->setText(tube->str2dz.isEmpty()   ? QString::number(tube->dz*2.0)   : tube->str2dz);
    }
    else qWarning() << "Update delegate: Tube shape not found!";
}

void AGeoTubeDelegate::onLocalShapeParameterChange() //now performed by "finalizelocalparameters"
{
    /*updatePteShape(QString("TGeoTube( %1, %2, %3 )").arg(0.5*ei->text().toDouble()).arg(0.5*eo->text().toDouble()).arg(0.5*ez->text().toDouble()));*/
    AGeoTube * tube = dynamic_cast<AGeoTube*>(ShapeCopy);
    if (!tube)
    {
        AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*> (ShapeCopy);
        tube = dynamic_cast<AGeoTube*> (scaled->BaseShape);
    }
    if (tube)
    {
        tube->str2rmax = eo->text();
        tube->str2rmin = ei->text();
        tube->str2dz   = ez->text();
        emit ContentChanged();
    }
    else qWarning() << "Read delegate: Tube shape not found!";
}

AGeoTubeSegDelegate::AGeoTubeSegDelegate(const QStringList & materials, QWidget * parent) :
    AGeoTubeDelegate(materials, parent)
{
    DelegateTypeName = "Tube segment";

    ShapeHelp = "A cylinderical segment shape\n"
            "\n"
            "Parameters are the outer and inner diameters, the full height\n"
            "and the segment angles from and to\n"
            "\n"
            "The XYZ position is given for the center point of the cylinder\n"
            "\n"
            "Implemented using TGeoTubeSeg(0.5*Inner_diameter, 0.5*Outer_diameter, 0.5*Height, phi_from, phi_to)";

    gr->addWidget(new QLabel("Phi from:"), 3, 0);
    gr->addWidget(new QLabel("Phi to:"), 4, 0);

    ep1 = new QLineEdit(); gr->addWidget(ep1, 3, 1);
    ep2 = new QLineEdit(); gr->addWidget(ep2, 4, 1);

    gr->addWidget(new QLabel("°"), 3, 2);
    gr->addWidget(new QLabel("°"), 4, 2);

    QVector<QLineEdit*> l = {ep1, ep2};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTubeSegDelegate::onLocalShapeParameterChange);
}

void AGeoTubeSegDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoTubeSeg * seg = dynamic_cast<const AGeoTubeSeg*>(tmpShape ? tmpShape : obj->Shape);
    if (seg)
    {
        eo->setText (QString::number(seg->rmax*2.0));
        ei->setText (QString::number(seg->rmin*2.0));
        ez->setText (QString::number(seg->dz*2.0));
        ep1->setText(QString::number(seg->phi1));
        ep2->setText(QString::number(seg->phi2));
    }
    delete tmpShape;
}

void AGeoTubeSegDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoTubeSeg( %1, %2, %3, %4, %5 )").arg(0.5*ei->text().toDouble()).arg(0.5*eo->text().toDouble()).arg(0.5*ez->text().toDouble())
                                                               .arg(ep1->text()).arg(ep2->text()) );
}

AGeoTubeSegCutDelegate::AGeoTubeSegCutDelegate(const QStringList &materials, QWidget *parent) :
    AGeoTubeSegDelegate(materials, parent)
{
    DelegateTypeName = "Tube segment cut";

    ShapeHelp = "A cylinderical segment cut shape\n"
            "\n"
            "Parameters are the outer and inner diameters,\n"
            "the full height in Z direction (from -dz to +dz in local coordinates),\n"
            "the segment angles from and to,\n"
            "and the unit vectors (Nx, Ny, Ny) of the normals for the lower\n"
            "and the upper faces at (0,0,-dz) and (0,0,+dz) local coordinates\n"
            "\n"
            "The XYZ position is given for the point with (0,0,0) local coordinates\n"
            "\n"
            "Implemented using TGeoCtub(0.5*Inner_diameter, 0.5*Outer_diameter, 0.5*Height, phi_from, phi_to, LNx, LNy, LNz, UNx, UNy, UNz)";

    gr->addWidget(new QLabel("Low Nx:"), 5, 0);
    gr->addWidget(new QLabel("Low Ny:"), 6, 0);
    gr->addWidget(new QLabel("Low Nz:"), 7, 0);
    gr->addWidget(new QLabel("Up  Nx:"), 8, 0);
    gr->addWidget(new QLabel("Up  Ny:"), 9, 0);
    gr->addWidget(new QLabel("Up  Nz:"), 10, 0);

    elnx = new QLineEdit(); gr->addWidget(elnx, 5, 1);
    elny = new QLineEdit(); gr->addWidget(elny, 6, 1);
    elnz = new QLineEdit(); gr->addWidget(elnz, 7, 1);
    eunx = new QLineEdit(); gr->addWidget(eunx, 8, 1);
    euny = new QLineEdit(); gr->addWidget(euny, 9, 1);
    eunz = new QLineEdit(); gr->addWidget(eunz, 10, 1);

    QVector<QLineEdit*> l = {elnx, elny, elnz, eunx, euny, eunz};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTubeSegCutDelegate::onLocalShapeParameterChange);
}

void AGeoTubeSegCutDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoCtub * seg = dynamic_cast<const AGeoCtub*>(tmpShape ? tmpShape : obj->Shape);
    if (seg)
    {
        eo->setText (QString::number(seg->rmax*2.0));
        ei->setText (QString::number(seg->rmin*2.0));
        ez->setText (QString::number(seg->dz*2.0));
        ep1->setText(QString::number(seg->phi1));
        ep2->setText(QString::number(seg->phi2));
        elnx->setText(QString::number(seg->nxlow));
        elny->setText(QString::number(seg->nylow));
        elnz->setText(QString::number(seg->nzlow));
        eunx->setText(QString::number(seg->nxhi));
        euny->setText(QString::number(seg->nyhi));
        eunz->setText(QString::number(seg->nzhi));
    }
    delete tmpShape;
}

void AGeoTubeSegCutDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoCtub( %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11 )")
                              .arg(0.5*ei->text().toDouble()).arg(0.5*eo->text().toDouble()).arg(0.5*ez->text().toDouble())
                              .arg(ep1->text()).arg(ep2->text())
                              .arg(elnx->text()).arg(elny->text()).arg(elnz->text())
                              .arg(eunx->text()).arg(euny->text()).arg(eunz->text()) );
}

AGeoParaDelegate::AGeoParaDelegate(const QStringList & materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Parallelepiped";

    ShapeHelp = "A parallelepiped is a shape having 3 pairs of parallel faces\n"
                "  out of which one is parallel with the XY plane (Z face).\n"
                "All faces are parallelograms. The Z faces have 2 edges\n"
                "  parallel to the X-axis.\n"
                "\n"
                "The shape has the center in the origin and is defined by\n"
                "  the full lengths of the projections of the edges on X, Y and Z.\n"
                "The lower Z face is positioned at -0.5*Zsize,\n"
                "  while the upper at +0.5*Zsize.\n"
                "Alpha: angle between the segment defined by the centers of the\n"
                "  X-parallel edges and Y axis [-90,90] in degrees\n"
                "Theta: theta angle of the segment defined by the centers of the Z faces\n"
                "Phi: phi angle of the same segment\n"
                "\n"
                "Implemented using TGeoPara(0.5*X_size, 0.5*Y_size, 0.5*Z_size, Alpha, Theta, Phi)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("X full size:"), 0, 0);
    gr->addWidget(new QLabel("Y full size:"), 1, 0);
    gr->addWidget(new QLabel("Z full size:"), 2, 0);
    gr->addWidget(new QLabel("Alpha:"),     3, 0);
    gr->addWidget(new QLabel("Theta:"),     4, 0);
    gr->addWidget(new QLabel("Phi:"),       5, 0);

    ex = new QLineEdit(); gr->addWidget(ex, 0, 1);
    ey = new QLineEdit(); gr->addWidget(ey, 1, 1);
    ez = new QLineEdit(); gr->addWidget(ez, 2, 1);
    ea = new QLineEdit(); gr->addWidget(ea, 3, 1);
    et = new QLineEdit(); gr->addWidget(et, 4, 1);
    ep = new QLineEdit(); gr->addWidget(ep, 5, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("°"),  3, 2);
    gr->addWidget(new QLabel("°"),  4, 2);
    gr->addWidget(new QLabel("°"),  5, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {ex, ey, ez, ea, et, ep};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoParaDelegate::onLocalShapeParameterChange);
}

void AGeoParaDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoPara * para = dynamic_cast<const AGeoPara*>(tmpShape ? tmpShape : obj->Shape);
    if (para)
    {
        ex->setText(QString::number(para->dx*2.0));
        ey->setText(QString::number(para->dy*2.0));
        ez->setText(QString::number(para->dz*2.0));
        ea->setText(QString::number(para->alpha));
        et->setText(QString::number(para->theta));
        ep->setText(QString::number(para->phi));
    }
    delete tmpShape;
}

void AGeoParaDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoPara( %1, %2, %3, %4, %5, %6 )").arg(0.5*ex->text().toDouble()).arg(0.5*ey->text().toDouble()).arg(0.5*ez->text().toDouble())
                                                                .arg(ea->text()).arg(et->text()).arg(ep->text())  );
}

AGeoSphereDelegate::AGeoSphereDelegate(const QStringList & materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Sphere";

    ShapeHelp = "A spherical sector, defined by\n"
                "the external and internal diameters,\n"
                "and two pairs of polar angles (in degrees):\n"
                "\n"
                "Theta from: [0, 180)\n"
                "Theta to:   (0, 180] with a condition theta_from < theta_to\n"
                "\n"
                "Phi from: [0, 360)\n"
                "Phi to:   (0, 360] with a condition phi_from < phi_to\n"
                "\n"
                "Implemented using TGeoSphere(0.5*Inner_Diameter, 0.5*Outer_Diameter, ThetaFrom, ThetaTo, PhiFrom, PhiTo)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Outer diameter:"), 0, 0);
    gr->addWidget(new QLabel("Inner diameter:"), 1, 0);
    gr->addWidget(new QLabel("Theta from:"), 2, 0);
    gr->addWidget(new QLabel("Theta to:"),     3, 0);
    gr->addWidget(new QLabel("Phi from:"),     4, 0);
    gr->addWidget(new QLabel("Phi to:"),       5, 0);

    eod = new QLineEdit(); gr->addWidget(eod, 0, 1);
    eid = new QLineEdit(); gr->addWidget(eid, 1, 1);
    et1 = new QLineEdit(); gr->addWidget(et1, 2, 1);
    et2 = new QLineEdit(); gr->addWidget(et2, 3, 1);
    ep1 = new QLineEdit(); gr->addWidget(ep1, 4, 1);
    ep2 = new QLineEdit(); gr->addWidget(ep2, 5, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("°"),  2, 2);
    gr->addWidget(new QLabel("°"),  3, 2);
    gr->addWidget(new QLabel("°"),  4, 2);
    gr->addWidget(new QLabel("°"),  5, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {eod, eid, et1, et2, ep1, ep2};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoSphereDelegate::onLocalShapeParameterChange);
}

void AGeoSphereDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoSphere * sph = dynamic_cast<const AGeoSphere*>(tmpShape ? tmpShape : obj->Shape);
    if (sph)
    {
        eid->setText(QString::number(sph->rmin*2.0));
        eod->setText(QString::number(sph->rmax*2.0));
        et1->setText(QString::number(sph->theta1));
        et2->setText(QString::number(sph->theta2));
        ep1->setText(QString::number(sph->phi1));
        ep2->setText(QString::number(sph->phi2));
    }
    delete tmpShape;
}

void AGeoSphereDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoSphere( %1, %2, %3, %4, %5, %6 )").arg(0.5*eid->text().toDouble()).arg(0.5*eod->text().toDouble())
                                                                  .arg(et1->text()).arg(et2->text())
                                                                  .arg(ep1->text()).arg(ep2->text())  );
}

AGeoConeDelegate::AGeoConeDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Cone";

    ShapeHelp = "A conical shape with the axis parallel to Z,\n"
                "  limited by two XY planes,\n"
                "  one at Z = -0.5*height, and\n"
                "  the other at Z = +0.5*height.\n"
                "\n"
                "The shape is also defined by two pairs\n"
                " of external/internal diameters,\n"
                " one pair at the lower plane and the other at the upper one.\n"
                "\n"
                "Implemented using TGeoCone(0.5*Height, 0.5*LowerInDiam, 0.5*LowerOutDiam, 0.5*UpperInDiam, 0.5*UpperOutDiam)";

    gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Height:"),               0, 0);
    gr->addWidget(new QLabel("Lower outer diameter:"), 1, 0);
    gr->addWidget(new QLabel("Lower inner diameter:"), 2, 0);
    gr->addWidget(new QLabel("Upper outer diameter:"), 3, 0);
    gr->addWidget(new QLabel("Upper inner diameter:"), 4, 0);

    ez  = new QLineEdit(); gr->addWidget(ez, 0, 1);
    elo = new QLineEdit(); gr->addWidget(elo, 1, 1);
    eli = new QLineEdit(); gr->addWidget(eli, 2, 1);
    euo = new QLineEdit(); gr->addWidget(euo, 3, 1);
    eui = new QLineEdit(); gr->addWidget(eui, 4, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("mm"), 3, 2);
    gr->addWidget(new QLabel("mm"), 4, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {ez, eli, elo, eui, euo};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoConeDelegate::onLocalShapeParameterChange);
}

void AGeoConeDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoCone * cone = dynamic_cast<const AGeoCone*>(tmpShape ? tmpShape : obj->Shape);
    if (cone)
    {
        ez ->setText(QString::number(cone->dz*2.0));
        eli->setText(QString::number(cone->rminL*2.0));
        elo->setText(QString::number(cone->rmaxL*2.0));
        eui->setText(QString::number(cone->rminU*2.0));
        euo->setText(QString::number(cone->rmaxU*2.0));
    }
    delete tmpShape;
}

void AGeoConeDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoCone( %1, %2, %3, %4, %5 )")
                   .arg(0.5*ez->text().toDouble())
                   .arg(0.5*eli->text().toDouble()).arg(0.5*elo->text().toDouble())
                   .arg(0.5*eui->text().toDouble()).arg(0.5*euo->text().toDouble()) );
}

AGeoConeSegDelegate::AGeoConeSegDelegate(const QStringList &materials, QWidget *parent)
    : AGeoConeDelegate(materials, parent)
{
    DelegateTypeName = "Cone segment";

    ShapeHelp = "A conical segment shape with the axis parallel to Z,\n"
                "  limited by two XY planes,\n"
                "  one at Z = -0.5*height, and\n"
                "  the other at Z = +0.5*height.\n"
                "\n"
                "The shape is also defined by two pairs\n"
                " of external/internal diameters,\n"
                " one pair at the lower plane and the other at the upper one.\n"
                "\n"
                "The segment angles are in degrees,\n"
                "  Phi from:  in the range [0, 360)\n"
                "  Phi to:    in the ramge (0, 360], should be smaller than Phi_from.\n"
                "\n"
                "Implemented using TGeoCone(0.5*Height, 0.5*LowerInDiam, 0.5*LowerOutDiam, 0.5*UpperInDiam, 0.5*UpperoutDiam, PhiFrom, PhiTo)";

    gr->addWidget(new QLabel("Phi from:"), 5, 0);
    gr->addWidget(new QLabel("Phi to:"),   6, 0);

    ep1 = new QLineEdit(); gr->addWidget(ep1, 5, 1);
    ep2 = new QLineEdit(); gr->addWidget(ep2, 6, 1);

    gr->addWidget(new QLabel("°"), 5, 2);
    gr->addWidget(new QLabel("°"), 6, 2);

    QVector<QLineEdit*> l = {ep1, ep2};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoConeSegDelegate::onLocalShapeParameterChange);
}

void AGeoConeSegDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoConeSeg * cone = dynamic_cast<const AGeoConeSeg*>(tmpShape ? tmpShape : obj->Shape);
    if (cone)
    {
        ez ->setText(QString::number(cone->dz*2.0));
        eli->setText(QString::number(cone->rminL*2.0));
        elo->setText(QString::number(cone->rmaxL*2.0));
        eui->setText(QString::number(cone->rminU*2.0));
        euo->setText(QString::number(cone->rmaxU*2.0));
        ep1->setText(QString::number(cone->phi1));
        ep2->setText(QString::number(cone->phi2));
    }
    delete tmpShape;
}

void AGeoConeSegDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoConeSeg( %1, %2, %3, %4, %5, %6, %7 )")
                   .arg(0.5*ez->text().toDouble())
                   .arg(0.5*eli->text().toDouble()).arg(0.5*elo->text().toDouble())
                   .arg(0.5*eui->text().toDouble()).arg(0.5*euo->text().toDouble())
                   .arg(ep1->text()).arg(ep2->text()) );
}

AGeoElTubeDelegate::AGeoElTubeDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Elliptical tube";

    ShapeHelp = "An elliptical tube\n"
            "\n"
            "Parameters are the diameters in X and Y directions and the full height\n"
            "\n"
            "The XYZ position is given for the center point\n"
            "\n"
            "Implemented using TGeoEltu(0.5*X_full_size, 0.5*Y_full_size, 0.5*Height)";

    gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("X full size:"), 0, 0);
    gr->addWidget(new QLabel("Y full size:"), 1, 0);
    gr->addWidget(new QLabel("Height:"),    2, 0);

    ex = new QLineEdit(); gr->addWidget(ex, 0, 1);
    ey = new QLineEdit(); gr->addWidget(ey, 1, 1);
    ez = new QLineEdit(); gr->addWidget(ez, 2, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {ex, ey, ez};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoElTubeDelegate::onLocalShapeParameterChange);
}

void AGeoElTubeDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoEltu * tube = dynamic_cast<const AGeoEltu*>(tmpShape ? tmpShape : obj->Shape);
    if (tube)
    {
        ex->setText(QString::number(tube->a*2.0));
        ey->setText(QString::number(tube->b*2.0));
        ez->setText(QString::number(tube->dz*2.0));
    }
    delete tmpShape;
}

void AGeoElTubeDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoEltu( %1, %2, %3 )").arg(0.5*ex->text().toDouble()).arg(0.5*ey->text().toDouble()).arg(0.5*ez->text().toDouble()) );
}

AGeoTrapXDelegate::AGeoTrapXDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Trapezoid simplified";

    ShapeHelp = "A trapezoid shape\n"
            "\n"
            "The two of the opposite faces are parallel to XY plane\n"
            "  and are positioned in Z at ± 0.5*Height.\n"
            "Full X size is given for the lower and upper planes.\n"
            "\n"
            "Implemented using TGeoTrd1(0.5*X_lower_size, 0.5*X_upper_size, 0.5*Y_size, 0.5*Height)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("X lower size:"), 0, 0);
    gr->addWidget(new QLabel("X upper size:"), 1, 0);
    gr->addWidget(new QLabel("Y size:"),       2, 0);
    gr->addWidget(new QLabel("Height:"),       3, 0);

    exl = new QLineEdit(); gr->addWidget(exl, 0, 1);
    exu = new QLineEdit(); gr->addWidget(exu, 1, 1);
    ey  = new QLineEdit(); gr->addWidget(ey,  2, 1);
    ez  = new QLineEdit(); gr->addWidget(ez,  3, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("mm"), 3, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {exl, exu, ey, ez};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTrapXDelegate::onLocalShapeParameterChange);
}

void AGeoTrapXDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoTrd1 * trap = dynamic_cast<const AGeoTrd1*>(tmpShape ? tmpShape : obj->Shape);
    if (trap)
    {
        exl->setText(QString::number(trap->dx1 * 2.0));
        exu->setText(QString::number(trap->dx2 * 2.0));
        ey-> setText(QString::number(trap->dy  * 2.0));
        ez-> setText(QString::number(trap->dz  * 2.0));
    }
    delete tmpShape;
}

void AGeoTrapXDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoTrd1( %1, %2, %3, %4 )")
                   .arg(0.5*exl->text().toDouble()).arg(0.5*exu->text().toDouble())
                   .arg(0.5* ey->text().toDouble()).arg(0.5* ez->text().toDouble()) );
}

AGeoTrapXYDelegate::AGeoTrapXYDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Trapezoid";

    ShapeHelp = "A trapezoid shape\n"
            "\n"
            "The two of the opposite faces are parallel to XY plane\n"
            "  and are positioned in Z at ± 0.5*Height.\n"
            "Full X and Y sizes are given for the lower and upper planes.\n"
            "\n"
            "Implemented using TGeoTrd2(0.5*X_lower_size, 0.5*X_upper_size, 0.5*Y_lower_size, 0.5*Y_upper_size, 0.5*Height)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("X lower size:"), 0, 0);
    gr->addWidget(new QLabel("X upper size:"), 1, 0);
    gr->addWidget(new QLabel("Y lower size:"), 2, 0);
    gr->addWidget(new QLabel("Y upper size:"), 3, 0);
    gr->addWidget(new QLabel("Height:"),       4, 0);

    exl = new QLineEdit(); gr->addWidget(exl, 0, 1);
    exu = new QLineEdit(); gr->addWidget(exu, 1, 1);
    eyl = new QLineEdit(); gr->addWidget(eyl, 2, 1);
    eyu = new QLineEdit(); gr->addWidget(eyu, 3, 1);
    ez  = new QLineEdit(); gr->addWidget(ez,  4, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("mm"), 3, 2);
    gr->addWidget(new QLabel("mm"), 4, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {exl, exu, eyl, eyu, ez};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTrapXYDelegate::onLocalShapeParameterChange);
}

void AGeoTrapXYDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoTrd2 * trap = dynamic_cast<const AGeoTrd2*>(tmpShape ? tmpShape : obj->Shape);
    if (trap)
    {
        exl->setText(QString::number(trap->dx1 * 2.0));
        exu->setText(QString::number(trap->dx2 * 2.0));
        eyl->setText(QString::number(trap->dy1 * 2.0));
        eyu->setText(QString::number(trap->dy2 * 2.0));
        ez-> setText(QString::number(trap->dz  * 2.0));
    }
    delete tmpShape;
}

void AGeoTrapXYDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoTrd2( %1, %2, %3, %4, %5)")
                   .arg(0.5*exl->text().toDouble()).arg(0.5*exu->text().toDouble())
                   .arg(0.5*eyl->text().toDouble()).arg(0.5*eyu->text().toDouble()).arg(0.5*ez->text().toDouble()) );
}

AGeoParaboloidDelegate::AGeoParaboloidDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Paraboloid";

    ShapeHelp = "A paraboloid shape\n"
            "\n"
            "Defined by the revolution surface generated by a parabola\n"
            "  and is bound by two planes perpendicular to Z axis.\n"
            "The parabola equation is taken in the form: z = a*r^2 + b,\n"
            "  where r^2 = x^2 + y^2.\n"
            "The coefficients a and b are computed from the input values\n"
            "  which are the diameters of the circular sections cut by\n"
            "  two planes, lower at -0.5*Height, and the upper at +0.5*Height:\n"
            "  a*(0.5*Lower_diameter)^2 + b   for the lower plane and\n"
            "  a*(0.5*Upper_diameter)^2 + b   for the upper one.\n"
            "\n"
            "Implemented using TGeoParaboloid(0.5*Lower_diameter, 0.5*Upper_diameter, 0.5*Height)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Lower diameter:"), 0, 0);
    gr->addWidget(new QLabel("Upper diameter:"), 1, 0);
    gr->addWidget(new QLabel("Height:"),         2, 0);

    el = new QLineEdit(); gr->addWidget(el, 0, 1);
    eu = new QLineEdit(); gr->addWidget(eu, 1, 1);
    ez = new QLineEdit(); gr->addWidget(ez, 2, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {el, eu, ez};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoParaboloidDelegate::onLocalShapeParameterChange);
}

void AGeoParaboloidDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoParaboloid * para = dynamic_cast<const AGeoParaboloid*>(tmpShape ? tmpShape : obj->Shape);
    if (para)
    {
        el->setText(QString::number(para->rlo * 2.0));
        eu->setText(QString::number(para->rhi * 2.0));
        ez->setText(QString::number(para->dz  * 2.0));
    }
    delete tmpShape;
}

void AGeoParaboloidDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoParaboloid( %1, %2, %3)")
                   .arg(0.5*el->text().toDouble())
                   .arg(0.5*eu->text().toDouble())
                   .arg(0.5*ez->text().toDouble()) );
}

AGeoTorusDelegate::AGeoTorusDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Torus";

    ShapeHelp = "A torus segment\n"
            "\n"
            "Defined by the axial, inner and outer diameters\n"
            "  and the segment angles (in degrees)\n"
            "Phi from: in the range [0, 360),\n"
            "Phi to:   in the range (0, 360], Phi_to > Phi_from\n"
            "\n"
            "Implemented using TGeoTorus(0.5*Axial_diameter, 0.5*Inner_diameter, 0.5*Outer_diameter, Phi_from, Phi_to)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Axial diameter:"), 0, 0);
    gr->addWidget(new QLabel("Outer diameter:"), 1, 0);
    gr->addWidget(new QLabel("Inner diameter:"), 2, 0);
    gr->addWidget(new QLabel("Phi from:"),       3, 0);
    gr->addWidget(new QLabel("Phi to:"),         4, 0);

    ead = new QLineEdit(); gr->addWidget(ead, 0, 1);
    edo = new QLineEdit(); gr->addWidget(edo, 1, 1);
    edi = new QLineEdit(); gr->addWidget(edi, 2, 1);
    ep0 = new QLineEdit(); gr->addWidget(ep0, 3, 1);
    epe= new QLineEdit();  gr->addWidget(epe, 4, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("°"),  3, 2);
    gr->addWidget(new QLabel("°"),  4, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {ead, edi, edo, ep0, epe};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTorusDelegate::onLocalShapeParameterChange);
}

void AGeoTorusDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoTorus * tor = dynamic_cast<const AGeoTorus*>(tmpShape ? tmpShape : obj->Shape);
    if (tor)
    {
        ead->setText(QString::number(tor->R    * 2.0));
        edi->setText(QString::number(tor->Rmin * 2.0));
        edo->setText(QString::number(tor->Rmax * 2.0));
        ep0->setText(QString::number(tor->Phi1));
        epe->setText(QString::number(tor->Dphi));
    }
    delete tmpShape;
}

void AGeoTorusDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoTorus( %1, %2, %3, %4, %5)")
                   .arg(0.5*ead->text().toDouble())
                   .arg(0.5*edi->text().toDouble())
                   .arg(0.5*edo->text().toDouble())
                   .arg(ep0->text())
                   .arg(epe->text()) );
}

AGeoPolygonDelegate::AGeoPolygonDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Polygon (simplified)";

    ShapeHelp = "A polygon section\n"
            "\n"
            "The shape is limited by the upper and lower planes,\n"
            "  positioned in Z at +0.5*Height and -0.5*Height\n"
            "For each plane are defined\n"
            "  the outer and inner diameters of the circles,\n"
            "  inscribed in the corresponding polygon.\n"
            "\n"
            "Section angle: in the range (0, 360] degrees\n"
            "\n"
            "Implemented using TGeoPgon";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Number of edges:"),      0, 0);
    gr->addWidget(new QLabel("Height:"),               1, 0);
    gr->addWidget(new QLabel("Lower outer diameter:"), 2, 0);
    gr->addWidget(new QLabel("Lower inner diameter:"), 3, 0);
    gr->addWidget(new QLabel("Upper outer diameter:"), 4, 0);
    gr->addWidget(new QLabel("Upper inner diameter:"), 5, 0);
    gr->addWidget(new QLabel("Angle:"),                6, 0);

    sbn = new QSpinBox();  gr->addWidget(sbn, 0, 1); sbn->setMinimum(3);
    ez  = new QLineEdit(); gr->addWidget(ez,  1, 1);
    elo = new QLineEdit(); gr->addWidget(elo, 2, 1);
    eli = new QLineEdit(); gr->addWidget(eli, 3, 1);
    euo = new QLineEdit(); gr->addWidget(euo, 4, 1);
    eui = new QLineEdit(); gr->addWidget(eui, 5, 1);
    edp = new QLineEdit(); gr->addWidget(edp, 6, 1);

    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("mm"), 3, 2);
    gr->addWidget(new QLabel("mm"), 4, 2);
    gr->addWidget(new QLabel("mm"), 5, 2);
    gr->addWidget(new QLabel("°"),  5, 2);

    addLocalLayout(gr);

    QObject::connect(sbn, SIGNAL(valueChanged(int)), this, SLOT(onLocalShapeParameterChange()));
    QVector<QLineEdit*> l = {edp, ez, eli, elo, eui, euo};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoPolygonDelegate::onLocalShapeParameterChange);
}

void AGeoPolygonDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoPolygon * pgon = dynamic_cast<const AGeoPolygon*>(tmpShape ? tmpShape : obj->Shape);
    if (pgon)
    {
        sbn->setValue(pgon->nedges);
        edp->setText(QString::number(pgon->dphi));
        ez-> setText(QString::number(pgon->dz    * 2.0));
        eli->setText(QString::number(pgon->rminL * 2.0));
        elo->setText(QString::number(pgon->rmaxL * 2.0));
        eui->setText(QString::number(pgon->rminU * 2.0));
        euo->setText(QString::number(pgon->rmaxU * 2.0));
    }
    delete tmpShape;
}

void AGeoPolygonDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoPolygon( %1, %2, %3, %4, %5, %6, %7 )")
                   .arg(sbn->value())
                   .arg(edp->text())
                   .arg(0.5*ez->text().toDouble())
                   .arg(0.5*eli->text().toDouble())
                   .arg(0.5*elo->text().toDouble())
                   .arg(0.5*eui->text().toDouble())
                   .arg(0.5*euo->text().toDouble()) );
}

AGeoPconDelegate::AGeoPconDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Polycone";

    ShapeHelp = "A shape constructed of an arbitrary number of cone sections\n"
            "\n"
            "A section is limited by the upper and lower planes,\n"
            "  shared by the neighboring sections.\n"
            "For each plane are defined:\n"
            "  Z position, and\n"
            "  outer and inner diameters.\n"
            "\n"
            "Phi from: in the range [0, 360) degrees,\n"
            "Phi to:   in the range (0, 360] degrees, Phi_to > Phi_from\n"
            "\n"
            "Implemented using TGeoPcon";

    lay = new QVBoxLayout();
    lay->setContentsMargins(50, 0, 50, 0);
    lay->setSpacing(3);

    lay->addWidget(new QLabel("Defined planes (should be monotonic in Z), all in mm:"));

        tab = new QTableWidget();
        tab->setColumnCount(3);
        tab->setHorizontalHeaderLabels(QStringList({"Z position", "Outer diameter", "Inner diameter"}));
        tab->setMaximumHeight(150);
        tab->verticalHeader()->setSectionsMovable(true);
        QObject::connect(tab->verticalHeader(), &QHeaderView::sectionMoved, this, [this](int /*logicalIndex*/, int oldVisualIndex, int newVisualIndex)
        {
            //qDebug() << logicalIndex << oldVisualIndex << newVisualIndex;
            tab->verticalHeader()->blockSignals(true);
            //tab->verticalHeader()->swapSections(oldVisualIndex, newVisualIndex);
            tab->verticalHeader()->moveSection(newVisualIndex, oldVisualIndex);
            tab->verticalHeader()->blockSignals(false);
            //swap table rows oldVisualIndex and newVisualIndex
            for (int i=0; i<3; i++)
            {
                QTableWidgetItem * from = tab->takeItem(oldVisualIndex, i);
                QTableWidgetItem * to = tab->takeItem(newVisualIndex, i);
                tab->setItem(oldVisualIndex, i, to);
                tab->setItem(newVisualIndex, i, from);
            }
        }, Qt::QueuedConnection);
        connect(tab, &QTableWidget::cellChanged, this, &AGeoPconDelegate::onLocalShapeParameterChange);

    lay->addWidget(tab);

        QHBoxLayout * hl = new QHBoxLayout();

        QPushButton * pbAddAbove = new QPushButton("Add above");
        connect(pbAddAbove, &QPushButton::clicked, [this]()
        {
            int row = tab->currentRow();
            if (row == -1) row = 0;
            tab->insertRow(row);
            tab->setRowHeight(row, rowHeight);
            onLocalShapeParameterChange();
        });
        hl->addWidget(pbAddAbove);
        QPushButton * pbAddBelow = new QPushButton("Add below");
        connect(pbAddBelow, &QPushButton::clicked, [this]()
        {
            const int num = tab->rowCount();
            int row = tab->currentRow();
            if (row == -1) row = num-1;
            row++;
            tab->insertRow(row);
            tab->setRowHeight(row, rowHeight);
            onLocalShapeParameterChange();
        });
        hl->addWidget(pbAddBelow);
        QPushButton * pbRemoveRow = new QPushButton("Remove plane");
        connect(pbRemoveRow, &QPushButton::clicked, [this]()
        {
            int row = tab->currentRow();
            if (row != -1) tab->removeRow(row);
            onLocalShapeParameterChange();
        });
        hl->addWidget(pbRemoveRow);

    lay->addLayout(hl);

    QGridLayout * gr = new QGridLayout();
        gr->setContentsMargins(0, 0, 0, 3);
        gr->setVerticalSpacing(1);

        gr->addWidget(new QLabel("Phi from:"), 0, 0);
        gr->addWidget(new QLabel("Phi to:"),   1, 0);

        ep0 = new QLineEdit(); gr->addWidget(ep0, 0, 1);
        epe = new QLineEdit(); gr->addWidget(epe, 1, 1);

        gr->addWidget(new QLabel("°"),  0, 2);
        gr->addWidget(new QLabel("°"),  1, 2);

    lay->addLayout(gr);

    addLocalLayout(lay);

    QVector<QLineEdit*> l = {ep0, epe};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoPconDelegate::onLocalShapeParameterChange);
}

void AGeoPconDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoPcon * pcon = dynamic_cast<const AGeoPcon*>(tmpShape ? tmpShape : obj->Shape);
    if (pcon)
    {
        ep0->setText(QString::number(pcon->phi));
        epe->setText(QString::number(pcon->dphi));

        tab->clearContents();
        const int numPlanes = pcon->Sections.size();
        tab->setRowCount(numPlanes);
        for (int iP = 0; iP < numPlanes; iP++)
        {
            const APolyCGsection & Section = pcon->Sections.at(iP);
            QTableWidgetItem * item = new QTableWidgetItem(QString::number(Section.z)); item->setTextAlignment(Qt::AlignCenter);
            tab->setItem(iP, 0, item);
            item = new QTableWidgetItem(QString::number(Section.rmax * 2.0)); item->setTextAlignment(Qt::AlignCenter);
            tab->setItem(iP, 1, item);
            item = new QTableWidgetItem(QString::number(Section.rmin * 2.0)); item->setTextAlignment(Qt::AlignCenter);
            tab->setItem(iP, 2, item);
            tab->setRowHeight(iP, rowHeight);
        }
    }
    delete tmpShape;
}

void AGeoPconDelegate::onLocalShapeParameterChange()
{
    QString s = QString("TGeoPcon( %1, %2")
            .arg(ep0->text())
            .arg(epe->text());

    if (!tab) return;
    const int rows = tab->rowCount();
    for (int ir = 0; ir < rows; ir++)
    {
        if (!tab->item(ir, 0) || !tab->item(ir, 1) || !tab->item(ir, 2)) continue;
        s += QString(", { %1 : %2 : %3 }")
                .arg(tab->item(ir, 0)->text())
                .arg(0.5*tab->item(ir, 2)->text().toDouble())
                .arg(0.5*tab->item(ir, 1)->text().toDouble());
    }
    s += " )";

    updatePteShape(s);
}

AGeoPgonDelegate::AGeoPgonDelegate(const QStringList &materials, QWidget *parent)
    : AGeoPconDelegate(materials, parent)
{
    DelegateTypeName = "Polygon";

    ShapeHelp = "A shape constructed of an arbitrary number of polygon sections\n"
            "\n"
            "A section is limited by the upper and lower planes,\n"
            "  shared by the neighboring sections.\n"
            "For each plane are defined:\n"
            "  Z position, and\n"
            "  outer and inner diameters of circles, inscribed in the polygon.\n"
            "\n"
            "Phi from: in the range [0, 360) degrees,\n"
            "Phi to:   in the range (0, 360] degrees, Phi_to > Phi_from\n"
            "\n"
            "Implemented using TGeoPgon";

    QHBoxLayout * h = new QHBoxLayout();
    h->setContentsMargins(0, 0, 0, 1);
    h->setSpacing(1);
    QLabel * lab = new QLabel("Edges:");
    h->addWidget(lab);
        sbn = new QSpinBox();
        sbn->setMinimum(3);
        sbn->setMaximum(100000);
    h->addWidget(sbn);
    h->addStretch();

    lay->insertLayout(0, h);

    tab->setHorizontalHeaderLabels(QStringList({"Z position", "Outer size", "Inner size"}));

    QObject::connect(sbn, SIGNAL(valueChanged(int)), this, SLOT(onLocalShapeParameterChange()));
}

void AGeoPgonDelegate::Update(const AGeoObject *obj)
{
    AGeoPconDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoPgon * pgon = dynamic_cast<const AGeoPgon*>(tmpShape ? tmpShape : obj->Shape);
    if (pgon)
        sbn->setValue(pgon->nedges);
    delete tmpShape;
}

void AGeoPgonDelegate::onLocalShapeParameterChange()
{
    QString s = QString("TGeoPgon( %1, %2, %3")
            .arg(ep0->text())
            .arg(epe->text())
            .arg(sbn->value());

    if (!tab) return;
    const int rows = tab->rowCount();
    for (int ir = 0; ir < rows; ir++)
    {
        if (!tab->item(ir, 0) || !tab->item(ir, 1) || !tab->item(ir, 2)) continue;
        s += QString(", { %1 : %2 : %3 }")
                .arg(tab->item(ir, 0)->text())
                .arg(0.5*tab->item(ir, 2)->text().toDouble())
                .arg(0.5*tab->item(ir, 1)->text().toDouble());
    }
    s += " )";

    updatePteShape(s);
}

AGeoCompositeDelegate::AGeoCompositeDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Composite shape";

    ShapeHelp = "Composite shape\n"
            "\n"
            "Composite shapes are boolean combinations\n"
            "  of two or more shape components. The supported operations are:\n"
            "\n"
            "  (+) - union\n"
            "  (*) - intersection\n"
            "  (-) - subtraction.\n"
            "\n"
            "If nested structures are needed, brackets can be used, e.g.,\n"
            "   (Shape1 + Shape2) - (Shape3 * (Shape4 + Shape5))\n"
            "\n"
            "Implemented using TGeoCompositeShape\n"
            "\n"
            "WARNING!\n"
            "Use composite shapes only as the last resort.\n"
            "Navigation in geometries containing composite shapes are very inefficient!";

    QVBoxLayout * v = new QVBoxLayout();
    v->setContentsMargins(50, 0, 50, 3);

    v->addWidget(new QLabel("Use logical volume names and\n'+', '*', and '-' operands; brackets for nested"));
        te = new QPlainTextEdit();
        QFont font = te->font();
        font.setPointSize(te->font().pointSize() + 2);
        te->setFont(font);
    v->addWidget(te);
    connect(te, &QPlainTextEdit::textChanged, this, &AGeoCompositeDelegate::onLocalShapeParameterChange);

    //cbScale->setChecked(false);
    //cbScale->setVisible(false);

    addLocalLayout(v);
}

void AGeoCompositeDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoComposite * combo = dynamic_cast<const AGeoComposite *>(tmpShape ? tmpShape : obj->Shape);
    if (combo)
    {
        QString s = combo->getGenerationString().simplified();
        s.remove("TGeoCompositeShape(");
        s.chop(1);

        te->clear();
        te->appendPlainText(s.simplified());
    }
    delete tmpShape;
}

void AGeoCompositeDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoCompositeShape( %1 )").arg(te->document()->toPlainText()));
}

AGeoArb8Delegate::AGeoArb8Delegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Arb8";

    ShapeHelp = "An Arb8 shape of Cern Root\n"
                "\n"
                "The shape is defined by two quadrilaterals sitting on parallel planes,\n"
                "   at +0.5*Height and -0.5*Height.\n"
                "Quadrilaterals are defined each by 4 vertices\n"
                "   with the coordinates (Xi,Yi,+/-0.5*Height), i from 0 to 3.\n"
                "\n"
                "The lateral surface of the Arb8 is defined by the 4 pairs of\n"
                "   edges corresponding to vertices (i,i+1) on both planes.\n"
                "If M and M' are the middles of the segments (i,i+1)\n"
                "   at the different planes,\n"
                "   a lateral surface is obtained by sweeping the edge at\n"
                "   -0.5*Height along MM',\n"
                "   so that it will match the corresponding one at +0.5*Height.\n"
                "Since the points defining the edges are arbitrary,\n"
                "   the lateral surfaces are not necessary planes –\n"
                "   but twisted planes having a twist angle linear-dependent on Z.\n"
                "\n"
                "Vertices have to be defined clockwise in the XY pane at both planes!\n"
                "\n"
                "Any two or more vertices in each plane\n"
                "   can have the same (X,Y) coordinates!\n"
                "   It this case, the top and bottom quadrilaterals become triangles,\n"
                "   segments or points.\n"
                "\n"
                "The lateral surfaces are not necessary defined by a pair of segments,\n"
                "   but by pair segment-point (making a triangle)\n"
                "   or point-point (making a line).\n"
                "Any choice is valid as long as at one of the end-caps is at least a triangle.\n";

    QVBoxLayout * v = new QVBoxLayout();
    v->setContentsMargins(50, 0, 50, 0);
    v->setSpacing(3);
    QGridLayout * gr = new QGridLayout();
        gr->setContentsMargins(0, 0, 0, 0);
        gr->addWidget(new QLabel("Height:"), 0, 0);
        ez = new QLineEdit(); gr->addWidget(ez,  0, 1);
        connect(ez, &QLineEdit::textChanged, this, &AGeoArb8Delegate::onLocalShapeParameterChange);
        gr->addWidget(new QLabel("mm"), 0, 2);
    v->addLayout(gr);

    ve.clear();
    for (int iul = 0; iul < 2; iul++)
    {
        v->addWidget(new QLabel(iul == 0 ? "Lower plane (positions clockwise!):" : "Upper plane (positions clockwise!):"));

        QVector<AEditEdit> tmpV(4);

        QGridLayout * gri = new QGridLayout();
        gri->setContentsMargins(0, 0, 0, 0);
        gri->setVerticalSpacing(3);

        for (int i=0; i < 4; i++)
        {
            gri->addWidget(new QLabel("  x:"),    i, 0);
            tmpV[i].X = new QLineEdit("");
            connect(tmpV[i].X, &QLineEdit::textChanged, this, &AGeoArb8Delegate::onLocalShapeParameterChange);
            gri->addWidget(tmpV[i].X,             i, 1);
            gri->addWidget(new QLabel("mm   y:"), i, 2);
            tmpV[i].Y = new QLineEdit("");
            connect(tmpV[i].Y, &QLineEdit::textChanged, this, &AGeoArb8Delegate::onLocalShapeParameterChange);
            gri->addWidget(tmpV[i].Y,             i, 3);
            gri->addWidget(new QLabel("mm"),      i, 4);
        }
        ve.push_back(tmpV);
        v->addLayout(gri);
    }
    addLocalLayout(v);
}

void AGeoArb8Delegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoArb8 * arb = dynamic_cast<const AGeoArb8 *>(tmpShape ? tmpShape : obj->Shape);
    if (arb)
    {
        ez->setText(QString::number(2.0 * arb->dz));

        for (int iul = 0; iul < 2; iul++)
        {
            for (int i = 0; i < 4; i++)
            {
                const int iInVert = iul * 4 + i;
                const QPair<double, double> & V = arb->Vertices.at(iInVert);
                AEditEdit & CEE = ve[iul][i];
                CEE.X->setText(QString::number(V.first));
                CEE.Y->setText(QString::number(V.second));
            }
        }
    }
    delete tmpShape;
}

void AGeoArb8Delegate::onLocalShapeParameterChange()
{
    QString s = QString("TGeoArb8( %1").arg(0.5 * ez->text().toDouble());
    for (int iul = 0; iul < 2; iul++)
    {
        for (int i = 0; i < 4; i++)
        {
            AEditEdit & CEE = ve[iul][i];
            s += QString(", %1,%2").arg(CEE.X->text()).arg(CEE.Y->text());
        }
    }
    s += ")";
    updatePteShape(s);
}

AGeoArrayDelegate::AGeoArrayDelegate(const QStringList &materials, QWidget *parent)
   : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Array";

    QVBoxLayout * v = new QVBoxLayout();
    v->setContentsMargins(50, 0, 50, 0);

    QGridLayout *grAW = new QGridLayout();
    grAW->setContentsMargins(5, 3, 5, 3);
    grAW->setVerticalSpacing(0);

    QLabel *la = new QLabel("Number in X:");
    grAW->addWidget(la, 0, 0);
    la = new QLabel("Number in Y:");
    grAW->addWidget(la, 1, 0);
    la = new QLabel("Number in Z:");
    grAW->addWidget(la, 2, 0);
    la = new QLabel("Step in X:");
    la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grAW->addWidget(la, 0, 2);
    la = new QLabel("Step in Y:");
    la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grAW->addWidget(la, 1, 2);
    la = new QLabel("Step in Z:");
    la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grAW->addWidget(la, 2, 2);
    la = new QLabel("mm");
    grAW->addWidget(la, 0, 4);
    la = new QLabel("mm");
    grAW->addWidget(la, 1, 4);
    la = new QLabel("mm");
    grAW->addWidget(la, 2, 4);

    sbNumX = new QSpinBox(Widget);
    sbNumX->setMaximum(100);
    sbNumX->setMinimum(0);
    sbNumX->setContextMenuPolicy(Qt::NoContextMenu);
    grAW->addWidget(sbNumX, 0, 1);
    connect(sbNumX, SIGNAL(valueChanged(int)), this, SLOT(onContentChanged()));
    sbNumY = new QSpinBox(Widget);
    sbNumY->setMaximum(100);
    sbNumY->setMinimum(0);
    sbNumY->setContextMenuPolicy(Qt::NoContextMenu);
    grAW->addWidget(sbNumY, 1, 1);
    connect(sbNumY, SIGNAL(valueChanged(int)), this, SLOT(onContentChanged()));
    sbNumZ = new QSpinBox(Widget);
    sbNumZ->setMaximum(100);
    sbNumZ->setMinimum(0);
    sbNumZ->setContextMenuPolicy(Qt::NoContextMenu);
    grAW->addWidget(sbNumZ, 2, 1);
    connect(sbNumZ, SIGNAL(valueChanged(int)), this, SLOT(onContentChanged()));
    ledStepX = new QLineEdit(Widget);
    ledStepX->setContextMenuPolicy(Qt::NoContextMenu);
    ledStepX->setMaximumWidth(75);
    connect(ledStepX, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
    grAW->addWidget(ledStepX, 0, 3);
    ledStepY = new QLineEdit(Widget);
    ledStepY->setMaximumWidth(75);
    ledStepY->setContextMenuPolicy(Qt::NoContextMenu);
    connect(ledStepY, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
    grAW->addWidget(ledStepY, 1, 3);
    ledStepZ = new QLineEdit(Widget);
    ledStepZ->setMaximumWidth(75);
    ledStepZ->setContextMenuPolicy(Qt::NoContextMenu);
    connect(ledStepZ, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
    grAW->addWidget(ledStepZ, 2, 3);

    addLocalLayout(grAW);

    cbScale->setChecked(false);
    cbScale->setVisible(false);

    lMat->setVisible(false);
    cobMat->setVisible(false);
    ledPhi->setText("0");
    ledPhi->setEnabled(false);
    ledTheta->setText("0");
    ledTheta->setEnabled(false);

    pbTransform->setVisible(false);
    pbShapeInfo->setVisible(false);
}

void AGeoArrayDelegate::Update(const AGeoObject * obj)
{
    AGeoObjectDelegate::Update(obj);

    if (obj->ObjectType->isArray())
    {
        ATypeArrayObject* array = static_cast<ATypeArrayObject*>(obj->ObjectType);
        sbNumX->setValue(array->numX);
        sbNumY->setValue(array->numY);
        sbNumZ->setValue(array->numZ);
        ledStepX->setText(QString::number(array->stepX));
        ledStepY->setText(QString::number(array->stepY));
        ledStepZ->setText(QString::number(array->stepZ));
    }
}

AGeoSetDelegate::AGeoSetDelegate(const QStringList &materials, QWidget *parent)
   : AGeoObjectDelegate(materials, parent)
{
     pbTransform->setVisible(false);
     pbShapeInfo->setVisible(false);

     cbScale->setChecked(false);
     cbScale->setVisible(false);
}

void AGeoSetDelegate::Update(const AGeoObject *obj)
{
    if (obj->ObjectType->isCompositeContainer())
    {
        DelegateTypeName = "Container of logical shapes";
        pbShow->setVisible(false);
        pbChangeAtt->setVisible(false);
        pbScriptLine->setVisible(false);
    }
    else
        DelegateTypeName = ( obj->ObjectType->isStack() ? "Stack" : "Group" );

    AGeoObjectDelegate::Update(obj);
}

AWorldDelegate::AWorldDelegate(const QStringList & materials, QWidget * ParentWidget) :
    AGeoBaseDelegate(ParentWidget)
{
    QFrame * frMainFrame = new QFrame();
    frMainFrame->setFrameShape(QFrame::Box);

    Widget = frMainFrame;
    Widget->setContextMenuPolicy(Qt::CustomContextMenu);

    QPalette palette = frMainFrame->palette();
    palette.setColor( Widget->backgroundRole(), QColor( 255, 255, 255 ) );
    frMainFrame->setPalette( palette );
    frMainFrame->setAutoFillBackground( true );

    QVBoxLayout * lMF = new QVBoxLayout();
      lMF->setContentsMargins(5,5,5,2);

      QLabel * labType = new QLabel("World");
      labType->setAlignment(Qt::AlignCenter);
      QFont font = labType->font();
      font.setBold(true);
      labType->setFont(font);
      lMF->addWidget(labType);

      QHBoxLayout* hl = new QHBoxLayout();
        hl->setContentsMargins(2,0,2,0);

        QLabel * lMat = new QLabel();
        lMat->setText("Material:");
        lMat->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lMat->setMaximumWidth(60);
        hl->addWidget(lMat);

        cobMat = new QComboBox();
        cobMat->setContextMenuPolicy(Qt::NoContextMenu);
        cobMat->addItems(materials);
        //connect(cobMat, &QComboBox::activated, this, &AWorldDelegate::onContentChanged);
        connect(cobMat, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
        cobMat->setMinimumWidth(120);
        hl->addWidget(cobMat);
      lMF->addLayout(hl);

      QHBoxLayout * h = new QHBoxLayout();
            h->addStretch();
            cbFixedSize = new QCheckBox("Fixed size");
            connect(cbFixedSize, &QCheckBox::clicked, this, &AWorldDelegate::onContentChanged);
            h->addWidget(cbFixedSize);

            QVBoxLayout * v1 = new QVBoxLayout();
                v1->setContentsMargins(2,0,2,0);
                v1->addWidget(new QLabel("Size XY:"));
                v1->addWidget(new QLabel("Size Z:"));
            h->addLayout(v1);

            QVBoxLayout * v2 = new QVBoxLayout();
                v2->setContentsMargins(2,0,2,0);
                ledSizeXY = new QLineEdit();
                connect(ledSizeXY, &QLineEdit::textChanged, this, &AWorldDelegate::onContentChanged);
                v2->addWidget(ledSizeXY);
                ledSizeZ  = new QLineEdit();
                connect(ledSizeZ, &QLineEdit::textChanged, this, &AWorldDelegate::onContentChanged);
                v2->addWidget(ledSizeZ);
            h->addLayout(v2);
            h->addStretch();
    lMF->addLayout(h);

    frMainFrame->setLayout(lMF);

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    ledSizeXY->setValidator(dv);
    ledSizeZ->setValidator(dv);
}

const QString AWorldDelegate::getName() const
{
    return "World";
}

bool AWorldDelegate::isValid(AGeoObject *)
{
    return true;
}

bool AWorldDelegate::updateObject(AGeoObject * obj) const
{
    obj->Material = cobMat->currentIndex();
    if (obj->Material == -1) obj->Material = 0; //protection

    AGeoBox * box = static_cast<AGeoBox*>(obj->Shape);
    box->dx = 0.5 * ledSizeXY->text().toDouble();
    box->dz = 0.5 * ledSizeZ->text().toDouble();
    ATypeWorldObject * typeWorld = static_cast<ATypeWorldObject *>(obj->ObjectType);
    typeWorld->bFixedSize = cbFixedSize->isChecked();

    return true;
}

void AWorldDelegate::Update(const AGeoObject *obj)
{
    int imat = obj->Material;
    if (imat < 0 || imat >= cobMat->count())
    {
        qWarning() << "Material index out of bounds!";
        imat = -1;
    }
    cobMat->setCurrentIndex(imat);

    const AGeoBox * box = static_cast<const AGeoBox*>(obj->Shape);
    ledSizeXY->setText(QString::number(box->dx*2.0));
    ledSizeZ->setText(QString::number(box->dz*2.0));
    ATypeWorldObject * typeWorld = static_cast<ATypeWorldObject *>(obj->ObjectType);
    cbFixedSize->setChecked(typeWorld->bFixedSize);
}

void AWorldDelegate::onContentChanged()
{
    emit ContentChanged();
}
