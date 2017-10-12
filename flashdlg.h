#ifndef _FLASHDLG_H_
#define _FLASHDLG_H_

#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QLabel>


class FlashDialog : public QDialog {
	Q_OBJECT
public:
	/// Pointer to tabs from parent
	QTabWidget *tabs;
	/// Status label
	QLabel *statusLab;
	/// Pointer to progress bar from parent
	QProgressBar *progBar;

	/// Default constructor
	FlashDialog(void);

private:
	void InitUI(void);
};

class FlashInfoTab : public QWidget {
	Q_OBJECT
public:
	FlashInfoTab(FlashDialog *dlg);

private:
	/// Parent dialog
	FlashDialog *dlg;
	void InitUI(void);
};

class FlashEraseTab : public QWidget {
	Q_OBJECT
public:
	FlashEraseTab(FlashDialog *dlg);

private:
	/// Parent dialog
	FlashDialog *dlg;
	void InitUI(void);
};

/// Class handling the READ tab
class FlashReadTab : public QWidget {
	Q_OBJECT
public:
	FlashReadTab(FlashDialog *dlg);

public slots:
	void Read(void);
	void ShowFileDialog(void);

private:
	/// Parent dialog
	FlashDialog *dlg;
	/// File to read
	QLineEdit *fileLe;
	void InitUI(void);
};

/// Class handling the WRITE tab
class FlashWriteTab : public QWidget {
	Q_OBJECT
public:
	FlashWriteTab(FlashDialog *dlg);

public slots:
	void Flash(void);
	void ShowFileDialog(void);

private:
	/// Parent dialog
	FlashDialog *dlg;
	/// File to flash
	QLineEdit *fileLe;
	QCheckBox *autoCb;
	QCheckBox *verifyCb;
	void InitUI(void);
	FlashWriteTab();
};

#endif /*_FLASHDLG_H_*/

