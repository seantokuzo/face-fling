/**
 * MainWindow implementation.
 * 
 * Central UI coordinator for Face-Fling.
 */

#include "MainWindow.h"
#include "FaceGridWidget.h"
#include "PersonListWidget.h"
#include "ScanProgressDialog.h"
#include "../core/Scanner.h"
#include "../core/Indexer.h"
#include "../core/Clusterer.h"
#include "../services/Database.h"
#include "../services/FaceService.h"
#include "../services/ImageLoader.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>
#include <QtConcurrent>
#include <QFutureWatcher>

namespace facefling {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    createActions();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    loadSettings();
    
    // Initialize core services
    initializeServices();
}

MainWindow::~MainWindow()
{
    // Cancel any running operations
    m_processingCancelled = true;
    if (m_scanner) m_scanner->cancel();
    if (m_indexer) m_indexer->cancel();
    
    saveSettings();
}

void MainWindow::initializeServices()
{
    // Create data directory
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    
    // Create thumbnails directory
    QString thumbPath = dataPath + "/thumbnails";
    QDir().mkpath(thumbPath);
    
    QString dbPath = dataPath + "/facefling.db";
    
    try {
        // Initialize database
        m_database = std::make_shared<Database>(dbPath.toStdString());
        m_database->initialize();
        
        // Initialize face service
        QString modelsPath = QCoreApplication::applicationDirPath() + "/../Resources/models";
        FaceService::Config faceConfig;
        faceConfig.shape_predictor_path = (modelsPath + "/shape_predictor_68_face_landmarks.dat").toStdString();
        faceConfig.face_recognition_path = (modelsPath + "/dlib_face_recognition_resnet_model_v1.dat").toStdString();
        m_faceService = std::make_shared<FaceService>(faceConfig);
        
        // Initialize image loader
        m_imageLoader = std::make_shared<ImageLoader>();
        
        // Initialize scanner
        m_scanner = std::make_unique<Scanner>();
        
        // Initialize indexer
        m_indexer = std::make_unique<Indexer>(m_database, m_faceService, m_imageLoader);
        m_indexer->set_thumbnail_dir(thumbPath.toStdString());
        
        // Initialize clusterer
        m_clusterer = std::make_unique<Clusterer>(m_database, m_faceService);
        
        // Pass database to widgets
        m_faceGrid->setDatabase(m_database);
        m_personList->setDatabase(m_database);
        
        statusBar()->showMessage(tr("Ready"));
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Initialization Error"),
            tr("Failed to initialize services: %1\n\n"
               "Make sure dlib models are in Resources/models/").arg(e.what()));
        statusBar()->showMessage(tr("Initialization failed"));
    }
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("Face-Fling"));
    setMinimumSize(900, 600);
    resize(1200, 800);
    
    // Central splitter with sidebar and main content
    m_splitter = new QSplitter(Qt::Horizontal, this);
    
    // Person list sidebar
    m_personList = new PersonListWidget(this);
    m_splitter->addWidget(m_personList);
    
    // Face grid (main content)
    m_faceGrid = new FaceGridWidget(this);
    m_splitter->addWidget(m_faceGrid);
    
    // Set initial sizes (sidebar 250px, rest for grid)
    m_splitter->setSizes({250, 950});
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    
    setCentralWidget(m_splitter);
    
    // Connect signals
    connect(m_personList, &PersonListWidget::personSelected,
            this, &MainWindow::onPersonSelected);
    connect(m_personList, &PersonListWidget::clusterSelected,
            this, &MainWindow::onClusterSelected);
    connect(m_faceGrid, &FaceGridWidget::clusterSelected,
            this, &MainWindow::onClusterSelected);
}

void MainWindow::createActions()
{
    // Open folder
    m_openAction = new QAction(tr("Open Folder..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip(tr("Open a folder to scan for photos"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFolder);
    
    // Export
    m_exportAction = new QAction(tr("Export Photos..."), this);
    m_exportAction->setShortcut(QKeySequence(tr("Ctrl+E")));
    m_exportAction->setStatusTip(tr("Export photos of selected person"));
    m_exportAction->setEnabled(false);
    connect(m_exportAction, &QAction::triggered, this, &MainWindow::exportPerson);
    
    // Merge clusters
    m_mergeAction = new QAction(tr("Merge Clusters"), this);
    m_mergeAction->setShortcut(QKeySequence(tr("Ctrl+M")));
    m_mergeAction->setStatusTip(tr("Merge selected clusters into one"));
    m_mergeAction->setEnabled(false);
    connect(m_mergeAction, &QAction::triggered, this, &MainWindow::mergeClusters);
    
    // Split cluster
    m_splitAction = new QAction(tr("Split Cluster"), this);
    m_splitAction->setStatusTip(tr("Split selected faces into a new cluster"));
    m_splitAction->setEnabled(false);
    connect(m_splitAction, &QAction::triggered, this, &MainWindow::splitCluster);
    
    // Settings
    m_settingsAction = new QAction(tr("Preferences..."), this);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    m_settingsAction->setMenuRole(QAction::PreferencesRole);
}

void MainWindow::setupMenuBar()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exportAction);
    
    // Edit menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_mergeAction);
    editMenu->addAction(m_splitAction);
    editMenu->addSeparator();
    editMenu->addAction(m_settingsAction);
    
    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QAction *sidebarAction = viewMenu->addAction(tr("Show Sidebar"));
    sidebarAction->setCheckable(true);
    sidebarAction->setChecked(true);
    sidebarAction->setShortcut(QKeySequence(tr("Ctrl+Shift+S")));
    connect(sidebarAction, &QAction::triggered, this, &MainWindow::toggleSidebar);
    
    // Window menu (macOS standard)
    QMenu *windowMenu = menuBar()->addMenu(tr("&Window"));
    windowMenu->addAction(tr("Minimize"), this, &QWidget::showMinimized, QKeySequence(tr("Ctrl+M")));
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("About Face-Fling"), [this]() {
        QMessageBox::about(this, tr("About Face-Fling"),
            tr("<h3>Face-Fling</h3>"
               "<p>Version %1</p>"
               "<p>Organize your photos by the people in them.</p>")
            .arg(QApplication::applicationVersion()));
    });
}

void MainWindow::setupToolBar()
{
    QToolBar *toolbar = addToolBar(tr("Main"));
    toolbar->setMovable(false);
    toolbar->addAction(m_openAction);
    toolbar->addAction(m_exportAction);
    toolbar->addSeparator();
    toolbar->addAction(m_mergeAction);
    toolbar->addAction(m_splitAction);
}

void MainWindow::setupStatusBar()
{
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(200);
    m_progressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::openFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, 
        tr("Select Photo Folder"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (dir.isEmpty()) {
        return;
    }
    
    // Check services are initialized
    if (!m_database || !m_faceService) {
        QMessageBox::warning(this, tr("Not Ready"),
            tr("Services not initialized. Please restart the application."));
        return;
    }
    
    runPipeline(dir);
}

void MainWindow::runPipeline(const QString &folderPath)
{
    m_currentScanPath = folderPath;
    m_processingCancelled = false;
    m_scannedFiles.clear();
    
    // Show progress dialog
    m_progressDialog = new ScanProgressDialog(this);
    connect(m_progressDialog, &ScanProgressDialog::cancelled, this, [this]() {
        m_processingCancelled = true;
        if (m_scanner) m_scanner->cancel();
        if (m_indexer) m_indexer->cancel();
    });
    
    m_progressDialog->setMessage(tr("Scanning for photos..."));
    m_progressDialog->show();
    
    // Use QtConcurrent for background processing
    auto *watcher = new QFutureWatcher<std::vector<std::string>>(this);
    
    connect(watcher, &QFutureWatcher<std::vector<std::string>>::finished, this, [this, watcher]() {
        m_scannedFiles = watcher->result();
        watcher->deleteLater();
        
        if (m_processingCancelled || m_scannedFiles.empty()) {
            if (m_progressDialog) {
                m_progressDialog->accept();
                m_progressDialog = nullptr;
            }
            if (!m_processingCancelled && m_scannedFiles.empty()) {
                QMessageBox::information(this, tr("No Images Found"),
                    tr("No image files found in the selected folder."));
            }
            statusBar()->showMessage(tr("Ready"));
            return;
        }
        
        onScanComplete();
    });
    
    // Start scanning in background
    m_scanner->reset();
    QFuture<std::vector<std::string>> future = QtConcurrent::run([this, folderPath]() {
        return m_scanner->scan(
            folderPath.toStdString(),
            [this](size_t found, const std::string& dir, const std::string& file) {
                QMetaObject::invokeMethod(this, [this, found, file]() {
                    if (m_progressDialog) {
                        m_progressDialog->setProgress(static_cast<int>(found), 0);
                        m_progressDialog->setCurrentFile(QString::fromStdString(file));
                    }
                }, Qt::QueuedConnection);
            }
        );
    });
    
    watcher->setFuture(future);
}

void MainWindow::exportPerson()
{
    // TODO: Implement export dialog
    QMessageBox::information(this, tr("Export"), 
        tr("Export functionality coming soon!"));
}

void MainWindow::mergeClusters()
{
    // TODO: Implement merge
    QMessageBox::information(this, tr("Merge"), 
        tr("Merge functionality coming soon!"));
}

void MainWindow::splitCluster()
{
    // TODO: Implement split
    QMessageBox::information(this, tr("Split"), 
        tr("Split functionality coming soon!"));
}

void MainWindow::toggleSidebar()
{
    m_personList->setVisible(!m_personList->isVisible());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

void MainWindow::onScanProgress(int current, int total, const QString &file)
{
    if (m_progressDialog) {
        m_progressDialog->setProgress(current, total);
        m_progressDialog->setCurrentFile(file);
    }
}

void MainWindow::onScanComplete()
{
    if (m_processingCancelled) return;
    
    // Move to indexing phase
    if (m_progressDialog) {
        m_progressDialog->setMessage(tr("Detecting faces in %1 images...").arg(m_scannedFiles.size()));
        m_progressDialog->setProgress(0, static_cast<int>(m_scannedFiles.size()));
    }
    
    // Run indexer in background
    auto *watcher = new QFutureWatcher<void>(this);
    
    connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        onIndexComplete();
    });
    
    QFuture<void> future = QtConcurrent::run([this]() {
        m_indexer->index(
            m_scannedFiles,
            [this](int current, int total, const std::string& file, int faces) {
                QMetaObject::invokeMethod(this, [this, current, total, file, faces]() {
                    onIndexProgress(current, total, QString::fromStdString(file), faces);
                }, Qt::QueuedConnection);
            }
        );
    });
    
    watcher->setFuture(future);
}

void MainWindow::onIndexProgress(int current, int total, const QString &file, int faces)
{
    if (m_progressDialog) {
        m_progressDialog->setProgress(current, total);
        m_progressDialog->setMessage(tr("Processing: %1/%2 images, found %3 faces")
            .arg(current).arg(total).arg(faces));
        m_progressDialog->setCurrentFile(file);
    }
}

void MainWindow::onIndexComplete()
{
    if (m_processingCancelled) {
        if (m_progressDialog) {
            m_progressDialog->accept();
            m_progressDialog = nullptr;
        }
        return;
    }
    
    // Move to clustering phase
    if (m_progressDialog) {
        m_progressDialog->setMessage(tr("Clustering faces..."));
        m_progressDialog->setProgress(0, 0); // Indeterminate
    }
    
    // Run clusterer in background
    auto *watcher = new QFutureWatcher<void>(this);
    
    connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        onClusterComplete();
    });
    
    QFuture<void> future = QtConcurrent::run([this]() {
        m_clusterer->cluster_all(
            [this](int processed, int total) {
                QMetaObject::invokeMethod(this, [this, processed, total]() {
                    onClusterProgress(processed, total);
                }, Qt::QueuedConnection);
            }
        );
    });
    
    watcher->setFuture(future);
}

void MainWindow::onClusterProgress(int current, int total)
{
    if (m_progressDialog && total > 0) {
        m_progressDialog->setProgress(current, total);
        m_progressDialog->setMessage(tr("Clustering faces: %1/%2").arg(current).arg(total));
    }
}

void MainWindow::onClusterComplete()
{
    // Close progress dialog
    if (m_progressDialog) {
        m_progressDialog->accept();
        m_progressDialog = nullptr;
    }
    
    // Refresh the UI to show results
    refreshUI();
    
    statusBar()->showMessage(tr("Scan complete - found %1 images").arg(m_scannedFiles.size()));
}

void MainWindow::onClusterSelected(int64_t clusterId)
{
    m_exportAction->setEnabled(clusterId > 0);
    m_splitAction->setEnabled(clusterId > 0);
    
    if (clusterId > 0) {
        m_faceGrid->showCluster(clusterId);
    }
}

void MainWindow::onPersonSelected(int64_t personId)
{
    m_exportAction->setEnabled(personId > 0);
    
    if (personId > 0) {
        m_faceGrid->showPerson(personId);
    } else {
        m_faceGrid->showAllClusters();
    }
}

void MainWindow::refreshUI()
{
    m_personList->refresh();
    m_faceGrid->showAllClusters();
}

QString MainWindow::getThumbnailPath(int64_t faceId) const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QString("%1/thumbnails/face_%2.jpg").arg(dataPath).arg(faceId);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    if (settings.contains("splitter")) {
        m_splitter->restoreState(settings.value("splitter").toByteArray());
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("splitter", m_splitter->saveState());
}

} // namespace facefling
