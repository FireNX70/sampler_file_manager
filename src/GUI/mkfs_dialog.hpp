#ifndef QT_MKFS_DIALOG_HEADER_INCLUDE_GUARD
#define QT_MKFS_DIALOG_HEADER_INCLUDE_GUARD

#include <vector>

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>

struct mkfs_params_t
{
	int type;
	std::string label;
};

class mkfs_dialog_t: public QDialog
{
	Q_OBJECT

public:
	mkfs_dialog_t(QWidget *parent = nullptr,
				  Qt::WindowFlags f = Qt::WindowFlags());

	void set_types(const std::vector<QString> &type_names);
	mkfs_params_t params();

private:
	QComboBox *type;
	QLineEdit *label;
};
#endif
