#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <searcher.h>
#include <ui_main_window.h>

namespace Ui {
    class MainWindow;
}

class main_window : public QMainWindow {
    Q_OBJECT
    QThread searchThread;

public:
    explicit main_window(QWidget* parent = nullptr);
    ~main_window();

public slots:
    void print_duplicates(QVector<QString> const &duplicates);
    void select_directory();
    void show_about_dialog();
    void delete_items();
    void open_file(QTreeWidgetItem*, int);
    void finish_searching();
    void stop_searching();
    void set_progress(qint8 const &percent);
signals:
    void find_duplicates(QString const &dir);

private:
    std::unique_ptr<Ui::MainWindow> ui;
    Searcher* searcher = nullptr;
    QString dir;
};

#endif // MAINWINDOW_H
