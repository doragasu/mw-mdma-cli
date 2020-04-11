/************************************************************************//**
 * \file
 *
 * \brief Flash manager dialog class implementation.
 *
 * Uses a dialog with a QTabWidget. Each tab is implemented in a separate
 * class.
 *
 * \author doragasu
 * \date   2017
 ****************************************************************************/
#include <QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include "flashdlg.h"

#include "flash_man.h"
#include "util.h"
#include "mdma.h"
#include "esp-prog.h"


/********************************************************************//**
 * Constructor
 *
 * \param[in] pointer to the owner of this class
 ************************************************************************/
FlashInfoTab::FlashInfoTab(FlashDialog *dlg) {
	this->dlg = dlg;
	InitUI();
}

/********************************************************************//**
 * Initialize the tab interface
 ************************************************************************/
void FlashInfoTab::InitUI(void) {
	QLabel *mdmaVerCaption = new QLabel("MDMA Version:");
	QLabel *mdmaVer = new QLabel(QString::asprintf("%d.%d", VERSION_MAJOR,
				VERSION_MINOR));
	mdmaVer->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *progVerCapion = new QLabel("Programmer version:");
	progVer = new QLabel("N/A");
	progVer->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *manIdCaption = new QLabel("Flash manufacturer ID:");
	manId = new QLabel("N/A");
	manId->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *devIdCaption = new QLabel("Flash device IDs:");
	devId = new QLabel("N/A");
	devId->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *about = new QLabel("MegaDrive Memory Administration,\n"
			"by Migue and doragasu, 2017");
	QPushButton *bootBtn = new QPushButton("Bootloader\nmode");

	// Connect signals and slots
	connect(dlg->tabs, SIGNAL(currentChanged(int)), this,
			SLOT(TabChange(int)));
	connect(bootBtn, SIGNAL(clicked()), this, SLOT(DfuEnter()));

	// Set layout
	QHBoxLayout *aboutLayout = new QHBoxLayout;
	aboutLayout->addWidget(about);
	aboutLayout->addStretch(1);
	aboutLayout->addWidget(bootBtn);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(mdmaVerCaption);
	mainLayout->addWidget(mdmaVer);
	mainLayout->addWidget(progVerCapion);
	mainLayout->addWidget(progVer);
	mainLayout->addWidget(manIdCaption);
	mainLayout->addWidget(manId);
	mainLayout->addWidget(devIdCaption);
	mainLayout->addWidget(devId);
	mainLayout->addLayout(aboutLayout);
	mainLayout->setAlignment(Qt::AlignTop);

	setLayout(mainLayout);
}

void FlashInfoTab::DfuEnter(void) {
	if (QMessageBox::warning(this, "Warning", "Programmer will enter DFU "
			"bootloader mode, and then MDMA will close. Once in bootloader "
			"mode, use a DFU programmer utility such as Atmel Flip to update "
			"the MDMA programmer firmware.\n\nProceed?", QMessageBox::Ok |
			QMessageBox::Cancel) == QMessageBox::Ok) {
		FlashMan fm;
		fm.DfuBootloader();
		dlg->close();
	}
}

/********************************************************************//**
 * This slot shall be run when the tab of the owner class changes. If
 * selected tab is the INFO tab, updates the programmer information
 *
 * \param[in] Selected tab index.
 ************************************************************************/
void FlashInfoTab::TabChange(int index) {
	uint16_t err;
	uint16_t manId;
	uint16_t devId[3];
	FlashMan fm;

	// If tab is the info tab, update fields
	if (index != 4) return;
	
	err = fm.ManIdGet(&manId);
	err |= fm.DevIdGet(devId);

	if (err) QMessageBox::warning(this, "Error", "Could not get IDs!");
	else {
		this->manId->setText(QString::asprintf("%04X", manId));
		this->devId->setText(QString::asprintf("%04X:%04X:%04X", devId[0],
				devId[1], devId[2]));
	}
}

/********************************************************************//**
 * Constructor
 *
 * \param[in] pointer to the owner of this class
 ************************************************************************/
FlashEraseTab::FlashEraseTab(FlashDialog *dlg) {
	this->dlg = dlg;

	InitUI();
}

/********************************************************************//**
 * Initialize the tab interface
 ************************************************************************/
void FlashEraseTab::InitUI(void) {
	// Create widgets
	fullCb = new QCheckBox("Full erase");
	QLabel *rangeLb = new QLabel("Range to erase (bytes):");
	QLabel *startLb = new QLabel("Start: ");
	startLe = new QLineEdit("0x000000");
	QLabel *lengthLb = new QLabel("Length: ");
	lengthLe = new QLineEdit(QString::asprintf("0x%06X", FM_CHIP_LENGTH));
	QPushButton *eraseBtn = new QPushButton("Erase!");
	
	// Connect signals and slots
	connect(eraseBtn, SIGNAL(clicked()), this, SLOT(Erase()));
	connect(fullCb, SIGNAL(stateChanged(int)), this, SLOT(ToggleFull(int)));

	// Set layout
	QHBoxLayout *rangeLayout = new QHBoxLayout;
	rangeLayout->addWidget(startLb);
	rangeLayout->addWidget(startLe);
	rangeLayout->addWidget(lengthLb);
	rangeLayout->addWidget(lengthLe);

	QVBoxLayout *rangeVLayout = new QVBoxLayout;
	rangeVLayout->addWidget(rangeLb);
	rangeVLayout->addLayout(rangeLayout);
	rangeFrame = new QWidget;
	rangeFrame->setLayout(rangeVLayout);

	QHBoxLayout *statLayout = new QHBoxLayout;
	statLayout->addWidget(eraseBtn);
	statLayout->setAlignment(Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(fullCb);
	mainLayout->addWidget(rangeFrame);
	mainLayout->addStretch(1);
	mainLayout->addLayout(statLayout);

	setLayout(mainLayout);
}

/********************************************************************//**
 * Hides the erase memory range input, depending on the check status of
 * the fullCb checkbox.
 *
 * \param[in] state Checked state of the fullCb checkbox.
 ************************************************************************/
void FlashEraseTab::ToggleFull(int state) {
	if (Qt::Checked == state) rangeFrame->hide();
	else rangeFrame->show();
}

/********************************************************************//**
 * Erases flash as specified in dialog data.
 ************************************************************************/
void FlashEraseTab::Erase(void) {
	int start, len;
	int status;
	bool ok;
	FlashMan fm;

	dlg->tabs->setEnabled(false);
	dlg->btnQuit->setVisible(false);

	if (fullCb->isChecked()) {
		dlg->statusLab->setText("Erasing...");
		dlg->repaint();
		status = fm.FullErase();
	} else {
		start = startLe->text().toInt(&ok, 0);
		if (ok) len = lengthLe->text().toInt(&ok, 0);
		if (!ok || ((start + len) > FM_CHIP_LENGTH)) {
			QMessageBox::warning(this, "MDMA", "Invalid erase range!");
			return;
		}
		dlg->statusLab->setText("Erasing...");
		dlg->repaint();
		// Partial erase, with word based range
		status = fm.RangeErase(start>>1, len>>1);
	}
	if (status) QMessageBox::warning(this, "Error", "Erase failed!");

	dlg->tabs->setEnabled(true);
	dlg->btnQuit->setVisible(true);
	dlg->statusLab->setText("Done!");
}


/********************************************************************//**
 * Constructor
 *
 * \param[in] pointer to the owner of this class
 ************************************************************************/
FlashReadTab::FlashReadTab(FlashDialog *dlg) {
	this->dlg = dlg;
	InitUI();
}

/********************************************************************//**
 * Initialize the tab interface
 ************************************************************************/
void FlashReadTab::InitUI(void) {
	// Create widgets
	QLabel *romLab = new QLabel("Read from cart to ROM:");	
	fileLe = new QLineEdit;
	QPushButton *fOpenBtn = new QPushButton("...");
	fOpenBtn->setFixedWidth(30);
	QLabel *rangeLb = new QLabel("Range to read (bytes):");
	QLabel *startLb = new QLabel("Start: ");
	startLe = new QLineEdit("0x000000");
	QLabel *lengthLb = new QLabel("Length: ");
	lengthLe = new QLineEdit(QString::asprintf("0x%06X", FM_CHIP_LENGTH));
	QPushButton *readBtn = new QPushButton("Read!");

	// Connect signals to slots
	connect(fOpenBtn, SIGNAL(clicked()), this, SLOT(ShowFileDialog()));
	connect(readBtn, SIGNAL(clicked()), this, SLOT(Read()));

	// Configure layout
	QHBoxLayout *fileLayout = new QHBoxLayout;
	fileLayout->addWidget(fileLe);
	fileLayout->addWidget(fOpenBtn);

	QHBoxLayout *rangeLayout = new QHBoxLayout;
	rangeLayout->addWidget(startLb);
	rangeLayout->addWidget(startLe);
	rangeLayout->addWidget(lengthLb);
	rangeLayout->addWidget(lengthLe);

	QHBoxLayout *statLayout = new QHBoxLayout;
	statLayout->addWidget(readBtn);
	statLayout->setAlignment(Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(romLab);
	mainLayout->addLayout(fileLayout);
	mainLayout->addWidget(rangeLb);
	mainLayout->addLayout(rangeLayout);
	mainLayout->addStretch(1);
	mainLayout->addLayout(statLayout);

	setLayout(mainLayout);
}

/********************************************************************//**
 * Opens a file dialog, for the user to select the file to write to.
 ************************************************************************/
void FlashReadTab::ShowFileDialog(void) {
	QString fileName;

	fileName = QFileDialog::getSaveFileName(this, tr("Write to ROM file"),
			NULL, tr("ROM Files (*.bin);;All files (*)"));
	if (!fileName.isEmpty()) fileLe->setText(fileName);
}

/********************************************************************//**
 * Reads a segment of the flash chip, depending on dialog data
 ************************************************************************/
void FlashReadTab::Read(void) {
	uint16_t *rdBuf = NULL;
	int start, len;
	bool ok;

	if (fileLe->text().isEmpty()) {
		QMessageBox::warning(this, "MDMA", "No file selected!");
		return;
	}
	start = startLe->text().toInt(&ok, 0);
	if (ok) len = lengthLe->text().toInt(&ok, 0);
	if (!ok || ((start + len) > FM_CHIP_LENGTH)) {
		QMessageBox::warning(this, "MDMA", "Invalid read range!");
		return;
	}
	dlg->tabs->setDisabled(true);
	dlg->btnQuit->setVisible(false);
	dlg->progBar->setVisible(true);
	// Create Flash Manager and connect signals to UI control slots
	FlashMan fm;
	connect(&fm, &FlashMan::RangeChanged, dlg->progBar,
			&QProgressBar::setRange);
	connect(&fm, &FlashMan::ValueChanged, dlg->progBar,
			&QProgressBar::setValue);
	connect(&fm, &FlashMan::StatusChanged, dlg->statusLab, &QLabel::setText);

	// Convert to word addresses
	start >>=1;
	len = (len>>1) + (len & 1);
	// Start reading
	rdBuf = fm.Read(start, len);
	if (!rdBuf) {
		QMessageBox::warning(this, "Read failed",
				"Cannot allocate buffer!");
	}
	// Cart readed, write to file
	if (rdBuf) {
		FILE *f;
		if (!(f = fopen(fileLe->text().toStdString().c_str(), "wb")))
			QMessageBox::warning(this, "ERROR", "Could not open file!");
		else {
			// Do byte-swaps
			for (int i = 0; i < len; i++) ByteSwapWord(rdBuf[i]);
	        if (fwrite(rdBuf, len<<1, 1, f) <= 0)
				QMessageBox::warning(this, "ERROR", "Could write to file!");
	        fclose(f);
		}
	}
	// Cleanup
	if (rdBuf) fm.BufFree(rdBuf);
	dlg->progBar->setVisible(false);
	dlg->btnQuit->setVisible(true);
	dlg->tabs->setDisabled(false);
	dlg->statusLab->setText("Done!");
	disconnect(this, 0, 0, 0);
}

/********************************************************************//**
 * Constructor
 *
 * \param[in] pointer to the owner of this class
 ************************************************************************/
FlashWriteTab::FlashWriteTab(FlashDialog *dlg) {
	this->dlg = dlg;
	InitUI();
}

/********************************************************************//**
 * Initialize the tab interface
 ************************************************************************/
void FlashWriteTab::InitUI(void) {
	// Create widgets
	QLabel *romLab = new QLabel("Write to cart from ROM:");	
	fileLe = new QLineEdit;
	QPushButton *fOpenBtn = new QPushButton("...");
	fOpenBtn->setFixedWidth(30);
	autoCb = new QCheckBox("Auto-erase");
	autoCb->setCheckState(Qt::Checked);
	verifyCb = new QCheckBox("Verify");
	verifyCb->setCheckState(Qt::Unchecked);
	QPushButton *flashBtn = new QPushButton("Flash!");

	// Connect signals to slots
	connect(fOpenBtn, SIGNAL(clicked()), this, SLOT(ShowFileDialog()));
	connect(flashBtn, SIGNAL(clicked()), this, SLOT(Flash()));

	// Configure layout
	QHBoxLayout *fileLayout = new QHBoxLayout;
	fileLayout->addWidget(fileLe);
	fileLayout->addWidget(fOpenBtn);

	QHBoxLayout *statLayout = new QHBoxLayout;
	statLayout->addWidget(flashBtn);
	statLayout->setAlignment(Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(romLab);
	mainLayout->addLayout(fileLayout);
	mainLayout->addWidget(autoCb);
	mainLayout->addWidget(verifyCb);
	mainLayout->addStretch(1);
	mainLayout->addLayout(statLayout);

	setLayout(mainLayout);
}

/********************************************************************//**
 * Opens the file dialog for the user to select the file to program.
 ************************************************************************/
void FlashWriteTab::ShowFileDialog(void) {
	QString fileName;

	fileName = QFileDialog::getOpenFileName(this, tr("Open ROM file"),
			NULL, tr("ROM Files (*.bin);;All files (*)"));
	if (!fileName.isEmpty()) fileLe->setText(fileName);
}

/********************************************************************//**
 * Programs a file to the flash chip, depending on dialog input.
 ************************************************************************/
void FlashWriteTab::Flash(void) {
	uint16_t *wrBuf = NULL;
	uint16_t *rdBuf = NULL;
	uint32_t start = 0;
	uint32_t len = 0;
	bool autoErase;

	if (fileLe->text().isEmpty()) {
		return;
	}
	dlg->tabs->setDisabled(true);
	dlg->btnQuit->setVisible(false);
	dlg->progBar->setVisible(true);
	// Create Flash Manager and connect signals to UI control slots
	FlashMan fm;
	connect(&fm, &FlashMan::RangeChanged, dlg->progBar,
			&QProgressBar::setRange);
	connect(&fm, &FlashMan::ValueChanged, dlg->progBar,
			&QProgressBar::setValue);
	connect(&fm, &FlashMan::StatusChanged, dlg->statusLab, &QLabel::setText);

	autoErase = autoCb->isChecked();
	// Should not be necessary doing this, but QT does not refresh dialog
	// unless forced with the repaint()
	if (autoErase) dlg->statusLab->setText("Auto erasing");
	dlg->repaint();
	// Start programming
	wrBuf = fm.Program(fileLe->text().toStdString().c_str(),
			autoErase, &start, &len);
	if (!wrBuf) {
		/// \todo show msg box with error and return
		QMessageBox::warning(this, "Program failed",
				"Cannot program file!");
		dlg->progBar->setVisible(false);
		dlg->btnQuit->setVisible(true);
		dlg->tabs->setDisabled(false);
		dlg->statusLab->setText("Done!");
		disconnect(this, 0, 0, 0);
		return;
	}
	// If verify requested, do it!
	if (verifyCb->isChecked()) {
		rdBuf = fm.Read(start, len);
		if (!rdBuf) {
			fm.BufFree(wrBuf);
			QMessageBox::warning(this, "Verify failed",
					"Cannot allocate buffer!");
			dlg->progBar->setVisible(false);
			dlg->btnQuit->setVisible(true);
			dlg->tabs->setDisabled(false);
			dlg->statusLab->setText("Done!");
			return;
		}
		for (uint32_t i = 0; i < len; i++) {
			if (wrBuf[i] != rdBuf[i]) {
				QString str;
				str.sprintf("Verify failed at pos: 0x%X!", i);
				QMessageBox::warning(this, "Verify failed", str);
				break;
			}
		}
		dlg->statusLab->setText("Verify OK!");
	} else {
		dlg->statusLab->setText("Program OK!");
	} if (wrBuf) fm.BufFree(wrBuf);
	if (rdBuf) fm.BufFree(rdBuf);
	dlg->progBar->setVisible(false);
	dlg->btnQuit->setVisible(true);
	dlg->tabs->setDisabled(false);
	disconnect(this, 0, 0, 0);
}

/********************************************************************//**
 * Default constructor.
 ************************************************************************/
FlashDialog::FlashDialog(void) {
	InitUI();
}

/********************************************************************//**
 * Initialize the dialog with the tabs.
 ************************************************************************/
void FlashDialog::InitUI(void) {
	tabs = new QTabWidget;
	tabs->addTab(new FlashWriteTab(this), tr("WRITE"));
	tabs->addTab(new FlashReadTab(this),  tr("READ"));
	tabs->addTab(new FlashEraseTab(this), tr("ERASE"));
	tabs->addTab(new FlashWifiTab(this),  tr("WIFI"));
	tabs->addTab(new FlashInfoTab(this),  tr("INFO"));

	statusLab = new QLabel("Ready!");
	statusLab->setFixedWidth(80);
	progBar = new QProgressBar;
	progBar->setVisible(false);
	btnQuit = new QPushButton("Exit");
	btnQuit->setDefault(true);

	connect(btnQuit, SIGNAL(clicked()), this, SLOT(close()));

	QHBoxLayout *statLayout = new QHBoxLayout;
	statLayout->addWidget(statusLab);
	statLayout->addWidget(progBar);
	statLayout->addWidget(btnQuit);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(tabs);
	mainLayout->addLayout(statLayout);

	setLayout(mainLayout);

	resize(350, 300);
	setWindowTitle("MegaDrive Memory Administration");
}

/********************************************************************//**
 * Constructor
 *
 * \param[in] pointer to the owner of this class
 ************************************************************************/
FlashWifiTab::FlashWifiTab(FlashDialog *dlg) {
	this->dlg = dlg;
	InitUI();
}

/********************************************************************//**
 * Initialize the tab interface
 ************************************************************************/
void FlashWifiTab::InitUI(void) {
	// Create widgets
	QLabel *romLab = new QLabel("WiFi module firmware blob:");	
	fileLe = new QLineEdit;
	QPushButton *fOpenBtn = new QPushButton("...");
	fOpenBtn->setFixedWidth(30);
	QLabel *addrLb = new QLabel("Flash to address: ");
	addrLe = new QLineEdit("0x10000");
	QLabel *spiTypeLb = new QLabel("SPI flash type: ");
	spiTypeCombo = new QComboBox();
	spiTypeCombo->insertItem(ESP_FLASH_QIO, "QIO");
	spiTypeCombo->insertItem(ESP_FLASH_QOUT, "QOUT");
	spiTypeCombo->insertItem(ESP_FLASH_DIO, "DIO");
	spiTypeCombo->insertItem(ESP_FLASH_DOUT, "DOUT");
	spiTypeCombo->insertItem(ESP_FLASH_UNCHANGED, "Do not change");
	spiTypeCombo->setCurrentIndex(ESP_FLASH_UNCHANGED);

	QPushButton *flashBtn = new QPushButton("Flash!");

	// Connect signals to slots
	connect(fOpenBtn, SIGNAL(clicked()), this, SLOT(ShowFileDialog()));
	connect(flashBtn, SIGNAL(clicked()), this, SLOT(UploadFirmware()));

	// Configure layout
	QHBoxLayout *fileLayout = new QHBoxLayout;
	fileLayout->addWidget(fileLe);
	fileLayout->addWidget(fOpenBtn);

	QHBoxLayout *addrLayout = new QHBoxLayout;
	addrLayout->addWidget(addrLb);
	addrLayout->addWidget(addrLe);

	QHBoxLayout *spiTypeLayout = new QHBoxLayout;
	spiTypeLayout->addWidget(spiTypeLb);
	spiTypeLayout->addWidget(spiTypeCombo);

	QHBoxLayout *statLayout = new QHBoxLayout;
	statLayout->addWidget(flashBtn);
	statLayout->setAlignment(Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(romLab);
	mainLayout->addLayout(fileLayout);
	mainLayout->addLayout(addrLayout);
	mainLayout->addLayout(spiTypeLayout);
	mainLayout->addStretch(1);
	mainLayout->addLayout(statLayout);

	setLayout(mainLayout);
}

/********************************************************************//**
 * Opens the file dialog for the user to select the file to program.
 ************************************************************************/
void FlashWifiTab::ShowFileDialog(void) {
	QString fileName;

	fileName = QFileDialog::getOpenFileName(this, tr("Open WiFi firmware file"),
			NULL, tr("WiFi Firmware Files (*.bin);;All files (*)"));
	if (!fileName.isEmpty()) fileLe->setText(fileName);
}

/********************************************************************//**
 * Upload firmware blob to the WiFi module.
 ************************************************************************/
void FlashWifiTab::UploadFirmware(void) {
	Flags f = {};
	EpBlobData *b = NULL;
	EpFlashStatus st;
	int err;
	uint32_t addr;
	bool ok = false;

	if (fileLe->text().isEmpty()) {
		return;
	}
	addr = addrLe->text().toInt(&ok, 0);
	if (!ok) {
		QMessageBox::warning(this, "Wrong address", "Enter a valid address");
		return;
	}
	f.flash_mode = (enum esp_flash_mode)spiTypeCombo->currentIndex();
	b = EpBlobLoad(fileLe->text().toStdString().c_str(), addr, &f);
	if (!b) {
		QMessageBox::warning(this, "Flash failed", "Cannot open file");
		return;
	}
	dlg->tabs->setDisabled(true);
	dlg->btnQuit->setVisible(false);
	dlg->progBar->setVisible(true);

	dlg->statusLab->setText("Erasing...");
	dlg->progBar->setRange(0, b->sect_total);
	dlg->progBar->setValue(0);
	dlg->repaint();

	err = EpSync();
	if (!err) {
		err = EpErase(b);
	}
	if (err) {
		QMessageBox::warning(this, "Cannot erase flash",
				"Reconnect programmer, restart MDMA and try again");
		goto out;
	}

	dlg->statusLab->setText("Program...");
	do {
		dlg->progBar->setValue(b->sect);
		QApplication::processEvents();
		st = EpFlashNext(b);
	} while (EP_FLASH_REMAINING == st);
	dlg->progBar->setValue(b->sect);
	if (EP_FLASH_DONE != st) {
		QMessageBox::warning(this, "Flash failed",
				"Reconnect programmer, restart MDMA and try again");
		goto out;
	}

	QApplication::processEvents();
	EpFinish(TRUE);

out:
	EpBlobFree(b);
	dlg->progBar->setVisible(false);
	dlg->btnQuit->setVisible(true);
	dlg->tabs->setDisabled(false);
	dlg->statusLab->setText("Done!");
}

