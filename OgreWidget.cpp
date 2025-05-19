#include "OgreWidget.h"
#include <QWindow>
#include <QResizeEvent>
#include <OgreRenderWindow.h>
#include <OgreWindowEventUtilities.h>
#include "QPlatformSurfaceEvent"
#include <QMouseEvent>
#include <QWheelEvent>
#include <OgreEntity.h>
#include <OgreMeshManager.h>
#include <urdf_model/link.h>
#include <urdf_model/pose.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

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

}

OgreWidget::~OgreWidget() {
    mRenderTimer.stop();
    if (mRoot) {
        mRoot->shutdown();
        delete mRoot;
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

void OgreWidget::loadURDF(const std::string &urdfFilePath)
{
    // Load file into string
    std::ifstream urdfFile(urdfFilePath);
    if (!urdfFile.is_open()) {
        qDebug() << "Failed to open URDF file: " << urdfFilePath;
        return;
    }

    std::string xmlString((std::istreambuf_iterator<char>(urdfFile)),
                          std::istreambuf_iterator<char>());

    // Parse URDF
    urdf::ModelInterfaceSharedPtr model = urdf::parseURDF(xmlString);
    if (!model) {
        qDebug() << "Failed to parse URDF." ;
        return;
    }

    qDebug() << "Loaded URDF model: " << model->getName() ;

    // Visualize root link
    urdf::LinkConstSharedPtr rootLink = model->getRoot();
    if (rootLink) {
        qDebug() << "Loaded URDF rootLink " ;
        visualizeLinkRecursive(rootLink, mSceneMgr->getRootSceneNode());
    }
}

void OgreWidget::visualizeLinkRecursive(urdf::LinkConstSharedPtr link, Ogre::SceneNode *parentNode)
{
    qDebug() << "visualizeLinkRecursive " ;
    Ogre::SceneNode* linkNode = parentNode->createChildSceneNode(link->name);
    // linkNode->setScale(Ogre::Vector3(10.0f, 10.0f, 10.0f));

    // Load visual geometry
    if (link->visual && link->visual->geometry) {
        urdf::GeometrySharedPtr geom = link->visual->geometry;

        if (geom->type == urdf::Geometry::MESH) {
            urdf::Mesh* mesh = dynamic_cast<urdf::Mesh*>(geom.get());
            std::string meshPath = mesh->filename;

            // Load the mesh as an OGRE entity
            std::string entityName = link->name + "_Entity";
            try {
                Ogre::Entity* entity = mSceneMgr->createEntity(entityName, meshPath);
                linkNode->attachObject(entity);
            } catch (const Ogre::Exception& e) {
                qDebug() << "Failed to load mesh:" << QString::fromStdString(meshPath)
                << "Error:" << QString::fromStdString(e.getFullDescription());
            }

            // TODO: apply scale from mesh->scale
        }
        if (geom->type == urdf::Geometry::BOX) {
            qDebug() << "visualizeLinkRecursive BOX " ;
            urdf::Box* box = dynamic_cast<urdf::Box*>(geom.get());
            Ogre::ManualObject* manual = createBoxMesh(mSceneMgr, "box_" + link->name, box->dim.x, box->dim.y, box->dim.z);  // ✅ Correct
            linkNode->attachObject(manual);
        }
        else if (geom->type == urdf::Geometry::SPHERE) {
            qDebug() << "visualizeLinkRecursive SPHERE " ;
            auto sphere = std::dynamic_pointer_cast<urdf::Sphere>(geom);
            if (sphere) {
                Ogre::Entity* sphereEntity = createPrimitiveEntity("Sphere", sphere->radius);
                linkNode->attachObject(sphereEntity);
            }
        }

        // TODO: handle primitive shapes like BOX, SPHERE, CYLINDER if needed
    }

    // Recursively process child links
    for (const auto& child : link->child_links) {
        urdf::JointSharedPtr joint = child->parent_joint;

        // Create transform from joint
        Ogre::SceneNode* jointNode = linkNode->createChildSceneNode(child->name + "_Joint");

        if (joint) {
            const urdf::Vector3& pos = joint->parent_to_joint_origin_transform.position;
            const urdf::Rotation& rot = joint->parent_to_joint_origin_transform.rotation;

            jointNode->setPosition(pos.x, pos.y, pos.z);

            double x, y, z, w;
            rot.getQuaternion(x, y, z, w);
            jointNode->setOrientation(Ogre::Quaternion(w, x, y, z));
        }

        visualizeLinkRecursive(child, jointNode);
    }
}
Ogre::ManualObject* OgreWidget::createBoxMesh(Ogre::SceneManager* sceneMgr, const std::string& name, float x, float y, float z) {

    qDebug() << "createBoxMesh " ;
    Ogre::ManualObject* box = sceneMgr->createManualObject(name);
    box->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);

    float hx = x * 0.5f, hy = y * 0.5f, hz = z * 0.5f;

    // Vertices
    Ogre::Vector3 vertices[8] = {
        {-hx, -hy, -hz}, {hx, -hy, -hz}, {hx, hy, -hz}, {-hx, hy, -hz},
        {-hx, -hy, hz},  {hx, -hy, hz},  {hx, hy, hz},  {-hx, hy, hz}
    };

    for (const auto& v : vertices) box->position(v);

    // Indices (12 triangles = 6 faces)
    uint16_t indices[] = {
        0,1,2,  2,3,0,  // back
        4,5,6,  6,7,4,  // front
        0,4,7,  7,3,0,  // left
        1,5,6,  6,2,1,  // right
        3,2,6,  6,7,3,  // top
        0,1,5,  5,4,0   // bottom
    };

    for (auto i : indices) box->index(i);

    box->end();

    qDebug() << "createBoxMesh created" ;
    return box;
}

Ogre::Entity* OgreWidget::createPrimitiveEntity(const std::string& type, float size)
{
    // Unique mesh name based on type and size
    std::string meshName = type + "_primitive_" + std::to_string(size);

    // Avoid duplicate mesh creation
    if (!Ogre::MeshManager::getSingleton().resourceExists(meshName))
    {
        Ogre::ManualObject* manual = mSceneMgr->createManualObject(meshName);
        manual->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);

        // Generate geometry based on type
        if (type == "Sphere") {
            generateSphere(manual, size);  // defined elsewhere
        }
        else if (type == "Box") {
            generateBox(manual, size, size, size);  // defined elsewhere
        }
        else {
            qDebug() << "Unsupported primitive type: " << QString::fromStdString(type);
            return nullptr;
        }

        manual->end();

        // Convert to mesh
        manual->convertToMesh(meshName);

        // Clean up the manual object (mesh is now stored)
        mSceneMgr->destroyManualObject(manual);
    }

    // Return the entity from the mesh
    return mSceneMgr->createEntity(meshName);
}

void OgreWidget::generateBox(Ogre::ManualObject* manual, float x, float y, float z)
{
    float hx = x * 0.5f, hy = y * 0.5f, hz = z * 0.5f;

    // 8 corners of a cube
    Ogre::Vector3 v[8] = {
        {-hx, -hy, -hz}, { hx, -hy, -hz}, { hx,  hy, -hz}, {-hx,  hy, -hz},
        {-hx, -hy,  hz}, { hx, -hy,  hz}, { hx,  hy,  hz}, {-hx,  hy,  hz}
    };

    int faces[6][4] = {
        {0,1,2,3}, {4,5,6,7}, {0,1,5,4},
        {2,3,7,6}, {1,2,6,5}, {0,3,7,4}
    };

    int vertIdx = 0;
    for (int f = 0; f < 6; ++f) {
        Ogre::Vector3 normal = (v[faces[f][1]] - v[faces[f][0]]).crossProduct(
                                                                    v[faces[f][2]] - v[faces[f][1]]).normalisedCopy();
        for (int i = 0; i < 4; ++i)
            manual->position(v[faces[f][i]]), manual->normal(normal), manual->textureCoord(i & 1, (i >> 1) & 1);

        manual->triangle(vertIdx, vertIdx+1, vertIdx+2);
        manual->triangle(vertIdx, vertIdx+2, vertIdx+3);
        vertIdx += 4;
    }
}

void OgreWidget::generateSphere(Ogre::ManualObject* manual, float radius, int rings , int segments )
{
    for (int r = 0; r <= rings; ++r) {
        float phi = Ogre::Math::PI * r / rings;
        for (int s = 0; s <= segments; ++s) {
            float theta = 2 * Ogre::Math::PI * s / segments;
            float x = std::sin(phi) * std::cos(theta);
            float y = std::cos(phi);
            float z = std::sin(phi) * std::sin(theta);

            manual->position(x * radius, y * radius, z * radius);
            manual->normal(x, y, z);
            manual->textureCoord((float)s / segments, (float)r / rings);
        }
    }

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            int i1 = r * (segments + 1) + s;
            int i2 = i1 + segments + 1;

            manual->triangle(i1, i2, i1 + 1);
            manual->triangle(i1 + 1, i2, i2 + 1);
        }
    }
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
    Ogre::SceneNode* lightNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    qDebug()<<"Started ogreWidget initializeOgre createChildSceneNode";

    lightNode->setPosition(20, 80, 50);
    lightNode->attachObject(light);

    qDebug()<<"Started ogreWidget initializeOgre lightNode";

    Ogre::RTShader::ShaderGenerator::initialize();
    Ogre::RTShader::ShaderGenerator::getSingleton().addSceneManager(mSceneMgr);
    qDebug()<<"Started ogreWidget initializeOgre addSceneManager";


    mCamNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    mCamNode->setPosition(0, 0, 80);
    mCamNode->lookAt(Ogre::Vector3(0, 0, 0), Ogre::Node::TS_WORLD);

    qDebug()<<"Started ogreWidget initializeOgre SceneNode ";


    //-------------------------------------------------------------
    // Create the pivot node at the center of the scene (e.g. where Sinbad is)
    Ogre::SceneNode* pivotNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("CameraPivot");

    // Yaw node for horizontal rotation
    Ogre::SceneNode* yawNode = pivotNode->createChildSceneNode("CameraYawNode");

    // Pitch node for vertical rotation
    Ogre::SceneNode* pitchNode = yawNode->createChildSceneNode("CameraPitchNode");

    // Create the camera
    mCamera = mSceneMgr->createCamera("MainCamera");
    mCamera->setNearClipDistance(1);
    mCamera->setAutoAspectRatio(true);

    pitchNode->attachObject(mCamera);

    // Set initial camera position (e.g., back and slightly up)
    pitchNode->setPosition(0, 0, 80);  // set camera 80 units away from pivot
    pitchNode->lookAt(Ogre::Vector3(0, 0, 0), Ogre::Node::TS_PARENT);

    //-------------------------------------------------------------

    // Register resources
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        "/home/joshika/workSpace/QtProject/Repo/OGRE/sdk-13.1/share/OGRE/Media/packs/Sinbad",
        "FileSystem",
        "General"
        );
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
    qDebug() << "Resources initialized";

    createGrid(100.0f, 50);


    Ogre::Viewport* vp = mRenderWindow->addViewport(mCamera);
    vp->setBackgroundColour(Ogre::ColourValue(0.3, 0.3, 0.3));


    qDebug()<<"Started ogreWidget initializeOgre calling URDF ";

    loadURDF("/home/joshika/workSpace/Ros/URDF/demo1.urdf");
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
    static QPoint lastPos;
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - lastPos;

        float yawAngle = -delta.x() * 0.3f;   // sensitivity
        float pitchAngle = -delta.y() * 0.3f;

        Ogre::SceneNode* yawNode = mSceneMgr->getSceneNode("CameraYawNode");
        Ogre::SceneNode* pitchNode = mSceneMgr->getSceneNode("CameraPitchNode");

        yawNode->yaw(Ogre::Degree(yawAngle), Ogre::Node::TS_WORLD);
        pitchNode->pitch(Ogre::Degree(pitchAngle), Ogre::Node::TS_LOCAL);
    }
    lastPos = event->pos();

}

void OgreWidget::wheelEvent(QWheelEvent *event)
{
    Ogre::SceneNode* pitchNode = mSceneMgr->getSceneNode("CameraPitchNode");

    float zoomAmount = event->angleDelta().y() * 0.05f;
    pitchNode->translate(0, 0, -zoomAmount, Ogre::Node::TS_LOCAL);
}


