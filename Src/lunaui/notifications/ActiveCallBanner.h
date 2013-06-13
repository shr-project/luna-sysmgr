/* @@@LICENSE
*
*      Copyright (c) 2010-2013 LG Electronics, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */




#ifndef ACTIVECALLBANNER_H
#define ACTIVECALLBANNER_H

#include "Common.h"

#include <stdint.h>
#include <QString>
#include <QTimer>
#include <QFont>
#include <QGraphicsObject>

class ActiveCallBanner : public QGraphicsObject
{
	Q_OBJECT

public:

	ActiveCallBanner(int width, int height, const std::string& iconFile, 
				     const std::string& message, uint32_t startTime);

	void updateProperties(uint32_t startTime, const std::string& iconFile, const std::string& msg);

	void handleTap();

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	virtual QRectF boundingRect() const { return m_boundingRect; }

private Q_SLOTS:

	void recomputeTime();
	 
private:

	void updateScreenPixmap();
	
	QString m_message;
	QString m_elidedMessage;
	QString m_time;
	uint32_t m_startTime;
	
	QFont m_font;
	QPixmap m_icon;
	QTimer m_timer;

	QRectF m_boundingRect;
	int m_originalWidth;
};

#endif /* ACTIVECALLBANNER_H */
