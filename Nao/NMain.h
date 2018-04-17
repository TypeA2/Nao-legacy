#ifndef NMAIN_H
#define NMAIN_H

#include <QtConcurrent/QtConcurrent>

#include <QMainWindow>
#include <QMenuBar>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>

#include <QProgressDialog>

#include <QDebug>

#include <libnao.h>
#include <NaoCRIWareReader.h>
#include <NaoDATReader.h>

class NMain : public QMainWindow {
		Q_OBJECT

	public:
        NMain();
        ~NMain() {}

    signals:
        void extractAllDialogProgress(qint64 v);

    private slots:
        void openFile();
        void openOptions() {}
        void loadFile(QString file);
        void about();
        void aboutQt();

        void dragEnterEvent(QDragEnterEvent* e);
        void dropEvent(QDropEvent* e);

        void firstTableSelection();

        void extractSingleFile();
        void extractAll();
        void extractRightClickEvent(const QPoint& p);

    private:
        enum TableRoles {
            FileNameRole = Qt::UserRole,
            FilePathRole,
            FileSizeEmbeddedRole,
            FileSizeExtractedRole,
            FileOffsetRole,
            FileExtraOffsetRole,
            FileIndexRole,
            FileDataTypeRole
        };

        QMenu* extractContextMenu       = nullptr;
        QPushButton* extract_button     = nullptr;
        QPushButton* extract_all_button = nullptr;
        QTableWidget* table             = nullptr;

        LibNao::FileType currentType = LibNao::None;

        NaoCRIWareReader* CRIWareReader = nullptr;
        NaoDATReader* PG_DATReader = nullptr;

        QString savePath;

        void CRIWareHandler(QString file);
        void PG_DATHandler(QString file);
        void setup_window();
        void setup_menus();
};

#endif // NMAIN_H
