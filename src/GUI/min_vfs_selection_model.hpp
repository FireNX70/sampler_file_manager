#ifndef QT_MIN_VFS_SELECTION_MODEL_HEADER_INCLUDE_GUARD
#define QT_MIN_VFS_SELECTION_MODEL_HEADER_INCLUDE_GUARD

#include <QItemSelectionModel>

class min_vfs_selection_model_t: public QItemSelectionModel
{
public:
	min_vfs_selection_model_t(QAbstractItemModel *model = nullptr);
	min_vfs_selection_model_t(QAbstractItemModel *model, QObject *parent);

	virtual void select(const QItemSelection &selection,
						QItemSelectionModel::SelectionFlags command) override;
	virtual void select(const QModelIndex &index,
						QItemSelectionModel::SelectionFlags command) override;
};
#endif
