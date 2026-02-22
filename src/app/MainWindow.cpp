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

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QStandardPaths>

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
    // TODO: Initialize database, face service, etc.
}

MainWindow::~MainWindow()
{
    saveSettings();
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
    
    // TODO: Start scanning
    statusBar()->showMessage(tr("Scanning %1...").arg(dir));
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Indeterminate
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
    if (total > 0) {
        m_progressBar->setRange(0, total);
        m_progressBar->setValue(current);
    }
    statusBar()->showMessage(tr("Scanning: %1").arg(file));
}

void MainWindow::onScanComplete()
{
    m_progressBar->setVisible(false);
    statusBar()->showMessage(tr("Scan complete"));
    
    // Refresh UI
    // TODO: Reload clusters and persons
}

void MainWindow::onClusterSelected(int64_t clusterId)
{
    m_exportAction->setEnabled(clusterId > 0);
    m_splitAction->setEnabled(clusterId > 0);
    
    // TODO: Show cluster details in face grid
}

void MainWindow::onPersonSelected(int64_t personId)
{
    m_exportAction->setEnabled(personId > 0);
    
    // TODO: Filter face grid to show only this person
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
