#pragma once

#include <QWidget>
#include <QScrollArea>
#include <cstdint>

namespace facefling {

/**
 * Grid widget displaying face thumbnails.
 * Can show all faces, faces in a cluster, or faces for a person.
 */
class FaceGridWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit FaceGridWidget(QWidget *parent = nullptr);
    ~FaceGridWidget() override = default;
    
    void showAllClusters();
    void showCluster(int64_t clusterId);
    void showPerson(int64_t personId);
    void clear();
    
signals:
    void clusterSelected(int64_t clusterId);
    void faceSelected(int64_t faceId);
    void facesSelected(const std::vector<int64_t> &faceIds);
    
private:
    void setupUi();
    
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_gridContainer = nullptr;
};

} // namespace facefling
