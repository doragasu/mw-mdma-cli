#ifndef _FLASHDLG_H_
#define _FLASHDLG_H_

#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>

class FlashDialog : public QDialog {
	Q_OBJECT
public:
	/// Pointer to tabs from parent
	QTabWidget *tabs;
	/// Status label
	QLabel *statusLab;
	/// Pointer to progress bar from parent
	QProgressBar *progBar;
	/// Pointer to exit button
	QPushButton *btnQuit;

	/// Default constructor
	FlashDialog(void);

private:
	void InitUI(void);
};

class FlashInfoTab : public QWidget {
	Q_OBJECT
public:
	FlashInfoTab(FlashDialog *dlg);

public slots:
	void TabChange(int index);

private:
	/// Parent dialog
	FlashDialog *dlg;
	QLabel *progVer;
	QLabel *manId;
	QLabel *devId;
	void InitUI(void);
};

class FlashEraseTab : public QWidget {
	Q_OBJECT
public:
	FlashEraseTab(FlashDialog *dlg);

public slots:
	void Erase(void);
	void ToggleFull(int state);

private:
	/// Parent dialog
	FlashDialog *dlg;
	QLineEdit *startLe;
	QLineEdit *lengthLe;
	QCheckBox *fullCb;
	QWidget *rangeFrame;
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
	/// Cartridge address to start reading
	QLineEdit *startLe;
	/// Read length
	QLineEdit *lengthLe;
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

