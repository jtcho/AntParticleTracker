#include "MainWindow.h"
#include "ui_mainwindow.h"

#include "Controller.h"
#include <QDebug>
#include <QMessageBox>

//Constructor
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Allocate dynamic memory for the Controller.
    controller = new Controller;

    //Initiate GUI and Signals/Slots...
    setInitialGUIState();
    signalSlotsInit();

    //Default
    isVideoLoaded = false;
    playing = false;
    currentSubjectID = 0;
    currentGroupID = 0;
}

//Destructor
MainWindow::~MainWindow()
{
    if (isVideoLoaded)
        controller->dropVideo();
    delete ui;
}

//SLOT, called when file is loaded...
void MainWindow::loadVideo() {
    //Open the file-dialog window and get chosen file path.
    QString tempPath = QFileDialog::getOpenFileName(
                this, tr("Select Video File"), dir.path(), 0);
    //Check if the file path is valid.
    if (! tempPath.isEmpty() && isValidPath(tempPath)) {

        if (isVideoLoaded) {
            controller->dropVideo();
        }

        //Update GUI components.
        ui->playButton->setEnabled(true);
        ui->frameBackward->setEnabled(false);
        ui->frameForward->setEnabled(true);
        //ui->addSubjectButton->setEnabled(true);
        //ui->removeSubjectButton->setEnabled(true);
        ui->addGroupButton->setEnabled(true);
        ui->removeGroupButton->setEnabled(true);

        ui->loadVideoText->setText(tempPath);

        //Tell the Controller to initiate Threads...
        qDebug() << "Passing loadVideo call to Controller";
        controller->loadVideo(tempPath, QThread::HighPriority, QThread::HighPriority, QThread::HighestPriority); //USING 1 FOR NOW

        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\\
        //%%%%% CONNECT SIGNALS AND SLOTS INVOLVING THREADS %%%%%\\
        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\\

        connect(ui->playButton, SIGNAL(clicked()), controller->captureThread, SLOT(togglePlayState()));
        connect(ui->frameBackward, SIGNAL(clicked()), controller->captureThread, SLOT(stepBackward()));
        connect(ui->frameForward, SIGNAL(clicked()), controller->captureThread, SLOT(stepForward()));
        connect(ui->subjectSelectorButton, SIGNAL(clicked()), this, SLOT(toggleCursorSubjectSelector()));
        connect(ui->groupClusterPickerButton, SIGNAL(clicked()), this, SLOT(toggleCursorGroupClusterPicker()));

        connect(controller->captureThread, SIGNAL(playStateChanged(int)), this, SLOT(onPlayStateChange(int)));

        connect(ui->videoFrame, SIGNAL(onMouseMoveEvent(QMouseEvent*)), controller->displayThread, SLOT(onMouseMove(QMouseEvent*)));
        connect(ui->videoFrame, SIGNAL(onMouseMoveEvent(QMouseEvent*)), this, SLOT(updateMousePosLabel()));
        connect(ui->videoFrame, SIGNAL(onMousePressEvent(QMouseEvent*)), controller->displayThread, SLOT(onMousePress(QMouseEvent*)));
        connect(ui->videoFrame, SIGNAL(onMouseReleaseEvent(QMouseEvent*)), controller->displayThread, SLOT(onMouseRelease(QMouseEvent*)));

        connect(controller->displayThread, SIGNAL(frameUpdate(QImage, int)), this, SLOT(updateFrame(QImage, int)));
        connect(controller->displayThread, SIGNAL(selectionBoxFormed(MouseData)), this, SLOT(onSelectionBoxForm(MouseData)));
        connect(controller->displayThread, SIGNAL(colorPicked(QPoint)), this, SLOT(setGroupColor(QPoint)));

        connect(ui->subjectsListWidget, SIGNAL(itemSelectionChanged()), controller->displayThread, SLOT(releaseSelectionBox()));
        connect(ui->subjectAngleDial, SIGNAL(valueChanged(int)), this, SLOT(updateSubjectDirection(int)));

        connect(this, SIGNAL(subjectUpdate()),  controller->processThread,  SLOT(updateSubjects()));

        isVideoLoaded = true;

        //Force Painting of First Frame (stepForward works because initializes at -1).
        controller->captureThread->stepForward();
    } else {
        QMessageBox::warning(this, tr("Invalid File Selection"),
            tr("The selected file path is invalid / is not a supported file type. Please click the adjacent button to view a list of accepted file types."));
    }
}

void MainWindow::dropVideo() {
    controller->dropVideo();
}

void MainWindow::onPlayStateChange(const int newState) {
    playing = false;

    switch(newState) {
    case PAUSE:
        //Disable Play/Pause Button
        if(ui->playButton->isChecked())
            ui->playButton->toggle();
        //Enable Frame Step Buttons
        ui->frameBackward->setEnabled(true);
        ui->frameForward->setEnabled(true);

        //If something exists in the Subjects List AND is selected, enable subject selection.
        if (ui->subjectsListWidget->currentRow() > -1)
            ui->subjectSelectorButton->setEnabled(true);

        //Enable all of the subject stuff.
        ui->addSubjectButton->setEnabled(true);
        ui->removeSubjectButton->setEnabled(true);
        ui->subjectsListWidget->setEnabled(true);
        break;
    case PLAY:
        playing = true;
        //Disable Frame Step Buttons
        ui->frameBackward->setEnabled(false);
        ui->frameForward->setEnabled(false);

        controller->displayThread->setMouseCursor(DEFAULT);
        setCursor(Qt::ArrowCursor);

        ui->subjectSelectorButton->setEnabled(false);
        ui->addSubjectButton->setEnabled(false);
        ui->addSubjectButton->setEnabled(false);
        ui->removeSubjectButton->setEnabled(false);
        ui->subjectsListWidget->setEnabled(false);
        break;
    case START_OF_VIDEO:
        //Disable Back Step Button
        ui->frameBackward->setEnabled(false);
        break;
    case END_OF_VIDEO:
        //Disable Forward Step Button
        ui->frameForward->setEnabled(false);
        break;
    }
}

//IMPLEMENTME
void MainWindow::about() {

}

bool MainWindow::isValidPath(QString path) {

    if (path.isNull()) return false;

    QFileInfo pathInfo(path);

    if (pathInfo.exists()) {
        qDebug() << "File exists @ " << path;
        qDebug() << "File Suffix : " << pathInfo.suffix();
        for (int i = 0;i < 8; i++) { //UPDATE THE SIZE AS NECESSARY
            if (pathInfo.suffix() == q_acceptedVideoTypes[i])
                return true;
        }
    }
    return false;
}

void MainWindow::setInitialGUIState() {
    ui->videoFrame->setText("No video loaded...");
    ui->videoFrame->setMouseTracking(true);
}

void MainWindow::signalSlotsInit() {
    connect(ui->loadVideoButton, SIGNAL(clicked()), this, SLOT(loadVideo()));

    connect(ui->addGroupButton, SIGNAL(clicked()), this, SLOT(addNewGroup()));
    connect(ui->removeGroupButton, SIGNAL(clicked()), this, SLOT(removeGroup()));
    connect(ui->addSubjectButton, SIGNAL(clicked()), this, SLOT(addNewSubject()));
    connect(ui->removeSubjectButton, SIGNAL(clicked()), this, SLOT(removeSubject()));

    connect(ui->groupsListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateColorPickerButton()));
    connect(ui->subjectsListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateSubjectSelectorButton()));
}

//Called upon a focus-change inside the SubjectListWidget.
//Disables the Selector button if no Subject is selected.
void MainWindow::updateSubjectSelectorButton() {
    if(ui->subjectsListWidget->currentRow() > -1 && ui->groupsListWidget->currentRow() > -1 && ! ui->groupsListWidget->currentItem()->icon().isNull()) {
        ui->subjectSelectorButton->setEnabled(true);
        if (ui->subjectsListWidget->currentItem()->data(1002).toInt()) {
            ui->subjectAngleDial->setEnabled(true);
            ui->subjectAngleLine->setText(QString::number(rad2Deg(
                                              controller->processThread->getSubject(
                                                              ui->subjectsListWidget->currentItem()->data(1001).toInt(),
                                                              ui->subjectsListWidget->currentItem()->data(1003).toInt())->dir())));
        } else ui->subjectAngleDial->setEnabled(false);
    }
    else {
        ui->subjectSelectorButton->setEnabled(false);
        ui->subjectAngleDial->setEnabled(false);
    }
}

//Called upon a focus change inside the GroupListWidget.
//Disables the Picker button if no Group is selected.
//Also disables the Subject UI bits if no Group is selected.
//If a group is selected, show boxes.
void MainWindow::updateColorPickerButton() {
    if(ui->groupsListWidget->currentRow() > -1) {
        //Paint subject boxes.
        controller->displayThread->showSubjectBoxes(ui->groupsListWidget->currentItem()->data(1001).toInt());
        ui->groupClusterPickerButton->setEnabled(true);

        bool toggleSubjectList = ! ui->groupsListWidget->currentItem()->icon().isNull();
        ui->subjectsListWidget->setEnabled(toggleSubjectList);
        ui->addSubjectButton->setEnabled(toggleSubjectList);
        ui->removeSubjectButton->setEnabled(toggleSubjectList);
        ui->subjectSelectorButton->setChecked(toggleSubjectList && ui->subjectSelectorButton->isChecked() && ui->subjectsListWidget->currentRow() > -1);
        ui->subjectSelectorButton->setEnabled(toggleSubjectList && ui->subjectsListWidget->currentRow() > -1);

        if (! toggleSubjectList) {
            //If Icon is Null and no Color exists, reset cursor
            setCursor(Qt::ArrowCursor);
            controller->displayThread->setMouseCursor(DEFAULT);
        }
    }
    else
    {
        ui->groupClusterPickerButton->setEnabled(false);

        ui->subjectsListWidget->setEnabled(false);
        ui->addSubjectButton->setEnabled(false);
        ui->removeSubjectButton->setEnabled(false);

        setCursor(Qt::ArrowCursor);
        controller->displayThread->setMouseCursor(DEFAULT);
    }
}

void MainWindow::addNewSubject() {
    QListWidgetItem* tempWidgetItem = new QListWidgetItem(tr("New Subject ")+QString::number(currentSubjectID));
    tempWidgetItem->setFlags(tempWidgetItem->flags() | Qt::ItemIsEditable);
    tempWidgetItem->setData(1001, QVariant(currentSubjectID++));
    tempWidgetItem->setData(1002, QVariant(0)); //0 if no selection box has been made yet
    ui->subjectsListWidget->addItem(tempWidgetItem);
}

void MainWindow::removeSubject() {
    if (ui->subjectsListWidget->currentRow() > -1) {
        int tempID = ui->subjectsListWidget->currentItem()->data(1001).toInt();
        if (ui->subjectsListWidget->currentItem()->data(1002).toInt()) {
            qDebug() << "Removing from Subject Group.";
            controller->processThread->getSubjectGroup(ui->subjectsListWidget->currentItem()->data(1003).toInt())->removeSubject(tempID);
            controller->processThread->updateSubjects();
        }
        ui->subjectsListWidget->takeItem(ui->subjectsListWidget->currentRow());

    }
}

void MainWindow::onSelectionBoxForm(MouseData mouseData) {
    if (ui->subjectsListWidget->currentItem()->data(1002).toInt()) {
        //If selection box has already been formed...
        //Remove the subject from its current group.
        controller->processThread->getSubjectGroup(ui->subjectsListWidget->currentItem()->data(1003).toInt())->removeSubject(
                    ui->subjectsListWidget->currentItem()->data(1001).toInt());
    }
    controller->processThread->addSubject(ui->groupsListWidget->currentItem()->data(1001).toInt(),
                                          ui->subjectsListWidget->currentItem()->data(1001).toInt(),
                                          mouseData.selectionBox,
                                          controller->displayThread->getCurrentSourceFrame(),
                                          controller->displayThread->getCurrentFrameIndex());
    ui->subjectsListWidget->currentItem()->setData(1002, 1); //Selection Box has been made.
    ui->subjectsListWidget->currentItem()->setData(1003, ui->groupsListWidget->currentItem()->data(1001).toInt());
    updateSubjectSelectorButton();
}

void MainWindow::updateSubjectDirection(int value) {
    qDebug() << "Received direction change.";
    ui->subjectAngleLine->setText(QString::number(value));
    controller->processThread->getSubject(
                ui->subjectsListWidget->currentItem()->data(1001).toInt(),
                ui->subjectsListWidget->currentItem()->data(1003).toInt())->setDirection(deg2Rad(value));
    qDebug() << "Updating Subjects...";
    controller->processThread->updateSubjects();

}

void MainWindow::addNewGroup() {
    QListWidgetItem* tempWidgetItem = new QListWidgetItem(tr("New Group ") + QString::number(currentGroupID));
    tempWidgetItem->setFlags(tempWidgetItem->flags() | Qt::ItemIsEditable);
    tempWidgetItem->setData(1001, QVariant(currentGroupID++));
    //Don't create a SubjectGroup yet. This is not defined until a color is created.
    ui->groupsListWidget->addItem(tempWidgetItem);
}

void MainWindow::setGroupColor(QPoint pos) {
    //Take the currently selected group ID and use it to instantiate a new Group in the Processing Thread's list.
    Point3_<uchar> tempRGB = controller->processThread->setGroupColor(pos, ui->groupsListWidget->currentItem()->data(1001).toInt());

    setMouseDefault();
    ui->groupClusterPickerButton->setChecked(false);

    QPixmap tempPix(20,20);
    tempPix.fill(QColor(tempRGB.z, tempRGB.y, tempRGB.x));
    ui->groupsListWidget->currentItem()->setIcon(QIcon(tempPix));

    ui->subjectsListWidget->setEnabled(true);
    ui->addSubjectButton->setEnabled(true);
    ui->removeSubjectButton->setEnabled(true);
    if (ui->groupsListWidget->currentRow() > -1 && ui->subjectsListWidget->currentRow() > -1)
        ui->subjectSelectorButton->setEnabled(true);
}

void MainWindow::removeGroup() {
    //Removes the currently-selected Group from both the list widget and the underlying map.
    if (ui->groupsListWidget->currentRow() > -1) {
        //qDebug() << "Removing Group...";
        QListWidgetItem* tempItem = ui->groupsListWidget->currentItem();
        int tempID = tempItem->data(1001).toInt();
        if(! tempItem->icon().isNull()) {
            controller->processThread->removeGroup(tempID);
            //qDebug() << "Removed from Map.";
        }
        ui->groupsListWidget->takeItem(ui->groupsListWidget->currentRow());
        //Later, be sure to remove associated Subjects from the SubjectList as well.
        for (int i = 0; i < ui->subjectsListWidget->count();) {
            //Iterate through all the subjects...
            if (ui->subjectsListWidget->item(i)->data(1003) == tempID)
                ui->subjectsListWidget->takeItem(i);
            else i++;
        }
        controller->processThread->updateSubjects();
    }
}

void MainWindow::toggleCursorSubjectSelector() {
    int currentMouseCursor = controller->displayThread->getMouseCursorType();

    if (currentMouseCursor != SUBJECT_SELECTOR) {
        setCursor(Qt::CrossCursor);
        controller->displayThread->setMouseCursor(SUBJECT_SELECTOR);
    } else {
        //If disabling Subject Selector, restore Default.
        setCursor(Qt::ArrowCursor);
        controller->displayThread->setMouseCursor(DEFAULT);
    }
}

void MainWindow::toggleCursorGroupClusterPicker() {
    int currentMouseCursor = controller->displayThread->getMouseCursorType();

    if (currentMouseCursor != COLOR_PICKER) {
        setCursor(Qt::PointingHandCursor);
        controller->displayThread->setMouseCursor(COLOR_PICKER);
    } else {
        setCursor(Qt::ArrowCursor);
        controller->displayThread->setMouseCursor(DEFAULT);
   }
}

//Generally called when play is started.
void MainWindow::setMouseDefault() {
    controller->displayThread->setMouseCursor(DEFAULT);
    setCursor(Qt::ArrowCursor);
}

void MainWindow::updateFrame(const QImage &frame, const int index) {
    //Handle Frame Paint Call
    ui->videoFrame->setPixmap(QPixmap::fromImage(frame));
    ui->frameIndexLabel->setText(QString("Frame: "+QString::number(index)));
}

void MainWindow::updateMousePosLabel() {
    ui->mousePosLabel->setText(QString("X: ")+QString::number(controller->displayThread->getMouseCursorPos().x()) +
                               QString(" Y:")+QString::number(controller->displayThread->getMouseCursorPos().y()));
}
