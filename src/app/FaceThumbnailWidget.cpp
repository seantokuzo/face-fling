/**
 * FaceThumbnailWidget implementation.
 */

#include "FaceThumbnailWidget.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QFile>

namespace facefling {

FaceThumbnailWidget::FaceThumbnailWidget(int64_t faceId, QWidget *parent)
    : QWidget(parent)
    , m_faceId(faceId)
{
    setupUi();
}

void FaceThumbnailWidget::setupUi()
{
    setFixedSize(m_size + 8, m_size + 8);
    setCursor(Qt::PointingHandCursor);
    
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(0);
    
    m_imageLabel = new QLabel(this);
    m_imageLabel->setFixedSize(m_size, m_size);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet(
        "QLabel {"
        "  background-color: #f0f0f0;"
        "  border-radius: 4px;"
        "}"
    );
    
    layout->addWidget(m_imageLabel);
}

void FaceThumbnailWidget::setThumbnailPath(const QString &path)
{
    if (QFile::exists(path)) {
        loadThumbnail(path);
    } else {
        // Show placeholder
        m_imageLabel->setText("?");
        m_pixmap = QPixmap();
    }
}

void FaceThumbnailWidget::loadThumbnail(const QString &path)
{
    m_pixmap = QPixmap(path);
    if (!m_pixmap.isNull()) {
        m_pixmap = m_pixmap.scaled(m_size, m_size, 
            Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        
        // Crop to square if needed
        if (m_pixmap.width() > m_size || m_pixmap.height() > m_size) {
            int x = (m_pixmap.width() - m_size) / 2;
            int y = (m_pixmap.height() - m_size) / 2;
            m_pixmap = m_pixmap.copy(x, y, m_size, m_size);
        }
        
        // Make circular
        QPixmap circular(m_size, m_size);
        circular.fill(Qt::transparent);
        
        QPainter painter(&circular);
        painter.setRenderHint(QPainter::Antialiasing);
        
        QPainterPath path;
        path.addEllipse(0, 0, m_size, m_size);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, m_pixmap);
        
        m_imageLabel->setPixmap(circular);
    }
}

void FaceThumbnailWidget::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        update();
    }
}

void FaceThumbnailWidget::setThumbnailSize(int size)
{
    m_size = size;
    setFixedSize(m_size + 8, m_size + 8);
    m_imageLabel->setFixedSize(m_size, m_size);
}

void FaceThumbnailWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_faceId);
    }
    QWidget::mousePressEvent(event);
}

void FaceThumbnailWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(m_faceId);
    }
    QWidget::mouseDoubleClickEvent(event);
}

void FaceThumbnailWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    
    if (m_selected) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        QPen pen(QColor("#007AFF")); // macOS accent blue
        pen.setWidth(3);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
    }
}

} // namespace facefling
