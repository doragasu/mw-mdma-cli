#ifndef _FLASHDLG_H_
#define _FLASHDLG_H_

#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QProgressBar>


class FlashDialog : public QDialog {
	Q_OBJECT
public:
	/// Pointer to tabs from parent
	QTabWidget *tabs;
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
	FlashInfoTab(void);

private:
	void InitUI(void);
};

class FlashEraseTab : public QWidget {
	Q_OBJECT
public:
	FlashEraseTab(void);

private:
	void InitUI(void);
};


class FlashReadTab : public QWidget {
	Q_OBJECT
public:
	FlashReadTab(void);

public slots:
	void ShowFileDialog(void);
	void Read(void);

private:
	/// File to read
	QLineEdit *fileLe;
	void InitUI(void);
};

class FlashWriteTab : public QWidget {
	Q_OBJECT
public:
	FlashWriteTab(FlashDialog *dlg);
//	static void SetRange(int min, int max);

public slots:
	void ShowFileDialog(void);
	void Flash(void);

private:
	/// Parent dialog
	FlashDialog *dlg;
	/// File to flash
	QLineEdit *fileLe;
	QCheckBox *autoCb;
	QCheckBox *verifyCb;
	void InitUI(void);
};

#endif /*_FLASHDLG_H_*/

