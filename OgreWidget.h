#ifndef OGREWIDGET_H
#define OGREWIDGET_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <Ogre.h>
#include <OgreApplicationContext.h>

#include <urdf_model/model.h>
#include <urdf_parser.h>

class QPlatformNativeInterface;
class OgreWidget : public QWidget
{
    Q_OBJECT
public:
    OgreWidget(QWidget *parent = nullptr);
    ~OgreWidget();



    void startSinbadAnimation(); // temp for animation


    void loadURDF(const std::string& urdfFilePath);


protected:
    void resizeEvent(QResizeEvent* evt) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:


    void                initializeOgre();
    void                createGrid(float size, int divisions) ;

    void                visualizeLinkRecursive(urdf::LinkConstSharedPtr link, Ogre::SceneNode* parentNode);

    Ogre::ManualObject* createBoxMesh(Ogre::SceneManager* sceneMgr, const std::string& name, float x, float y, float z);
    Ogre::Entity*       createPrimitiveEntity(const std::string& type, float size);

    void                generateSphere(Ogre::ManualObject* manual, float radius, int rings = 12, int segments = 12);
    void                generateBox(Ogre::ManualObject* manual, float x, float y, float z);


private:
    QPoint          mLastMousePos;
    bool            mIsLeftButtonPressed = false;

    Ogre::Root*         mRoot = nullptr;
    Ogre::SceneManager* mSceneMgr = nullptr;
    Ogre::RenderWindow* mRenderWindow = nullptr;
    Ogre::Camera*       mCamera = nullptr;
    Ogre::SceneNode*    mCamNode = nullptr;
    QTimer              mRenderTimer;

    // QTimer* mAnimationTimer;  for animation
    // Ogre::AnimationState* mAnimationState;
    // void                updateAnimation();

};

#endif // OGREWIDGET_H
