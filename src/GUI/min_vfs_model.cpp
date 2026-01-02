#include <string_view>
#include <string>

#include "Utils/ints.hpp"
#include "Utils/str_util.hpp"
#include "min_vfs_model.hpp"
#include "min_vfs/min_vfs.hpp"
#include "min_vfs/min_vfs_base.hpp"

constexpr uint8_t COL_CNT = 2;

min_vfs_model_t::min_vfs_model_t(QObject *parent): QAbstractItemModel(parent)
{
	//NOP
}

int min_vfs_model_t::rowCount(const QModelIndex &parent) const
{
	if(parent.isValid()) return 0;

	return cur_dir.size();
}

int min_vfs_model_t::columnCount(const QModelIndex &parent) const
{
	return COL_CNT; //Name, size
}

QModelIndex min_vfs_model_t::index(int row, int column,
								   const QModelIndex &parent) const
{
	if(row < cur_dir.size() && column < COL_CNT)
		return createIndex(row, column);

	return QModelIndex();
}

QModelIndex min_vfs_model_t::parent(const QModelIndex &index) const
{
	//TODO
	return QModelIndex();
}

Qt::ItemFlags min_vfs_model_t::flags(const QModelIndex &index) const
{
	//Didn't actually need to make my own selection model after all...

	if(!index.isValid())
		return Qt::NoItemFlags | Qt::ItemNeverHasChildren;

	if(index.column())
		return Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
}

bool min_vfs_model_t::hasChildren(const QModelIndex &parent) const
{
	if(!parent.isValid()) return true;
	if(parent.column() || parent.row() >= cur_dir.size()) return false;

	return cur_dir[parent.row()].ftype == min_vfs::ftype_t::dir;
}

u16 min_vfs_model_t::set_dir(const std::filesystem::path &path)
{
	u16 err;

	cur_dir.clear();
	emit layoutAboutToBeChanged();
	err = min_vfs::list(path, cur_dir, false);
	emit layoutChanged();
	/*emit dataChanged(index(0, 0),
					 index(cur_dir.size(), COL_CNT));*/

	return err;
}

std::vector<min_vfs::dentry_t>& min_vfs_model_t::get_dir()
{
	return cur_dir;
}

QVariant min_vfs_model_t::data(const QModelIndex &index, int role) const
{
	if(!index.isValid() || index.row() >= cur_dir.size()
		|| index.column() >= COL_CNT) return QVariant();

	switch(role)
	{
		case Qt::DisplayRole:
			switch(index.column())
			{
				case 0:
					return QString::fromStdString(cur_dir[index.row()].fname);

				case 1:
					return QString::fromStdString(
						str_util::size_to_str<char>(
							cur_dir[index.row()].fsize));

				default:
					return QVariant();
			}

		case Qt::DecorationRole:
			if(index.column()) return QVariant();

			if(cur_dir[index.row()].ftype == min_vfs::ftype_t::dir)
				return QIcon::fromTheme(QIcon::ThemeIcon::FolderVisiting);
			else
				return QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen);

		default:
			return QVariant();
	}
}

QVariant min_vfs_model_t::headerData(int section, Qt::Orientation orientation,
									 int role) const
{
	if(role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();

	switch(section)
	{
		case 0:
			return QStringLiteral("Name");

		case 1:
			return QStringLiteral("Size");

		default:
			return QVariant();
	}
}
