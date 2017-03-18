#include "slab.h"

#include <QDebug>
#include <QListWidget>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QLine>
#include <QSpinBox>
#include <QDoubleValidator>

//ROOT
#include "TROOT.h"
#include "TColor.h"

// ======= Delegate for XY properties =======
ASlabXYDelegate::ASlabXYDelegate()
{
  ShowState = ASlabXYDelegate::ShowNothing;
  Shapes << "Rectangular" << "Round" << "Polygon";

  // creating GUI
  QGridLayout* layXY = new QGridLayout(this);
  layXY->setContentsMargins(5,4,5,4);
  layXY->setVerticalSpacing(1);
  layXY->setHorizontalSpacing(3);
  comShape = new QComboBox();
  comShape->addItems(Shapes);
  comShape->setMaximumHeight(20);
  comShape->setMinimumWidth(80);
  comShape->setMaximumWidth(100);
  comShape->setContextMenuPolicy(Qt::NoContextMenu);
  layXY->addWidget(comShape, 0,0, 1, 2);
  sbSides = new QSpinBox();
  sbSides->setMinimum(3);
  sbSides->setValue(6);
  sbSides->setContextMenuPolicy(Qt::NoContextMenu);
  layXY->addWidget(sbSides, 0,2);
  ledSize1 = new QLineEdit();
  ledSize1->setMaximumWidth(50);
  ledSize1->setContextMenuPolicy(Qt::NoContextMenu);
  layXY->addWidget(ledSize1, 1, 0);
  ledSize2 = new QLineEdit();
  ledSize2->setMaximumWidth(50);
  ledSize2->setContextMenuPolicy(Qt::NoContextMenu);
  layXY->addWidget(ledSize2, 1, 1);
  ledAngle = new QLineEdit();
  ledAngle->setMaximumWidth(35);
  ledAngle->setContextMenuPolicy(Qt::NoContextMenu);
  layXY->addWidget(ledAngle, 1, 2);

  // setting tooltips
  comShape->setToolTip("Shape in XY plane");
  sbSides->setToolTip("Number of sides");  
  ledAngle->setToolTip("Rotation angle, degrees");
  //dynamic tooltips are set on UpdateGui()

  //validator
  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  dv->setBottom(0.001);
  QDoubleValidator* dv1 = new QDoubleValidator(this);
  dv1->setNotation(QDoubleValidator::ScientificNotation);

  ledSize1->setValidator(dv);
  ledSize2->setValidator(dv);
  ledAngle->setValidator(dv1);

  // reactions to user actions
  connect(comShape, SIGNAL(activated(int)), this, SLOT(onUserAction()));
  connect(sbSides, SIGNAL(editingFinished()), this, SLOT(onUserAction()));
  connect(ledSize1, SIGNAL(editingFinished()), this, SLOT(onUserAction()));
  connect(ledSize2, SIGNAL(editingFinished()), this, SLOT(onUserAction()));
  connect(ledAngle, SIGNAL(editingFinished()), this, SLOT(onUserAction()));

  connect(comShape, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
  connect(sbSides, SIGNAL(valueChanged(int)), this, SLOT(onContentChanged()));
  connect(ledSize1, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
  connect(ledSize2, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
  connect(ledAngle, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
}

void ASlabXYDelegate::CopyFrom(const ASlabXYDelegate &Another)
{
  comShape->setCurrentIndex( Another.comShape->currentIndex() );
  sbSides->setValue( Another.sbSides->value() );
  sbSides->setVisible( Another.comShape->currentIndex()==2 );
  ledSize2->setVisible( Another.comShape->currentIndex()==0 );
  ledAngle->setVisible( Another.comShape->currentIndex()!=1 );
  ledSize1->setText( Another.ledSize1->text() );
  ledSize2->setText( Another.ledSize2->text() );
  ledAngle->setText( Another.ledAngle->text() );
  ShowState = Another.ShowState;
}

const ASlabXYModel ASlabXYDelegate::GetData()
{
  ASlabXYModel Rec;
  Rec.shape = comShape->currentIndex();
  Rec.sides = sbSides->value();
  Rec.size1 = ledSize1->text().toDouble();
  Rec.size2 = ledSize2->text().toDouble();
  Rec.angle = ledAngle->text().toDouble();
  return Rec;
}

void ASlabXYDelegate::SetShowState(ASlabXYDelegate::ShowStates State)
{
  ShowState = State;
}

void ASlabXYDelegate::updateComponentVisibility()
{
  //qDebug() << ShowState << comShape->currentIndex();
  switch (ShowState)
    {
    case (ShowNothing):
      comShape->setVisible(false);
      sbSides->setVisible(false);
      ledSize1->setVisible(false);
      ledSize2->setVisible(false);
      ledAngle->setVisible(false);
      break;
    case (ShowSize):
      comShape->setVisible(false);
      sbSides->setVisible(false);
      ledSize1->setVisible(true);
      ledSize2->setVisible( comShape->currentIndex()==0 );
      ledAngle->setVisible(false);
      break;
    default: qWarning() << "Unknown Show state!";
     case (ShowAll):
      comShape->setVisible(true);
      sbSides->setVisible(comShape->currentIndex()==2);
      ledSize1->setVisible(true);
      ledSize2->setVisible(comShape->currentIndex()==0);
      ledAngle->setVisible(comShape->currentIndex()!=1);
      break;
    }
}

void ASlabXYDelegate::UpdateGui(const ASlabXYModel &ModelRecord)
{
  if (ModelRecord.shape<0 || ModelRecord.shape>comShape->count()-1) comShape->setCurrentIndex(-1);
  else comShape->setCurrentIndex(ModelRecord.shape);
  sbSides->setValue(ModelRecord.sides);
  ledSize1->setText( QString::number(ModelRecord.size1) );
  ledSize2->setText( QString::number(ModelRecord.size2) );
  ledAngle->setText( QString::number(ModelRecord.angle) );
  switch (comShape->currentIndex())
    {
    case 0:
      ledSize1->setToolTip("Size1, mm");
      ledSize2->setToolTip("Size2, mm");
      break;
    case 1:
      ledSize1->setToolTip("Diameter, mm");
      break;
    case 2:
      ledSize1->setToolTip("Size, mm");
      break;
    }

  updateComponentVisibility();
}

void ASlabXYDelegate::onContentChanged()
{  
  emit ContentChanged();
}

// ======= Layer delegate =======
ASlabDelegate::ASlabDelegate(QWidget *parent) : QWidget(parent)
{
  // creating GUI
  frMain = new QFrame(this);
  frMain->setFrameShape(QFrame::Box);
  frMain->setGeometry(3,2, 470, 50);

  frColor = new QFrame(this);
  frColor->setAutoFillBackground(true);
  frColor->setBackgroundRole(QPalette::Background);
  color = -1;
  frColor->setGeometry(474, 2, 5, 50);  //resized externally to follow frMain geometry

  QHBoxLayout* layMain = new QHBoxLayout(this);
  frMain->setLayout(layMain);
  layMain->setContentsMargins(5,0,0,0);
  cbOnOff = new QCheckBox();
  cbOnOff->setToolTip("Enable/disable this slab");
  cbOnOff->setChecked(true);
  layMain->addWidget(cbOnOff);

  QFrame* frEnable = new QFrame();
  frEnable->setMinimumHeight(30);
  frEnable->setFrameShape(QFrame::NoFrame);
  connect(cbOnOff, SIGNAL(toggled(bool)), frEnable, SLOT(setEnabled(bool)));
  layMain->addWidget(frEnable);

  QHBoxLayout* layZ = new QHBoxLayout();
  layZ->setContentsMargins(0,0,0,0);
  //layZ->setSpacing(5);
  frEnable->setLayout(layZ);

  leName = new QLineEdit();
  leName->setMaximumHeight(20);
  leName->setToolTip("Slab name");
  leName->setMinimumWidth(70);
  leName->setMaximumWidth(100);
  leName->setContextMenuPolicy(Qt::NoContextMenu);
  layZ->addWidget(leName);
  ledHeight = new QLineEdit();
  ledHeight->setMaximumHeight(20);
  ledHeight->setMaximumWidth(50);
  ledHeight->setMinimumWidth(40);
  ledHeight->setToolTip("Slab height, mm");
  ledHeight->setContextMenuPolicy(Qt::NoContextMenu);
  layZ->addWidget(ledHeight);

  comMaterial = new QComboBox();
  comMaterial->setMaximumHeight(20);
  comMaterial->setMinimumWidth(90);
  comMaterial->setContextMenuPolicy(Qt::NoContextMenu);
  comMaterial->setToolTip("Slab material");
  layZ->addWidget(comMaterial);

  // XY properties are sub-delegated
  XYdelegate = new ASlabXYDelegate();
  XYdelegate->setMinimumWidth(150);
  layZ->addWidget(XYdelegate);

  // center (Z=0) indication
  frCenterZ = new QFrame(this);
  frCenterZ->setFrameShape(QFrame::Box);
  frCenterZ->setLineWidth(1);
  frCenterZ->setGeometry(0,0, 2,54);
  fCenter = false;

  //validator
  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  dv->setBottom(0.001);
  ledHeight->setValidator(dv);

  // reactions to user actions
  connect(cbOnOff, SIGNAL(clicked(bool)), this, SLOT(onUserAction()));
  connect(leName, SIGNAL(editingFinished()), this, SLOT(onUserAction()));
  connect(ledHeight, SIGNAL(editingFinished()), this, SLOT(onUserAction()));
  connect(comMaterial, SIGNAL(activated(int)), this, SLOT(onUserAction()));
  connect(XYdelegate, SIGNAL(RequestModelUpdate()), this, SLOT(onUserAction()));

  connect(cbOnOff, SIGNAL(clicked(bool)), this, SLOT(onContentChanged()));
  connect(leName, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
  connect(ledHeight, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
  connect(comMaterial, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
  connect(XYdelegate, SIGNAL(ContentChanged()), this, SLOT(onContentChanged()));

  //initial values
  ASlabModel empty;
  UpdateGui(empty);
}

ASlabDelegate::~ASlabDelegate()
{
  //no need to delete gui components - they have this widget as parent
  //qDebug() << "Delete delegate triggered";
}

void ASlabDelegate::UpdateModel(ASlabModel *record)
{
  record->fActive = cbOnOff->isChecked();
  record->name = leName->text();
  record->height = ledHeight->text().toDouble();
  record->material = comMaterial->currentIndex();
  //record->fCenter = frCenterZ->isVisible();
  record->fCenter = fCenter;
  record->XYrecord = XYdelegate->GetData();
}

const ASlabModel ASlabDelegate::GetData()
{
  ASlabModel r;
  UpdateModel(&r);
  return r;
}

void ASlabDelegate::SetColorIndicator(int color)
{
  this->color = color;
  if (color == -1) return;

  TColor *tcol = gROOT->GetColor(color);
  int red = 255*tcol->GetRed();
  int green = 255*tcol->GetGreen();
  int blue = 255*tcol->GetBlue();

  QPalette palette = frColor->palette();
  palette.setColor( QPalette::Normal, frColor->backgroundRole(), QColor( red, green, blue ) );
  palette.setColor( QPalette::Inactive, frColor->backgroundRole(), QColor( red, green, blue ) );
  frColor->setPalette( palette );
}

QListWidgetItem *ASlabDelegate::CreateNewWidgetItem()
{
  QListWidgetItem* item = new QListWidgetItem();
  item->setSizeHint(QSize(300, 54));
  item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));
  return item;
}

void ASlabDelegate::UpdateGui(const ASlabModel &ModelRecord)
{
  cbOnOff->setChecked(ModelRecord.fActive);
  leName->setText(ModelRecord.name);
  ledHeight->setText(QString::number(ModelRecord.height));

  if (ModelRecord.material<0 || ModelRecord.material>comMaterial->count()-1) comMaterial->setCurrentIndex(-1);
  else comMaterial->setCurrentIndex(ModelRecord.material);

  //if (ModelRecord.fCenter) frCenterZ->setVisible(true);
  //else frCenterZ->setVisible(false);
  fCenter = ModelRecord.fCenter;
  frCenterZ->setVisible(fCenter);

  XYdelegate->UpdateGui(ModelRecord.XYrecord);
}

void ASlabDelegate::onUserAction()
{    
  emit RequestModelUpdate(this);
}

void ASlabXYModel::writeToJson(QJsonObject &json)
{
  json["shape"] = shape;
  json["sides"] = sides;
  json["size1"] = size1;
  json["size2"] = size2;
  json["angle"] = angle;
}

void ASlabXYModel::readFromJson(QJsonObject &json)
{
  *this = ASlabXYModel(); //to load default values
  if (json.contains("shape")) shape = json["shape"].toInt();
  if (json.contains("sides")) sides = json["sides"].toInt();
  if (json.contains("size1")) size1 = json["size1"].toDouble();
  if (json.contains("size2")) size2 = json["size2"].toDouble();
  if (json.contains("angle")) angle = json["angle"].toDouble();
}

void ASlabModel::writeToJson(QJsonObject &json)
{
  json["fActive"] = fActive;
  json["name"] = name;
  json["height"] = height;
  json["material"] = material;
  json["fCenter"] = fCenter;
  json["color"] = color;
  json["style"] = style;
  json["width"] = width;

  QJsonObject XYjson;
  XYrecord.writeToJson(XYjson);
  json["XY"] = XYjson;
}

void ASlabModel::readFromJson(QJsonObject &json)
{
  *this = ASlabModel(); //to load default values
  if (json.contains("fActive")) fActive = json["fActive"].toBool();
  if (json.contains("name")) name = json["name"].toString();
  if (json.contains("height")) height = json["height"].toDouble();
  if (json.contains("material")) material = json["material"].toInt();
  if (json.contains("fCenter")) fCenter = json["fCenter"].toBool();
  if (json.contains("XY"))
    {
      QJsonObject XYjson = json["XY"].toObject();
      XYrecord.readFromJson(XYjson);
    }
  if (json.contains("color")) color = json["color"].toInt();
  if (json.contains("style")) style = json["style"].toInt();
  if (json.contains("width")) width = json["width"].toInt();
}

void ASlabModel::importFromOldJson(QJsonObject &json)
{
  *this = ASlabModel(); //to load default values
  if (json.contains("Active")) fActive = json["Active"].toBool();
  if (json.contains("name")) name = "Undefined";
  if (json.contains("Height")) height = json["Height"].toDouble();
  if (json.contains("Material")) material = json["Material"].toInt();

  if (json.contains("Shape")) XYrecord.shape = json["Shape"].toInt();
  if (json.contains("Sides")) XYrecord.sides = json["Sides"].toInt();
  if (json.contains("Size1")) XYrecord.size1 = json["Size1"].toDouble();
  if (json.contains("Size2")) XYrecord.size2 = json["Size2"].toDouble();
  if (json.contains("Angle")) XYrecord.angle = json["Angle"].toDouble();

  //name and center status has to be given on detector level - they depend on index
}
