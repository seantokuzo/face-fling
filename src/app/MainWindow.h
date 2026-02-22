#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QProgressBar>
#include <memory>

namespace facefling {

// Forward declarations
class FaceGridWidget;
class PersonListWidget;
class Scanner;
class Indexer;
class Clusterer;
class Database;
class FaceService;

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
    std::unique_ptr<Scanner> m_scanner;
    std::unique_ptr<Indexer> m_indexer;
    std::unique_ptr<Clusterer> m_clusterer;
};

} // namespace facefling
