#include "kmultiverttabbar.h"
#include "kmultiverttabbar.moc"
#include <qbutton.h>
#include <qpopupmenu.h>
#include <qlayout.h>
#include <qpainter.h>

KMultiVertTabBarInternal::KMultiVertTabBarInternal(QWidget *parent):QScrollView(parent)
	{
		tabs.setAutoDelete(true);
		setHScrollBarMode(AlwaysOff);
		setVScrollBarMode(AlwaysOff);
		box=new QVBox(viewport());
		box->setFixedWidth(24);
		setFixedWidth(24);
		addChild(box);
		setFrameStyle(NoFrame);
		viewport()->setBackgroundMode(Qt::PaletteBackground);
	}


KMultiVertTabBarTab* KMultiVertTabBarInternal::getTab(int id)
{
        for (uint pos=0;pos<tabs.count();pos++)
        {
                if (tabs.at(pos)->id()==id)
                        return tabs.at(pos);
        }
        return 0;
}


int KMultiVertTabBarInternal::insertTab(QPixmap pic ,int id,const QString& text)
{
	KMultiVertTabBarTab  *tab;
	tabs.append(tab= new KMultiVertTabBarTab(pic,QString::null,id,box,position));
	tab->show();
	return 0;
}

void KMultiVertTabBarInternal::removeTab(int id)
{
	for (uint pos=0;pos<tabs.count();pos++)
	{
		if (tabs.at(pos)->id()==id)
		{
			tabs.remove(pos);
			break;
		}
	}
}

void KMultiVertTabBarInternal::setPosition(enum KMultiVertTabBar::KMultiVertTabBarPosition pos)
{
	position=pos;
	for (uint i=0;i<tabs.count();i++)
		tabs.at(i)->setPosition(position);
}


KMultiVertTabBarButton::KMultiVertTabBarButton(const QPixmap& pic,const QString& text, QPopupMenu *popup,
		int id,QWidget *parent,KMultiVertTabBar::KMultiVertTabBarPosition pos)
	:QPushButton(pic,text,parent)
{
	position=pos;
  	if (popup) setPopup(popup);
	setFlat(true);
	setFixedHeight(24);
	setFixedWidth(24);
	m_id=id;
	connect(this,SIGNAL(clicked()),this,SLOT(slotClicked()));
}


void KMultiVertTabBarButton::slotClicked()
{
	emit clicked(m_id);
}

void KMultiVertTabBarButton::setPosition(KMultiVertTabBar::KMultiVertTabBarPosition pos)
{
	position=pos;
	repaint();
}




KMultiVertTabBarTab::KMultiVertTabBarTab(const QPixmap& pic, const QString& text,
		int id,QWidget *parent,KMultiVertTabBar::KMultiVertTabBarPosition pos)
	:KMultiVertTabBarButton(pic,text,0,id,parent,pos)
{
	setToggleButton(true);
}


void KMultiVertTabBarTab::drawButton(QPainter *paint)
{
        QPixmap pixmap = iconSet()->pixmap( QIconSet::Small, QIconSet::Normal );
	if (!isOn())
	{

		if (position==KMultiVertTabBar::Right)
		{
			paint->fillRect(0,0,21,21,QBrush(colorGroup().background()));

			paint->setPen(colorGroup().light());
			paint->drawLine(0,23,23,23);
			paint->drawLine(0,22,23,22);

			paint->drawPixmap(12-pixmap.width()/2,12-pixmap.height()/2,pixmap);

			paint->setPen(colorGroup().light());
			paint->drawLine(19,21,21,21);
			paint->drawLine(21,19,21,21);

			paint->setPen(colorGroup().shadow());
			paint->drawLine(0,0,23,0);
			paint->drawLine(0,0,0,23);
			paint->drawLine(1,0,1,23);

		}
		else
		{
			paint->setPen(colorGroup().light());
			paint->drawLine(0,23,23,23);
			paint->drawLine(0,22,23,22);
			paint->setPen(colorGroup().shadow());
			paint->drawLine(0,0,23,0);
//			paint->drawLine(1,0,23,23);
			paint->fillRect(0,1,23,21,QBrush(colorGroup().background()));
			paint->drawPixmap(12-pixmap.width()/2,12-pixmap.height()/2,pixmap);
			//paint->drawLine(2,21,3,21);
			//paint->drawLine(2,19,2,21);

			paint->setPen(colorGroup().light());
			paint->drawLine(23,0,23,23);
			paint->drawLine(22,0,22,23);

			paint->setPen(colorGroup().shadow());
			paint->drawLine(0,0,0,23);
			paint->drawLine(1,0,1,23);


		}


	}
	else
	{
		if (position==KMultiVertTabBar::Right)
		{
			paint->setPen(colorGroup().shadow());
			paint->drawLine(0,23,23,23);
			paint->drawLine(0,22,23,22);
			paint->drawLine(23,0,23,23);
			paint->drawLine(22,0,22,23);
			paint->fillRect(0,0,21,21,QBrush(colorGroup().light()));
			paint->drawPixmap(10-pixmap.width()/2,10-pixmap.height()/2,pixmap);
			paint->drawLine(19,21,21,21);
			paint->drawLine(21,19,21,21);
		}
		else
		{
			paint->setPen(colorGroup().shadow());
			paint->drawLine(0,23,23,23);
			paint->drawLine(0,22,23,22);
			//paint->drawLine(0,0,0,23);
			//paint->drawLine(1,0,0,23);
			paint->fillRect(0,0,23,21,QBrush(colorGroup().light()));
			paint->drawPixmap(10-pixmap.width()/2,10-pixmap.height()/2,pixmap);
			//paint->drawLine(2,21,3,21);
			//paint->drawLine(2,19,2,21);
		}

	}
}







KMultiVertTabBar::KMultiVertTabBar(QWidget *parent):QWidget(parent)
{
	buttons.setAutoDelete(true);
	setFixedWidth(24);
	l=new QVBoxLayout(this);
	l->setMargin(0);
	l->setAutoAdd(false);
	
	internal=new KMultiVertTabBarInternal(this);
	l->insertWidget(0,internal);
	QFrame *tmp;
	l->insertWidget(0,tmp=new QFrame(this));
	tmp->setFixedHeight(4);
	tmp->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	tmp->setLineWidth(2);
	setPosition(KMultiVertTabBar::Right);
}

/*int KMultiVertTabBar::insertButton(QPixmap pic,int id ,const QString&)
{
  (new KToolbarButton(pic,id,internal))->show();
  return 0;
}*/

int KMultiVertTabBar::insertButton(QPixmap pic ,int id,QPopupMenu *popup,const QString&)
{
	KMultiVertTabBarButton  *btn;
	buttons.append(btn= new KMultiVertTabBarButton(pic,QString::null,
			popup,id,this,position));
	l->insertWidget(0,btn);
	btn->show();
	return 0;
}

int KMultiVertTabBar::insertTab(QPixmap pic ,int id ,const QString& text)
{
 internal->insertTab(pic,id,text);
 return 0;
}

KMultiVertTabBarButton* KMultiVertTabBar::getButton(int id)
{
	for (uint pos=0;pos<buttons.count();pos++)
	{
		if (buttons.at(pos)->id()==id)
			return buttons.at(pos);
	}
	return 0;
}

KMultiVertTabBarTab* KMultiVertTabBar::getTab(int id)
{
	return internal->getTab(id);
}



void KMultiVertTabBar::removeButton(int id)
{
	for (uint pos=0;pos<buttons.count();pos++)
	{
		if (buttons.at(pos)->id()==id)
		{
			buttons.remove(pos);
			break;
		}
	}
}

void KMultiVertTabBar::removeTab(int id)
{
	internal->removeTab(id);
}

void KMultiVertTabBar::setTab(int id,bool state)
{
	KMultiVertTabBarTab *tab=getTab(id);
	if (tab)
	{
		if(state) tab->setOn(true); else tab->setOn(false);
	}
}

bool KMultiVertTabBar::isTabRaised(int id)
{
	KMultiVertTabBarTab *tab=getTab(id);
	if (tab)
	{
		if (tab->isOn()) return true;
	}
	return false;
}


void KMultiVertTabBar::setPosition(KMultiVertTabBarPosition pos)
{
	position=pos;
	internal->setPosition(pos);
	for (uint i=0;i<buttons.count();i++)
		buttons.at(i)->setPosition(pos);
}
