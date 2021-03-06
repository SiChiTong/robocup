#include "labelgenerator.h"
#include "ui_labelgenerator.h"
#include "Vision/VisionWrapper/visioncontrolwrappertraining.h"
#include <QApplication>
#include <QMessageBox>

LabelGenerator::LabelGenerator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LabelGenerator)
{
    ui->setupUi(this);
    QObject::connect(ui->cancelPB, SIGNAL(clicked()), this, SLOT(cancel()));
}

LabelGenerator::~LabelGenerator()
{
    delete ui;
}

/** @brief runs the automatic label generator application.
*   @param dir Directory for input and output files
*   @return Integer return code.
*/
bool LabelGenerator::run(const std::string &dir)
{
    //find number of images
    int num_images=0,
        cur_image=0;
    NUImage temp;
    ifstream infile((dir + std::string("/image.strm")).c_str());
    ofstream outfile((dir + std::string("/auto_labels.lbl")).c_str());

    cancelled = false;
    while(!cancelled && infile.good()) {
        infile >> temp;
        infile.peek();
        num_images++;
    }
    infile.close();
    
    //exit if input failed
    if(num_images == 0) {
        QMessageBox::warning(this, "Error", QString("Failed to read image stream: ") + QString((dir+string("image.strm")).c_str()));
        return false;
    }
            
    //setup vision system
    VisionControlWrapper* vision = VisionControlWrapper::getInstance();
    if(!vision->setImageStream(dir + std::string("/image.strm"))) {
        QMessageBox::warning(this, "Error", QString("Failed to read image stream: ") + QString((dir+string("image.strm")).c_str()));
        return false;
    }
    if(!vision->setSensorStream(dir + string("/sensor.strm"))) {
        QMessageBox::warning(this, "Error", QString("Failed to read sensor stream: ") + QString((dir+string("sensor.strm")).c_str()));
        return false;
    }
    if(!vision->setLUT(dir + string("default.lut"))) {
        QMessageBox::warning(this, "Error", QString("Failed to read lookup table: ") + QString((dir+string("default.lut")).c_str()));
        return false;
    }
    
    ui->progressBar->setMaximum(num_images);
    
    //whilever the vision processes frames and the user hasn't cancelled the process
    while(!cancelled && vision->runFrame() == 0)
    {
        vision->printLabels(outfile);            //use the wrappers to print the labels
        ui->progressBar->setValue(cur_image);
        QApplication::processEvents();
        cur_image++;
    }

    QMessageBox::information(this, "Finished", QString("Label generation complete for ") + QString((dir+string("image.strm")).c_str()) + QString(" and ") + QString((dir+string("sensor.strm")).c_str()));

    close();
    return cancelled;
}
