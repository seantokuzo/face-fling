#pragma once

#include <QWidget>
#include <QListWidget>
#include <cstdint>

namespace facefling {

/**
 * Sidebar widget showing list of identified persons.
 */
class PersonListWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit PersonListWidget(QWidget *parent = nullptr);
    ~PersonListWidget() override = default;
    
    void refresh();
    void clear();
    
signals:
    void personSelected(int64_t personId);
    
private slots:
    void onItemClicked(QListWidgetItem *item);
    
private:
    void setupUi();
    
    QListWidget *m_listWidget = nullptr;
};

} // namespace facefling
