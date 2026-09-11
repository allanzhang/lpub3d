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

#ifndef CSIANNOTATION_H
#define CSIANNOTATION_H

#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QLineF>
#include "range_element.h"
#include "csiitem.h"
#include "ranges.h"
#include "resize.h"
#include <QCoreApplication>

class Step;
class QGraphicsView;
class CsiAnnotation;
class PliPart;

class PlacementCsiPart : public Placement,
                         public QGraphicsRectItem
{
public:
    bool outline;
    int stepNumber;
    Where top, bottom;
    PlacementCsiPart(){}
    PlacementCsiPart(
        CsiPartMeta   &_csiPartMeta,
        QGraphicsItem *_parent = nullptr);
    bool hasOffset();
    void toggleOutline();
    void setOutline(QPainter *painter);
    void paint( QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w);
};

class CsiAnnotation : public Placement
{
public:
  CsiAnnotationMeta caMeta;
  CsiPartMeta       csiPartMeta;
  Where             partLine,metaLine; // Part / Meta line in the model file
  CsiAnnotationKind kind;   // Which ASSEM ANNOTATION sub-command created this annotation
  bool              hidden; // Hidden flag of the active sub-command data

  CsiAnnotation(){}
  CsiAnnotation(
     const Where       &_here,
     const Where       &_partLine,
     CsiAnnotationMeta &_caMeta,
     CsiAnnotationKind  _kind = CsiAnnotationIcon);
  virtual ~CsiAnnotation(){}
  const CsiAnnotationIconData &activeData();
  bool setPlacement();
  bool setCsiPartLoc(int csiSize[]);
  bool setAnnotationLoc(float iconOffset[]);
};

class CsiAnnotationItem : public ResizeTextItem
{
public:
  PlacementCsiPart *placementCsiPart;
  Where             topOf,bottomOf;
  Where             partLine, metaLine;
  BorderMeta        border;
  BackgroundMeta    background;
  IntMeta           style;
  PlacementType     parentRelativeType;
  int               submodelLevel;
  StringListMeta    subModelColor;
  CsiAnnotationIconMeta icon;
  QRectF            textRect;
  QRectF            styleRect;
  int               stepNumber;

  // DEBUG TRACE STUFF
  PageMeta          pageMeta;
  QRect             csiItemRect;

  CsiAnnotationItem(
    QGraphicsItem *_parent = nullptr);

  virtual ~CsiAnnotationItem(){}

  void sizeIt();

  void setText(
    QString &text,
    QString &fontString,
    QString &toolTip)
  {
    setPlainText(text);
    QFont font;
    font.fromString(fontString);
    setFont(font);
    setToolTip(toolTip);
    setData(ObjectId, AssemAnnotationObj);
  }

  virtual void addGraphicsItems(
     CsiAnnotation *_ca,
     Step          *_step,
     PliPart       *_part,
     CsiItem       *_csiItem,
     bool           _movable);

  void setPos(double x, double y)
  {
    QGraphicsTextItem::setPos(x,y);
  }

  void setFlag(GraphicsItemFlag flag, bool value)
  {
    QGraphicsItem::setFlag(flag,value);
  }

  void setAlignment( Qt::Alignment flags )
  {
    alignment = flags;
  }

  void scaleDownFont();

protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
  virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
  virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
  void setAnnotationStyle(QPainter *painter);
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w);
  void change();

  QPointF              textOffset;
  Qt::Alignment	       alignment;
  bool isHovered;
  bool mouseIsDown;
  // tr() must resolve to THIS class, not to the Q_OBJECT base it inherits
  // (QGraphicsTextItem / QObject). Without this declaration the context used at
  // runtime is the base class name, which no longer matches the context lupdate
  // assigns (the enclosing class), and every translation for this class is dead.
Q_DECLARE_TR_FUNCTIONS(CsiAnnotationItem);
};

/*
 * Renders the ASSEM ANNOTATION ARROW sub-command: a straight black arrow whose
 * tail is the annotation anchor and whose tip points at the annotated part
 * (ray from the anchor to the part centre clipped at the part bounding box).
 */
class CsiAnnotationArrowItem : public QGraphicsPathItem, public Placement
{
public:
  PlacementCsiPart *placementCsiPart;
  Where             topOf,bottomOf;
  Where             partLine, metaLine;
  CsiAnnotationIconData icon;
  int               stepNumber;

  CsiAnnotationArrowItem(QGraphicsItem *_parent = nullptr);
  void addGraphicsItems(
     CsiAnnotation *_ca,
     Step          *_step,
     PliPart       *_part,
     CsiItem       *_csiItem);
  void setArrowPath();
};

/*
 * Renders the ASSEM ANNOTATION STEP_BADGE sub-command: a circular step
 * number badge centred on the annotated part's bounding-box centre. The
 * badge is anchored purely by the part geometry (partOffset 0 0); no leader
 * line is drawn.
 */
class CsiAnnotationBadgeItem : public QGraphicsTextItem, public Placement
{
public:
  PlacementCsiPart *placementCsiPart;
  Where             topOf,bottomOf;
  Where             partLine, metaLine;
  CsiAnnotationIconData icon;
  int               stepNumber;
  QRectF            badgeRect;

  CsiAnnotationBadgeItem(QGraphicsItem *_parent = nullptr);
  void addGraphicsItems(
     CsiAnnotation *_ca,
     Step          *_step,
     PliPart       *_part,
     CsiItem       *_csiItem);

  QRectF boundingRect() const override;

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w);
};

#endif // CSIANNOTATION_H
