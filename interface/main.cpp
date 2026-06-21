#include "MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName("LENS");
	app.setOrganizationName("LENS");

	app.setStyleSheet(
		"* { font-family: 'Segoe UI', 'Inter', 'SF Pro Display', Arial, sans-serif; }"

		"QWidget { background-color: #07090f; color: #c8d6ef; }"

		"QMainWindow { background-color: #07090f; }"

		"QDialog { background-color: #0a0e1a; }"

		"QLabel { background: transparent; }"

		"QLineEdit {"
		"  background-color: #0f1525;"
		"  border: 1px solid #1a2640;"
		"  border-radius: 7px;"
		"  padding: 8px 12px;"
		"  color: #c8d6ef;"
		"  font-size: 13px;"
		"}"
		"QLineEdit:focus {"
		"  border-color: #2563eb;"
		"  background-color: #111828;"
		"}"

		"QScrollArea { border: none; background: transparent; }"

		"QMessageBox { background-color: #0a0e1a; }"
		"QMessageBox QLabel { color: #c8d6ef; }"
		"QMessageBox QPushButton {"
		"  background: #0f1525; color: #8aaed4; border: 1px solid #1a2e4a;"
		"  border-radius: 6px; padding: 5px 16px; min-width: 70px;"
		"}"
		"QMessageBox QPushButton:hover { background: #162035; }"
	);

	MainWindow w;
	w.show();

	return app.exec();
}
