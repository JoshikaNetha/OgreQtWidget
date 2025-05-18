#ifndef OGREWIDGET_H
#define OGREWIDGET_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <Ogre.h>
#include <OgreApplicationContext.h>
class QPlatformNativeInterface;
class OgreWidget : public QWidget
{
    Q_OBJECT
public:
     OgreWidget(QWidget *parent = nullptr);
    ~OgreWidget();

    void startSinbadAnimation();

    void createGrid(float size, int divisions) ;

protected:
    void resizeEvent(QResizeEvent* evt) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QPoint mLastMousePos;
    bool mIsLeftButtonPressed = false;


    void initializeOgre();
    void updateAnimation();

    Ogre::Root* mRoot = nullptr;
    Ogre::SceneManager* mSceneMgr = nullptr;
    Ogre::RenderWindow* mRenderWindow = nullptr;
    Ogre::Camera* mCamera = nullptr;
    Ogre::SceneNode* mCamNode = nullptr;
    QTimer mRenderTimer;

    QTimer* mAnimationTimer;
    Ogre::AnimationState* mAnimationState;

};

#endif // OGREWIDGET_H
