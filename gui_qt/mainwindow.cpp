#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ProcessorWidget.h"
#include "easylogging++.h"
#include "ResizeImage.h"
#include "Roi.h"

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        frameSourceNavigation(&controller)
{
    ui->setupUi(this);

    // create vertical layout
    ui->centralWidget->setLayout(new QHBoxLayout());
    ui->centralWidget->layout()->setSizeConstraint(QLayout::SetMinimumSize);

    //register processors
    controller.registerImageProcessor<RoiProcessor>("roi");
    controller.registerImageProcessor<ResizeImage>("resize");

    // set processors
    const std::vector<std::string> pipeLineDescription{"roi",  "resize"};
    controller.loadPipeline(pipeLineDescription);

    // get loaded processors

    // create tabs with each processors processor
    for(auto &processor : pipeLineDescription)
    {
        // add processor widget
        ProcessorWidget *processorWidget = new ProcessorWidget(&controller, tabWidget.count());
        tabWidget.addTab(processorWidget, QString::fromStdString(processor));
    }

    // add widgets to splitter
    auto *splitter = new QSplitter();
    splitter->addWidget(&frameSourceNavigation);
    splitter->addWidget(&tabWidget);
    ui->centralWidget->layout()->addWidget(splitter);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 5);

    // register main window as an observer of controller to be notified when image processing is finished
    controller.registerObserver(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::update() {
    for(int iTab = 0; iTab < tabWidget.count(); iTab++)
    {
        try {
            static_cast<ProcessorWidget *>(tabWidget.widget(iTab))->setDebugImage(controller.getDebugImage(iTab));
        }catch(const std::exception& e)
        {
            LOG(WARNING) << e.what();
        }
    }
}
