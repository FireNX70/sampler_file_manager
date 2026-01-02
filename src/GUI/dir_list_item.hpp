#ifndef QT_DIR_LIST_ITEM_HEADER_INCLUDE_GUARD
#define QT_DIR_LIST_ITEM_HEADER_INCLUDE_GUARD

#include <QListWidgetItem>
#include <filesystem>

class dir_list_item_t: public QListWidgetItem
{
public:
	const std::filesystem::path path;

	dir_list_item_t(QListWidget *parent = nullptr, int type = Type);
	dir_list_item_t(const QString &text, const std::filesystem::path &path,
					QListWidget *parent = nullptr, int type = Type);
	dir_list_item_t(const QIcon &icon, const QString &text,
					const std::filesystem::path &path,
				 QListWidget *parent = nullptr, int type = Type);
};
#endif
