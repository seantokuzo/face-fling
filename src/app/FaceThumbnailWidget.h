#pragma once

#include <QWidget>
#include <QLabel>
#include <cstdint>

namespace facefling {

/**
 * Widget displaying a single face thumbnail with selection support.
 */
class FaceThumbnailWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit FaceThumbnailWidget(int64_t faceId, QWidget *parent = nullptr);
    ~FaceThumbnailWidget() override = default;
    
    int64_t faceId() const { return m_faceId; }
    
    void setThumbnailPath(const QString &path);
    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }
    
    // Set size for thumbnail (default 120)
    void setThumbnailSize(int size);
    int thumbnailSize() const { return m_size; }
    
signals:
    void clicked(int64_t faceId);
    void doubleClicked(int64_t faceId);
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    
private:
    void setupUi();
    void loadThumbnail(const QString &path);
    
    int64_t m_faceId = 0;
    QLabel *m_imageLabel = nullptr;
    int m_size = 120;
    bool m_selected = false;
    QPixmap m_pixmap;
};

} // namespace facefling
