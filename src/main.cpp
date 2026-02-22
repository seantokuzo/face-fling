/**
 * Face-Fling - Family Photo Organizer by Face
 * 
 * A native macOS application that scans photo folders, detects faces,
 * clusters similar faces, and helps organize photos by people.
 * 
 * Entry point for the application.
 */

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include "app/MainWindow.h"

int main(int argc, char *argv[])
{
    // High DPI support
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    QApplication app(argc, argv);
    
    // Set application metadata
    app.setApplicationName("Face-Fling");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Face-Fling");
    app.setOrganizationDomain("facefling.app");
    
    // Set up application data directory
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    
    // Create and show main window
    facefling::MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}
