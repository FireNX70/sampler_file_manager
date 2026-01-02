#ifndef QT_FS_LIST_MODEL_HEADER_INCLUDE_GUARD
#define QT_FS_LIST_MODEL_HEADER_INCLUDE_GUARD

#include <mutex>
#include <vector>

#include <QAbstractItemModel>
#include <QIcon>
#include <QModelIndex>
#include <QFileInfo>
#include <QString>
#include <QDir>
#include <QAbstractFileIconProvider>
#include <QDateTime>
#include <QVariant>
#include <QStringList>
#include <QtTypes>
#include <QMimeData>
#include <QHash>
#include <QByteArray>

#include "Utils/ints.hpp"
#include "min_vfs/min_vfs.hpp"

class fs_list_model_t: public QAbstractItemModel
{
	Q_OBJECT

public:
	fs_list_model_t(QObject *parent = nullptr);

	void refresh();
	std::vector<min_vfs::mount_stats_t>& get_fs_stats();

	//QAbstractItemModel overriden functions
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
		override;
	bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
						int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column,
					  const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;

private:
	std::mutex mtx;
	std::vector<min_vfs::mount_stats_t> fs_stats;
};

#endif
