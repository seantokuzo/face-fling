#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

namespace facefling {

/**
 * Modal dialog showing scan progress.
 */
class ScanProgressDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ScanProgressDialog(QWidget *parent = nullptr);
    ~ScanProgressDialog() override = default;
    
    void setProgress(int current, int total);
    void setCurrentFile(const QString &file);
    void setMessage(const QString &message);
    
signals:
    void cancelled();
    
private:
    void setupUi();
    
    QLabel *m_messageLabel = nullptr;
    QLabel *m_fileLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QPushButton *m_cancelButton = nullptr;
};

} // namespace facefling
