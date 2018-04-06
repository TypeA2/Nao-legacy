#include "NMain.h"

#include <QProgressDialog>

NMain::NMain()
    : QMainWindow(),
    CRIWareSavePath(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0)) {
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

            table->setColumnCount(0);
            while (table->rowCount() > 0) {
                table->removeRow(0);
            }

            extract_button->setDisabled(true);
            extract_all_button->setDisabled(true);

            disconnect(extract_button, &QPushButton::clicked, this, 0);
            disconnect(extract_all_button, &QPushButton::clicked, this, 0);

            break;
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

void NMain::CRIWareHandler(QString file) {
    CRIWareReader = new NaoCRIWareReader(file);
    const QVector<NaoCRIWareReader::EmbeddedFile>& files = CRIWareReader->getFiles();

    table->setColumnCount(5);
    table->setRowCount(files.size());
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    table->setHorizontalHeaderItem(0, new QTableWidgetItem("#"));
    table->setHorizontalHeaderItem(1, new QTableWidgetItem("File name"));
    table->setHorizontalHeaderItem(2, new QTableWidgetItem("Embedded size"));
    table->setHorizontalHeaderItem(3, new QTableWidgetItem("Extracted size"));
    table->setHorizontalHeaderItem(4, new QTableWidgetItem("Compression"));

    extract_all_button->setDisabled(false);

    connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NMain::firstTableSelection);
    connect(table, &QTableWidget::customContextMenuRequested, this, &NMain::CRIWareRightclickEvent);
    connect(extract_button, &QPushButton::clicked, this, &NMain::CRIWareExtractSingleFile);
    connect(extract_all_button, &QPushButton::clicked, this, &NMain::CRIWareExtractAll);

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
}

void NMain::CRIWareRightclickEvent(const QPoint& p) {
    table->selectRow(table->rowAt(p.y()));

    CRIWareContextMenu->popup(table->viewport()->mapToGlobal(p));
}

void NMain::CRIWareExtractSingleFile() {
    QTableWidgetItem* file = table->item(table->selectionModel()->selectedRows().at(0).row(), 0);

    if (file->data(FileSizeEmbeddedRole).toULongLong() == 0ULL) {
        QMessageBox::warning(
                    this,
                    "Can't extract file",
                    "The following file could not be extracted because it does not have a size:\n\n" + file->data(FileNameRole).toString(),
                    QMessageBox::Ok,
                    QMessageBox::Ok);
    } else {
        QString output = QFileDialog::getSaveFileName(
                    this,
                    "Select output file",
                    CRIWareSavePath + "/" + file->data(FileNameRole).toString());

        if (!output.isEmpty()) {
            CRIWareSavePath = QFileInfo(output).absolutePath();

            QFile outfile(output);

            if (!outfile.open(QIODevice::WriteOnly)) {
                QMessageBox::critical(
                            this,
                            "File sav error",
                            "Could not save the following file:\n\n" + file->data(FileNameRole).toString(),
                            QMessageBox::Ok,
                            QMessageBox::Ok);
            } else {
                outfile.write(CRIWareReader->extractFileAt(file->data(FileIndexRole).toLongLong()));
                outfile.close();
            }
        }
    }
}

void NMain::CRIWareExtractAll() {
    QString output = QFileDialog::getExistingDirectory(
                this,
                "Select output directory",
                CRIWareSavePath);

    if (!output.isEmpty()) {
        CRIWareSavePath = output;

        QString originFileName = QFileInfo(CRIWareReader->getFileName()).fileName();
        QDir outdir(output + "/" + originFileName);

        QSet<QString> dirs;
        const QVector<NaoCRIWareReader::EmbeddedFile>& files = CRIWareReader->getFiles();
        int fileCount = files.length();
        qint64 totalFilesEmbeddedSize = 0;
        qint64 totalFilesExtractedSize = 0;

        for (int i = 0; i < fileCount; i++) {
            totalFilesEmbeddedSize += files.at(i).size;
            totalFilesExtractedSize += files.at(i).extractedSize;

            if (!files.at(i).path.isEmpty())
                dirs.insert(files.at(i).path);
        }

        QSet<QString>::const_iterator it = dirs.constBegin();

        while (it != dirs.constEnd()) {
            if (!outdir.mkpath(*it))
                qFatal((QString("Could not create directory ") + outdir.path() + "/" + *it).toLatin1().data());

            ++it;
        }

        QProgressDialog* dialog = new QProgressDialog(
                    "Extracting files...",
                    "",
                    0,
                    (totalFilesExtractedSize > 0xFFFFFFFFULL) ?
                        (totalFilesExtractedSize >> 10) : totalFilesExtractedSize,
                    this);
        dialog->setCancelButton(nullptr);
        dialog->setModal(true);
        dialog->setFixedWidth(this->width() / 2);
        dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowCloseButtonHint & ~Qt::WindowContextHelpButtonHint);
        dialog->show();

        connect(this, &NMain::CRIWareDialogProgress, this, [=](int v) {
            qint64 target = dialog->value() + ((totalFilesExtractedSize > 0xFFFFFFFFULL) ? (v >> 10) : v);
            dialog->setValue(target);
        });

        QFutureWatcher<void>* watcher = new QFutureWatcher<void>();

        connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
            QMessageBox::information(
                        this,
                        "Done",
                        QString("Extraction complete.\n\n") +
                                "Files:\t" + QString::number(files.size()) + "\n"
                                "Read:\t" + LibNao::Utils::getShortSize(totalFilesEmbeddedSize) + "\n"
                                "Wrote:\t" + LibNao::Utils::getShortSize(totalFilesExtractedSize),
                        QMessageBox::Ok,
                        QMessageBox::Ok);

            disconnect(this, &NMain::CRIWareDialogProgress, this, 0);

            watcher->deleteLater();
            dialog->deleteLater();
        });

        QFuture<void> future = QtConcurrent::run([=]() {
            for (int i = 0; i < fileCount; i++) {
                NaoCRIWareReader::EmbeddedFile file = files.at(i);
                QString target = outdir.absolutePath() + "/" + file.path + "/" + file.name;
                QFile outfile(target);

                if (!outfile.open(QIODevice::WriteOnly))
                    qFatal((QString("Could not open file ") + target + " for writing").toLatin1().data());

                outfile.write(CRIWareReader->extractFileAt(i));
                outfile.close();

                emit CRIWareDialogProgress(file.extractedSize);
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
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

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

    CRIWareContextMenu = new QMenu(this);
    QAction* CRIWareExtractAction = new QAction("Extract");
    QAction* CRIWareExtractAllAction = new QAction("Extract all");

    CRIWareContextMenu->addAction(CRIWareExtractAction);
    CRIWareContextMenu->addSeparator();
    CRIWareContextMenu->addAction(CRIWareExtractAllAction);

    connect(CRIWareExtractAction, &QAction::triggered, this, &NMain::CRIWareExtractSingleFile);
    connect(CRIWareExtractAllAction, &QAction::triggered, this, &NMain::CRIWareExtractAll);
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
