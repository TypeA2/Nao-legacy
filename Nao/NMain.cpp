#include "NMain.h"

#include <QProgressDialog>

NMain::NMain()
    : QMainWindow(),
    savePath(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0)) {
    setAcceptDrops(true);

    setup_window();
}

void NMain::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void NMain::dropEvent(QDropEvent *e) {
   e->accept();
   loadFile(e->mimeData()->urls().at(0).path().mid(1));
}

void NMain::openFile() {
    QString file = QFileDialog::getOpenFileName(
                this,
                "Select a file",
                LibNao::Steam::getGamePath("NieRAutomata",
                                           QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0)
                                           ));

    if (!file.isEmpty())
        loadFile(file);
}

void NMain::loadFile(QString file) {
    switch (currentType) {
        case LibNao::CRIWare:
            delete CRIWareReader;
            break;
    }

    if (currentType != LibNao::None) {
        table->clear();

        extract_button->setDisabled(true);
        extract_all_button->setDisabled(true);

        disconnect(table, &QTableWidget::customContextMenuRequested, this, 0);
        disconnect(extract_button, &QPushButton::clicked, this, 0);
        disconnect(extract_all_button, &QPushButton::clicked, this, 0);
    }

    if (!LibNao::Utils::isFileSupported(file)) {
        QMessageBox::warning(
                    this,
                    "File not supported",
                    "The following file is not supported:\n\n" + file,
                    QMessageBox::Ok,
                    QMessageBox::Ok);
    } else {
        switch (currentType = LibNao::Utils::getFileType(file)) {
            case LibNao::CRIWare:
                CRIWareHandler(file);

                break;

            case LibNao::WWise:
                // WEM/WSP stuff
                break;

            case LibNao::MS_DDS:
                // DDS stuff
                break;

            case LibNao::PG_DAT:
                PG_DATHandler(file);
                break;

            case LibNao::None:
            default:
                QMessageBox::warning(
                            this,
                            "File not supported",
                            "The following file is not supported:\n\n" + file,
                            QMessageBox::Ok,
                            QMessageBox::Ok);
        }
    }
}

void NMain::PG_DATHandler(QString file) {
    PG_DATReader = new NaoDATReader(file);
    const QVector<NaoDATReader::EmbeddedFile>& files = PG_DATReader->getFiles();

    table->setRowCount(files.size());
    table->setColumnCount(4);
    extract_all_button->setDisabled(false);

    connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NMain::firstTableSelection);
    connect(table, &QTableWidget::customContextMenuRequested, this, &NMain::extractRightClickEvent);
    connect(extract_button, &QPushButton::clicked, this, &NMain::extractSingleFile);
    connect(extract_all_button, &QPushButton::clicked, this, &NMain::extractAll);

    table->setHorizontalHeaderItem(0, new QTableWidgetItem("#"));
    table->setHorizontalHeaderItem(1, new QTableWidgetItem("File name"));
    table->setHorizontalHeaderItem(2, new QTableWidgetItem("File size"));
    table->setHorizontalHeaderItem(3, new QTableWidgetItem("File offset"));

    for (qint64 i = 0; i < files.size(); ++i) {
        NaoDATReader::EmbeddedFile file = files.at(i);

        QTableWidgetItem* primary = new QTableWidgetItem(QString::number(i));
        primary->setData(FileNameRole, file.name);
        primary->setData(FileSizeEmbeddedRole, file.size);
        primary->setData(FileSizeExtractedRole, file.size);
        primary->setData(FileOffsetRole, file.offset);
        primary->setData(FileIndexRole, i);

        table->setItem(i, 0, primary);

        table->setItem(i, 1, new QTableWidgetItem(file.name));

        QTableWidgetItem* sizeItem = new QTableWidgetItem(LibNao::Utils::getShortSize(file.size));
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(i, 2, sizeItem);

        QTableWidgetItem* offsetItem = new QTableWidgetItem(QString::number(file.offset));
        offsetItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(i, 3, offsetItem);
    }
}

void NMain::CRIWareHandler(QString file) {
    CRIWareReader = new NaoCRIWareReader(file);
    const QVector<NaoCRIWareReader::EmbeddedFile>& files = CRIWareReader->getFiles();

    table->setRowCount(files.size());
    extract_all_button->setDisabled(false);

    connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NMain::firstTableSelection);
    connect(table, &QTableWidget::customContextMenuRequested, this, &NMain::extractRightClickEvent);
    connect(extract_button, &QPushButton::clicked, this, &NMain::extractSingleFile);
    connect(extract_all_button, &QPushButton::clicked, this, &NMain::extractAll);

    if (CRIWareReader->isPak()) {
        table->setColumnCount(5);

        table->setHorizontalHeaderItem(0, new QTableWidgetItem("#"));
        table->setHorizontalHeaderItem(1, new QTableWidgetItem("File name"));
        table->setHorizontalHeaderItem(2, new QTableWidgetItem("Embedded size"));
        table->setHorizontalHeaderItem(3, new QTableWidgetItem("Extracted size"));
        table->setHorizontalHeaderItem(4, new QTableWidgetItem("Compression"));

        for (qint64 i = 0; i < files.size(); i++) {
            NaoCRIWareReader::EmbeddedFile file = files.at(i);

            QTableWidgetItem* primary = new QTableWidgetItem(QString::number(i));
            primary->setData(FileNameRole, file.name);
            primary->setData(FilePathRole, file.path);
            primary->setData(FileSizeEmbeddedRole, file.size);
            primary->setData(FileSizeExtractedRole, file.extractedSize);
            primary->setData(FileOffsetRole, file.offset);
            primary->setData(FileExtraOffsetRole, file.extraOffset);
            primary->setData(FileIndexRole, i);

            table->setItem(i, 0, primary);

            table->setItem(i, 1, new QTableWidgetItem((file.path + (file.path.isEmpty() ? "" : "/")) + file.name));

            QTableWidgetItem* embeddedSizeItem = new QTableWidgetItem(LibNao::Utils::getShortSize(file.size));
            embeddedSizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(i, 2, embeddedSizeItem);

            QTableWidgetItem* extractedSizeItem = new QTableWidgetItem(LibNao::Utils::getShortSize(file.extractedSize));
            extractedSizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(i, 3, extractedSizeItem);

            QTableWidgetItem* compressionLevelItem = new QTableWidgetItem(
                        (file.size != 0 && file.extractedSize != 0) ?
                            QString::number((static_cast<qreal>(file.size) / static_cast<qreal>(file.extractedSize) * 100), 'f', 0) + "%" : "NaN");
            compressionLevelItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(i, 4, compressionLevelItem);
        }
    } else {
        table->setColumnCount(6);
        table->setRowCount(files.size());

        table->setHorizontalHeaderItem(0, new QTableWidgetItem("#"));
        table->setHorizontalHeaderItem(1, new QTableWidgetItem("Original file name"));
        table->setHorizontalHeaderItem(2, new QTableWidgetItem("File size"));
        table->setHorizontalHeaderItem(3, new QTableWidgetItem("Type"));
        table->setHorizontalHeaderItem(4, new QTableWidgetItem("Avg. bitrate"));
        table->setHorizontalHeaderItem(5, new QTableWidgetItem("Est. duration"));

        for (qint64 i = 0; i < files.size(); i++) {
            NaoCRIWareReader::EmbeddedFile file = files.at(i);

            QTableWidgetItem* primary = new QTableWidgetItem(QString::number(i));
            primary->setData(FileNameRole, file.name);
            primary->setData(FileSizeEmbeddedRole, file.size);
            primary->setData(FileSizeExtractedRole, file.size);
            primary->setData(FileDataTypeRole, file.type);
            primary->setData(FileIndexRole, i);

            table->setItem(i, 0, primary);
            table->setItem(i, 1, new QTableWidgetItem(file.name));
            QTableWidgetItem* fileSizeItem = new QTableWidgetItem(LibNao::Utils::getShortSize(file.size));
            fileSizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(i, 2, fileSizeItem);
            table->setItem(i, 3, new QTableWidgetItem(
                               (file.type == NaoCRIWareReader::EmbeddedFile::Video) ?
                                   "Video" : "Audio"));

            QTableWidgetItem* avbpsItem = new QTableWidgetItem(LibNao::Utils::getShortSize(file.avbps, true));
            avbpsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(i, 4, avbpsItem);

            QTableWidgetItem* estDurationItem = new QTableWidgetItem(LibNao::Utils::getShortTime(
                                                                         file.size / (file.avbps / 8.)));
            estDurationItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table->setItem(i, 5, estDurationItem);
        }
    }
}

void NMain::extractRightClickEvent(const QPoint& p) {
    table->selectRow(table->rowAt(p.y()));

    extractContextMenu->popup(table->viewport()->mapToGlobal(p));
}

void NMain::extractSingleFile() {
    QTableWidgetItem* file = table->item(table->selectionModel()->selectedRows().at(0).row(), 0);

    if (file->data(FileSizeEmbeddedRole).toULongLong() == 0ULL) {
        QMessageBox::warning(
                    this,
                    "Can't extract file",
                    "The following file could not be extracted because it does not have a size:\n\n" + file->data(FileNameRole).toString(),
                    QMessageBox::Ok,
                    QMessageBox::Ok);
    } else {
        QString outname = file->data(FileNameRole).toString();

        switch (currentType) {
            case LibNao::CRIWare: {
                if (!CRIWareReader->isPak()) {
                    outname = QFileInfo(outname).baseName() +
                            ((static_cast<NaoCRIWareReader::EmbeddedFile::Type>(file->data(FileDataTypeRole).toInt())
                              == NaoCRIWareReader::EmbeddedFile::Video) ? ".mpeg" : ".adx");
                }

                break;
            }
        }

        QString output = QFileDialog::getSaveFileName(
                    this,
                    "Select output file",
                    savePath + "/" + LibNao::Utils::sanitizeFileName(outname)
                    );

        if (!output.isEmpty()) {
            savePath = QFileInfo(output).absolutePath();

            QFile* outfile = new QFile(output);

            if (!outfile->open(QIODevice::WriteOnly)) {
                QMessageBox::critical(
                            this,
                            "File save error",
                            "Could not save the following file:\n\n" + file->data(FileNameRole).toString(),
                            QMessageBox::Ok,
                            QMessageBox::Ok);
            } else {
                QProgressDialog* dialog = new QProgressDialog(
                            "Extracting files...",
                            "",
                            0,
                            0,
                            this);
                dialog->setCancelButton(nullptr);
                dialog->setModal(true);
                dialog->setFixedWidth(this->width() / 2);
                dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowCloseButtonHint & ~Qt::WindowContextHelpButtonHint);
                dialog->show();

                switch (currentType) {
                    case LibNao::CRIWare:
                        connect(CRIWareReader, &NaoCRIWareReader::extractProgress, this, [=](const qint64 current, const qint64 max) {
                            if (dialog->maximum() != (max >> 10)) {
                                dialog->setMaximum(max >> 10);
                            }

                            dialog->setValue(current >> 10);
                        });
                        break;

                    case LibNao::PG_DAT:
                        connect(PG_DATReader, &NaoDATReader::setExtractMaximum, this, [dialog](const qint64 max) {
                            dialog->setMaximum(max);
                        });

                        connect(PG_DATReader, &NaoDATReader::extractProgress, this,  [dialog](const qint64 current) {
                            dialog->setValue(current);
                        });
                }


                QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>();

                connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
                    outfile->close();

                    if (watcher->result()) {
                        QMessageBox::information(
                                    this,
                                    "Done",
                                    "Extraction complete.",
                                    QMessageBox::Ok,
                                    QMessageBox::Ok);
                    } else {
                        QMessageBox::critical(
                                    this,
                                    "File save error",
                                    "Could not write the following file:\n\n" + file->data(FileNameRole).toString(),
                                    QMessageBox::Ok,
                                    QMessageBox::Ok);
                    }

                    switch (currentType) {
                        case LibNao::CRIWare:
                            disconnect(CRIWareReader, &NaoCRIWareReader::extractProgress, this, 0);
                            break;

                        case LibNao::PG_DAT:
                            disconnect(PG_DATReader, &NaoDATReader::setExtractMaximum, this, 0);
                            disconnect(PG_DATReader, &NaoDATReader::extractProgress, this, 0);
                            break;
                    }

                    watcher->deleteLater();
                    dialog->deleteLater();
                    outfile->deleteLater();
                });

                QFuture<bool> future;

                switch (currentType) {
                    case LibNao::CRIWare:
                        future = QtConcurrent::run(
                                    CRIWareReader,
                                    &NaoCRIWareReader::extractFileTo,
                                    file->data(FileIndexRole).toLongLong(),
                                    outfile
                                );

                        break;

                    case LibNao::PG_DAT:
                        future = QtConcurrent::run(
                                    PG_DATReader,
                                    &NaoDATReader::extractFileTo,
                                    file->data(FileIndexRole).toLongLong(),
                                    outfile
                                );
                }

                watcher->setFuture(future);
            }
        }
    }
}

void NMain::extractAll() {
    QString output = QFileDialog::getExistingDirectory(
                this,
                "Select output directory",
                savePath);

    if (!output.isEmpty()) {
        savePath = output;

        QString originFileName;

        switch (currentType) {
            case LibNao::CRIWare: {
                originFileName = CRIWareReader->getFileName();

                break;
            }

            case LibNao::PG_DAT: {
                originFileName = PG_DATReader->getFileName();

                break;
            }

        }

        QDir outdir(output + "/" + QFileInfo(originFileName).fileName());

        qint64 totalEmbeddedSize = 0;
        qint64 totalExtractedSize = 0;
        qint64 fileCount = 0;

        switch (currentType) {
            case LibNao::CRIWare: {
                QSet<QString> dirs;
                const QVector<NaoCRIWareReader::EmbeddedFile>& files = CRIWareReader->getFiles();
                fileCount = files.length();

                for (int i = 0; i < fileCount; i++) {
                    totalEmbeddedSize += files.at(i).size;
                    totalExtractedSize += files.at(i).extractedSize;

                    if (CRIWareReader->isPak()) {
                        if (!files.at(i).path.isEmpty())
                            dirs.insert(files.at(i).path);
                    }
                }

                if (CRIWareReader->isPak()) {
                    QSet<QString>::const_iterator it = dirs.constBegin();

                    while (it != dirs.constEnd()) {
                        if (!outdir.mkpath(*it))
                            qFatal((QString("Could not create directory ") + outdir.path() + "/" + *it).toLatin1().data());

                        ++it;
                    }
                } else {
                    outdir.mkdir(".");
                }

                break;
            }

            case LibNao::PG_DAT: {
                outdir.mkdir(".");

                const QVector<NaoDATReader::EmbeddedFile>& files = PG_DATReader->getFiles();
                fileCount = files.size();

                for (qint64 i = 0; i < fileCount; ++i) {
                    totalExtractedSize += files.at(i).size;
                }

                break;
            }
        }


        QProgressDialog* dialog = new QProgressDialog(
                    "Extracting files...",
                    "",
                    0,
                    (totalExtractedSize > 0x8FFFFFFFULL) ?
                        (totalExtractedSize >> 10) : totalExtractedSize,
                    this);

        dialog->setCancelButton(nullptr);
        dialog->setModal(true);
        dialog->setFixedWidth(this->width() / 2);
        dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowCloseButtonHint & ~Qt::WindowContextHelpButtonHint);
        dialog->show();

        connect(this, &NMain::extractAllDialogProgress, this, [=](qint64 v) {
            dialog->setValue(dialog->value() + ((totalExtractedSize > 0x8FFFFFFFULL) ? (v >> 10) : v));
        });

        QFutureWatcher<void>* watcher = new QFutureWatcher<void>();

        connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
            QMessageBox::information(
                        this,
                        "Done",
                        QString("Extraction complete.\n\n") +
                                "Files:\t" + QString::number(fileCount) + "\n"
                                "Read:\t" + LibNao::Utils::getShortSize(totalEmbeddedSize) + "\n"
                                "Wrote:\t" + LibNao::Utils::getShortSize(totalExtractedSize),
                        QMessageBox::Ok,
                        QMessageBox::Ok);

            disconnect(this, &NMain::extractAllDialogProgress, this, 0);

            watcher->deleteLater();
            dialog->deleteLater();
        });

        QFuture<void> future = QtConcurrent::run([=]() {
            for (int i = 0; i < fileCount; ++i) {
                switch (currentType) {
                    case LibNao::CRIWare: {
                        NaoCRIWareReader::EmbeddedFile file = CRIWareReader->getFiles().at(i);

                        QString target;
                        if (CRIWareReader->isPak()) {
                            target = outdir.absolutePath() + "/" + file.path + "/" +
                                    LibNao::Utils::sanitizeFileName(file.name);
                        } else {
                            target = outdir.absolutePath() +
                                    "/" + LibNao::Utils::sanitizeFileName(QFileInfo(file.name).baseName()) +
                                    ((file.type == NaoCRIWareReader::EmbeddedFile::Video) ? ".mpeg" : ".adx");
                        }

                        QFile outfile(target);

                        if (!outfile.open(QIODevice::WriteOnly))
                            qFatal(QString("Could not open file %0 for writing").arg(target).toLatin1().data());

                        outfile.write(CRIWareReader->extractFileAt(i));
                        outfile.close();

                        emit extractAllDialogProgress(file.extractedSize);

                        break;
                    }

                    case LibNao::PG_DAT: {
                        NaoDATReader::EmbeddedFile file = PG_DATReader->getFiles().at(i);

                        QFile* outfile = new QFile(outdir.absolutePath() + "/" + file.name);

                        if (!outfile->open(QIODevice::WriteOnly)) {
                            qFatal(QString("Could not open file %0 for writing").arg(outfile->fileName()).toLatin1().data());
                        }

                        PG_DATReader->extractFileTo(i, outfile);

                        outfile->close();

                        delete outfile;

                        emit extractAllDialogProgress(file.size);

                        break;
                    }
                }

            }
        });

        watcher->setFuture(future);
    }
}

void NMain::setup_window() {
    setup_menus();

    this->setWindowTitle("Nao");
    this->setMinimumSize(640, 360);
    this->resize(1000, 550);

    QWidget* widget = new QWidget(this);
    QGridLayout* layout = new QGridLayout(widget);
    QHBoxLayout* buttons_layout = new QHBoxLayout();
    extract_button = new QPushButton("Extract", widget);
    extract_all_button = new QPushButton("Extract all", widget);
    table = new QTableWidget(widget);

    extract_button->setDisabled(true);
    extract_all_button->setDisabled(true);

    extract_button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

    table->verticalHeader()->hide();
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setContextMenuPolicy(Qt::CustomContextMenu);

    buttons_layout->addWidget(extract_button, 0, Qt::AlignLeft);
    buttons_layout->addWidget(extract_all_button, 1, Qt::AlignLeft);

    layout->setContentsMargins(4, 0, 0, 4);
    buttons_layout->setContentsMargins(4, 8, 4, 4);

    layout->addLayout(buttons_layout, 0, 0);
    layout->addWidget(table, 1, 0);

    widget->setLayout(layout);
    this->setCentralWidget(widget);
}

void NMain::setup_menus() {
    QMenuBar* menu = new QMenuBar(this);
    QMenu* file_menu = new QMenu("File", menu);
    QMenu* edit_menu = new QMenu("Edit", menu);
    QMenu* about_menu = new QMenu("About", menu);
    QAction* open_file_action = new QAction("Open file");
    QAction* exit_app_action = new QAction("Exit");
    QAction* options_action = new QAction("Options");
    QAction* about_nao_action = new QAction("About Nao");
    QAction* about_qt_action = new QAction("About Qt");

    connect(open_file_action, &QAction::triggered, this, &NMain::openFile);
    connect(exit_app_action, &QAction::triggered, this, &QMainWindow::close);
    connect(options_action, &QAction::triggered, this, &NMain::openOptions);
    connect(about_nao_action, &QAction::triggered, this, &NMain::about);
    connect(about_qt_action, &QAction::triggered, this, &NMain::aboutQt);

    open_file_action->setShortcuts(QKeySequence::Open);
    exit_app_action->setShortcuts(QKeySequence::Quit);

    file_menu->addAction(open_file_action);
    file_menu->addSeparator();
    file_menu->addAction(exit_app_action);
    edit_menu->addAction(options_action);
    about_menu->addAction(about_nao_action);
    about_menu->addAction(about_qt_action);

    menu->addMenu(file_menu);
    menu->addMenu(edit_menu);
    menu->addMenu(about_menu);

    this->setMenuBar(menu);

    extractContextMenu = new QMenu(this);
    QAction* extractAction = new QAction("Extract");
    QAction* extractAllAction = new QAction("Extract all");

    extractContextMenu->addAction(extractAction);
    extractContextMenu->addSeparator();
    extractContextMenu->addAction(extractAllAction);

    connect(extractAction, &QAction::triggered, this, &NMain::extractSingleFile);
    connect(extractAllAction, &QAction::triggered, this, &NMain::extractAll);
}

void NMain::about() {
    QMessageBox* about = new QMessageBox(this);
    about->setWindowTitle("About Nao");
    about->setText(
                "<b>Nao v1.0</b>"
                "<br><br>"
                "Nao is a weird program. It talks about itself in 3rd person, who made it do that?<br>"
                "Be gentle with it, and be sure to supply it with enough pocky.<br><br>"
                "Bug reports <a href=\"https://github.com/TypeA2/Nao\">here</a><br><br><br>"
                "<i>Also the best waifu</i>"
                );
    about->setStandardButtons(QMessageBox::Ok);

    about->exec();
}

void NMain::aboutQt() {
    QMessageBox::aboutQt(this, "About Qt");
}

void NMain::firstTableSelection() {
    extract_button->setDisabled(false);

    disconnect(table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NMain::firstTableSelection);
}
