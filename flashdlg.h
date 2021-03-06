/************************************************************************//**
 * \file
 *
 * \brief Flash manager dialog class implementation.
 *
 * \defgroup flashdlg flashdlg
 * \{
 * \brief Flash manager dialog class implementation.
 *
 * Uses a dialog with a QTabWidget. Each tab is implemented in a separate
 * class.
 *
 * \author doragasu
 * \date   2017
 ****************************************************************************/

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
#include <QtWidgets/QComboBox>

/************************************************************************//**
 * Main dialog class
 ****************************************************************************/
class FlashDialog : public QDialog {
	Q_OBJECT
public:
	/// Tabs class handler
	QTabWidget *tabs;
	/// Status label
	QLabel *statusLab;
	/// Pointer to progress bar from parent
	QProgressBar *progBar;
	/// Pointer to exit button
	QPushButton *btnQuit;

	/********************************************************************//**
	 * Default constructor.
	 ************************************************************************/
	FlashDialog(void);

private:
	/********************************************************************//**
	 * Initialize the dialog with the tabs.
	 ************************************************************************/
	void InitUI(void);
};

/************************************************************************//**
 * Widget class handling the INFO tab
 ****************************************************************************/
class FlashInfoTab : public QWidget {
	Q_OBJECT
public:
	/********************************************************************//**
	 * Constructor
	 *
	 * \param[in] pointer to the owner of this class
	 ************************************************************************/
	FlashInfoTab(FlashDialog *dlg);

public slots:
	/********************************************************************//**
	 * This slot shall be run when the tab of the owner class changes. If
	 * selected tab is the INFO tab, updates the programmer information
	 *
	 * \param[in] Selected tab index.
	 ************************************************************************/
	void TabChange(int index);

	void DfuEnter(void);

private:
	/// Parent dialog
	FlashDialog *dlg;
	/// Programmer version label
	QLabel *progVer;
	/// Manufacturer ID label
	QLabel *manId;
	/// Device ID label
	QLabel *devId;

	/********************************************************************//**
	 * Initialize the tab interface
	 ************************************************************************/
	void InitUI(void);

	/********************************************************************//**
	 * Do not allow using default constructor
	 ************************************************************************/
	FlashInfoTab(void);
};

/************************************************************************//**
 * Widget class handling the ERASE tab
 ****************************************************************************/
class FlashEraseTab : public QWidget {
	Q_OBJECT
public:
	/********************************************************************//**
	 * Constructor
	 *
	 * \param[in] pointer to the owner of this class
	 ************************************************************************/
	FlashEraseTab(FlashDialog *dlg);

public slots:
	/********************************************************************//**
	 * Erases flash as specified in dialog data.
	 ************************************************************************/
	void Erase(void);

	/********************************************************************//**
	 * Hides the erase memory range input, depending on the check status of
	 * the fullCb checkbox.
	 *
	 * \param[in] state Checked state of the fullCb checkbox.
	 ************************************************************************/
	void ToggleFull(int state);

private:
	/// Parent dialog
	FlashDialog *dlg;
	/// Start of the erase range
	QLineEdit *startLe;
	/// Length of the erase range
	QLineEdit *lengthLe;
	/// Full erase checkbox
	QCheckBox *fullCb;
	/// Widget holding the frame range controls
	QWidget *rangeFrame;

	/********************************************************************//**
	 * Initialize the tab interface
	 ************************************************************************/
	void InitUI(void);

	/********************************************************************//**
	 * Do not allow using default constructor
	 ************************************************************************/
	FlashEraseTab(void);
};

/************************************************************************//**
 * Widget class handling the READ tab
 ****************************************************************************/
class FlashReadTab : public QWidget {
	Q_OBJECT
public:
	/********************************************************************//**
	 * Constructor
	 *
	 * \param[in] pointer to the owner of this class
	 ************************************************************************/
	FlashReadTab(FlashDialog *dlg);

public slots:
	/********************************************************************//**
	 * Reads a segment of the flash chip, depending on dialog data
	 ************************************************************************/
	void Read(void);

	/********************************************************************//**
	 * Opens a file dialog, for the user to select the file to write to.
	 ************************************************************************/
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

	/********************************************************************//**
	 * Initialize the tab interface
	 ************************************************************************/
	void InitUI(void);

	/********************************************************************//**
	 * Do not allow using default constructor
	 ************************************************************************/
	FlashReadTab(void);
};

/************************************************************************//**
 * Widget class handling the WRITE tab
 ****************************************************************************/
class FlashWriteTab : public QWidget {
	Q_OBJECT
public:
	/********************************************************************//**
	 * Constructor
	 *
	 * \param[in] pointer to the owner of this class
	 ************************************************************************/
	FlashWriteTab(FlashDialog *dlg);

public slots:
	/********************************************************************//**
	 * Programs a file to the flash chip, depending on dialog input.
	 ************************************************************************/
	void Flash(void);

	/********************************************************************//**
	 * Opens the file dialog for the user to select the file to program.
	 ************************************************************************/
	void ShowFileDialog(void);

private:
	/// Parent dialog
	FlashDialog *dlg;
	/// File to flash
	QLineEdit *fileLe;
	/// Auto-erase checkbox
	QCheckBox *autoCb;
	/// Verify checkbox
	QCheckBox *verifyCb;

	/********************************************************************//**
	 * Initialize the tab interface
	 ************************************************************************/
	void InitUI(void);

	/********************************************************************//**
	 * Do not allow using default constructor
	 ************************************************************************/
	FlashWriteTab(void);
};

class FlashWifiTab : public QWidget {
	Q_OBJECT
public:
		FlashWifiTab(FlashDialog *dlg);

public slots:
	/********************************************************************//**
	 * Upload firmware blob to the WiFi module.
	 ************************************************************************/
		void UploadFirmware(void);

	/********************************************************************//**
	 * Opens a file dialog, for the user to select the firmawre file to upload.
	 ************************************************************************/
	void ShowFileDialog(void);

private:
	/// Parent dialog
	FlashDialog *dlg;
	/// Firmware file to upload
	QLineEdit *fileLe;
	/// Address to upload the firmware to
	QLineEdit *addrLe;
	/// SPI flash mode
	QComboBox *spiTypeCombo;

	/********************************************************************//**
	 * Initialize the tab interface
	 ************************************************************************/
	void InitUI(void);
};

#endif /*_FLASHDLG_H_*/

/** \} */

