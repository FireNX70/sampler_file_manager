#ifndef QT_TASK_MESSAGE_BOX_PROXY_HEADER_INCLUDE_GUARD
#define QT_TASK_MESSAGE_BOX_PROXY_HEADER_INCLUDE_GUARD

#include <QObject>
#include <QMessageBox>

class task_msg_proxy_t: public QObject
{
	Q_OBJECT

public:
	task_msg_proxy_t();

signals:
	void req_msg_box(QMessageBox::Icon icon, const QString &title,
					 const QString &text, QMessageBox::StandardButtons buttons
						= QMessageBox::NoButton,
						Qt::WindowFlags flags = Qt::Dialog
							| Qt::MSWindowsFixedSizeDialogHint);
};
#endif
