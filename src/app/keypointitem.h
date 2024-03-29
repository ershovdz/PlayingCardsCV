/*
 * Copyright (C) 2011, Mathieu Labbe - IntRoLab - Universite de Sherbrooke
 */

#ifndef KEYPOINTITEM_H_
#define KEYPOINTITEM_H_

#include <QtGui/QGraphicsEllipseItem>
#include <QtGui/QGraphicsTextItem>
#include <QtGui/QPen>
#include <QtGui/QBrush>

class KeypointItem : public QGraphicsEllipseItem
{
public:
	KeypointItem(int id, qreal x, qreal y, int r, const QString & info, const QColor & color = Qt::green, QGraphicsItem * parent = 0);
	virtual ~KeypointItem();

	void setColor(const QColor & color);
	int id() const {return _id;}

protected:
	virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
	virtual void hoverLeaveEvent ( QGraphicsSceneHoverEvent * event );
	virtual void focusInEvent ( QFocusEvent * event );
	virtual void focusOutEvent ( QFocusEvent * event );

private:
	void showDescription();
	void hideDescription();

private:
	QString _info;
	QGraphicsRectItem * _placeHolder;
	int _width;
	int _id;
};


#endif /* KEYPOINTITEM_H_ */
