#include "OgreWidget.h"
#include <QWindow>
#include <QResizeEvent>
#include <OgreRenderWindow.h>
#include <OgreWindowEventUtilities.h>
#include "QPlatformSurfaceEvent"
#include <QMouseEvent>
#include <QWheelEvent>
OgreWidget::OgreWidget(QWidget* parent)
    : QWidget(parent)
{

    qDebug()<<"Started ogreWidget Constructor";
    setMouseTracking(true);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setUpdatesEnabled(false);  // we handle repainting manually

    initializeOgre();

    connect(&mRenderTimer, &QTimer::timeout, [this]() {
        if (mRoot && mRoot->renderOneFrame())
            Ogre::WindowEventUtilities::messagePump();
    });
    mRenderTimer.start(16);  // ~60fps


    mAnimationTimer = new QTimer(this);
    connect(mAnimationTimer, &QTimer::timeout, this, &OgreWidget::updateAnimation);
    mAnimationTimer->start(16);  // ~60 FPS
}

OgreWidget::~OgreWidget() {
    mRenderTimer.stop();
    if (mRoot) {
        mRoot->shutdown();
        delete mRoot;
    }
}

void OgreWidget::startSinbadAnimation()
{
    qDebug()<<"OgreWidget::startSinbadAnimation()";
    if (mAnimationState) {
        mAnimationState->setEnabled(true);
    }
}

void OgreWidget::createGrid(float size, int divisions)
{
    Ogre::ManualObject* grid = mSceneMgr->createManualObject("Grid");
    grid->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST);

    float half = size / 2.0f;
    float step = size / divisions;

    for (int i = 0; i <= divisions; ++i) {
        float pos = -half + i * step;

        // Lines parallel to X
        grid->position(pos, 0, -half);
        grid->position(pos, 0, half);

        // Lines parallel to Z
        grid->position(-half, 0, pos);
        grid->position(half, 0, pos);
    }

    grid->end();

    Ogre::SceneNode* gridNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("GridNode");
    gridNode->attachObject(grid);
}

void OgreWidget::initializeOgre() {

    qDebug()<<"Started ogreWidget initializeOgre";
    mRoot = new Ogre::Root();

    // Load plugin manually
    mRoot->loadPlugin("/home/joshika/workSpace/QtProject/Repo/OGRE/sdk-13.1/lib/OGRE/RenderSystem_GL.so.13.6");

    // OR if symlinked:
    mRoot->loadPlugin("/home/joshika/workSpace/QtProject/Repo/OGRE/sdk-13.1/lib/OGRE/RenderSystem_GL.so");

    qDebug()<<"Started ogreWidget initializeOgre loaded";



    // mRoot->loadPlugin("RenderSystem_GL");  // or "RenderSystem_GL3Plus", "RenderSystem_Direct3D11"
    Ogre::RenderSystem* rs = mRoot->getRenderSystemByName("OpenGL Rendering Subsystem");
    mRoot->setRenderSystem(rs);
    mRoot->initialise(false);  // don't auto-create window
    qDebug()<<"Started ogreWidget initializeOgre initialise false";

    // Get native window handle
    Ogre::NameValuePairList params;


    WId winid = winId();
    params["externalWindowHandle"] = Ogre::StringConverter::toString((size_t)winid);


    mRenderWindow = mRoot->createRenderWindow("OgreRenderWindow", width(), height(), false, &params);
    mRenderWindow->setActive(true);
    qDebug()<<"Started ogreWidget initializeOgre createRenderWindow ";

    mSceneMgr = mRoot->createSceneManager();
    qDebug()<<"Started ogreWidget initializeOgre createSceneManager";
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));
    Ogre::Light* light = mSceneMgr->createLight("MainLight");
    qDebug()<<"Started ogreWidget initializeOgre MainLight";
    Ogre::SceneNode* lightNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    qDebug()<<"Started ogreWidget initializeOgre createChildSceneNode";
    lightNode->setPosition(20, 80, 50);
    lightNode->attachObject(light);
    qDebug()<<"Started ogreWidget initializeOgre lightNode";

    Ogre::RTShader::ShaderGenerator::initialize();

    Ogre::RTShader::ShaderGenerator::getSingleton().addSceneManager(mSceneMgr);
    qDebug()<<"Started ogreWidget initializeOgre addSceneManager";

    mCamera = mSceneMgr->createCamera("MainCamera");
    qDebug()<<"Started ogreWidget initializeOgre MainCamera ";
    mCamera->setNearClipDistance(1);
    mCamera->setAutoAspectRatio(true);
    mCamNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    mCamNode->attachObject(mCamera);
    mCamNode->setPosition(0, 20, 80);
    mCamNode->lookAt(Ogre::Vector3(0, 0, 0), Ogre::Node::TS_WORLD);
    qDebug()<<"Started ogreWidget initializeOgre SceneNode ";

    // Register resources
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        "/home/joshika/workSpace/QtProject/Repo/OGRE/sdk-13.1/share/OGRE/Media/packs/Sinbad",
        "FileSystem",
        "General"
        );
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
    qDebug() << "Resources initialized";


    Ogre::Entity* ogreHead = mSceneMgr->createEntity("Sinbad.mesh");
    Ogre::SceneNode* headNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();

    headNode->setScale(5.0f, 5.0f, 5.0f);
    headNode->setPosition(0.0f, 25.0f, 0.0f);
    headNode->attachObject(ogreHead);
    headNode->yaw(Ogre::Degree(0));

    // Get animation state
    mAnimationState = ogreHead->getAnimationState("Dance");
    mAnimationState->setLoop(true);
    mAnimationState->setEnabled(true);
    createGrid(100.0f, 50);


    Ogre::Viewport* vp = mRenderWindow->addViewport(mCamera);
    vp->setBackgroundColour(Ogre::ColourValue(1, 0, 0));
    qDebug()<<"Started ogreWidget initializeOgre Viewport ";
}

void OgreWidget::resizeEvent(QResizeEvent* evt) {
    if (mRenderWindow) {
        mRenderWindow->resize(width(), height());
        mRenderWindow->windowMovedOrResized();
        qDebug() << "OgreWidget resized to:" << width() << "x" << height();
    }
    QWidget::resizeEvent(evt);
}

void OgreWidget::mousePressEvent(QMouseEvent *event)
{
    mLastMousePos = event->pos();
    if (event->button() == Qt::LeftButton)
        mIsLeftButtonPressed = true;
}

void OgreWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        mIsLeftButtonPressed = false;
}

void OgreWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (mIsLeftButtonPressed) {
        QPoint delta = event->pos() - mLastMousePos;
        mLastMousePos = event->pos();

        // Rotate camera node
        float sensitivity = 0.2f;
        mCamNode->yaw(Ogre::Degree(-delta.x() * sensitivity), Ogre::Node::TS_WORLD);
        mCamNode->pitch(Ogre::Degree(-delta.y() * sensitivity), Ogre::Node::TS_LOCAL);
    }
}

void OgreWidget::wheelEvent(QWheelEvent *event)
{
    float zoomSpeed = 1.1f;
    if (event->angleDelta().y() > 0)
        mCamNode->translate(Ogre::Vector3(0, 0, -zoomSpeed), Ogre::Node::TS_LOCAL);
    else
        mCamNode->translate(Ogre::Vector3(0, 0, zoomSpeed), Ogre::Node::TS_LOCAL);
}

void OgreWidget::updateAnimation() {
    if (mAnimationState) {
        mAnimationState->addTime(0.016);  // advance 16ms
    }

    mRenderWindow->update();  // ensure OGRE renders
}

