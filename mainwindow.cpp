#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QDebug>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // // Button
    // QPushButton* button = new QPushButton("Animate Sinbad", this);
    // button->setGeometry(10, 10, 150, 30);
    // connect(button, &QPushButton::clicked, this, &MainWindow::onAnimateClicked);

    // // OgreWidget below button
    // ogreWidget = new OgreWidget(this);
    // ogreWidget->setGeometry(0, 50, 800, 800 - 50);
    // ogreWidget->resize(800, 800);
    // ogreWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // // ogreWidget->show();

    // QVBoxLayout* layout = new QVBoxLayout;
    // layout->addWidget(ogreWidget);

    // QWidget* central = new QWidget;
    // central->setLayout(layout);
    // setCentralWidget(central);

    // Left Panel
    QWidget* leftPanel = new QWidget;
    QVBoxLayout* leftLayout = new QVBoxLayout;

    QPushButton* animateButton = new QPushButton("Animate Sinbad");
    leftLayout->addWidget(animateButton);
    leftLayout->addStretch();  // Push everything to the top
    leftPanel->setLayout(leftLayout);
    leftPanel->setMinimumWidth(200);  // Adjust as needed

    // === Ogre Widget (Center) ===
    ogreWidget = new OgreWidget(this);
    ogreWidget->resize(800, 800);
    ogreWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // === Right Panel ===
    QWidget* rightPanel = new QWidget;
    QVBoxLayout* rightLayout = new QVBoxLayout;

    QLabel* infoLabel = new QLabel("Info / Controls Panel");
    rightLayout->addWidget(infoLabel);
    rightLayout->addStretch();
    rightPanel->setLayout(rightLayout);
    rightPanel->setMinimumWidth(200);  // Adjust as needed

    // === Main Layout (Horizontal) ===
    QWidget* central = new QWidget;
    QHBoxLayout* mainLayout = new QHBoxLayout;

    mainLayout->addWidget(leftPanel, 1);   // 25%
    mainLayout->addWidget(ogreWidget, 2);  // 50%
    mainLayout->addWidget(rightPanel, 1);  // 25%

    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    central->setLayout(mainLayout);
    setCentralWidget(central);

    // === Signal Connections ===
    connect(animateButton, &QPushButton::clicked, this, &MainWindow::onAnimateClicked);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onAnimateClicked()
{
    qDebug()<<"MainWindow::onAnimateClicked()";
    ogreWidget->startSinbadAnimation();
}
