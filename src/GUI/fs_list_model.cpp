#include "fs_list_model.hpp"
#include "min_vfs/min_vfs.hpp"

constexpr u8 COL_CNT = 1;

fs_list_model_t::fs_list_model_t(QObject *parent): QAbstractItemModel(parent)
{
	//NOP
}

void fs_list_model_t::refresh()
{
	u16 err;

	fs_stats.clear();
	emit layoutAboutToBeChanged();
	mtx.lock();
	min_vfs::lsmount(fs_stats);
	mtx.unlock();
	emit layoutChanged();
}

std::vector<min_vfs::mount_stats_t>& fs_list_model_t::get_fs_stats()
{
	return fs_stats;
}

int fs_list_model_t::columnCount(const QModelIndex &parent) const
{
	return 1;
}

QVariant fs_list_model_t::data(const QModelIndex &index, int role) const
{
	if(!index.isValid() || index.row() >= fs_stats.size()
		|| index.column() >= COL_CNT || role != Qt::DisplayRole)
		return QVariant();

	return QString::fromStdString(
		fs_stats[index.row()].path.filename().string());
}

bool fs_list_model_t::hasChildren(const QModelIndex &parent) const
{
	return false;
}

QVariant fs_list_model_t::headerData(int section, Qt::Orientation orientation,
									 int role) const
{
	if(role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();

	return QStringLiteral("Name");
}

QModelIndex fs_list_model_t::index(int row, int column,
								   const QModelIndex &parent) const
{
	if(row < fs_stats.size() && column < COL_CNT)
		return createIndex(row, column);

	return QModelIndex();
}

QModelIndex fs_list_model_t::parent(const QModelIndex &index) const
{
	return QModelIndex();
}

int fs_list_model_t::rowCount(const QModelIndex &parent) const
{
	if(parent.isValid()) return 0;

	return fs_stats.size();
}
