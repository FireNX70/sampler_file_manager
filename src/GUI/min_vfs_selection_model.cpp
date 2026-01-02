#include <limits>
#include <QModelIndex>

#include "min_vfs_selection_model.hpp"

min_vfs_selection_model_t::min_vfs_selection_model_t(QAbstractItemModel *model):
	QItemSelectionModel(model)
{
	//NOP
}

min_vfs_selection_model_t::min_vfs_selection_model_t(QAbstractItemModel *model,
													 QObject *parent):
											QItemSelectionModel(model, parent)
{
	//NOP
}

void min_vfs_selection_model_t::select(const QItemSelection &selection,
									QItemSelectionModel::SelectionFlags command)
{
	int min_row, max_row, min_col;

	min_col = std::numeric_limits<int>::max();
	min_row = std::numeric_limits<int>::max();
	max_row = 0;

	const QModelIndex parent = selection.indexes().empty() ? QModelIndex()
		: selection.indexes()[0].parent();

	for(const QModelIndex &index: selection.indexes())
	{
		if(index.column() < min_col) min_col = index.column();
		if(index.row() < min_row) min_row = index.row();
		if(index.row() > max_row) max_row = index.row();
	}

	/*If the selection behavior is row; this somehow still selects the
	 *entire row.*/

	if(!min_col)
		return QItemSelectionModel::select(
			QItemSelection(model()->index(min_row, 0, parent),
						   model()->index(max_row, 0, parent)), command);
	else
	{
		/*This seems to be the way clearing the selection by clicking empty
		 *space usually works. For future reference; clear() and reset() don't
		 *work, and neither do select and QItemSelectionModel::select with an
		 *invalid QModelIndex and QItemSelectionModel::Clear command. I think
		 *they're all causing recursion loops.*/

		QItemSelectionModel::select(QItemSelection(),
									QItemSelectionModel::Select);

		return QItemSelectionModel::select(QItemSelection(),
										   QItemSelectionModel::ClearAndSelect);
	}
}

void min_vfs_selection_model_t::select(const QModelIndex &index,
									QItemSelectionModel::SelectionFlags command)
{
	if(!index.isValid() || !index.column())
		return QItemSelectionModel::select(index, command);
}
