#include "dir_list_item.hpp"

dir_list_item_t::dir_list_item_t(QListWidget *parent, int type): QListWidgetItem(parent, type)
{
	//NOP
}

dir_list_item_t::dir_list_item_t(const QString &text, const std::filesystem::path &path,
				QListWidget *parent, int type): QListWidgetItem(text, parent, type), path(path)
{
	//NOP
}

dir_list_item_t::dir_list_item_t(const QIcon &icon, const QString &text,
				const std::filesystem::path &path, QListWidget *parent,
				int type): QListWidgetItem(text, parent, type), path(path)
{
	//NOP
}
