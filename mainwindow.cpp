#include "mainwindow.h"
#include "fingerprintworker.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDateTime>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>

extern "C" {
#include <libavformat/avformat.h>
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);
    resize(1200, 800);
    setWindowTitle("CineIris Studio");

    initUI();
    initStyle();

    loadSession();
}

MainWindow::~MainWindow() {}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSession();
    QMainWindow::closeEvent(event);
}

int MainWindow::findTaskIndex(QString taskId) {
    for(int i=0; i<m_tasks.size(); ++i) {
        if(m_tasks[i].id == taskId) return i;
    }
    return -1;
}

// --- 持久化逻辑 ---
void MainWindow::saveSession() {
    QJsonObject root;
    root["maxConcurrent"] = m_maxConcurrentTasks;

    QJsonArray arr;
    QString cacheDir = qApp->applicationDirPath() + "/cache";
    QDir().mkpath(cacheDir);

    for(const auto &task : m_tasks) {
        QJsonObject obj = task.toJson();
        if(!task.resultImage.isNull()) {
            QString imgPath = cacheDir + "/" + task.id + ".png";
            task.resultImage.save(imgPath);
            obj["hasResult"] = true;
        }
        arr.append(obj);
    }
    root["tasks"] = arr;

    QFile file(qApp->applicationDirPath() + "/app_data.json");
    if(file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
    }
}

void MainWindow::loadSession() {
    QFile file(qApp->applicationDirPath() + "/app_data.json");
    if(!file.open(QIODevice::ReadOnly)) return;

    QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    if(root.contains("maxConcurrent")) {
        m_maxConcurrentTasks = root["maxConcurrent"].toInt();
    }

    QJsonArray arr = root["tasks"].toArray();
    QString cacheDir = qApp->applicationDirPath() + "/cache";

    for(const auto &val : arr) {
        TaskData task = TaskData::fromJson(val.toObject());

        if(val.toObject()["hasResult"].toBool()) {
            QString imgPath = cacheDir + "/" + task.id + ".png";
            if(QFile::exists(imgPath)) {
                task.resultImage.load(imgPath);
            }
        }

        if(!QFileInfo::exists(task.filePath)) {
            task.status = Status_Error;
            task.fileName += " (文件丢失)";
        }
        addNewTask(task.filePath, &task);
    }
}

// --- UI 构建 ---
QGroupBox* MainWindow::createMetadataBox(QWidget* parent, QString suffix) {
    QGroupBox *grp = new QGroupBox("媒体信息 (Metadata)", parent);
    QGridLayout *grid = new QGridLayout(grp);
    grid->setVerticalSpacing(8);
    grid->setHorizontalSpacing(20);
    QString labelStyle = "color: #8898AA; font-weight: normal;";
    QString valStyle = "color: #32325D; font-weight: bold;";
    auto addRow = [&](int r, QString title, QString objName) {
        QLabel *l = new QLabel(title); l->setStyleSheet(labelStyle);
        QLabel *v = new QLabel("-"); v->setObjectName(objName + suffix); v->setStyleSheet(valStyle);
        grid->addWidget(l, r, 0); grid->addWidget(v, r, 1);
    };
    addRow(0, "编码:", "valCodec"); addRow(1, "分辨率:", "valRes");
    addRow(2, "帧率:", "valFps"); addRow(3, "时长:", "valDur");
    addRow(4, "大小:", "valSize");
    return grp;
}

void MainWindow::fillMetadataBox(QWidget* container, const TaskData& task, QString suffix) {
    auto setText = [&](QString name, QString val) {
        QLabel *lbl = container->findChild<QLabel*>(name + suffix);
        if (lbl) lbl->setText(val);
    };
    setText("valCodec", task.codec); setText("valRes", task.resolution);
    setText("valFps", task.frameRate); setText("valDur", task.durationStr);
    setText("valSize", task.fileSize);
}

void MainWindow::initUI() {
    QWidget *c = new QWidget(this); setCentralWidget(c);
    QHBoxLayout *mainLayout = new QHBoxLayout(c); mainLayout->setContentsMargins(0,0,0,0);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this); splitter->setHandleWidth(1);

    // === 左侧 ===
    QWidget *leftContainer = new QWidget(); leftContainer->setObjectName("leftPanel");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer); leftLayout->setContentsMargins(0,0,0,0);

    QWidget *toolbar = new QWidget(); toolbar->setObjectName("leftHeader"); toolbar->setFixedHeight(60);
    QHBoxLayout *tbLayout = new QHBoxLayout(toolbar); tbLayout->setContentsMargins(10,10,10,10);

    QPushButton *btnAdd = new QPushButton(" 📂 添加", toolbar); btnAdd->setObjectName("btnAdd");
    btnAdd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(btnAdd, &QPushButton::clicked, this, [=](){
        QStringList fs = QFileDialog::getOpenFileNames(this, "选择", "", "Video (*.mp4 *.avi *.mkv *.mov)");
        for(const auto &f : fs) addNewTask(f);
    });

    m_btnBatchDelete = new QPushButton("🗑️", toolbar);
    m_btnBatchDelete->setFixedSize(40, 40); m_btnBatchDelete->setObjectName("btnSettingsSmall");
    connect(m_btnBatchDelete, &QPushButton::clicked, this, &MainWindow::onBatchDelete);

    m_btnGlobalSettings = new QPushButton("⚙️", toolbar);
    m_btnGlobalSettings->setFixedSize(40, 40); m_btnGlobalSettings->setObjectName("btnSettingsSmall");
    connect(m_btnGlobalSettings, &QPushButton::clicked, this, &MainWindow::onGlobalSettingsClicked);

    tbLayout->addWidget(btnAdd);
    tbLayout->addWidget(m_btnBatchDelete);
    tbLayout->addWidget(m_btnGlobalSettings);

    m_taskList = new QListWidget(this);
    m_taskList->setFrameShape(QFrame::NoFrame);

    // [修改] 改为单选模式 (查看详情)，复选框用于批量操作
    m_taskList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_taskList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_taskList, &QListWidget::currentItemChanged, this, &MainWindow::onTaskSelected);
    connect(m_taskList, &QListWidget::customContextMenuRequested, this, &MainWindow::onContextMenu);

    leftLayout->addWidget(toolbar); leftLayout->addWidget(m_taskList);

    // === 右侧 ===
    m_rightPanel = new QStackedWidget(this); m_rightPanel->setObjectName("rightPanel");

    m_pageEmpty = new QWidget();
    QVBoxLayout *lEmpty = new QVBoxLayout(m_pageEmpty);
    QLabel *lblE = new QLabel("请点击左侧任务查看详情\n勾选复选框可进行批量操作", m_pageEmpty);
    lblE->setAlignment(Qt::AlignCenter);
    lEmpty->addWidget(lblE);

    // Page 1: Settings (Draft)
    m_pageSettings = new QWidget();
    QVBoxLayout *lSet = new QVBoxLayout(m_pageSettings);
    lSet->setContentsMargins(40, 30, 40, 30); lSet->setSpacing(15);

    m_lblVideoName = new QLabel("Title");
    m_lblVideoName->setStyleSheet("font-size:26px; font-weight:bold; margin-bottom:5px;");
    m_lblVideoName->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QWidget *card = new QWidget(); card->setObjectName("settingCard");
    // 添加阴影
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setOffset(0, 4); shadow->setColor(QColor(0, 0, 0, 25)); shadow->setBlurRadius(20);
    card->setGraphicsEffect(shadow);

    QVBoxLayout *lCard = new QVBoxLayout(card);
    lCard->setSpacing(20); lCard->setContentsMargins(25, 25, 25, 25);

    QGroupBox *gType = new QGroupBox("1. 选择指纹类型"); QHBoxLayout *hlType = new QHBoxLayout(gType);
    m_rbIris = new QRadioButton("CineIris (虹膜)"); m_rbBarcode = new QRadioButton("Barcode (条形码)");
    hlType->addWidget(m_rbIris); hlType->addWidget(m_rbBarcode); hlType->addStretch();

    QGroupBox *gSize = new QGroupBox("2. 输出分辨率 (像素)"); QHBoxLayout *hlSize = new QHBoxLayout(gSize);
    m_sliderSize = new QSlider(Qt::Horizontal); m_sliderSize->setRange(500, 4096);
    m_lblSizeVal = new QLabel("1024 px"); m_lblSizeVal->setFixedWidth(60);
    hlSize->addWidget(m_sliderSize); hlSize->addWidget(m_lblSizeVal);
    connect(m_sliderSize, &QSlider::valueChanged, [=](int v){ m_lblSizeVal->setText(QString::number(v) + " px"); });

    QGroupBox *gCount = new QGroupBox("3. 抽样密度 (帧数)"); QHBoxLayout *hlCount = new QHBoxLayout(gCount);
    m_sliderCount = new QSlider(Qt::Horizontal); m_sliderCount->setRange(100, 5000);
    m_lblCountVal = new QLabel("1000 帧"); m_lblCountVal->setFixedWidth(60);
    hlCount->addWidget(m_sliderCount); hlCount->addWidget(m_lblCountVal);
    connect(m_sliderCount, &QSlider::valueChanged, [=](int v){ m_lblCountVal->setText(QString::number(v) + " 帧"); });

    lCard->addWidget(gType); lCard->addWidget(gSize); lCard->addWidget(gCount);

    QHBoxLayout *lAction = new QHBoxLayout();
    m_btnCancelEdit = new QPushButton("取消");
    m_btnCancelEdit->setFixedSize(100, 45); m_btnCancelEdit->hide();
    m_btnCancelEdit->setStyleSheet("background-color: #FFF; border: 1px solid #CCC; color: #666;");
    connect(m_btnCancelEdit, &QPushButton::clicked, this, &MainWindow::onCancelEditClicked);

    m_btnStartTask = new QPushButton("加入生成队列"); m_btnStartTask->setObjectName("btnStart");
    m_btnStartTask->setFixedHeight(45); m_btnStartTask->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_btnStartTask, &QPushButton::clicked, this, &MainWindow::onStartClicked);

    lAction->addWidget(m_btnCancelEdit); lAction->addWidget(m_btnStartTask);

    lSet->addWidget(m_lblVideoName);
    lSet->addWidget(createMetadataBox(m_pageSettings, "_Set"));
    lSet->addWidget(card);
    lSet->addLayout(lAction);
    lSet->addStretch();

    // Page 2: Gallery
    m_pageGallery = new QWidget();
    QVBoxLayout *lGal = new QVBoxLayout(m_pageGallery); lGal->setContentsMargins(20,20,20,20);
    m_lblResultImage = new QLabel(); m_lblResultImage->setAlignment(Qt::AlignCenter);
    m_lblResultImage->setStyleSheet("background:#EEE; border-radius:8px;");
    QHBoxLayout *lGalBtn = new QHBoxLayout();
    m_btnRerun = new QPushButton("⚙️ 调整参数");
    m_btnSave = new QPushButton("💾 保存图片");
    m_btnRerun->setFixedSize(120, 40); m_btnSave->setFixedSize(120, 40);
    m_btnSave->setStyleSheet("background-color: #2DCE89; border: none; color: white;");

    connect(m_btnRerun, &QPushButton::clicked, this, &MainWindow::onRerunClicked);
    connect(m_btnSave, &QPushButton::clicked, this, &MainWindow::onSaveResultClicked);
    lGalBtn->addStretch(); lGalBtn->addWidget(m_btnRerun); lGalBtn->addWidget(m_btnSave); lGalBtn->addStretch();
    lGal->addWidget(m_lblResultImage, 1); lGal->addLayout(lGalBtn);

    // Page 3: Processing
    m_pageProcessing = new QWidget();
    QVBoxLayout *lProc = new QVBoxLayout(m_pageProcessing); lProc->setContentsMargins(40,40,40,40);
    m_lblProcTitle = new QLabel("Processing..."); m_lblProcTitle->setStyleSheet("font-size:24px; font-weight:bold; margin-bottom:20px;");

    QGroupBox *gConfig = new QGroupBox("任务配置 (Configuration)"); QGridLayout *glConf = new QGridLayout(gConfig);
    auto addConf = [&](int r, QString t, QString n){
        glConf->addWidget(new QLabel(t),r,0); QLabel* v=new QLabel("-"); v->setObjectName(n);
        v->setStyleSheet("color: #32325D; font-weight:bold;"); glConf->addWidget(v,r,1);
    };
    addConf(0,"指纹类型:", "confType"); addConf(1,"输出分辨率:", "confSize"); addConf(2,"抽样帧数:", "confCount");

    m_lblProcStatus = new QLabel("初始化..."); m_lblProcStatus->setAlignment(Qt::AlignCenter);
    m_lblProcStatus->setStyleSheet("color: #5E72E4; font-weight: bold; margin-bottom: 10px;");

    m_procProgressBar = new QProgressBar(); m_procProgressBar->setFixedHeight(8); m_procProgressBar->setTextVisible(false);
    m_procProgressBar->setStyleSheet("QProgressBar { background-color: #E9ECEF; border-radius: 4px; border: none; } QProgressBar::chunk { background-color: #5E72E4; border-radius: 4px; }");

    lProc->addWidget(m_lblProcTitle);
    lProc->addWidget(createMetadataBox(m_pageProcessing, "_Proc"));
    lProc->addWidget(gConfig);
    lProc->addStretch();
    lProc->addWidget(m_lblProcStatus);
    lProc->addWidget(m_procProgressBar);

    m_rightPanel->addWidget(m_pageEmpty); m_rightPanel->addWidget(m_pageSettings);
    m_rightPanel->addWidget(m_pageGallery); m_rightPanel->addWidget(m_pageProcessing);

    splitter->addWidget(leftContainer); splitter->addWidget(m_rightPanel);
    splitter->setStretchFactor(0, 3); splitter->setStretchFactor(1, 9);
    mainLayout->addWidget(splitter);
}

void MainWindow::initStyle() {
    QString qss = R"(
        /* === 全局基础样式 === */
        QMainWindow { background-color: #F4F7FC; }
        QWidget { font-family: 'Segoe UI', 'Microsoft YaHei'; font-size: 14px; color: #32325D; }

        /* === 布局容器背景 === */
        QWidget#leftPanel { background-color: #FFFFFF; border-right: 1px solid #E0E0E0; }
        QWidget#leftHeader { background-color: #FAFBFE; border-bottom: 1px solid #E0E0E0; }
        QWidget#rightPanel { background-color: #F4F7FC; }
        QWidget#settingCard { background-color: #FFFFFF; border-radius: 12px; border: 1px solid #E6E9F0; }

        /* === 按钮样式 === */
        QPushButton#btnAdd { background-color: #5E72E4; color: white; border-radius: 6px; font-weight: bold; font-size: 14px; border: none; }
        QPushButton#btnAdd:hover { background-color: #485FDF; }
        QPushButton#btnSettingsSmall { background-color: transparent; border: 1px solid #D0D0D0; border-radius: 6px; }
        QPushButton#btnSettingsSmall:hover { background-color: #EEE; }

        QPushButton#btnStart { background-color: #5E72E4; color: white; border-radius: 8px; font-size: 16px; padding: 12px; font-weight: bold; border:none; }
        QPushButton#btnStart:hover { background-color: #485FDF; margin-top: -2px; }
        QPushButton#btnStart:disabled { background-color: #CBD5E0; }

        /* === 列表样式 (只保留了行高和选中变色，完全移除了 indicator) === */
        QListWidget { border: none; background: transparent; outline: none; }
        QListWidget::item {
            height: 50px;
            border-bottom: 1px solid #F5F5F5;
            padding: 0 5px;
            color: #525F7F;
        }
        /* 选中状态 (高亮) */
        QListWidget::item:selected {
            background-color: #F6F9FC;
            color: #5E72E4;
            border-left: 4px solid #5E72E4;
        }

        /* !!! 注意 !!!
           这里删除了所有的 QListWidget::indicator 代码
           现在它会显示 Windows 默认的复选框样式
        */

        /* === 其他组件 === */
        QGroupBox { border: 1px solid #E6E9F0; border-radius: 6px; margin-top: 10px; padding-top: 20px; font-weight: bold; background: #FAFBFE; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }

        QSlider::groove:horizontal { border: 1px solid #E6E9F0; height: 6px; background: #F6F9FC; border-radius: 3px; }
        QSlider::handle:horizontal { background: #5E72E4; width: 16px; height: 16px; margin: -6px 0; border-radius: 8px; border: 2px solid white; }
        QSlider::sub-page:horizontal { background: #5E72E4; border-radius: 3px; }

        QProgressBar { background: #E9ECEF; border-radius: 3px; }
        QProgressBar::chunk { background: #5E72E4; border-radius: 3px; }
    )";
    this->setStyleSheet(qss);
}

void MainWindow::addNewTask(const QString &path, const TaskData *loadedData) {
    TaskData task;
    if (loadedData) {
        task = *loadedData;
    } else {
        task.id = QString::number(QDateTime::currentMSecsSinceEpoch()) + "_" + QString::number(qrand());
        task.filePath = path;
        task.fileName = QFileInfo(path).fileName();
        task.status = Status_Draft;
        probeVideoInfo(task);
        calculateSmartDefaults(task);
    }

    m_tasks.append(task);

    QListWidgetItem *item = new QListWidgetItem(m_taskList);
    item->setData(Qt::UserRole, task.id);

    // [修改] 开启复选框
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);

    QString prefix = "";
    if (task.status == Status_Finished) prefix = "✅ ";
    else if (task.status == Status_Error) prefix = "❌ ";
    else if (task.status == Status_Paused) prefix = "⏸️ ";
    item->setText(prefix + "  " + task.fileName); // 加点空格美观
    item->setToolTip(path);
}

void MainWindow::onTaskSelected(QListWidgetItem *current, QListWidgetItem *previous) {
    Q_UNUSED(previous);
    if (!current) { m_rightPanel->setCurrentIndex(0); return; }
    refreshRightPanel(current->data(Qt::UserRole).toString());
}

void MainWindow::refreshRightPanel(const QString &taskId) {
    int idx = findTaskIndex(taskId);
    if (idx < 0) return;
    const TaskData &task = m_tasks[idx];

    if (task.status == Status_Finished) {
        m_rightPanel->setCurrentIndex(2);
        if (!task.resultImage.isNull()) {
            QPixmap pix = QPixmap::fromImage(task.resultImage);
            m_lblResultImage->setPixmap(pix.scaled(600, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    else if (task.status == Status_Running || task.status == Status_Pending || task.status == Status_Paused) {
        m_rightPanel->setCurrentIndex(3);
        m_lblProcTitle->setText(task.fileName);
        m_procProgressBar->setValue(task.progress);
        fillMetadataBox(m_pageProcessing, task, "_Proc");

        m_pageProcessing->findChild<QLabel*>("confType")->setText(task.isIris ? "虹膜" : "条形码");
        m_pageProcessing->findChild<QLabel*>("confSize")->setText(QString::number(task.outputSize));
        m_pageProcessing->findChild<QLabel*>("confCount")->setText(QString::number(task.sampleCount));

        if (task.status == Status_Paused) m_lblProcStatus->setText("⏸️ 已暂停");
        else if (task.status == Status_Pending) m_lblProcStatus->setText("⏳ 等待调度...");
        else m_lblProcStatus->setText("🚀 正在生成... " + QString::number(task.progress) + "%");
    }
    else {
        m_rightPanel->setCurrentIndex(1);
        m_lblVideoName->setText(task.fileName);
        fillMetadataBox(m_pageSettings, task, "_Set");
        if(task.isIris) m_rbIris->setChecked(true); else m_rbBarcode->setChecked(true);
        m_sliderSize->setValue(task.outputSize);
        m_sliderCount->setValue(task.sampleCount);
        if (!task.resultImage.isNull()) { m_btnCancelEdit->show(); m_btnStartTask->setText("保存并重跑"); }
        else { m_btnCancelEdit->hide(); m_btnStartTask->setText("加入生成队列"); }
        m_btnStartTask->setEnabled(true);
    }
}

// --- 批量操作 (基于 CheckState) ---

void MainWindow::onContextMenu(const QPoint &pos) {
    QMenu menu(this);
    menu.addAction("🚀 启动已勾选任务", this, &MainWindow::onBatchStart);
    menu.addAction("⏸️ 暂停已勾选任务", this, &MainWindow::onBatchPause);
    menu.addSeparator();
    menu.addAction("🗑️ 删除已勾选任务", this, &MainWindow::onBatchDelete);
    menu.exec(m_taskList->mapToGlobal(pos));
}

void MainWindow::onBatchStart() {
    for (int i = 0; i < m_taskList->count(); ++i) {
        QListWidgetItem *item = m_taskList->item(i);
        if (item->checkState() == Qt::Checked) {
            QString id = item->data(Qt::UserRole).toString();
            int idx = findTaskIndex(id);
            if (idx >= 0 && (m_tasks[idx].status == Status_Draft || m_tasks[idx].status == Status_Paused || m_tasks[idx].status == Status_Error)) {
                m_tasks[idx].status = Status_Pending;
                item->setText("⏳  " + m_tasks[idx].fileName);
            }
        }
    }
    checkQueue();
    if(m_taskList->currentItem()) refreshRightPanel(m_taskList->currentItem()->data(Qt::UserRole).toString());
}

void MainWindow::onBatchPause() {
    for (int i = 0; i < m_taskList->count(); ++i) {
        QListWidgetItem *item = m_taskList->item(i);
        if (item->checkState() == Qt::Checked) {
            QString id = item->data(Qt::UserRole).toString();
            int idx = findTaskIndex(id);
            if (idx < 0) continue;
            if (m_tasks[idx].status == Status_Running) {
                if (m_runningThreads.contains(id)) {
                    QThread *t = m_runningThreads[id];
                    if (t && t->isRunning()) t->requestInterruption();
                }
                m_tasks[idx].status = Status_Paused;
                item->setText("⏸️  " + m_tasks[idx].fileName);
            } else if (m_tasks[idx].status == Status_Pending) {
                m_tasks[idx].status = Status_Paused;
                item->setText("⏸️  " + m_tasks[idx].fileName);
            }
        }
    }
    if(m_taskList->currentItem()) refreshRightPanel(m_taskList->currentItem()->data(Qt::UserRole).toString());
}

void MainWindow::onBatchDelete() {
    QList<QListWidgetItem*> itemsToDelete;
    for(int i = 0; i < m_taskList->count(); ++i) {
        QListWidgetItem *item = m_taskList->item(i);
        if(item->checkState() == Qt::Checked) itemsToDelete.append(item);
    }

    if (itemsToDelete.isEmpty()) {
        QMessageBox::information(this, "提示", "请先勾选左侧列表中的任务。");
        return;
    }

    if (QMessageBox::question(this, "删除", "确定删除已勾选的 " + QString::number(itemsToDelete.size()) + " 个任务吗？") != QMessageBox::Yes) return;

    for (auto item : itemsToDelete) {
        QString id = item->data(Qt::UserRole).toString();
        if (m_runningThreads.contains(id)) {
            QThread *t = m_runningThreads[id];
            if(t && t->isRunning()) t->requestInterruption();
            m_runningThreads.remove(id);
            m_runningTasks--;
        }
        int idx = findTaskIndex(id);
        if (idx >= 0) m_tasks.removeAt(idx);
        QFile::remove(qApp->applicationDirPath() + "/cache/" + id + ".png");
        delete m_taskList->takeItem(m_taskList->row(item));
    }
    checkQueue();
}

void MainWindow::onStartClicked() {
    QListWidgetItem *item = m_taskList->currentItem();
    if (!item) return;
    QString id = item->data(Qt::UserRole).toString();
    int idx = findTaskIndex(id);
    if(idx < 0) return;
    TaskData &task = m_tasks[idx];
    task.isIris = m_rbIris->isChecked();
    task.outputSize = m_sliderSize->value();
    task.sampleCount = m_sliderCount->value();
    task.status = Status_Pending;
    item->setText("⏳  " + task.fileName);
    checkQueue();
    refreshRightPanel(id);
}

void MainWindow::onCancelEditClicked() {
    QListWidgetItem *item = m_taskList->currentItem();
    if(!item) return;
    QString id = item->data(Qt::UserRole).toString();
    int idx = findTaskIndex(id);
    if(idx >= 0 && !m_tasks[idx].resultImage.isNull()) {
        m_tasks[idx].status = Status_Finished;
        refreshRightPanel(id);
    }
}

void MainWindow::onRerunClicked() {
    QListWidgetItem *item = m_taskList->currentItem();
    if(!item) return;
    QString id = item->data(Qt::UserRole).toString();
    int idx = findTaskIndex(id);
    if(idx >= 0) { m_tasks[idx].status = Status_Draft; refreshRightPanel(id); }
}

void MainWindow::onSaveResultClicked() {
    QListWidgetItem *item = m_taskList->currentItem();
    if(!item) return;
    int idx = findTaskIndex(item->data(Qt::UserRole).toString());
    if(idx >= 0 && !m_tasks[idx].resultImage.isNull()) {
        QString name = m_tasks[idx].fileName + ".png";
        QString path = QFileDialog::getSaveFileName(this, "保存", name, "Images (*.png)");
        if(!path.isEmpty()) m_tasks[idx].resultImage.save(path);
    }
}

void MainWindow::checkQueue() {
    if (m_runningTasks >= m_maxConcurrentTasks) return;
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].status == Status_Pending) {
             startTask(m_tasks[i].id);
             if (m_runningTasks >= m_maxConcurrentTasks) return;
        }
    }
}

void MainWindow::startTask(QString taskId) {
    int index = findTaskIndex(taskId);
    if (index < 0) return;
    m_runningTasks++;
    TaskData &task = m_tasks[index];
    task.status = Status_Running;

    for(int i=0; i<m_taskList->count(); ++i) {
        if(m_taskList->item(i)->data(Qt::UserRole).toString() == taskId) {
            m_taskList->item(i)->setText("🚀  " + task.fileName); break;
        }
    }

    QThread *thread = new QThread;
    FingerprintWorker *worker = new FingerprintWorker;
    worker->moveToThread(thread);
    m_runningThreads.insert(taskId, thread);
    TaskData params = task;
    connect(thread, &QThread::started, worker, [=](){ worker->processTask(params); });
    connect(worker, &FingerprintWorker::progressUpdated, this, [=](QString tid, int p){
        int idx = findTaskIndex(tid);
        if(idx >= 0) {
            m_tasks[idx].progress = p;
            if(m_taskList->currentItem() && m_taskList->currentItem()->data(Qt::UserRole).toString() == tid) {
                m_procProgressBar->setValue(p);
                m_lblProcStatus->setText("🚀 正在生成... " + QString::number(p) + "%");
            }
        }
    });
    connect(worker, &FingerprintWorker::finished, this, [=](QString tid, QImage img){
        m_runningTasks--;
        m_runningThreads.remove(tid);
        int idx = findTaskIndex(tid);
        if(idx >= 0) {
            m_tasks[idx].status = Status_Finished;
            m_tasks[idx].resultImage = img;
            m_tasks[idx].progress = 100;
            for(int i=0; i<m_taskList->count(); ++i) {
                if(m_taskList->item(i)->data(Qt::UserRole).toString() == tid) {
                    m_taskList->item(i)->setText("✅  " + m_tasks[idx].fileName); break;
                }
            }
            if(m_taskList->currentItem() && m_taskList->currentItem()->data(Qt::UserRole).toString() == tid) refreshRightPanel(tid);
        }
        thread->quit(); thread->wait(); thread->deleteLater(); worker->deleteLater();
        checkQueue();
    });
    connect(worker, &FingerprintWorker::errorOccurred, this, [=](QString tid, QString){
        m_runningTasks--; m_runningThreads.remove(tid);
        int idx = findTaskIndex(tid);
        if(idx >= 0) {
             m_tasks[idx].status = Status_Error;
             for(int i=0; i<m_taskList->count(); ++i) {
                if(m_taskList->item(i)->data(Qt::UserRole).toString() == tid) {
                    m_taskList->item(i)->setText("❌  " + m_tasks[idx].fileName); break;
                }
             }
        }
        thread->quit(); thread->wait(); thread->deleteLater(); worker->deleteLater();
        checkQueue();
    });
    thread->start();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) { if (event->mimeData()->hasUrls()) event->acceptProposedAction(); }
void MainWindow::dropEvent(QDropEvent *event) {
    QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        QString path = url.toLocalFile();
        if (path.endsWith(".mp4") || path.endsWith(".avi") || path.endsWith(".mkv") || path.endsWith(".mov")) {
            addNewTask(path);
        }
    }
}
QString MainWindow::formatSize(qint64 bytes) {
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}
QString MainWindow::formatTime(int64_t seconds) {
    int h = seconds / 3600; int m = (seconds % 3600) / 60; int s = seconds % 60;
    return QString("%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}
void MainWindow::probeVideoInfo(TaskData &task) {
    AVFormatContext *fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, task.filePath.toLocal8Bit().data(), nullptr, nullptr) < 0) return;
    avformat_find_stream_info(fmtCtx, nullptr);
    QFileInfo fi(task.filePath); task.fileSize = formatSize(fi.size());
    if (fmtCtx->duration != AV_NOPTS_VALUE) { task.rawDurationSec = fmtCtx->duration / AV_TIME_BASE; task.durationStr = formatTime(task.rawDurationSec); }
    for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVCodecParameters *p = fmtCtx->streams[i]->codecpar;
            task.rawWidth = p->width; task.rawHeight = p->height;
            task.resolution = QString("%1 x %2").arg(p->width).arg(p->height);
            const AVCodec *codec = avcodec_find_decoder(p->codec_id);
            task.codec = codec ? QString(codec->name) : "Unknown";
            if (fmtCtx->streams[i]->avg_frame_rate.den > 0) {
                double fps = av_q2d(fmtCtx->streams[i]->avg_frame_rate);
                task.frameRate = QString::number(fps, 'f', 0) + " fps";
            }
            break;
        }
    }
    avformat_close_input(&fmtCtx);
}

void MainWindow::calculateSmartDefaults(TaskData &task) {
    // 智能抽帧
    if (task.rawDurationSec > 0) {
        int smartCount = task.rawDurationSec * 5;

        // 限制最小值
        if (smartCount < 200) {
            smartCount = 200;
        }
        // 限制最大值
        if (smartCount > 2500) {
            smartCount = 2500;
        }

        task.sampleCount = smartCount;
    }

    // 智能分辨率
    if (task.rawWidth > 0) {
        int smartSize = 2048;

        // 如果原视频比较小，就不要强行放大
        if (task.rawWidth < smartSize) {
            smartSize = task.rawWidth;
        }

        // 保证是偶数 (奇数宽度的图片在某些编码下会报错)
        if (smartSize % 2 != 0) {
            smartSize--;
        }

        task.outputSize = smartSize;
    }
}

void MainWindow::onGlobalSettingsClicked() {
    QDialog dlg(this); dlg.setWindowTitle("应用设置"); dlg.setFixedSize(300, 150);
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QLabel *lbl = new QLabel("最大同时运行任务数:");
    QSpinBox *spin = new QSpinBox(); spin->setRange(1, 16); spin->setValue(m_maxConcurrentTasks);
    QPushButton *btnOk = new QPushButton("确定");
    connect(btnOk, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(lbl); layout->addWidget(spin); layout->addWidget(btnOk);
    if (dlg.exec() == QDialog::Accepted) {
        m_maxConcurrentTasks = spin->value();
        checkQueue();
    }
}
