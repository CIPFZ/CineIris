#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QRadioButton>
#include <QSpinBox>
#include <QVector>
#include <QDialog>
#include <QProgressBar>
#include <QGroupBox>
#include <QMap>
#include <QMenu>
#include <QGraphicsDropShadowEffect>
#include "taskdata.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onTaskSelected(QListWidgetItem *current, QListWidgetItem *previous);
    void onStartClicked();
    void onSaveResultClicked();
    void onRerunClicked();
    void onCancelEditClicked();
    void onGlobalSettingsClicked();

    // 批量操作 (逻辑已修改为针对“勾选”项)
    void onBatchStart();
    void onBatchPause();
    void onBatchDelete();
    void onContextMenu(const QPoint &pos);

    void checkQueue();
    void startTask(QString taskId);

private:
    // UI 组件
    QListWidget *m_taskList;
    QStackedWidget *m_rightPanel;
    QPushButton *m_btnGlobalSettings;
    QPushButton *m_btnBatchDelete;

    // Pages
    QWidget *m_pageEmpty;
    QWidget *m_pageSettings;
    QWidget *m_pageGallery;
    QWidget *m_pageProcessing;

    // Controls
    QLabel *m_lblVideoName;
    QRadioButton *m_rbIris;
    QRadioButton *m_rbBarcode;
    QSlider *m_sliderSize;
    QLabel *m_lblSizeVal;
    QSlider *m_sliderCount;
    QLabel *m_lblCountVal;
    QPushButton *m_btnStartTask;
    QPushButton *m_btnCancelEdit;

    QLabel *m_lblResultImage;
    QPushButton *m_btnSave;
    QPushButton *m_btnRerun;

    QLabel *m_lblProcTitle;
    QProgressBar *m_procProgressBar;
    QLabel *m_lblProcStatus;

    // 数据
    QVector<TaskData> m_tasks;
    QMap<QString, QThread*> m_runningThreads;

    int m_maxConcurrentTasks = 2;
    int m_runningTasks = 0;

    // 内部函数
    void initUI();
    void initStyle();
    void addNewTask(const QString &path, const TaskData *loadedData = nullptr);
    void refreshRightPanel(const QString &taskId);
    int findTaskIndex(QString taskId);

    void probeVideoInfo(TaskData &task);
    void calculateSmartDefaults(TaskData &task);
    QString formatSize(qint64 bytes);
    QString formatTime(int64_t seconds);

    QGroupBox* createMetadataBox(QWidget* parent, QString suffix);
    void fillMetadataBox(QWidget* container, const TaskData& task, QString suffix);

    void saveSession();
    void loadSession();
};

#endif // MAINWINDOW_H
