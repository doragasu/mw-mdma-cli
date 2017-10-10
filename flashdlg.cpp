#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>

#include "flashdlg.h"

#include "flash_man.h"


FlashInfoTab::FlashInfoTab(void) {
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

FlashEraseTab::FlashEraseTab(void) {

	InitUI();
}

void FlashEraseTab::InitUI(void) {
}

FlashReadTab::FlashReadTab(void) {
	InitUI();
}

void FlashReadTab::InitUI(void) {
	// Create widgets
	QLabel *romLab = new QLabel("Read from cart to ROM:");	
	fileLe = new QLineEdit;
	QPushButton *fOpenBtn = new QPushButton("...");
	fOpenBtn->setFixedWidth(30);
	QPushButton *readBtn = new QPushButton("Read!");

	// Connect signals to slots
	connect(fOpenBtn, SIGNAL(clicked()), this, SLOT(ShowFileDialog()));
	connect(readBtn, SIGNAL(clicked()), this, SLOT(Read()));

	// Configure layout
	QHBoxLayout *fileLayout = new QHBoxLayout;
	fileLayout->addWidget(fileLe);
	fileLayout->addWidget(fOpenBtn);

	QHBoxLayout *statLayout = new QHBoxLayout;
	statLayout->addWidget(readBtn);
	statLayout->setAlignment(Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(romLab);
	mainLayout->addLayout(fileLayout);
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
	setDisabled(true);
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
	uint16_t *buf;
	if (fileLe->text().isEmpty()) return;
	dlg->tabs->setDisabled(true);
	FlashMan *fm = new FlashMan();
	connect(fm, &FlashMan::RangeChanged, dlg->progBar,
			&QProgressBar::setRange);
	dlg->progBar->setVisible(true);

	buf = fm->Program(fileLe->text().toStdString().c_str(),
			autoCb->isChecked());
	if (!buf) {
		/// \todo show msg box with error and return
		dlg->progBar->setVisible(false);
		dlg->tabs->setDisabled(false);
		return;
	}
	if (verifyCb->isChecked()) {
	}
}

FlashDialog::FlashDialog(void) {
	InitUI();
}

void FlashDialog::InitUI(void) {
	tabs = new QTabWidget;
	tabs->addTab(new FlashWriteTab(this), tr("WRITE"));
	tabs->addTab(new FlashReadTab,  tr("READ"));
	tabs->addTab(new FlashEraseTab, tr("ERASE"));
	tabs->addTab(new FlashInfoTab,  tr("INFO"));
	progBar = new QProgressBar;
	progBar->setVisible(false);
	QPushButton *btnQuit = new QPushButton("Exit");
	btnQuit->setDefault(true);

	QHBoxLayout *btnLayout = new QHBoxLayout;
	btnLayout->addWidget(progBar);
	btnLayout->addWidget(btnQuit);
	btnLayout->setAlignment(Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(tabs);
	mainLayout->addLayout(btnLayout);

	setLayout(mainLayout);

	setGeometry(300, 300, 350, 300);	
	setWindowTitle("MDMA");
}
