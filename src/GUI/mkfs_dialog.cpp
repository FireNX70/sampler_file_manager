#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QDialogButtonBox>

#include "mkfs_dialog.hpp"

mkfs_dialog_t::mkfs_dialog_t(QWidget *parent, Qt::WindowFlags flags):
	QDialog(parent, flags)
{
	QWidget *type_row, *label_row;
	QDialogButtonBox *button_box;

	setWindowTitle(tr("MKFS"));
	setLayout(new QVBoxLayout(this));

	type_row = new QWidget(this);
	type_row->setLayout(new QHBoxLayout(type_row));
	type_row->layout()->addWidget(new QLabel(tr("Type"), type_row));
	type = new QComboBox(type_row);
	type_row->layout()->addWidget(type);
	layout()->addWidget(type_row);

	label_row = new QWidget(this);
	label_row->setLayout(new QHBoxLayout(label_row));
	label_row->layout()->addWidget(new QLabel(tr("Label"), label_row));
	label = new QLineEdit(label_row);
	label_row->layout()->addWidget(label);
	layout()->addWidget(label_row);

	button_box = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel);

	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	layout()->addWidget(button_box);
}

void mkfs_dialog_t::set_types(const std::vector<QString> &type_names)
{
	for(const QString &type_name: type_names) type->addItem(type_name);
}

mkfs_params_t mkfs_dialog_t::params()
{
	return {type->currentIndex(), label->text().toStdString()};
}
