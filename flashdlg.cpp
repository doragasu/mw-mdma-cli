#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include "flashdlg.h"

#include "flash_man.h"


FlashInfoTab::FlashInfoTab(FlashDialog *dlg) {
	this->dlg = dlg;
	InitUI();
}

void FlashInfoTab::InitUI(void) {
	QLabel *mdmaVerCaption = new QLabel("MDMA Version:");
	QLabel *mdmaVer = new QLabel("0.4");
	mdmaVer->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *progVerCapion = new QLabel("Programmer version:");
	QLabel *progVer = new QLabel("");
	progVer->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *chrIdsCaption = new QLabel("CHR Flash ID:");
	QLabel *chrIds = new QLabel;
	chrIds->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *prgIdsCaption = new QLabel("PRG Flash ID:");
	QLabel *prgIds = new QLabel;
	prgIds->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(mdmaVerCaption);
	mainLayout->addWidget(mdmaVer);
	mainLayout->addWidget(progVerCapion);
	mainLayout->addWidget(progVer);
	mainLayout->addWidget(chrIdsCaption);
	mainLayout->addWidget(chrIds);
	mainLayout->addWidget(prgIdsCaption);
	mainLayout->addWidget(prgIds);
	mainLayout->setAlignment(Qt::AlignTop);

	setLayout(mainLayout);
}

FlashEraseTab::FlashEraseTab(FlashDialog *dlg) {
	this->dlg = dlg;

	InitUI();
}

void FlashEraseTab::InitUI(void) {
}

FlashReadTab::FlashReadTab(FlashDialog *dlg) {
	this->dlg = dlg;
	InitUI();
}

void FlashReadTab::InitUI(void) {
	// Create widgets
	QLabel *romLab = new QLabel("Read from cart to ROM:");	
	fileLe = new QLineEdit;
	QPushButton *fOpenBtn = new QPushButton("...");
	fOpenBtn->setFixedWidth(30);
	QLabel *startLb = new QLabel("Start: ");
	QLineEdit *start = new QLineEdit("0x000000");
	QLabel *lengthLb = new QLabel("End: ");
	QLineEdit *length = new QLineEdit("0x400000");
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
	rangeLayout->addWidget(start);
	rangeLayout->addWidget(lengthLb);
	rangeLayout->addWidget(length);

	QHBoxLayout *statLayout = new QHBoxLayout;
	statLayout->addWidget(readBtn);
	statLayout->setAlignment(Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(romLab);
	mainLayout->addLayout(fileLayout);
	mainLayout->addLayout(rangeLayout);
	mainLayout->addStretch(1);
	mainLayout->addLayout(statLayout);

	setLayout(mainLayout);
}

void FlashReadTab::ShowFileDialog(void) {
	QString fileName;

	fileName = QFileDialog::getSaveFileName(this, tr("Write to ROM file"),
			NULL, tr("ROM Files (*.bin);;All files (*)"));
	if (!fileName.isEmpty()) fileLe->setText(fileName);
}

void FlashReadTab::Read(void) {
	uint16_t *rdBuf = NULL;

	if (fileLe->text().isEmpty()) return;
	dlg->tabs->setDisabled(true);
	dlg->progBar->setVisible(true);
	// Create Flash Manager and connect signals to UI control slots
	FlashMan fm;
	connect(&fm, &FlashMan::RangeChanged, dlg->progBar,
			&QProgressBar::setRange);
	connect(&fm, &FlashMan::ValueChanged, dlg->progBar,
			&QProgressBar::setValue);
	connect(&fm, &FlashMan::StatusChanged, dlg->statusLab, &QLabel::setText);

//	// Start reading
//	rdBuf = fm.Read(start, len);
//	if (!rdBuf) {
//		fm.BufFree(wrBuf);
//		QMessageBox::warning(this, "Verify failed",
//				"Cannot allocate buffer!");
//		dlg->progBar->setVisible(false);
//		dlg->tabs->setDisabled(false);
//		dlg->statusLab->setText("Done!");
//		return;
//	}
}

FlashWriteTab::FlashWriteTab(FlashDialog *dlg) {
	this->dlg = dlg;
	InitUI();
}

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

void FlashWriteTab::ShowFileDialog(void) {
	QString fileName;

	fileName = QFileDialog::getOpenFileName(this, tr("Open ROM file"),
			NULL, tr("ROM Files (*.bin);;All files (*)"));
	if (!fileName.isEmpty()) fileLe->setText(fileName);
}

void FlashWriteTab::Flash(void) {
	uint16_t *wrBuf = NULL;
	uint16_t *rdBuf = NULL;
	uint32_t start = 0;
	uint32_t len = 0;

	if (fileLe->text().isEmpty()) return;
	dlg->tabs->setDisabled(true);
	dlg->progBar->setVisible(true);
	// Create Flash Manager and connect signals to UI control slots
	FlashMan fm;
	connect(&fm, &FlashMan::RangeChanged, dlg->progBar,
			&QProgressBar::setRange);
	connect(&fm, &FlashMan::ValueChanged, dlg->progBar,
			&QProgressBar::setValue);
	connect(&fm, &FlashMan::StatusChanged, dlg->statusLab, &QLabel::setText);

	// Start programming
	wrBuf = fm.Program(fileLe->text().toStdString().c_str(),
			autoCb->isChecked(), &start, &len);
	if (!wrBuf) {
		/// \todo show msg box with error and return
		QMessageBox::warning(this, "Program failed",
				"Cannot program file!");
		dlg->progBar->setVisible(false);
		dlg->tabs->setDisabled(false);
		dlg->statusLab->setText("Done!");
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
			dlg->tabs->setDisabled(false);
			dlg->statusLab->setText("Done!");
			return;
		}
		for (uint32_t i = 0; i < len; i++) {
			if (wrBuf[i] != rdBuf[i]) {
				QString str;
				str.sprintf("Verify failed at pos: 0x%X!", i);
				QMessageBox::warning(this, "Verify failed", str);
			}
		}
	}
	if (wrBuf) fm.BufFree(wrBuf);
	if (rdBuf) fm.BufFree(rdBuf);
	dlg->progBar->setVisible(false);
	dlg->tabs->setDisabled(false);
	disconnect(this, 0, 0, 0);
	dlg->statusLab->setText("Done!");
}

FlashDialog::FlashDialog(void) {
	InitUI();
}

void FlashDialog::InitUI(void) {
	tabs = new QTabWidget;
	tabs->addTab(new FlashWriteTab(this), tr("WRITE"));
	tabs->addTab(new FlashReadTab(this),  tr("READ"));
	tabs->addTab(new FlashEraseTab(this), tr("ERASE"));
	tabs->addTab(new FlashInfoTab(this),  tr("INFO"));

	statusLab = new QLabel("Ready!");
	statusLab->setFixedWidth(60);
	progBar = new QProgressBar;
	progBar->setVisible(false);
	QPushButton *btnQuit = new QPushButton("Exit");
	btnQuit->setDefault(true);

	connect(btnQuit, SIGNAL(clicked()), this, SLOT(close()));

	QHBoxLayout *btnLayout = new QHBoxLayout;
	btnLayout->addWidget(statusLab);
	btnLayout->addWidget(progBar, 1);
//	btnLayout->addStretch(1);
	btnLayout->addWidget(btnQuit);
//	btnLayout->setAlignment(Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(tabs);
	mainLayout->addLayout(btnLayout);

	setLayout(mainLayout);

	setGeometry(0, 0, 350, 300);	
	setWindowTitle("MDMA");
}

