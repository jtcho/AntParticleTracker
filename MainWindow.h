#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define QT_NO_DEBUG true;

#include <QMainWindow>
#include <QFileDialog>
#include <QVariant>
#include "Controller.h"
#include <QAbstractSlider>
#include "Utilities.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    //Called in the constructor, instantiates the various GUI components.
    void setInitialGUIState();
    //Called in the constructor, instantiates the initial signals and corresponding slots. Threads that have not been started are not used here.1
    void signalSlotsInit();
    //Simple method for determining whether the file chosen is
    bool isValidPath(QString path);
private:
    //Stored User Interface, holds all references to implemented Widgets.
    Ui::MainWindow *ui;
    //The Controller handles any implemented threads besides that of the GUI. Please try to reduce the amount of thread-interference...
    Controller *controller;
    //Current Application Version
    QString appVersion;
    //Holds the Current Directory.
    QDir dir;
    //Width of the source file. Used explicitly for rendering.
    int sourceWidth;
    //Height of the source file. Used explicitly for rendering.
    int sourceHeight;
    //Current loaded-state of the program.
    bool isVideoLoaded;
    //Stores the ID of the most recent Subject to avoid new-naming/definition conflicts.
    int currentSubjectID;
    //Stores the ID of them most recent Group
    int currentGroupID;
    //Current playing state of the program.
    bool playing;
public slots:
    //Linked directly to GUI input, passes to Controller for thread-handling.
    void loadVideo();
    //Linked directly to GUI input, passes to Controller for thread-handling.
    void dropVideo();
    //Linked to AboutAction from GUI menu. Displays a simple dialog window.
    void about();
private slots:
    //Receives an integer describing the change in play state.
    void onPlayStateChange(const int);
    //Receives new Mouse Data containing the Bounding Box for a Subject.
    void onSelectionBoxForm(MouseData);
    //Linked to ProcessingThread's signal. When the ProcessingThread has completed analyzing/filtering its frame, it will emit a completed signal.
    void updateFrame(const QImage &frame, const int index);
    //Updates the mouse-coordinate display.
    void updateMousePosLabel();
    //
    void setMouseDefault();
    //
    void toggleCursorGroupClusterPicker();
    //
    void updateColorPickerButton();
    //
    void updateSubjectDirection(int value);

    //%%%%% SUBJECT HANDLING %%%%%\\\
    //
    void updateSubjectSelectorButton();
    //
    void toggleCursorSubjectSelector();
    //Linked to AddSubject button.
    void addNewSubject();
    //
    void removeSubject();

    //%%%%% GROUP HANDLING %%%%%
    //
    void addNewGroup();
    //
    void setGroupColor(QPoint);
    //
    void removeGroup();
signals:
    void subjectUpdate();
};

#endif // MAINWINDOW_H

static const QString q_acceptedVideoTypes[] = {"AVI", "MP4", "MPG", "MOV", "avi", "mp4", "mpg", "mov"}; //change hasValidPaths() in ParticleTracker.cpp whenever number of elements is changed.


