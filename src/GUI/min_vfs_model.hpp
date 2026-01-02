#ifndef QT_MIN_VFS_LIST_MODEL_HEADER_INCLUDE_GUARD
#define QT_MIN_VFS_LIST_MODEL_HEADER_INCLUDE_GUARD

#include <vector>
#include <filesystem>

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

class min_vfs_model_t: public QAbstractItemModel
{
	Q_OBJECT

public:
	min_vfs_model_t(QObject *parent = nullptr);

	//QFileSystemModel functions
	/*QIcon fileIcon(const QModelIndex &index) const;
	QFileInfo fileInfo(const QModelIndex &index) const;
	QString fileName(const QModelIndex &index) const;
	QString filePath(const QModelIndex &index) const;
	QDir::Filters filter() const;
	QAbstractFileIconProvider *iconProvider() const;
	QModelIndex index(const QString &path, int column = 0) const;
	bool isDir(const QModelIndex &index) const;
	bool isReadOnly() const;
	QDateTime lastModified(const QModelIndex &index) const;
	QDateTime lastModified(const QModelIndex &index, const QTimeZone &tz) const;
	QModelIndex mkdir(const QModelIndex &parent, const QString &name);
	QVariant myComputer(int role = Qt::DisplayRole) const;
	bool nameFilterDisables() const;
	QStringList nameFilters() const;
	bool remove(const QModelIndex &index);
	bool resolveSymlinks() const;
	bool rmdir(const QModelIndex &index);
	QDir rootDirectory() const;
	QString rootPath() const;
	void setIconProvider(QAbstractFileIconProvider *provider);
	QModelIndex setRootPath(const QString &newPath);
	qint64 size(const QModelIndex &index) const;
	QString type(const QModelIndex &index) const;*/
	u16 set_dir(const std::filesystem::path &path);
	std::vector<min_vfs::dentry_t>& get_dir();

	//QFileSystemModel overriden functions
	//bool canFetchMore(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
		override;
	/*bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row,
					  int column, const QModelIndex &parent) override;
	void fetchMore(const QModelIndex &parent) override;*/
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
						int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column,
					  const QModelIndex &parent = QModelIndex()) const override;
	/*QMimeData *mimeData(const QModelIndexList &indexes) const override;
	QStringList mimeTypes() const override;*/
	QModelIndex parent(const QModelIndex &index) const override;
	//QHash<int, QByteArray> roleNames() const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	/*bool setData(const QModelIndex &idx, const QVariant &value,
				 int role = Qt::EditRole) override;
	QModelIndex sibling(int row, int column, const QModelIndex &idx) const
		override;
	void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
	Qt::DropActions supportedDropActions() const override;*/

private:
	std::vector<min_vfs::dentry_t> cur_dir;
};

#endif
