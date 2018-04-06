#include "NMain.h"
#include <QApplication>

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	NMain w;
	w.show();

	return a.exec();
}
