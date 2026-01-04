#ifndef QT_GUI_MAIN_WINDOW_HEADER_INCLUDE_GUARD
#define QT_GUI_MAIN_WINDOW_HEADER_INCLUDE_GUARD

#include <filesystem>
#include <list>
#include <utility>
#include <thread>
#include <mutex>

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QSplitter>
#include <QToolButton>
#include <QLineEdit>
#include <QListWidget>
#include <QListView>
#include <QTreeView>
#include <QAction>
#include <QShortcut>
#include <QMessageBox>

#include "GUI/min_vfs_model.hpp"
#include "GUI/min_vfs_selection_model.hpp"
#include "GUI/fs_list_model.hpp"
#include "GUI/dir_history.hpp"

struct task_t
{
	std::thread thread;
	std::mutex mtx;
	bool interruptible;
};

class main_window_t: public QMainWindow
{
	Q_OBJECT

public:
	main_window_t();
	~main_window_t() = default;

	void closeEvent(QCloseEvent *event) override;

	typedef std::list<task_t> task_list_t;

	template<class F, class... Args>
	friend void task_inner(main_window_t &main_window,
			const task_list_t::iterator thread_it,
			F &&f, Args &&...args);

	template<class F, class... Args>
	friend void start_task(main_window_t &main_window, const bool interruptible,
						   F &&f, Args &&...args);

public slots:
	void spawn_msg_box(QMessageBox::Icon icon, const QString &title,
					   const QString &text, QMessageBox::StandardButtons buttons
						= QMessageBox::NoButton,
						Qt::WindowFlags flags = Qt::Dialog
							| Qt::MSWindowsFixedSizeDialogHint);

private:
	std::filesystem::path workpath; //I don't really need this
	dir_history_t dir_history;

	bool copy_mode;
	std::list<std::filesystem::path> to_copy; //0 is base path
	std::list<task_t> threads;
	std::mutex threads_mtx;

	//Views only take ownership of models if they're set as the model's parent
	min_vfs_model_t min_vfs_model;
	min_vfs_selection_model_t min_vfs_selection_model;
	fs_list_model_t fs_list_model;

	//Owned by main window
	QWidget *top_widget;

	/*QMainWindow's documentation doesn't explicitly say it takes ownership of
	 *the toolbar, but it seems like the reasonable assumption here.*/
	QToolBar *toolbar;

	//Owned by top_widget
	QVBoxLayout *top_vbox;

	//Owned by top_vbox
	QSplitter *h_splitter;

	//Owned by toolbar
	QToolButton *back_button, *fwd_button, *up_button;
	QLineEdit *cur_path_disp;

	//Owned by h_splitter
	QWidget *left_widget;
	QVBoxLayout *left_vbox;
	QTreeView *cur_dir_list;

	//cur_dir_list context menu
	QAction refresh, open_dir, mkdir, rename, copy, cut, paste, remove, mkfs,
		fsck, mount;

	//cur_dir_list shortcuts. Does cur_dir_list own these?
	QShortcut *cur_dir_refresh_shortcut, *open_shortcut;

	//Owned by v_splitter
	QListWidget *dir_list;
	QListView *fs_list;

	//fs_list context menu
	QAction refresh_fs_list, unmount;

	void toolbar_setup();
	void init_dir_shortcuts();
	void fs_list_setup();
	void setup_cur_dir_list_ctx_menu();
	void cur_dir_list_setup();
	void fs_list_ctx_menu_setup();

	void cd(const std::filesystem::path &new_path,
			const bool update_history = true);
	void try_mount(const std::filesystem::path &path);
	void umount(const std::filesystem::path &path);
	void copy_setup(const bool move);
	void paste_op();
	void remove_op();
	void mkfs_op();
	void fsck_op();

	template<const bool modify>
	bool filename_entry_dialog(const QString &text_init, QString &res);

	QString get_err_msg(const u16 err);
	QString compose_err_msg(const std::filesystem::path &path, const u16 err);
	QString compose_err_msg(const std::filesystem::path &src_path,
							const std::filesystem::path &dst_path,
							const u16 err);

	void error_msg(const QString &op_name, const QString &msg);
};

#endif
