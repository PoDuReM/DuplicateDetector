#include <main_window.h>

main_window::main_window(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    ui->progressBar->reset();
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QCommonStyle style;
    setWindowTitle(QString("Duplicate Searching"));
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));

    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);
    connect(ui->actionStop, SIGNAL(released()), SLOT(stop_searching()));
    connect(ui->actionDelete, SIGNAL(released()), SLOT(delete_items()));
    ui->actionDelete->setHidden(true);
    ui->actionStop->setHidden(true);
    ui->progressBar->setHidden(true);
}

main_window::~main_window() {
    searchThread.exit();
    searchThread.wait();
}

void main_window::select_directory() {
    dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                            QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks) + "/";

    if (dir.size() == 0) {
        return;
    }
    ui->treeWidget->clear();
    setWindowTitle(QString("Duplicates in directory - %1").arg(dir));
    searcher = new Searcher();
    searcher->moveToThread(&searchThread);
    connect(&searchThread, &QThread::finished, searcher, &QObject::deleteLater);

    qRegisterMetaType<QVector<QString>>("QVector<QString>");

    connect(searcher, &Searcher::progress, this, &main_window::set_progress);         
    connect(searcher,
           SIGNAL(send_duplicates(QVector<QString> const &)),
           this,
           SLOT(print_duplicates(QVector<QString> const &)));
    connect(searcher, &Searcher::finish, this, &main_window::finish_searching);
    connect(this, &main_window::find_duplicates, searcher, &Searcher::get_duplicates);
    connect(ui->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(open_file(QTreeWidgetItem*, int)));

    ui->progressBar->setHidden(false);
    ui->progressBar->reset();
    ui->progressBar->setValue(1);
    ui->actionScan_Directory->setDisabled(true);
    ui->actionStop->setHidden(false);
    ui->actionDelete->setHidden(true);
    searchThread.start();
    find_duplicates(dir);
}

void main_window::print_duplicates(QVector<QString> const &duplicates) {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, QString("Duplicate " + QString::number(duplicates.size())) + " files");
    QFileInfo file_info(duplicates[0]);
    item->setText(1, QString::number(file_info.size()));

    for (QString const &filepath : duplicates) {
        QTreeWidgetItem* child_item = new QTreeWidgetItem();
        child_item->setText(0, filepath.mid(dir.size(), filepath.size() - dir.size()));
        item->addChild(child_item);
    }
    ui->treeWidget->addTopLevelItem(item);
}

void main_window::set_progress(qint8 const &percent) {
    ui->progressBar->setValue(percent);
}

void main_window::delete_items() {
    auto answer = QMessageBox::question(this, "Deleting", "Do you want to delete selected files?");
    if (answer == QMessageBox::No) {
        return;
    }
    auto sel_items = ui->treeWidget->selectedItems();
//    for (auto const &item : sel_items) {
//        if (!QFile(item->text(0)).exists()) {
//            for (auto const &child : item->takeChildren()) {
//                child->setSelected(true);
//            }
//        }
//    }
//    sel_items = ui->treeWidget->selectedItems();
    for (auto const &item : sel_items) {
        QFile file(dir + item->text(0));
        if (file.remove()) {
            if (item->parent()->childCount() < 2) {
                delete item->parent();
            } else {
                item->parent()->setText(0, QString("Duplicate " + QString::number(item->parent()->childCount() - 1) + " files"));
                item->parent()->removeChild(item);
            }
        }
    }
}

void main_window::open_file(QTreeWidgetItem *item, int) {
    QFile file(item->text(0));
    if (file.exists()) {
        QDesktopServices::openUrl(item->text(0));
    }
}

void main_window::finish_searching() {
    ui->actionDelete->setHidden(false);
    ui->actionStop->setHidden(true);
    ui->actionScan_Directory->setDisabled(false);
    searchThread.quit();
    searchThread.wait();
}

void main_window::stop_searching() {
    searchThread.requestInterruption();
}

void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}
