#ifndef __BEHAVIOUR_H__
#define __BEHAVIOUR_H__


#include <kcmodule.h>
#include <kconfig.h>


class QCheckBox;
class QSlider;
class QLabel;


//-----------------------------------------------------------------------------


class KBehaviourOptions : public KCModule
{
  Q_OBJECT
public:
  /**
   * @param showBuiltinGroup : set to true for konqueror, false for kdesktop
   */
  KBehaviourOptions(KConfig *config, QString group, bool showBuiltinGroup, QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();


protected slots:

  virtual void slotClick();
  void changed();


private:

  KConfig *g_pConfig;
  QString groupname;

  QCheckBox *cbSingleClick;
  QCheckBox *cbAutoSelect;
  QLabel *lDelay;
  QSlider *slAutoSelect;
  QCheckBox *cbCursor;
  QCheckBox *cbUnderline;

  QCheckBox *cbEmbedText;
  QCheckBox *cbEmbedImage;
  QCheckBox *cbEmbedOther;
};


#endif		// __BEHAVIOUR_H__

