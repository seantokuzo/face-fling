/**
 * PersonListWidget implementation.
 */

#include "PersonListWidget.h"
#include "../services/Database.h"
#include "../models/Person.h"
#include "../models/Cluster.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QInputDialog>
#include <QDateTime>

namespace facefling {

// Custom data roles for list items
enum ItemRole {
    PersonIdRole = Qt::UserRole,
    ClusterIdRole = Qt::UserRole + 1,
    ItemTypeRole = Qt::UserRole + 2  // 0 = person, 1 = cluster
};

PersonListWidget::PersonListWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PersonListWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Header
    auto *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: #f5f5f5; border-bottom: 1px solid #ddd;");
    auto *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(12, 8, 12, 8);
    
    auto *label = new QLabel(tr("People"), headerWidget);
    label->setStyleSheet("font-weight: bold; font-size: 14px;");
    headerLayout->addWidget(label);
    
    layout->addWidget(headerWidget);
    
    // Show All button
    m_showAllButton = new QPushButton(tr("Show All Faces"), this);
    m_showAllButton->setStyleSheet(
        "QPushButton { "
        "  text-align: left; "
        "  padding: 8px 12px; "
        "  border: none; "
        "  background-color: white; "
        "  border-bottom: 1px solid #eee;"
        "}"
        "QPushButton:hover { background-color: #f0f0f0; }"
    );
    connect(m_showAllButton, &QPushButton::clicked, this, &PersonListWidget::onShowAllClicked);
    layout->addWidget(m_showAllButton);
    
    // List widget
    m_listWidget = new QListWidget(this);
    m_listWidget->setStyleSheet(
        "QListWidget { border: none; }"
        "QListWidget::item { padding: 8px 12px; border-bottom: 1px solid #eee; }"
        "QListWidget::item:hover { background-color: #f0f0f0; }"
        "QListWidget::item:selected { background-color: #e3f2fd; color: black; }"
    );
    layout->addWidget(m_listWidget);
    
    connect(m_listWidget, &QListWidget::itemClicked,
            this, &PersonListWidget::onItemClicked);
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &PersonListWidget::onItemDoubleClicked);
}

void PersonListWidget::setDatabase(std::shared_ptr<IDatabase> database)
{
    m_database = database;
}

void PersonListWidget::refresh()
{
    m_listWidget->clear();
    
    if (!m_database) return;
    
    // First, add identified persons
    auto persons = m_database->get_all_persons();
    for (const auto& person : persons) {
        auto faces = m_database->get_faces_for_person(person.id);
        
        QString text = QString("%1 (%2 faces)")
            .arg(QString::fromStdString(person.name))
            .arg(faces.size());
        
        auto *item = new QListWidgetItem(text, m_listWidget);
        item->setData(PersonIdRole, QVariant::fromValue(person.id));
        item->setData(ItemTypeRole, 0);
        item->setIcon(QIcon::fromTheme("user", QIcon(":/icons/person.png")));
    }
    
    // Then, add unidentified clusters
    auto clusters = m_database->get_all_clusters();
    int unknownCount = 0;
    
    for (const auto& cluster : clusters) {
        // Skip if cluster has a person assigned
        if (cluster.person_id.has_value()) continue;
        
        auto faces = m_database->get_faces_for_cluster(cluster.id);
        if (faces.empty()) continue;
        
        unknownCount++;
        QString text = tr("Unknown %1 (%2 faces)")
            .arg(unknownCount)
            .arg(faces.size());
        
        auto *item = new QListWidgetItem(text, m_listWidget);
        item->setData(ClusterIdRole, QVariant::fromValue(cluster.id));
        item->setData(ItemTypeRole, 1);
        item->setForeground(QColor("#666"));
        item->setIcon(QIcon::fromTheme("help-about", QIcon(":/icons/unknown.png")));
    }
    
    // Show count in header
    if (persons.empty() && unknownCount == 0) {
        auto *item = new QListWidgetItem(tr("No faces detected yet"), m_listWidget);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        item->setForeground(QColor("#999"));
    }
}

void PersonListWidget::clear()
{
    m_listWidget->clear();
}

int64_t PersonListWidget::selectedPersonId() const
{
    auto *item = m_listWidget->currentItem();
    if (item && item->data(ItemTypeRole).toInt() == 0) {
        return item->data(PersonIdRole).toLongLong();
    }
    return 0;
}

int64_t PersonListWidget::selectedClusterId() const
{
    auto *item = m_listWidget->currentItem();
    if (item && item->data(ItemTypeRole).toInt() == 1) {
        return item->data(ClusterIdRole).toLongLong();
    }
    return 0;
}

void PersonListWidget::onItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    int itemType = item->data(ItemTypeRole).toInt();
    
    if (itemType == 0) {
        // Person
        int64_t personId = item->data(PersonIdRole).toLongLong();
        emit personSelected(personId);
    } else {
        // Cluster
        int64_t clusterId = item->data(ClusterIdRole).toLongLong();
        emit clusterSelected(clusterId);
    }
}

void PersonListWidget::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item || !m_database) return;
    
    int itemType = item->data(ItemTypeRole).toInt();
    
    if (itemType == 1) {
        // Double-click on cluster = rename (assign person)
        int64_t clusterId = item->data(ClusterIdRole).toLongLong();
        
        bool ok;
        QString name = QInputDialog::getText(this, tr("Identify Person"),
            tr("Who is this person?"), QLineEdit::Normal, "", &ok);
        
        if (ok && !name.isEmpty()) {
            // Create new person
            Person person;
            person.name = name.toStdString();
            person.created_date = QDateTime::currentDateTime()
                .toString(Qt::ISODate).toStdString();
            
            int64_t personId = m_database->insert_person(person);
            
            // Assign cluster to person
            auto faces = m_database->get_faces_for_cluster(clusterId);
            for (const auto& face : faces) {
                m_database->update_face_person(face.id, personId);
            }
            
            // Update cluster
            auto cluster = m_database->get_cluster(clusterId);
            if (cluster) {
                // Update cluster's person_id (we'd need to add this method)
                // For now, faces are updated directly
            }
            
            refresh();
            emit personSelected(personId);
        }
    } else if (itemType == 0) {
        // Double-click on person = edit name
        int64_t personId = item->data(PersonIdRole).toLongLong();
        auto person = m_database->get_person(personId);
        
        if (person) {
            bool ok;
            QString name = QInputDialog::getText(this, tr("Edit Name"),
                tr("Person name:"), QLineEdit::Normal, 
                QString::fromStdString(person->name), &ok);
            
            if (ok && !name.isEmpty()) {
                person->name = name.toStdString();
                m_database->update_person(*person);
                refresh();
            }
        }
    }
}

void PersonListWidget::onShowAllClicked()
{
    m_listWidget->clearSelection();
    emit personSelected(0); // Signal to show all
}

} // namespace facefling
