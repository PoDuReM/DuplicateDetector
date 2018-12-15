#include <searcher.h>

const qint64 READK = 30;

bool Searcher::check_stop() {
    return QThread::currentThread()->isInterruptionRequested();
}

void Searcher::get_duplicates(QString const &dir) {
    QMap<qint64, QVector<QString>> size2Files;

    QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
    qint64 view_size = 0, all_size = 0;
    qint8 percent = 1;
    auto changed_progress = [&view_size, &all_size, &percent]() {
        qint8 new_percent = all_size == 0 ? 100 : static_cast<qint8>(view_size * 100 / all_size);
        if (new_percent > percent) {
            percent = new_percent;
            return true;
        }
        return false;
    };
    QVector<QString> cantOpen;
    while (it.hasNext()) {
        QString path = it.next();
        qint64 sz = QFileInfo(path).size();
        size2Files[sz].push_back(path);
        all_size += sz;
        if (check_stop()) {
            stop_search(cantOpen);
            return;
        }
    }
    for (auto itSize = size2Files.begin(); itSize != size2Files.end(); ++itSize) {
        if (itSize.value().size() == 1) {
            view_size += itSize.key();
            if (changed_progress()) {
                emit progress(percent);
            }
            continue;
        }
        if (check_stop()) {
            stop_search(cantOpen);
            return;
        }
        QMap<QString, QVector<QString>> first2Files;
        for (auto const &file : itSize.value()) {
            try {
                first2Files[read_first_k(file, std::min(itSize.key(), READK))].push_back(file);
            } catch (QString &) {
                //emit send_message("Can't open " + file.mid(dir.size(), file.size() - dir.size()));
                cantOpen.append(file.mid(dir.size(), file.size() - dir.size()));
                view_size += itSize.key();
                if (changed_progress()) {
                    emit progress(percent);
                }
            }
        }
        for (auto itFirst = first2Files.begin(); itFirst != first2Files.end(); ++itFirst) {
            if (itFirst.value().size() == 1) {
                view_size += itSize.key();
                if (changed_progress()) {
                    emit progress(percent);
                }
                continue;
            }
            if (check_stop()) {
                stop_search(cantOpen);
                return;
            }
            QMap<QByteArray, QVector<QString>> hash2Files;
            for (auto const &file : itFirst.value()) {
                try {
                    hash2Files[get_hash(file)].push_back(file);
                } catch (QString &) {
                    //emit send_message("Can't open " + file.mid(dir.size(), file.size() - dir.size()));
                    cantOpen.append(file.mid(dir.size(), file.size() - dir.size()));
                }
                view_size += itSize.key();
                if (changed_progress()) {
                    emit progress(percent);
                }
            }
            for (auto itHash = hash2Files.begin(); itHash != hash2Files.end(); ++itHash) {
                if (itHash.value().size() == 1) {
                    continue;
                }
                if (check_stop()) {
                    stop_search(cantOpen);
                    return;
                }
                emit send_duplicates(itHash.value());
            }
        }
    }
    stop_search(cantOpen);
}

void Searcher::stop_search(QVector<QString> const &cantOpen) {
    if (cantOpen.size() > 0) {
        QString message = "Can't open files :\n";
        for (auto const &filepath : cantOpen) {
            message += filepath + "\n";
        }
        emit send_message(message);
    }
    emit finish();
}

QString Searcher::read_first_k(QString const &filepath, qint64 k) {
    QFile file(filepath);
    if (file.open(QIODevice::ReadOnly)) {
        return file.read(k);
    }
    throw QString("File can't open");
}

QByteArray Searcher::get_hash(const QString &filepath) {
    QFile file(filepath);
    if (file.open(QIODevice::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(file.readAll());
        return hash.result();
    }
    throw QString("File can't open");
}
