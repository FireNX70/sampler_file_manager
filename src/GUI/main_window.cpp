#include <filesystem>
#include <string>
#include <thread>
#include <list>
#include <functional>

#if _WIN32
	#include <algorithm>
#endif

#include <QMessageBox>
#include <QAction>
#include <QStandardPaths>
#include <QMenu>
#include <QCloseEvent>
#include <QHeaderView>
#include <QInputDialog>

#include "Utils/utils.hpp"
#include "main_window.hpp"
#include "GUI/dir_list_item.hpp"
#include "GUI/filename_validator.hpp"
#include "GUI/task_msg_proxy.hpp"
#include "GUI/mkfs_dialog.hpp"
#include "min_vfs/min_vfs.hpp"
#include "min_vfs/min_vfs_base.hpp"
#include "E-MU/EMU_FS_drv.hpp"
#include "Roland/S5XX/S5XX_FS_drv.hpp"
#include "Roland/S7XX/S7XX_FS_drv.hpp"

#if _WIN32
	std::filesystem::path concat_path_func(const std::filesystem::path &A,
										   const std::filesystem::path &B)
	{
		return A.string() + "/" + B.string();
	}

	#define CONCAT_PATHS(A, B) concat_path_func(A, B)
#else
	#define CONCAT_PATHS(A, B) A / B
#endif

void main_window_t::spawn_msg_box(QMessageBox::Icon icon, const QString &title,
						const QString &text,
						QMessageBox::StandardButtons buttons,
						Qt::WindowFlags flags)
{
	QMessageBox msg_box(icon, title, text, buttons, this, flags);
	msg_box.exec();
}

void main_window_t::closeEvent(QCloseEvent *event)
{
	threads_mtx.lock();
	for(const task_t &entry: threads)
	{
		if(!entry.interruptible)
		{
			event->ignore();
			threads_mtx.unlock();
			QMessageBox::warning(this, "",
								  tr("Can't quit. Pending operations.'\n"));
			return;
		}
	}

	event->accept();
}

void main_window_t::cd(const std::filesystem::path &new_path,
					   const bool update_history)
{
	u16 err;

	workpath = new_path;
	cur_path_disp->setText(QString::fromStdString(workpath.string()));
	err = min_vfs_model.set_dir(workpath);
	min_vfs_selection_model.clearSelection();

	if(err)
	{
		spawn_msg_box(QMessageBox::Warning, tr("Change directory")
			+ QString(' ') + tr("failed"), get_err_msg(err)
			+ QString::fromStdString("\n\n") + tr("Path: ")
			+ QString::fromStdString(new_path.string()));
	}

	if(update_history) dir_history.fwd(workpath);
}

#if _WIN32
std::string replace_win_path_separators(const std::filesystem::path &path)
{
	std::string tgt_path_str = path.string();
	std::replace(tgt_path_str.begin(), tgt_path_str.end(), '\\', '/');
	return tgt_path_str;
}

void replace_win_path_separators(std::string &path_str)
{
	std::replace(path_str.begin(), path_str.end(), '\\', '/');
}
#endif

void main_window_t::toolbar_setup()
{
	toolbar = new QToolBar(/*top_widget*/);
	addToolBar(toolbar);

	/*If the toolbar is added to a widget instead of the main window, it won't
	 *be movable, but here we gotta disable it.*/
	toolbar->setMovable(false);
	toolbar->setFloatable(false);

	//We also wanna disable this
	toolbar->toggleViewAction()->setEnabled(false);
	toolbar->toggleViewAction()->setVisible(false);

	back_button = new QToolButton(toolbar);
	fwd_button = new QToolButton(toolbar);
	up_button = new QToolButton(toolbar);
	cur_path_disp = new QLineEdit(toolbar);
	connect(cur_path_disp, &QLineEdit::editingFinished, this,
	[this]()
	{
		#if _WIN32
			std::string path_str = cur_path_disp->text().toStdString();
			replace_win_path_separators(path_str);
			cd(path_str);
		#else
			cd(cur_path_disp->text().toStdString());
		#endif
	});

	/*While QToolButton::setArrowType exists, the arrows used by that are
	 *smaller than the arrow icons. The size appears to match KDE's minimize and
	 *maximize window buttons. I chose to go with the icons because they're
	 *gonna be more visible, and it appears to be what Dolphin uses anyway.*/
	back_button->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoPrevious));
	connect(back_button, &QToolButton::clicked, this,
	[this](bool checked)
	{
		cd(dir_history.back(), false);
	});

	fwd_button->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoNext));
	connect(fwd_button, &QToolButton::clicked, this,
	[this](bool checked)
	{
		cd(dir_history.fwd(), false);
	});

	up_button->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::GoUp));
	connect(up_button, &QToolButton::clicked, this,
	[this](bool checked)
	{
		cd(workpath.parent_path());
	});

	toolbar->addWidget(back_button);
	toolbar->addWidget(fwd_button);
	toolbar->addWidget(up_button);
	toolbar->addWidget(cur_path_disp);
}

void main_window_t::init_dir_shortcuts()
{
	//TODO Make translatable

	QListWidgetItem *items[5];

	/*QListView takes ownership of QListItems, even if the docs don't mention
	this.*/

	items[0] = new dir_list_item_t(tr("Root"), std::filesystem::path(
		QStandardPaths::standardLocations(
			QStandardPaths::HomeLocation)[0].toStdString()).root_path(),
								   dir_list);
	items[1] = new dir_list_item_t(QStandardPaths::displayName(
		QStandardPaths::HomeLocation), QStandardPaths::standardLocations(
			QStandardPaths::HomeLocation)[0].toStdString(), dir_list);
	items[2] = new dir_list_item_t(QStandardPaths::displayName(
		QStandardPaths::DocumentsLocation), QStandardPaths::standardLocations(
			QStandardPaths::DocumentsLocation)[0].toStdString(), dir_list);
	items[3] = new dir_list_item_t(QStandardPaths::displayName(
		QStandardPaths::MusicLocation), QStandardPaths::standardLocations(
			QStandardPaths::MusicLocation)[0].toStdString(), dir_list);
	items[4] = new dir_list_item_t(QStandardPaths::displayName(
		QStandardPaths::DownloadLocation), QStandardPaths::standardLocations(
			QStandardPaths::DownloadLocation)[0].toStdString(), dir_list);

	dir_list->addItem(items[0]);
	dir_list->addItem(items[1]);
	dir_list->addItem(items[2]);
	dir_list->addItem(items[3]);
	dir_list->addItem(items[4]);

	connect(dir_list, &QListWidget::itemClicked, this,
	[this, items](QListWidgetItem *item)
	{
		cd(((dir_list_item_t*)item)->path);
	});
}

void main_window_t::copy_setup(const bool move)
{
	copy_mode = move;
	to_copy.clear();

	to_copy.push_back(workpath);

	for(const QModelIndex index:
		cur_dir_list->selectionModel()->selectedIndexes())
	{
		to_copy.push_back(min_vfs_model.get_dir()[index.row()].fname);
	}
}

/*This was initially a lambda within start_task, passed directly to
 *std::thread's constructor and capturing main_window (as lvalue ref),
 *thread_it, f and args. It worked fine when passing it lambdas; but was only a
 *proof of concept and would have run into problems. Implementing proper
 *forwarding for lambda captures is a bit of a PITA (see
 *https://vittorioromeo.info/index/blog/
 *capturing_perfectly_forwarded_objects_in_lambdas.html), so I opted for this
 *instead.
 *
 *Further reading:
 *	https://stackoverflow.com/a/3582313
 *	https://en.cppreference.com/w/cpp/standard_library/decay-copy.html
 */
template<class F, class... Args>
void task_inner(main_window_t &main_window,
			const main_window_t::task_list_t::iterator thread_it,
			F &&f, Args &&...args)
{
	//NOTE: I think auto(x) might not actually be necessary here.
	std::invoke(auto(std::forward<F>(f)),
				auto(std::forward<Args>(args))...);

	thread_it->mtx.lock();
	thread_it->thread.detach();

	main_window.threads_mtx.lock();
	main_window.threads.erase(thread_it);
	main_window.threads_mtx.unlock();
}

template<class F, class... Args>
void start_task(main_window_t &main_window, const bool interruptible, F &&f,
				Args &&...args)
{
	main_window.threads_mtx.lock();
	main_window.threads.emplace_back();
	const main_window_t::task_list_t::iterator thread_it =
		--main_window.threads.end();
	thread_it->interruptible = interruptible;
	main_window.threads_mtx.unlock();

	/*FIXME: This all works fine with lambdas (at least it does when they take
	 *no arguments), but with function pointers it only works when those are
	 *taken explicitly (passing the function as &func instead of func).
	 *Calling task_inner directly works fine (regardless of whether auto(x) is
	 *used on std::forward), but trying to call it through an std::thread just
	 *fails. Tested on GCC 14.2.0, 14.3.0 and 15.2.0 (the last two via Godbolt).
	 */

	/*task_inner(main_window, thread_it, std::forward<F>(f),
			   std::forward<Args>(args)...);*/

	/*NOTE: After some testing, I think auto(x) (for f and args) should be
	 *avoided here (or, at the very least, it's unnecessary). task_inner already
	 *has it, so it should take care of that, but I might be mistaken.*/
	thread_it->mtx.lock();
	thread_it->thread = std::thread(task_inner<F, Args...>,
									std::ref(main_window),
									thread_it, std::forward<F>(f),
									std::forward<Args>(args)...);
	thread_it->mtx.unlock();
}

typedef u16 (*copy_or_rename_f)(std::filesystem::path, std::filesystem::path);

void main_window_t::paste_op()
{
	std::list<std::filesystem::path>::iterator to_copy_it;
	copy_or_rename_f copy_or_rename = copy_mode ? min_vfs::rename
		: min_vfs::copy;

	to_copy_it = ++to_copy.begin();
	for(size_t i = 1; i < to_copy.size(); i++)
	{
		/*These notes/todos all apply to an older version where we were just
		passing copy_or_rename and its arguments to start_task (instead of a
		lambda). I'd still like to look into this at some point.*/

		/*NOTE: For whatever fucking reason, I have to cast copy_or_rename to
		 *its own type for std::thread's constructor within start_task to
		 *actually take it. What the fuck.*/

		/*TODO: Despite the fact that feeding copy_or_rename raw to an
		 *std::thread would work just fine, it doesn't work when going through
		 *start_task. I should try and figure out why start_task causes so many
		 *problems with function pointers, like the fact that it needs me to
		 *explicitly take the function's address.*/

		/*NOTE: g++ was unhappy about getting workpath raw as an argument. I
		 *guess the problem was it didn't know whether to take it as an lvalue
		 *reference or as a copy. Both std::ref and the copy constructor would
		 *compile, but we want a copy here.*/

		/*TODO: Check how std::thread behaves with lvalues. I know I need to use
		 *std::ref if I want it to take a reference, but would passing it an
		 *lvalue work if the function expects a value?*/

		start_task(*this, false,
		[this, src_dir = *to_copy.begin(), tgt_dir = this->workpath,
				   copy_or_rename, fname = *to_copy_it]()
		{
			const std::filesystem::path src_path = CONCAT_PATHS(src_dir, fname);
			const u16 err = copy_or_rename(src_path, tgt_dir);

			if(err) error_msg(tr("Paste"), src_path, tgt_dir, err);
			else
			{
				if(this->workpath == tgt_dir) this->refresh.trigger();
				else
				{
					/*TODO: Notify on rename/copy/move success if not in dst.*/
				}
			}
		});

		to_copy_it++;
	}

	to_copy.clear();
}

void main_window_t::remove_op()
{
	//TODO: Ask for confirmation before deletion

	for(const QModelIndex index:
		cur_dir_list->selectionModel()->selectedIndexes())
	{
		start_task(*this, false,
		[this, index, tgt_dir = this->workpath,
			fname = min_vfs_model.get_dir()[index.row()].fname]()
		{
			const std::filesystem::path tgt_path = CONCAT_PATHS(workpath, fname);
			const u16 err = min_vfs::remove(tgt_path);

			if(err) error_msg(tr("Remove"), tgt_path, err);
			else
			{
				if(this->workpath == tgt_dir) this->refresh.trigger();
				else
				{
					/*TODO: Notify on success if not in dst.*/
				}
			}
		});
	}
}

QString main_window_t::get_err_msg(const u16 err)
{
	switch(err)
	{
		case ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::WRONG_FS):
			return tr("Unknown filesystem.");

		case ret_val_setup(min_vfs::LIBRARY_ID,
			(u8)min_vfs::ERR::UNSUPPORTED_OPERATION):
			return tr("Unsupported operation.");

		case ret_val_setup(min_vfs::LIBRARY_ID,
			(u8)min_vfs::ERR::DISK_TOO_SMALL):
			return tr("Disk too small.");

		case ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::IO_ERROR):
			return tr("IO error.");

		case ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NO_PERM):
			return tr("No permission.");

		case ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_FOUND):
			return tr("Not found.");

		case ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::NOT_A_DIR):
			return tr("Not a directory.");

		case ret_val_setup(min_vfs::LIBRARY_ID, (u8)min_vfs::ERR::INVALID_PATH):
			return tr("Invalid path.");

		default:
			return tr("min_vfs error: 0x") + QString::number(err, 16).toUpper();
	}
}

void main_window_t::error_msg(const QString &op_name,
							  const std::filesystem::path &path, const u16 err)
{
	/*Can't create children from a different thread, gotta do it with
	 *signals. Wait, could I get rid of the proxy by making the signal part
	 *of main_window_t?*/
	task_msg_proxy_t msg_proxy;

	//Apparently connect() and disconnect() are thread-safe, so fuck it.
	connect(&msg_proxy, &task_msg_proxy_t::req_msg_box, this,
			&main_window_t::spawn_msg_box);

	emit msg_proxy.req_msg_box(QMessageBox::Warning,
							   op_name + QString(' ') + tr("failed"),
							   get_err_msg(err)
							   + QString::fromStdString("\n\n") + tr("Path: ")
							   + QString::fromStdString(
								   path.string()));

	disconnect(&msg_proxy, &task_msg_proxy_t::req_msg_box, this,
			   &main_window_t::spawn_msg_box);
	return;
}

void main_window_t::error_msg(const QString &op_name,
							  const std::filesystem::path &src_path,
							  const std::filesystem::path &dst_path,
							  const u16 err)
{
	/*Can't create children from a different thread, gotta do it with
	 *signals. Wait, could I get rid of the proxy by making the signal part
	 *of main_window_t?*/
	task_msg_proxy_t msg_proxy;

	//Apparently connect() and disconnect() are thread-safe, so fuck it.
	connect(&msg_proxy, &task_msg_proxy_t::req_msg_box, this,
			&main_window_t::spawn_msg_box);

	emit msg_proxy.req_msg_box(QMessageBox::Warning,
							   op_name + QString(' ') + tr("failed"),
							   get_err_msg(err)
							   + QString::fromStdString("\n\n") + tr("Source: ")
							   + QString::fromStdString(src_path.string())
							   + QString::fromStdString("\n\n")
							   + tr("Destination: ")
							   + QString::fromStdString(dst_path.string()));

	disconnect(&msg_proxy, &task_msg_proxy_t::req_msg_box, this,
			   &main_window_t::spawn_msg_box);
	return;
}

void main_window_t::try_mount(const std::filesystem::path &path)
{
	const u16 err = min_vfs::mount(path);

	if(err)
	{
		error_msg(tr("Mount"), path, err);
		return;
	}

	fs_list_model.refresh();
}

typedef uint16_t (*mkfs_f)(const std::filesystem::path &fs_path,
					  const std::string &label);

constexpr mkfs_f mkfs_funcs[] =
{
	EMU::FS::mkfs,
	S5XX::FS::mkfs,
	S7XX::FS::mkfs
};

void main_window_t::mkfs_op()
{
	for(const QModelIndex index:
		cur_dir_list->selectionModel()->selectedIndexes())
	{
		mkfs_dialog_t mkfs_dialog(this);
		mkfs_dialog.set_types({"EMU", "S5XX", "S7XX"});

		const bool ok = mkfs_dialog.exec();

		if(ok)
		{
			const mkfs_params_t mkfs_params = mkfs_dialog.params();

			start_task(*this, true,
			[this, mkfs_params, path = CONCAT_PATHS(workpath,
				min_vfs_model.get_dir()[index.row()].fname)]()
			{
				u16 err;

				if(mkfs_params.type >= 0 && mkfs_params.type <= 2)
					err = mkfs_funcs[mkfs_params.type](path, mkfs_params.label);
				else err = 0;

				if(err) error_msg(tr("MKFS"), path, err);
			});
		}
	}
}

void main_window_t::fsck_op()
{
	for(const QModelIndex index:
		cur_dir_list->selectionModel()->selectedIndexes())
	{
		start_task(*this, true,
		[this, path = CONCAT_PATHS(workpath,
			min_vfs_model.get_dir()[index.row()].fname)]()
		{
			const u16 err = min_vfs::fsck(path);

			if(err)
			{
				error_msg(tr("FSCK"), path, err);
				return;
			}
			else
			{
				task_msg_proxy_t msg_proxy;

				//Apparently connect() and disconnect() are thread-safe, so fuck it.
				connect(&msg_proxy, &task_msg_proxy_t::req_msg_box, this,
						&main_window_t::spawn_msg_box);

				emit msg_proxy.req_msg_box(QMessageBox::Information,
										   tr("FSCK"),
										   tr("Filesystem") + QString(' ')
										   + QString::fromStdString(
											   path.string()) + QString(' ')
										   + tr("OK"));

				disconnect(&msg_proxy, &task_msg_proxy_t::req_msg_box, this,
						   &main_window_t::spawn_msg_box);
			}
		});
	}
}

template<const bool modify>
bool main_window_t::filename_entry_dialog(const QString &text_init,
										  QString &res)
{
	filename_validator_t filename_validator; //Does the QLineEdit own this?
	QInputDialog input_dialog(this, Qt::Dialog);
	QLineEdit *dialog_line_edit;

	if constexpr(modify) input_dialog.setWindowTitle(tr("Rename"));
	else input_dialog.setWindowTitle(tr("Make directory"));
	input_dialog.setLabelText(tr("Filename:"));
	input_dialog.setInputMode(QInputDialog::TextInput);
	input_dialog.setTextEchoMode(QLineEdit::Normal);
	input_dialog.setTextValue(text_init);

	dialog_line_edit = input_dialog.findChild<QLineEdit*>();

	if(dialog_line_edit != nullptr)
		dialog_line_edit->setValidator(&filename_validator);

	const bool ok = input_dialog.exec();
	res = input_dialog.textValue();

	if(!ok || res.isEmpty() || res.contains("/")) return false;

	return true;
}

void main_window_t::setup_cur_dir_list_ctx_menu()
{
	refresh.setText(tr("Refresh"));
	connect(&refresh, &QAction::triggered, this,
	[this]()
	{
		cd(workpath);
	});

	open_dir.setText(tr("Open"));
	connect(&open_dir, &QAction::triggered, this,
	[this]()
	{
		if(cur_dir_list->selectionModel()->selection().size() == 1)
		{
			const min_vfs::dentry_t dentry = min_vfs_model.get_dir()[
				cur_dir_list->selectionModel()->selectedIndexes()[0].row()];

			if(dentry.ftype == min_vfs::ftype_t::dir)
				cd(CONCAT_PATHS(workpath, dentry.fname));
		}
	});

	mkdir.setText(tr("Make directory"));
	connect(&mkdir, &QAction::triggered, this,
	[this]()
	{
		QString res;

		if(!filename_entry_dialog<false>(QString(), res)) return;

		start_task(*this, false,
		[this, dir_path = CONCAT_PATHS(this->workpath, res.toStdString())]()
		{
			const u16 err = min_vfs::mkdir(dir_path);
			if(err)
			{
				error_msg(tr("Make directory"), dir_path, err);
				return;
			}

			this->refresh.trigger();
		});
	});

	rename.setText(tr("Rename"));
	connect(&rename, &QAction::triggered, this,
	[this]()
	{
		const QModelIndexList selected_indices =
			cur_dir_list->selectionModel()->selectedIndexes();

		QString res;

		if(selected_indices.isEmpty()) return; //~How did I get here?

		const std::string cur_fname =
			min_vfs_model.get_dir()[selected_indices[0].row()].fname;

		if(!filename_entry_dialog<true>(QString::fromStdString(cur_fname), res))
			return;

		start_task(*this, false,
		[this, cur_path = CONCAT_PATHS(this->workpath, cur_fname),
			dst_path = CONCAT_PATHS(this->workpath, res.toStdString())](void)
		{
			const u16 err = min_vfs::rename(cur_path, dst_path);
			if(err) error_msg(tr("Rename"), cur_path, dst_path, err);
			else this->refresh.trigger();
		});
	});

	copy.setText(tr("Copy"));
	connect(&copy, &QAction::triggered, this,
	[this]()
	{
		copy_setup(false);
	});

	cut.setText(tr("Cut"));
	connect(&cut, &QAction::triggered, this,
	[this]()
	{
		copy_setup(true);
	});

	paste.setText(tr("Paste"));
	connect(&paste, &QAction::triggered, this,
	[this]()
	{
		paste_op();
	});

	remove.setText(tr("Remove"));
	connect(&remove, &QAction::triggered, this,
	[this]()
	{
		remove_op();
	});

	mkfs.setText(tr("MKFS"));
	connect(&mkfs, &QAction::triggered, this,
	[this]()
	{
		mkfs_op();
	});

	fsck.setText(tr("FSCK"));
	connect(&fsck, &QAction::triggered, this,
	[this]()
	{
		fsck_op();
	});

	mount.setText(tr("Try mount"));
	connect(&mount, &QAction::triggered, this,
	[this]()
	{
		for(const QModelIndex index:
			cur_dir_list->selectionModel()->selectedIndexes())
		{
			start_task(*this, true,
			[this, path = CONCAT_PATHS(workpath,
				min_vfs_model.get_dir()[index.row()].fname)]()
			{
				try_mount(path);
			});
		}
	});
}

void main_window_t::cur_dir_list_setup()
{
	cur_dir_list->setModel((QAbstractItemModel*)&min_vfs_model);
	min_vfs_selection_model.setModel(&min_vfs_model);
	cur_dir_list->setSelectionModel(&min_vfs_selection_model);
	cur_dir_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	cur_dir_list->setSelectionBehavior(QAbstractItemView::SelectItems);
	cur_dir_list->header()->setDefaultSectionSize(200);

	cur_dir_list->setItemsExpandable(false);
	cur_dir_list->setExpandsOnDoubleClick(false);
	cur_dir_list->setRootIsDecorated(false);

	connect(cur_dir_list, &QTreeView::doubleClicked, this,
	[this](const QModelIndex &index)
	{
		const min_vfs::dentry_t &dentry = min_vfs_model.get_dir()[index.row()];

		if(dentry.ftype == min_vfs::ftype_t::dir)
		{
			cd(CONCAT_PATHS(workpath, dentry.fname));
		}
	});

	setup_cur_dir_list_ctx_menu();
	cur_dir_list->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(cur_dir_list, &QTreeView::customContextMenuRequested, this,
	[this](const QPoint &point)
	{
		bool selected_files, selected_dirs;
		QMenu context_menu(cur_dir_list);

		selected_files = false;
		selected_dirs = false;

		for(const QModelIndex index:
			cur_dir_list->selectionModel()->selectedIndexes())
		{
			const min_vfs::dentry_t dentry = min_vfs_model.get_dir()[index.row()];

			switch(dentry.ftype)
			{
				using enum min_vfs::ftype_t;

				case file:
					selected_files = true;
					break;

				case dir:
					selected_dirs = true;
					break;

				default:
					break;
			}
		}

		context_menu.addAction(&refresh);

		if(cur_dir_list->selectionModel()->selectedIndexes().empty())
			context_menu.addAction(&mkdir);

		if(cur_dir_list->selectionModel()->selection().size() == 1
			&& selected_dirs)
		{
			context_menu.addSeparator();
			context_menu.addAction(&open_dir);
		}

		if(selected_files || selected_dirs)
		{
			context_menu.addSeparator();
			if(cur_dir_list->selectionModel()->selectedIndexes().size() == 1)
				context_menu.addAction(&rename);
			context_menu.addAction(&copy);
			context_menu.addAction(&cut);

			context_menu.addAction(&remove);
		}

		if(to_copy.size()) context_menu.addAction(&paste);
		if(selected_files || selected_dirs) context_menu.addAction(&remove);

		if(selected_files && !selected_dirs)
		{
			context_menu.addSeparator();
			context_menu.addAction(&mkfs);
			context_menu.addAction(&fsck);
			context_menu.addAction(&mount);
		}

		context_menu.exec(cur_dir_list->viewport()->mapToGlobal(point));
	});

	cur_dir_refresh_shortcut = new QShortcut(QKeySequence(tr("F5", "Refresh")),
											 cur_dir_list, nullptr, nullptr,
										  Qt::WidgetWithChildrenShortcut);
	connect(cur_dir_refresh_shortcut, &QShortcut::activated, &refresh,
			&QAction::trigger);

	//"Enter" maps to the numpad's enter key. The main one is "Return".
	open_shortcut = new QShortcut(QKeySequence(tr("Return", "Open")),
								  cur_dir_list, nullptr, nullptr,
							   Qt::WidgetWithChildrenShortcut);
	connect(open_shortcut, &QShortcut::activated, &open_dir,
			&QAction::trigger);
}

void main_window_t::umount(const std::filesystem::path &path)
{
	u16 err;

	err = min_vfs::umount(path);
	if(err) return; //TODO: Popup?

	fs_list_model.refresh();
}

void main_window_t::fs_list_ctx_menu_setup()
{
	refresh_fs_list.setText(tr("Refresh"));
	connect(&refresh_fs_list, &QAction::triggered, this,
	[this]()
	{
		fs_list_model.refresh();
	});

	unmount.setText(tr("Unmount"));
	connect(&unmount, &QAction::triggered, this,
	[this]()
	{
		for(const QModelIndex index:
			fs_list->selectionModel()->selectedIndexes())
		{
			start_task(*this, true,
			[this, index]()
			{
				umount(fs_list_model.get_fs_stats()[index.row()].path);
			});
		}
	});
}

void main_window_t::fs_list_setup()
{
	connect(fs_list, &QListView::doubleClicked, this,
	[this](const QModelIndex &index)
	{
		if(index.isValid())
		{
			#if _WIN32
				cd(replace_win_path_separators(
					fs_list_model.get_fs_stats()[index.row()].path));
			#else
				cd(fs_list_model.get_fs_stats()[index.row()].path);
			#endif
		}
	});

	fs_list_ctx_menu_setup();
	fs_list->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(fs_list, &QTreeView::customContextMenuRequested, this,
	[this](const QPoint &point)
	{
		QMenu context_menu(fs_list);

		context_menu.addAction(&refresh_fs_list);

		if(fs_list->selectionModel()->hasSelection())
			context_menu.addAction(&unmount);

		context_menu.exec(fs_list->viewport()->mapToGlobal(point));
	});
}

main_window_t::main_window_t(): QMainWindow(),
	dir_history
	(
		#if _WIN32
			replace_win_path_separators(std::filesystem::current_path())
		#else
			std::filesystem::current_path()
		#endif
	)
{
	top_widget = new QWidget();
	top_vbox = new QVBoxLayout(top_widget);

	toolbar_setup();

	h_splitter = new QSplitter(top_widget);

	left_widget = new QWidget(h_splitter);
	left_vbox = new QVBoxLayout(left_widget);
	cur_dir_list = new QTreeView(h_splitter);
	cur_dir_list_setup();

	/*To get the QListWidget to properly shrink to match its contents, you have
	 *to set its QAbstractScrollArea's SizeAdjustPolicy to AdjustToContents. To
	 *remove the extra space at the bottom, you have to set the ScrollMode to
	 *ScrollPerPixel (it defaults to ScrollPerItem).*/
	dir_list = new QListWidget(left_widget);
	dir_list->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
	dir_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	dir_list->setSizePolicy(dir_list->sizePolicy().horizontalPolicy(),
							QSizePolicy::Maximum);
	init_dir_shortcuts();

	fs_list = new QListView(left_widget);
	fs_list->setSizePolicy(fs_list->sizePolicy().horizontalPolicy(),
						   QSizePolicy::Expanding);
	fs_list->setModel(&fs_list_model);
	fs_list_setup();
	top_vbox->addWidget(h_splitter);

	h_splitter->setOrientation(Qt::Horizontal);
	h_splitter->addWidget(left_widget);
	h_splitter->addWidget(cur_dir_list);
	h_splitter->setChildrenCollapsible(false);
	h_splitter->setStretchFactor(0, 3);
	h_splitter->setStretchFactor(1, 7);

	left_vbox->addWidget(dir_list);
	left_vbox->addWidget(fs_list);

	#if _WIN32
		cd(replace_win_path_separators(std::filesystem::current_path()), false);
	#else
		cd(std::filesystem::current_path(), false);
	#endif

	setCentralWidget(top_widget);
	show();
}
