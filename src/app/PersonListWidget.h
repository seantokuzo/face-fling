#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <memory>
#include <cstdint>

namespace facefling {

// Forward declarations
class IDatabase;

/**
 * Sidebar widget showing list of identified persons and unidentified clusters.
 */
class PersonListWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit PersonListWidget(QWidget *parent = nullptr);
    ~PersonListWidget() override = default;
    
    void setDatabase(std::shared_ptr<IDatabase> database);
    void refresh();
    void clear();
    
    int64_t selectedPersonId() const;
    int64_t selectedClusterId() const;
    
signals:
    void personSelected(int64_t personId);
    void clusterSelected(int64_t clusterId);
    void renameRequested(int64_t clusterId);
    
private slots:
    void onItemClicked(QListWidgetItem *item);
    void onItemDoubleClicked(QListWidgetItem *item);
    void onShowAllClicked();
    
private:
    void setupUi();
    
    std::shared_ptr<IDatabase> m_database;
    QListWidget *m_listWidget = nullptr;
    QPushButton *m_showAllButton = nullptr;
};

} // namespace facefling
