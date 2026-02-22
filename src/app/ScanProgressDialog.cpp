/**
 * ScanProgressDialog implementation.
 */

#include "ScanProgressDialog.h"
#include <QVBoxLayout>

namespace facefling {

ScanProgressDialog::ScanProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    setWindowTitle(tr("Scanning..."));
    setModal(true);
    setMinimumWidth(400);
}

void ScanProgressDialog::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    
    m_messageLabel = new QLabel(tr("Scanning for photos..."), this);
    layout->addWidget(m_messageLabel);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0); // Indeterminate initially
    layout->addWidget(m_progressBar);
    
    m_fileLabel = new QLabel(this);
    m_fileLabel->setStyleSheet("color: gray; font-size: 11px;");
    m_fileLabel->setWordWrap(true);
    layout->addWidget(m_fileLabel);
    
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    layout->addWidget(m_cancelButton, 0, Qt::AlignRight);
    
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
        emit cancelled();
        reject();
    });
}

void ScanProgressDialog::setProgress(int current, int total)
{
    if (total > 0) {
        m_progressBar->setRange(0, total);
        m_progressBar->setValue(current);
        m_messageLabel->setText(tr("Found %1 of %2 images").arg(current).arg(total));
    } else {
        m_progressBar->setRange(0, 0);
        m_messageLabel->setText(tr("Found %1 images").arg(current));
    }
}

void ScanProgressDialog::setCurrentFile(const QString &file)
{
    m_fileLabel->setText(file);
}

void ScanProgressDialog::setMessage(const QString &message)
{
    m_messageLabel->setText(message);
}

} // namespace facefling
