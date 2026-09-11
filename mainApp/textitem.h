 
/****************************************************************************
**
** Copyright (C) 2007-2009 Kevin Clague. All rights reserved.
** Copyright (C) 2015 - 2025 Trevor SANDY. All rights reserved.
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

/****************************************************************************
 *
 * This class provides a simple mechanism for displaying arbitrary text
 * on the page.  
 *
 ***************************************************************************/

#ifndef textItemH
#define textItemH

#include <QGraphicsTextItem>
#include <QFont>
#include "placement.h"
#include "meta.h"
#include "metaitem.h"
#include <QCoreApplication>

class TextItem : public QGraphicsTextItem, public Placement, public MetaItem
{
public:
  InsertMeta    meta;
  bool          positionChanged;
  PlacementType parentRelativeType;
  QPointF       position;
  PageTypeEnc   pageType;
  bool          pagePlaced;
  bool          textPlacement;
  bool          textChanged;
  bool          richText;

  TextItem()
  {
  }
  TextItem(
    InsertMeta meta,
    PageTypeEnc pageType,
    bool placement,
    QGraphicsItem *parent);

 void formatText(const QString &input, QString &output);

protected:
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent * /* event */);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * /* event */);
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
  virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
  virtual void focusInEvent(QFocusEvent *event);
  virtual void focusOutEvent(QFocusEvent *event);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void keyReleaseEvent(QKeyEvent *event);
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
  bool isHovered;
  bool mouseIsDown;
  // tr() must resolve to THIS class, not to the Q_OBJECT base it inherits
  // (QGraphicsTextItem / QObject). Without this declaration the context used at
  // runtime is the base class name, which no longer matches the context lupdate
  // assigns (the enclosing class), and every translation for this class is dead.
Q_DECLARE_TR_FUNCTIONS(TextItem);
};

#endif
