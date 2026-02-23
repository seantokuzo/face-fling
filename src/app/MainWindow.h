#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QProgressBar>
#include <QThread>
#include <memory>
#include <atomic>

namespace facefling {

// Forward declarations
class FaceGridWidget;
class PersonListWidget;
class ScanProgressDialog;
class Scanner;
class Indexer;
class Clusterer;
class Database;
class FaceService;
class ImageLoader;

/**
 * Main application window.
 * 
 * Layout:
 * ┌─────────────────────────────────────────────────────────┐
 * │ Menu Bar                                                │
 * ├─────────────────────────────────────────────────────────┤
 * │ Toolbar                                                 │
 * ├────────────────┬────────────────────────────────────────┤
 * │                │                                        │
 * │  Person List   │           Face Grid                    │
 * │  (sidebar)     │                                        │
 * │                │                                        │
 * │                │                                        │
 * ├────────────────┴────────────────────────────────────────┤
 * │ Status Bar                              [Progress]      │
 * └─────────────────────────────────────────────────────────┘
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    
public slots:
    // File menu actions
    void openFolder();
    void exportPerson();
    
    // Edit menu actions
    void mergeClusters();
    void splitCluster();
    
    // View menu actions
    void toggleSidebar();
    
protected:
    void closeEvent(QCloseEvent *event) override;
    
private slots:
    void onScanProgress(int current, int total, const QString &file);
    void onScanComplete();
    void onIndexProgress(int current, int total, const QString &file, int faces);
    void onIndexComplete();
    void onClusterProgress(int current, int total);
    void onClusterComplete();
    void onClusterSelected(int64_t clusterId);
    void onPersonSelected(int64_t personId);
    
private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void createActions();
    void loadSettings();
    void saveSettings();
    
    // UI components
    QSplitter *m_splitter = nullptr;
    FaceGridWidget *m_faceGrid = nullptr;
    PersonListWidget *m_personList = nullptr;
    QProgressBar *m_progressBar = nullptr;
    
    // Actions
    QAction *m_openAction = nullptr;
    QAction *m_exportAction = nullptr;
    QAction *m_mergeAction = nullptr;
    QAction *m_splitAction = nullptr;
    QAction *m_settingsAction = nullptr;
    
    // Core services
    std::shared_ptr<Database> m_database;
    std::shared_ptr<FaceService> m_faceService;
    std::shared_ptr<ImageLoader> m_imageLoader;
    std::unique_ptr<Scanner> m_scanner;
    std::unique_ptr<Indexer> m_indexer;
    std::unique_ptr<Clusterer> m_clusterer;
    
    // Processing state
    QString m_currentScanPath;
    std::vector<std::string> m_scannedFiles;
    ScanProgressDialog *m_progressDialog = nullptr;
    std::atomic<bool> m_processingCancelled{false};
    
    // Helper methods
    void initializeServices();
    void runPipeline(const QString &folderPath);
    void refreshUI();
    QString getThumbnailPath(int64_t faceId) const;
};

} // namespace facefling
