/****************************************************************************
**
** Copyright (C) 2019 - 2025 Trevor SANDY. All rights reserved.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/*********************************************************************
 *
 * This class creates a CSI Annotation icon
 *
 ********************************************************************/

#include <QGraphicsSceneContextMenuEvent>
#include "csiannotation.h"
#include "commonmenus.h"
#include "lpub_object.h"
#include "metaitem.h"
#include "color.h"
#include "step.h"
#include "calloutbackgrounditem.h"
#include "lpub.h"

PlacementCsiPart::PlacementCsiPart(
    CsiPartMeta   &_csiPartMeta,
    QGraphicsItem *_parent)
{
  relativeType = CsiPartType;
  placement    = _csiPartMeta.placement;
  margin       = _csiPartMeta.margin;
  size[XX]     = _csiPartMeta.size.valuePixels(XX);
  size[YY]     = _csiPartMeta.size.valuePixels(YY);
  loc[XX]     += _csiPartMeta.loc.valuePixels(XX);
  loc[YY]     += _csiPartMeta.loc.valuePixels(YY);
  top          = _csiPartMeta.placement.here();
  bottom       = _csiPartMeta.placement.here();
  outline      = false;

  setData(ObjectId, AssemAnnotationPartObj);
  setZValue(ASSEMANNOTATIONPART_ZVALUE_DEFAULT);
  setParentItem(_parent);
}


// Compute the reference rectangle (page or callout) in CSI-local coordinates
// for annotations placed relative to a frame other than the assembly.  The
// page/callout rectangles live in scene coordinates; mapping them by the CSI's
// scene position puts them into the CSI item's local space where every
// annotation child lives.  When *direct is true the returned reference must be
// used to place the annotation itself (PAGE/CALLOUT); otherwise the original
// anchor chain (CSI -> anchor -> annotation) is kept (ASSEM).
static Placement csiAnnotationReference(CsiItem      *csiItem,
                                        Step         *step,
                                        PlacementType relativeTo,
                                        bool         *direct)
{
    Placement reference;
    *direct = false;
    if (relativeTo == PageType) {
        int pageW = lpub->pageSize(lpub->page.meta.LPub.page, 0);
        int pageH = lpub->pageSize(lpub->page.meta.LPub.page, 1);
        // Default bleed margin: keep page-edge annotations fully visible inside
        // the page instead of being clipped at the export boundary.  The badge
        // is centred on the reference edge (its size is still 0 when placed),
        // so the inset must be at least the badge radius.
        int bleed = qRound(0.16f * lpub->page.meta.LPub.resolution.value());
        QPointF csiScenePos = csiItem->scenePos();
        reference.loc[XX]  = qRound(-csiScenePos.x()) + bleed;
        reference.loc[YY]  = qRound(-csiScenePos.y()) + bleed;
        reference.size[XX] = pageW - 2*bleed;
        reference.size[YY] = pageH - 2*bleed;
        *direct = true;
    } else if (relativeTo == CalloutType && step) {
        for (int i = 0; i < step->list.size(); ++i) {
            Callout *callout = step->list.at(i);
            QRectF cref;
            if (callout->background)
                cref = callout->background->sceneBoundingRect();
            else
                cref = QRectF(callout->loc[XX], callout->loc[YY],
                              callout->size[XX], callout->size[YY]);
            QPointF csiScenePos = csiItem->scenePos();
            reference.loc[XX]  = qRound(cref.left() - csiScenePos.x());
            reference.loc[YY]  = qRound(cref.top()  - csiScenePos.y());
            reference.size[XX] = qRound(cref.width());
            reference.size[YY] = qRound(cref.height());
            *direct = true;
        }
    }
    return reference;
}

bool PlacementCsiPart::hasOffset()
{
    bool zero;
    zero  = placement.value().offsets[XX] == 0.0f;
    zero &= placement.value().offsets[YY] == 0.0f;
    return !zero;
}

void PlacementCsiPart::paint( QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w)
{
//#ifdef QT_DEBUG_MODE
    if (outline)
        setOutline(painter);
//#endif
    QGraphicsRectItem::paint(painter, o, w);
}

void PlacementCsiPart::toggleOutline()
{
    bool curState = outline;
    outline = ! curState;
    update();
}
void PlacementCsiPart::setOutline(QPainter *painter)
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    painter->setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing);
#else
    painter->setRenderHints(QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing);
#endif
    int ibt = int(1.0f/32.0f);

    /* BORDER */
    QPen borderPen;
    borderPen.setColor(QColor(Qt::blue));       // Qt::transparent
    borderPen.setCapStyle(Qt::SquareCap);
    borderPen.setJoinStyle(Qt::RoundJoin);
    borderPen.setStyle(Qt::DashLine);           // Qt::SolidLine
    borderPen.setWidth(ibt);
    painter->setPen(borderPen);

    /* BACKGROUND */
    painter->setBrush(QColor(Qt::transparent)); // Qt::blue

    QRectF irect(ibt/2,ibt/2,size[XX]-ibt,size[YY]-ibt);
    painter->drawRect(irect);
}

CsiAnnotation::CsiAnnotation(
    const Where       &_here,
    const Where       &_partLine,
    CsiAnnotationMeta &_caMeta,
    CsiAnnotationKind  _kind)
{
    caMeta        = _caMeta;
    metaLine      = _here;
    partLine      = _partLine;
    kind          = _kind;
    hidden        = activeData().hidden;

    if (hidden)
        return;

    PlacementData pld;

    // set PlacementCsiPart placement
    pld             = csiPartMeta.placement.value();
    pld.offsets[XX] = activeData().partOffset[XX];
    pld.offsets[YY] = activeData().partOffset[YY];
    csiPartMeta.placement.setValue(pld);
    csiPartMeta.size.setValuePixels(XX,activeData().partSize[XX]);
    csiPartMeta.size.setValuePixels(YY,activeData().partSize[YY]);

    // set CsiAnnotation placement
    pld             = caMeta.placement.value();
    pld.offsets[XX] = activeData().iconOffset[XX];
    pld.offsets[YY] = activeData().iconOffset[YY];
    caMeta.placement.setValue(pld);
    setPlacement();

    margin.setValuesInches(0.0f,0.0f);
    placement       = caMeta.placement;
    relativeType    = CsiAnnotationType;
}

const CsiAnnotationIconData &CsiAnnotation::activeData()
{
    switch (kind) {
      case CsiAnnotationArrow:
        return caMeta.arrow.value();
      case CsiAnnotationBadge:
        return caMeta.stepBadge.value();
      case CsiAnnotationIcon:
      default:
        return caMeta.icon.value();
    }
}

bool CsiAnnotation::setPlacement()
{
    // Raw placement tokens are stored as PlacementEnc/PrepositionEnc ints.
    // Match them against placementDecode (meta.cpp) - the same canonical table
    // PlacementMeta::parse uses - so every annotation keeps its own edge.
    int placement, justification, preposition;
    if (activeData().placements.size() == 2) {
        // 2-token form "<PLACEMENT> <INSIDE|OUTSIDE>": no justification given,
        // default to CENTER (placementDecode stores CENTER for all these entries,
        // e.g. TopOutside = {Top, Center, Outside}).
        placement     = activeData().placements.at(0).toInt();
        justification = Center;
        preposition   = activeData().placements.at(1).toInt();
    }
    else
    if (activeData().placements.size() == 3) {
        placement     = activeData().placements.at(0).toInt();
        justification = activeData().placements.at(1).toInt();
        preposition   = activeData().placements.at(2).toInt();
    }
    else {
        return false;
    }

    int i;
    for (i = 0; i < NumSpots; i++) {
        if (placementDecode[i][0] == placement &&
            placementDecode[i][1] == justification &&
            placementDecode[i][2] == preposition) {
            break;
          }
    }
    if (i == NumSpots) {
        return false;
    }
    RectPlacement  _placementR = RectPlacement(i);;
    PlacementType  _relativeTo = (activeData().relativeTo >= 0)
                               ? PlacementType(activeData().relativeTo)
                               : caMeta.placement.value().relativeTo;
    caMeta.placement.setValue(_placementR,_relativeTo);
    return true;
}

bool CsiAnnotation::setCsiPartLoc(int csiSize[])
{
    float partOffset[2] =
    { activeData().partOffset[XX],
      activeData().partOffset[YY]};

    if (partOffset[XX] != 0.0f || partOffset[YY] != 0.0f) {
        csiPartMeta.loc.setValuePixels(XX,int(csiSize[XX] * partOffset[XX]));
        csiPartMeta.loc.setValuePixels(YY,int(csiSize[YY] * partOffset[YY]));
    } else {
        csiPartMeta.loc.setValue(XX,0.0f);
        csiPartMeta.loc.setValue(YY,0.0f);
        return false;
    }
    return true;
}

bool CsiAnnotation::setAnnotationLoc(float iconOffset[])
{
    if (iconOffset[XX] != 0.0f || iconOffset[YY] != 0.0f) {
        loc[XX] += iconOffset[XX];
        loc[YY] += iconOffset[YY];
    } else {
        loc[XX] = 0;
        loc[YY] = 0;
        return false;
    }
    return true;
}

CsiAnnotationItem::CsiAnnotationItem(
   QGraphicsItem  *_parent)
  : ResizeTextItem(_parent)
  , alignment( Qt::AlignCenter | Qt::AlignVCenter )
  , isHovered(false)
  , mouseIsDown(false)
{
   relativeType         = CsiAnnotationType;
   placementCsiPart     = nullptr;

   setAcceptHoverEvents(true);
   setData(ObjectId, AssemAnnotationObj);
   setZValue(ASSEMANNOTATION_ZVALUE_DEFAULT);
}

void CsiAnnotationItem::addGraphicsItems(
   CsiAnnotation        *_ca,
   Step                 *_step,
   PliPart              *_part,
   CsiItem              *_csiItem,
   bool                  _movable)
{
    icon                = _ca->caMeta.icon;
    partLine            = _ca->partLine;
    metaLine            = _ca->metaLine;
    placement           = _ca->placement;
    margin              = _part->styleMeta.margin;
    border              = _part->styleMeta.border;
    background          = _part->styleMeta.background;
    style               = _part->styleMeta.style;
    submodelLevel       = _csiItem->submodelLevel;
    parentRelativeType  = _csiItem->parentRelativeType;
    subModelColor       = _step->pli.pliMeta.subModelColor;
    stepNumber          = _step->stepNumber.number;
    positionChanged     = false;
    switch (parentRelativeType) {
      case CalloutType:
        topOf           = _step->topOfCallout();
        bottomOf        = _step->bottomOfCallout();
        break;
      default:
        topOf           = _step->topOfStep();
        bottomOf        = _step->bottomOfStep();
        break;
    }

    setParentItem(_csiItem);

    QString textString  = _part->text;
    QString fontString  = _part->styleMeta.font.valueFoo();
    QString colorString = _part->styleMeta.color.value();

    QString toolTip = tr("CSI Part Annotation %1 %2 (%3) \"%4\" - right-click to modify")
                         .arg(_part->type, LDrawColor::name(_part->color), _part->color, _part->description);

    setText(textString,fontString,toolTip);

    QColor color(colorString);
    setDefaultTextColor(color);

    textRect  = QRectF(0,0,document()->size().width(),document()->size().height());

    if (style.value() == AnnotationStyle::none) {
        styleRect = textRect;
    } else {
        // set rectangle size and dimensions parameters
        bool fixedStyle  = style.value() != AnnotationStyle::rectangle;
        bool isRectangle = style.value() == AnnotationStyle::rectangle;
        UnitsMeta rectSize;
        if (isRectangle) {
            if ((_part->styleMeta.size.valueInches(XX) > STYLE_SIZE_DEFAULT  ||
                 _part->styleMeta.size.valueInches(XX) < STYLE_SIZE_DEFAULT) ||
                (_part->styleMeta.size.valueInches(YY) > STYLE_SIZE_DEFAULT  ||
                 _part->styleMeta.size.valueInches(YY) < STYLE_SIZE_DEFAULT)) {
                rectSize = _part->styleMeta.size;
            } else {
                int widthInPx  = int(textRect.width());
                int heightInPx = int(textRect.height());
                rectSize.setValuePixels(XX,widthInPx);
                rectSize.setValuePixels(YY,heightInPx);
            }
        }
        QRectF _styleRect = QRectF(0,0,fixedStyle ? _part->styleMeta.size.valuePixels(XX) : isRectangle ? rectSize.valuePixels(XX) : textRect.width(),
                                       fixedStyle ? _part->styleMeta.size.valuePixels(YY) : isRectangle ? rectSize.valuePixels(YY) : textRect.height());
        styleRect = boundingRect().adjusted(0,0,_styleRect.width()-textRect.width(),_styleRect.height()-textRect.height());

        // scale down the font as needed
        scaleDownFont();

        // center document text in style size
        setTextWidth(-1);
        setTextWidth(styleRect.width());
        QTextBlockFormat format;
        format.setAlignment(alignment);
        QTextCursor cursor = textCursor();
        cursor.select(QTextCursor::Document);
        cursor.mergeBlockFormat(format);
        cursor.clearSelection();
        setTextCursor(cursor);

        // adjust text horizontal alignment
        textOffset.setX(border.valueInches().thickness/2);
        // adjust text vertical alignment
        textOffset.setY((styleRect.height()-textRect.height())/2);
    }
    size[XX] = int(styleRect.size().width());
    size[YY] = int(styleRect.size().height());

    sizeIt();

    // set PlacementCsiPart location based on csi size

    _ca->setCsiPartLoc(_csiItem->size);

    bool direct = false;
    Placement reference = csiAnnotationReference(_csiItem, _step,
                                                 placement.value().relativeTo,
                                                 &direct);

    // place PlacementCsiPart relative to CSI - the anchor stays at the part
    // edge so ARROW/STEP_BADGE leader lines keep pointing at the part

    placementCsiPart = new PlacementCsiPart(_ca->csiPartMeta,_csiItem);
    placementCsiPart->top = topOf;
    placementCsiPart->bottom = bottomOf;
    placementCsiPart->stepNumber = stepNumber;
    if (! placementCsiPart->hasOffset())
        _csiItem->placeRelative(placementCsiPart);

    placementCsiPart->setPos(placementCsiPart->loc[XX],
                             placementCsiPart->loc[YY]);

    // place CsiAnnotation Icon - PAGE/CALLOUT place the annotation directly
    // against the reference frame; ASSEM keeps the anchor chain

    bool hasLoc = _ca->setAnnotationLoc(placement.value().offsets);
    if (hasLoc) {
        loc[XX] = _ca->loc[XX];
        loc[YY] = _ca->loc[YY];
    } else if (direct) {
        reference.placeRelative(this);
        _ca->assign(this);
    } else {
        placementCsiPart->placeRelative(this);
        _ca->assign(this);
    }
    setPos(loc[XX],loc[YY]);

    setData(ObjectId, AssemAnnotationObj);
    setZValue(ASSEMANNOTATION_ZVALUE_DEFAULT);

    setFlag(QGraphicsItem::ItemIsMovable, _movable);
    setFlag(QGraphicsItem::ItemIsSelectable, _movable);
}

void CsiAnnotationItem::scaleDownFont() {
  qreal widthRatio  = styleRect.width()  / textRect.width();
  qreal heightRatio = styleRect.height() / textRect.height();
  if (widthRatio < 1 || heightRatio < 1) {
    QFont font = this->QGraphicsTextItem::font();
    qreal saveFontSizeF = font.pointSizeF();
    font.setPointSizeF(font.pointSizeF()*qMin(widthRatio,heightRatio));
    setFont(font);
    textRect = QRectF(0,0,document()->size().width(),document()->size().height());

    if (textRect.width() > styleRect.width() || textRect.height() > styleRect.height()) {
      scaleDownFont();
    }

    emit gui->messageSig(LOG_INFO,QMessageBox::tr("CSI annotation font size was adjusted from %1 to %2.")
                                                  .arg(saveFontSizeF).arg(font.pointSizeF()));
  }
}

void CsiAnnotationItem::sizeIt()
{
    size[XX] += int(border.valuePixels().margin[XX]);
    size[YY] += int(border.valuePixels().margin[YY]);
    size[XX] += int(border.valuePixels().thickness);
    size[YY] += int(border.valuePixels().thickness);
}

void CsiAnnotationItem::setAnnotationStyle(QPainter *painter)
{
    // set painter and render hints
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    painter->setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing);
#else
    painter->setRenderHints(QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing);
#endif
    // set the background then set the border and paint both in one go.

    /* BACKGROUND */
    QColor brushColor;
    BackgroundData backgroundData = background.value();

    switch(backgroundData.type) {
    case BackgroundData::BgColor:
        brushColor = LDrawColor::color(backgroundData.string);
        break;
    case BackgroundData::BgSubmodelColor:
        brushColor = LDrawColor::color(subModelColor.value(0));
        break;
    default:
        brushColor = Qt::transparent;
        break;
    }
    painter->setBrush(brushColor);

    /* BORDER */
    QPen borderPen;
    QColor borderPenColor;
    BorderData borderData = border.valuePixels();
    if (borderData.type == BorderData::BdrNone) {
        borderPenColor = Qt::transparent;
    } else {
        borderPenColor =  LDrawColor::color(borderData.color);
    }
    borderPen.setColor(borderPenColor);
    borderPen.setCapStyle(Qt::RoundCap);
    borderPen.setJoinStyle(Qt::RoundJoin);
    if (borderData.line == BorderData::BdrLnNone) {
          borderPen.setStyle(Qt::NoPen);
    }
    else if (borderData.line == BorderData::BdrLnSolid) {
        borderPen.setStyle(Qt::SolidLine);
    }
    else if (borderData.line == BorderData::BdrLnDash) {
        borderPen.setStyle(Qt::DashLine);
    }
    else if (borderData.line == BorderData::BdrLnDot) {
        borderPen.setStyle(Qt::DotLine);
    }
    else if (borderData.line == BorderData::BdrLnDashDot) {
        borderPen.setStyle(Qt::DashDotLine);
    }
    else if (borderData.line == BorderData::BdrLnDashDotDot) {
        borderPen.setStyle(Qt::DashDotDotLine);
    }
     borderPen.setWidth(int(borderData.thickness));

    painter->setPen(borderPen);

    // draw icon shape - background and border
    int bt = int(borderData.thickness);
    QRectF bgRect(bt/2,bt/2,size[XX]-bt,size[YY]-bt);
    if (style.value() != AnnotationStyle::circle) {
        if (borderData.type == BorderData::BdrRound) {
            // set icon border dimensions
            qreal rx = double(borderData.radius);
            qreal ry = double(borderData.radius);
            qreal dx = size[XX];
            qreal dy = size[YY];
            if (int(dx) && int(dy)) {
                if (dx > dy) {
                    rx *= dy;
                    rx /= dx;
                } else {
                    ry *= dx;
                    ry /= dy;
                }
            }
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
            painter->drawRoundedRect(bgRect,int(rx),int(ry));
#else
            painter->drawRoundRect(bgRect,int(rx),int(ry));
#endif
        } else {
            painter->drawRect(bgRect);
        }
    } else {
        painter->drawEllipse(bgRect);
    }
}

void CsiAnnotationItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w)
{
    if (style.value() != AnnotationStyle::none) {
        setAnnotationStyle(painter);
        QRectF textBounds = boundingRect();
        textBounds.translate(textOffset);
        painter->translate(textBounds.left(), textBounds.top());
    }
    QPen pen;
    pen.setColor(isHovered ? QColor(Preferences::sceneGuideColor) : Qt::black);
    pen.setWidth(0/*cosmetic*/);
    pen.setStyle(isHovered ? Qt::PenStyle(Preferences::sceneGuidesLine) : Qt::NoPen);
    painter->setPen(pen);
    painter->setBrush(Qt::transparent);
    painter->drawRect(this->boundingRect());
    QGraphicsTextItem::paint(painter, o, w);
}

void CsiAnnotationItem::change() {
    updateCsiAnnotationIconMeta(metaLine, &icon);
}

void CsiAnnotationItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    isHovered = !this->isSelected() && !mouseIsDown;
    QGraphicsItem::hoverEnterEvent(event);
}

void CsiAnnotationItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    isHovered = false;
    QGraphicsItem::hoverLeaveEvent(event);
}

void CsiAnnotationItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  mouseIsDown = true;
  position = pos();
  positionChanged = false;
  QGraphicsItem::mousePressEvent(event);
  //placeGrabbers();
}

void CsiAnnotationItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  positionChanged = true;
  QGraphicsItem::mouseMoveEvent(event);
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {
      //placeGrabbers();
  }
}

void CsiAnnotationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    mouseIsDown = false;
    QGraphicsItem::mouseReleaseEvent(event);

    if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {

        // back annotate the movement of the PLI into the LDraw file.

        if (positionChanged) {

            beginMacro(QString("DragCsiAnnotation"));

            QPointF newPosition;
            newPosition = pos() - position;

            if (newPosition.x() || newPosition.y()) {

                positionChanged = true;

                PlacementData placementData    = placement.value();
                if (relativeToSize[XX] > 1 || relativeToSize[YY] > 1) {
                    placementData.offsets[XX] += newPosition.x()+loc[XX];
                    placementData.offsets[YY] += newPosition.y()+loc[YY];
                } else {
                    placementData.offsets[XX] += newPosition.x()/relativeToSize[XX];
                    placementData.offsets[YY] += newPosition.y()/relativeToSize[YY];
                }
                placement.setValue(placementData);

                CsiAnnotationIconData caiData = icon.value();
                caiData.iconOffset[XX]        = placementData.offsets[XX];
                caiData.iconOffset[YY]        = placementData.offsets[YY];
                icon.setValue(caiData);

                change();
            }
            endMacro();
        }
    }
}

void CsiAnnotationItem::contextMenuEvent(
    QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
  const QString name = tr("CSI Part Annotation");

  QAction *placementAction    = lpub->getAct("placementAction.1");
  PlacementData placementData = placement.value();
  placementAction->setWhatsThis(commonMenus.naturalLanguagePlacementWhatsThis(CsiAnnotationType,placementData,name));
  commonMenus.addAction(placementAction,menu,name);

  QAction *hideAction         = lpub->getAct("hideCsiAnnotationAction.1");
  commonMenus.addAction(hideAction,menu,name);

  QAction *toggleCsiPartRectAction = lpub->getAct("toggleCsiPartRectAction.1");
  commonMenus.addAction(toggleCsiPartRectAction,menu);

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == nullptr) {
      return;
  }
  else if (selectedAction == toggleCsiPartRectAction) {
      placementCsiPart->toggleOutline();
      gui->pagescene()->update();
  } else if (selectedAction == placementAction) {
      changeCsiAnnotationPlacement(
              parentRelativeType,
              CsiAnnotationType,
              tr("%1 Placement").arg(name),
              metaLine,
              metaLine,
             &placement,
             &icon,true,1,false);
   } else if (selectedAction == hideAction) {
              CsiAnnotationIconData caid = icon.value();
              caid.hidden = true;
              icon.setValue(caid);
              updateCsiAnnotationIconMeta(metaLine, &icon);
   }
}

/*-------------------------------------------------------------------------*
 * ASSEM ANNOTATION ARROW
 *
 * Renders a straight black arrow whose tail is the annotation anchor and
 * whose tip points at the annotated part.  The tip is the intersection of
 * the ray (anchor -> part centre) with the part bounding box, computed with
 * the same rectLineIntersect geometry the callout pointers use.
 *-------------------------------------------------------------------------*/

CsiAnnotationArrowItem::CsiAnnotationArrowItem(QGraphicsItem *_parent)
  : QGraphicsPathItem(_parent)
{
  relativeType     = CsiAnnotationType;
  placementCsiPart = nullptr;

  setData(ObjectId, AssemAnnotationObj);
  setZValue(ASSEMANNOTATION_ZVALUE_DEFAULT);
}

void CsiAnnotationArrowItem::addGraphicsItems(
   CsiAnnotation *_ca,
   Step          *_step,
   PliPart       *_part,
   CsiItem       *_csiItem)
{
    Q_UNUSED(_part)

    icon            = _ca->activeData();
    partLine        = _ca->partLine;
    metaLine        = _ca->metaLine;
    placement       = _ca->placement;
    stepNumber      = _step->stepNumber.number;
    switch (_csiItem->parentRelativeType) {
      case CalloutType:
        topOf    = _step->topOfCallout();
        bottomOf = _step->bottomOfCallout();
        break;
      default:
        topOf    = _step->topOfStep();
        bottomOf = _step->bottomOfStep();
        break;
    }

    setParentItem(_csiItem);

    bool direct = false;
    Placement reference = csiAnnotationReference(_csiItem, _step,
                                                 placement.value().relativeTo,
                                                 &direct);

    // place the anchor part rectangle exactly like the ICON annotation - the
    // anchor stays at the part edge so the arrow tip keeps pointing at the part
    _ca->setCsiPartLoc(_csiItem->size);

    placementCsiPart = new PlacementCsiPart(_ca->csiPartMeta,_csiItem);
    placementCsiPart->top = topOf;
    placementCsiPart->bottom = bottomOf;
    placementCsiPart->stepNumber = stepNumber;
    if (! placementCsiPart->hasOffset())
        _csiItem->placeRelative(placementCsiPart);

    placementCsiPart->setPos(placementCsiPart->loc[XX],
                             placementCsiPart->loc[YY]);

    // place the arrow tail - PAGE/CALLOUT place the arrow directly against
    // the reference frame; ASSEM keeps the anchor chain
    bool hasLoc = _ca->setAnnotationLoc(placement.value().offsets);
    if (hasLoc) {
        loc[XX] = _ca->loc[XX];
        loc[YY] = _ca->loc[YY];
    } else if (direct) {
        reference.placeRelative(this);
        _ca->assign(this);
    } else {
        placementCsiPart->placeRelative(this);
        _ca->assign(this);
    }
    setPos(loc[XX],loc[YY]);

    setArrowPath();

    QSizeF itemSize = boundingRect().size();
    size[XX] = qRound(itemSize.width());
    size[YY] = qRound(itemSize.height());
}

void CsiAnnotationArrowItem::setArrowPath()
{
    // part rectangle in our local coordinates
    QPointF partPos = placementCsiPart->pos() - pos();
    QRectF  partRect(partPos.x(), partPos.y(),
                     placementCsiPart->size[XX], placementCsiPart->size[YY]);
    QPointF anchor(0.0, 0.0);
    QPointF partCentre = partRect.center();

    // rectLineIntersect expects the rectangle in the (0,0,w,h) frame, so
    // translate the part rect to the origin, then shift the intersection back.
    QPointF partTopLeft = partRect.topLeft();
    QRect   originRect(0, 0, qRound(partRect.width()), qRound(partRect.height()));
    QPoint  tip(qRound(partCentre.x() - partTopLeft.x()),
                qRound(partCentre.y() - partTopLeft.y()));
    QPoint  loc(qRound(anchor.x() - partTopLeft.x()),
                qRound(anchor.y() - partTopLeft.y()));
    QPoint  intersect;
    PlacementEnc enc;
    bool hit = PointerItem::rectLineIntersect(tip, loc, originRect, 0, intersect, enc);
    if (! hit)
        intersect = tip;
    intersect += QPoint(qRound(partTopLeft.x()), qRound(partTopLeft.y()));

    QLineF shaft(anchor, QPointF(intersect));
    qreal  shaftLen = shaft.length();
    if (shaftLen < 1.0) {
        setPath(QPainterPath());
        return;
    }

    QPointF dir = QPointF(shaft.dx() / shaftLen, shaft.dy() / shaftLen);
    QPointF perp(-dir.y(), dir.x());

    // Minimum visible shaft length.  With OUTSIDE placement the anchor sits
    // right at the part edge, so the anchor->part intersection is only a few
    // pixels and no line is visible.  Pull the tail back along the reversed
    // direction until the shaft is at least this long, keeping the tip at the
    // part boundary so the arrow still points exactly at the part.
    const qreal minShaft = 0.8 * lpub->page.meta.LPub.resolution.value();
    QPointF tail = anchor;
    if (shaftLen < minShaft)
        tail = anchor - dir * (minShaft - shaftLen);

    // arrow head geometry (filled triangle at the part boundary)
    const qreal headLen      = 10.0;
    const qreal headHalfWide = 6.0;
    QPointF headBase = QPointF(intersect) - dir * headLen;

    QPainterPath path;
    path.moveTo(tail);                         // tail (pulled back if needed)
    path.lineTo(headBase);                     // shaft
    path.moveTo(QPointF(intersect));           // tip at the part boundary
    path.lineTo(headBase + perp * headHalfWide);
    path.lineTo(headBase - perp * headHalfWide);
    path.closeSubpath();

    // keep the tail anchored at the item origin (0,0) so the arrow tip and
    // tail stay exactly on the anchor/part geometry; Qt handles negative
    // bounding-rect coordinates natively.
    setPath(path);

    QPen pen(QColor(0x00,0x00,0x00));
    pen.setWidthF(2.0);
    pen.setCapStyle(Qt::SquareCap);
    pen.setJoinStyle(Qt::MiterJoin);
    setPen(pen);
    setBrush(QColor(0x00,0x00,0x00));
}

/*-------------------------------------------------------------------------*
 * ASSEM ANNOTATION STEP_BADGE
 *
 * Renders a circular step-number badge centred on the annotated part's
 * bounding-box centre. Anchored purely by the part geometry (partOffset 0 0,
 * neutral anchor); no leader line is drawn.
 *-------------------------------------------------------------------------*/

CsiAnnotationBadgeItem::CsiAnnotationBadgeItem(QGraphicsItem *_parent)
  : QGraphicsTextItem(_parent)
{
  relativeType     = CsiAnnotationType;
  placementCsiPart = nullptr;

  setData(ObjectId, AssemAnnotationObj);
  setZValue(ASSEMANNOTATION_ZVALUE_DEFAULT);
}

void CsiAnnotationBadgeItem::addGraphicsItems(
   CsiAnnotation *_ca,
   Step          *_step,
   PliPart       *_part,
   CsiItem       *_csiItem)
{
    icon            = _ca->activeData();
    partLine        = _ca->partLine;
    metaLine        = _ca->metaLine;
    placement       = _ca->placement;
    // Badge number = 1-based ordinal of this STEP_BADGE command within the
    // page (user requirement: number by command count). Previously every
    // badge on a page showed the same LPub step number (e.g. all badges on
    // page 1 showed "1"); now the 1st badge shows 1, 2nd shows 2, ...
    stepNumber      = 0;
    for (int i = 0; i < _step->csiAnnotations.size(); ++i) {
        CsiAnnotation *ann = _step->csiAnnotations.at(i);
        if (ann->kind != CsiAnnotationBadge)
            continue;
        ++stepNumber;
        if (ann == _ca)
            break;
    }
    switch (_csiItem->parentRelativeType) {
      case CalloutType:
        topOf    = _step->topOfCallout();
        bottomOf = _step->bottomOfCallout();
        break;
      default:
        topOf    = _step->topOfStep();
        bottomOf = _step->bottomOfStep();
        break;
    }

    setParentItem(_csiItem);

    // Badge anchor = the annotated part's bounding-box centre, computed
    // purely from the part geometry (Ground Truth). partOffset 0 0 (neutral
    // anchor): the badge is placed exactly on the part centre - no partOffset
    // projection, no placement-chain offset. The badge therefore sits on the
    // part itself and no leader line is drawn.
    _ca->setCsiPartLoc(_csiItem->size);

    placementCsiPart = new PlacementCsiPart(_ca->csiPartMeta,_csiItem);
    placementCsiPart->top = topOf;
    placementCsiPart->bottom = bottomOf;
    placementCsiPart->stepNumber = stepNumber;
    if (! placementCsiPart->hasOffset())
        _csiItem->placeRelative(placementCsiPart);
    placementCsiPart->setPos(placementCsiPart->loc[XX],
                             placementCsiPart->loc[YY]);

    // part bounding box in this item's frame (= the CSI item frame)
    QPointF partPos = placementCsiPart->pos();
    QRectF  partRect(partPos.x(), partPos.y(),
                     placementCsiPart->size[XX], placementCsiPart->size[YY]);
    QPointF partCentre = partRect.center();
    loc[XX] = partCentre.x();
    loc[YY] = partCentre.y();

    // badge metrics: step number text padded to a rounded rectangle
    QString text = QString::number(stepNumber);
    QFont   font;
    font.fromString(_part->styleMeta.font.valueFoo());
    if (font.pointSizeF() < 20.0)
        font.setPointSizeF(20.0);
    font.setBold(true);
    setFont(font);

    QFontMetricsF fm(font);
    QRectF textRect = fm.boundingRect(text);
    const qreal padX = 8.0;
    const qreal padY = 4.0;
    qreal badgeD = qMax(textRect.width() + 2*padX, textRect.height() + 2*padY);
    badgeRect = QRectF(0, 0, badgeD, badgeD);   // square -> circular badge
    size[XX]  = qRound(badgeRect.width());
    size[YY]  = qRound(badgeRect.height());

    // centre the badge on the anchor
    setPos(loc[XX] - badgeRect.width()/2.0,
           loc[YY] - badgeRect.height()/2.0);

    // Clamp the badge fully inside the page (with a bleed margin) so badges
    // placed at a page edge are never clipped by the export boundary.  The
    // badge rect is square, so only its width is needed.
    {
        int pageW = lpub->pageSize(lpub->page.meta.LPub.page, 0);
        int pageH = lpub->pageSize(lpub->page.meta.LPub.page, 1);
        int bleed = qRound(0.16f * lpub->page.meta.LPub.resolution.value());
        QPointF csiScenePos = _csiItem->scenePos();
        qreal pageL = -csiScenePos.x() + bleed;
        qreal pageT = -csiScenePos.y() + bleed;
        qreal pageR = pageL + pageW - 2*bleed - badgeRect.width();
        qreal pageB = pageT + pageH - 2*bleed - badgeRect.height();
        if (pageR < pageL) pageR = pageL;
        if (pageB < pageT) pageB = pageT;
        qreal bx = pos().x();
        qreal by = pos().y();
        bx = qMax(pageL, qMin(pageR, bx));
        by = qMax(pageT, qMin(pageB, by));
        setPos(bx, by);
    }
}

QRectF CsiAnnotationBadgeItem::boundingRect() const
{
    return badgeRect.adjusted(-2, -2, 2, 2);
}

void CsiAnnotationBadgeItem::paint(
   QPainter                        *painter,
   const QStyleOptionGraphicsItem  *o,
   QWidget                         *w)
{
    Q_UNUSED(o)
    Q_UNUSED(w)

    painter->setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing);

    // The badge item overrides paint() and draws the step number directly, so
    // the item font must be applied to the painter explicitly here (setFont()
    // alone would only affect the underlying QGraphicsTextItem document).
    painter->setFont(font());

    // badge background + border
    QPen borderPen(QColor(0x00,0x00,0x00));
    borderPen.setWidthF(1.5);
    painter->setPen(borderPen);
    painter->setBrush(QColor(0xFF,0xFF,0xFF));
    painter->drawEllipse(badgeRect);

    // step number text centred in the badge
    painter->setPen(QColor(0x00,0x00,0x00));
    painter->drawText(badgeRect, Qt::AlignCenter, QString::number(stepNumber));
}
