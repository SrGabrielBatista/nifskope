#ifndef LIGHTINGWIDGET_H
#define LIGHTINGWIDGET_H


///Mod copy from https://github.com/niftools/nifskope/pull/147/commits/30954e7f01f3d779a2a1fd37d363e8a6ad560bd3
#include <QWidget>
#include <QAction>

#include <memory>

class GLView;


namespace Ui {
class LightingWidget;
}

class LightingWidget : public QWidget
{
    Q_OBJECT

public:
    LightingWidget( GLView * ogl, QWidget * parent = nullptr);
    ~LightingWidget();

	void setDefaults();
	void setActions( QVector<QAction *> actions );

private:
	std::unique_ptr<Ui::LightingWidget> ui;

	enum
	{
		BRIGHT = 1440,
		POS = 720,

		DirMin = 0,
		DirMax = BRIGHT,
		AmbientMin = 0,
		AmbientMax = BRIGHT,
		DeclinationMin = -POS,
		DeclinationMax = POS,
		PlanarAngleMin = -POS,
		PlanarAngleMax = POS,

		DirDefault = DirMax / 2,
		AmbientDefault = AmbientMax * 3 / 8,
		DeclinationDefault = (DeclinationMax + DeclinationMin),
		PlanarAngleDefault = (PlanarAngleMax + PlanarAngleMin)
	};
};

#endif // LIGHTINGWIDGET_H
