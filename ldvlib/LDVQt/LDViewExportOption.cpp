/****************************************************************************
**
** Copyright (C) 2018 - 2025 Trevor SANDY. All rights reserved.
**
** This file may be used under the terms of the
** GNU General Public Liceense (GPL) version 3.0
** which accompanies this distribution, and is
** available at http://www.gnu.org/licenses/gpl.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QFileDialog>
#include <QCheckBox>
#include <QShowEvent>
#include <QTimer>
#include <QToolTip>
#include <TCFoundation/TCStringArray.h>
#include <TCFoundation/TCUserDefaults.h>
#include <TCFoundation/TCLocalStrings.h>
#include <TCFoundation/mystring.h>
#include <LDLib/LDUserDefaultsKeys.h>
#include <LDExporter/LDExporter.h>
#include <LDExporter/LDExporterSetting.h>
#include <LDExporter/LDPovExporter.h>
#include <LDVWidgetDefaultKeys.h>
#include <LDVMisc.h>
#include "LDViewExportOption.h"
#include "LDVWidget.h"

#include "ui_LDVExportOptionPanel.h"

#include "lpub_preferences.h"
#include "declarations.h"
#include "commonmenus.h"


static void configureExportLabel(QLabel *label)
{
	label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	label->setWordWrap(true);
	label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

TCStringArray* LDViewExportOption::extraSearchDirs = nullptr;

#define	MAX_EXTRA_DIR	10

LDViewExportOption::LDViewExportOption(LDVWidget *modelWidget, QWidget *parent)
	:QDialog(parent),
	LDVExportOptionPanel(),
	m_modelViewer(modelWidget->getModelViewer() ? ((LDrawModelViewer*)modelWidget->getModelViewer()->retain()) : nullptr),
	m_exporter(nullptr),
	m_box(nullptr),
	m_lay(nullptr)
{
	setupUi(this);

	IniFlag iniFlag = modelWidget->getIniFlag();

	if (m_modelViewer) {
		switch (iniFlag) {
		case NativePOVIni:
		case LDViewPOVIni:
		case POVRayRender:
			m_exporter  = m_modelViewer->getExporter(LDrawModelViewer::ETPov, true);
			break;
		case NativeSTLIni:
			m_exporter  = m_modelViewer->getExporter(LDrawModelViewer::ETStl, true);
			break;
		case Native3DSIni:
			m_exporter  = m_modelViewer->getExporter(LDrawModelViewer::ET3ds, true);
			break;
		default:
			break;
		}
	}

	connect( okButton,     SIGNAL( clicked() ), this, SLOT( doOk() ) );
	connect( applyButton,  SIGNAL( pressed() ), this, SLOT( doApply() ) );
	connect( cancelButton, SIGNAL( clicked() ), this, SLOT( doCancel() ) );
	connect( resetButton,  SIGNAL( clicked() ), this, SLOT( doReset() ) );

	if (!extraSearchDirs) {
		extraSearchDirs = new TCStringArray;
		captureExtraSearchDirs();
	}

	populateExportSettings();

	const QString exportOptionsSuffix =
		QString::fromWCharArray(TCLocalStrings::get(L"ExportOptionsSuffix"));
	this->setWindowTitle(modelWidget->getIniTitle().append(
		exportOptionsSuffix.isEmpty() ? QStringLiteral(" Export Options") : exportOptionsSuffix));

	this->setWhatsThis(lpubWT(WT_DIALOG_LDVIEW_POV_EXPORT_OPTIONS,windowTitle()));

	QPalette readOnlyPalette = QApplication::palette();
	if (modelWidget->getDarkTheme())
		readOnlyPalette.setColor(QPalette::Base,QColor(Preferences::themeColors[THEME_DARK_PALETTE_MIDLIGHT]));
	else
		readOnlyPalette.setColor(QPalette::Base,QColor(Preferences::themeColors[THEME_DEFAULT_PALETTE_LIGHT]));
	readOnlyPalette.setColor(QPalette::Text,QColor(LPUB3D_DISABLED_TEXT_COLOUR));

	scrollArea->setWhatsThis(lpubWT(WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_SCROLL_AREA,tr("Export Options")));
	iniBox->setWhatsThis(lpubWT(WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_INI_FILE,tr("Export INI File")));

	m_lay->addStretch();
	scrollArea->setWidget(m_box);

	QString iniFileMessage;
	if (TCUserDefaults::isIniFileSet()) {
		QString prefSet = TCUserDefaults::getSessionName();
		iniFileMessage = QString("%1").arg(modelWidget->getIniFile());
		const QString iniTitleFormat =
			QString::fromWCharArray(TCLocalStrings::get(L"IniFilePrefSet"));
		const QString defaultSet =
			QString::fromWCharArray(TCLocalStrings::get(L"IniFileDefault"));
		const QString setName = prefSet.isEmpty()
			? (defaultSet.isEmpty() ? QStringLiteral("Default") : defaultSet)
			: prefSet;
		iniBox->setTitle(iniTitleFormat.isEmpty()
			? QStringLiteral("INI file using '%1' preference set").arg(setName)
			: iniTitleFormat.arg(setName));
	} else {
		const QString notSet =
			QString::fromWCharArray(TCLocalStrings::get(L"IniNotSpecifiedNotice"));
		iniFileMessage = notSet.isEmpty()
			? QStringLiteral("INI file not specified. Using built-in default settings.")
			: notSet;
		iniFileEdit->setStyleSheet("QLineEdit { background-color : red; color : white; }");
	}

	iniFileEdit->setReadOnly(true);
	iniFileEdit->setPalette(readOnlyPalette);
	iniFileEdit->setAlignment(Qt::AlignLeft);
	iniFileEdit->setText(iniFileMessage);
	iniFileEdit->setToolTip(iniFileMessage);
	iniFileEdit->setCursorPosition(0);

	applyButton->setEnabled(false);

	// The .ui width is too narrow for the Chinese labels, and locking it
	// together with a disabled horizontal scrollbar clips both labels and the INI path.
	setMinimumWidth(640);
	resize(720, qMax(height(), 640));
	setSizeGripEnabled(true);
}

LDViewExportOption::~LDViewExportOption()
{
}

void LDViewExportOption::show(void)
{
	applyButton->setEnabled(false);
	QDialog::show();
	raise();
	revealIniPathStart();
}

void LDViewExportOption::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	revealIniPathStart();
}

void LDViewExportOption::revealIniPathStart(void)
{
	// QLineEdit keeps the cursor at the end, so a long path looks right-cropped.
	iniFileEdit->setCursorPosition(0);
	iniFileEdit->deselect();
	QTimer::singleShot(0, iniFileEdit, [this]() {
		iniFileEdit->setCursorPosition(0);
		iniFileEdit->deselect();
	});
}

void LDViewExportOption::populateExportSettings(void)
{
	if (!m_exporter)
		return;

	QHash<QString, WT_Type> whatsThisMap;
	auto localName = [](const wchar_t *key) {
		return QString::fromWCharArray(TCLocalStrings::get(key));
	};
	whatsThisMap[localName(L"PovGeneral")] = WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_GENERAL;
	whatsThisMap[localName(L"LDXGeometry")] = WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_GEOMETRY;
	whatsThisMap[localName(L"PovNativeGeometry")] = WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_POV_GEOMETRY;
	whatsThisMap[localName(L"PovLighting")] = WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_LIGHTING;
	whatsThisMap[localName(L"PovLights")] = WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_POVRAY_LIGHTS;
	whatsThisMap[localName(L"PovMaterialProps")] = WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_MATERIAL;
	whatsThisMap[localName(L"PovTransMaterialProps")] = WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_TRANSPARENT_MATERIAL;
	whatsThisMap[localName(L"PovRubberMaterialProps")] = WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_RUBBER_MATERIAL;
	whatsThisMap[localName(L"PovChromeMaterialProps")] = WT_CONTROL_LDVIEW_POV_EXPORT_OPTIONS_CHROME_MATERIAL;

	QWidget *parent;
	parent = m_box;
	LDExporterSettingList &settings = m_exporter->getSettings();
	LDExporterSettingList::iterator it;

	if (m_box != nullptr) {
		scrollArea->adjustSize();
		delete m_box;
	}
	m_box = new QWidget();
	m_box->setObjectName("m_box");
	m_lay = new QVBoxLayout();
	m_lay->setObjectName("m_lay");
#if QT_VERSION < 0x60000
	m_lay->setMargin(11);
#endif
	m_lay->setSpacing(4);
	m_box->setLayout(m_lay);
	QVBoxLayout *vbl= nullptr;
	QSpacerItem *sp = nullptr;
	std::stack<int> groupSizes;
	std::stack<QWidget *> parents;
	int groupSize = 0;
	QVBoxLayout *povLightsLayout = nullptr;
	auto addGroupResetRow = [&](QVBoxLayout *groupLayout) {
		if (!groupLayout)
			return;
		QHBoxLayout *resetRow = new QHBoxLayout();
		resetRow->setSpacing(12);
		QPushButton *rg = new QPushButton(tr("Reset Group"));
		rg->setObjectName("Reset Group");
		resetRow->addWidget(rg);
		connect(rg, SIGNAL(clicked()), this, SLOT(doResetGroup()));
		if (groupLayout == povLightsLayout && m_addPovLightBtn && m_updatePovLightBtn && m_removePovLightBtn) {
			resetRow->addWidget(m_addPovLightBtn);
			resetRow->addWidget(m_updatePovLightBtn);
			resetRow->addWidget(m_removePovLightBtn);
			povLightsLayout = nullptr;
		}
		resetRow->addStretch(1);
		groupLayout->addLayout(resetRow);
	};


	for (it = settings.begin(); it != settings.end(); it++) {
		bool inGroup = groupSize > 0;

		if (groupSize > 0) {
			groupSize--;
			if (groupSize == 0)
			{
				// We just got to the end of a group, so pop the previous
				// groupSize value off the groupSizes stack.
				groupSize = groupSizes.top();
				groupSizes.pop();
				parent = parents.top();
				parents.pop();
			}
		}
		if (it->getGroupSize() > 0) {
			// This item is the start of a group.
			if (inGroup) {
				// At the beginning of this iteration we were in a group, so
				// use a bool setting instead of a group setting.
				QString qstmp;
				ucstringtoqstring(qstmp,it->getName());

				QCheckBox *check;
				check = new QCheckBox(qstmp,parent);
				check->setObjectName(qstmp);
				check->setChecked(it->getBoolValue());
				m_settings[&*it] = check;
				if (vbl) {
					vbl->addWidget(check);
					m_groups[vbl][&*it] = check;
				}
				connect( check, SIGNAL( toggled(bool) ), this, SLOT( enableApply() ) );
			} else {
				// Top level group; use a group setting.
				if (vbl)
					addGroupResetRow(vbl);
				QString qstmp;
				ucstringtoqstring(qstmp,it->getName());
				QGroupBox *gb;
				gb = new QGroupBox (qstmp, m_box);
				gb->setObjectName(qstmp);
				gb->setWhatsThis(lpubWT(whatsThisMap[qstmp],gb->title()));
				m_lay->addWidget(gb);
				vbl = new QVBoxLayout(gb);
				gb->setLayout(vbl);
				parent=gb;
				if (it->getType() == LDExporterSetting::TBool) {
					gb->setCheckable(true);
					gb->setChecked(it->getBoolValue());
					m_settings[&*it] = gb;
					m_groups[vbl][&*it] = gb;
				}
			}
			parents.push(parent);
			// We're now in a new group, so push the current groupSize onto
			// the groupSizes stack.
			groupSizes.push(groupSize);
			// Update groupSize based on the new group's size.
			groupSize = it->getGroupSize();
		} else {
			// This setting isn't the start of a group; add the appropriate type
			// of option UI to the canvas.
			QString qstmp;
			ucstringtoqstring(qstmp,it->getName());
			QHBoxLayout *hbox;
			QVBoxLayout *vbox;
			QLineEdit *li;
			QLabel *label;
			QHBoxLayout *hbox2;
			QCheckBox *check;
			hbox = new QHBoxLayout();
			hbox->setSpacing(4);
			switch (it->getType()) {
			case LDExporterSetting::TBool:
				check = new QCheckBox(qstmp);
				check->setChecked(it->getBoolValue());
				check->setObjectName(qstmp);
				hbox->addWidget(check);
				m_settings[&*it] = check;
				if (vbl != nullptr) {
					m_groups[vbl][&*it] = check;
				}
				connect( check, SIGNAL( toggled(bool) ), this, SLOT( enableApply() ) );
				break;
			case LDExporterSetting::TFloat:
			case LDExporterSetting::TLong:
				// Long and float are intentionally handled the same.
				label = new QLabel(qstmp);
				label->setObjectName(qstmp);
				configureExportLabel(label);
				// Word wrap inside this horizontal row reports a one-line height,
				// so a long Chinese caption is painted on top of itself.
				label->setWordWrap(false);
				label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
				hbox->addWidget(label, 0, Qt::AlignLeft | Qt::AlignVCenter);
				li = new QLineEdit(qstmp);
				li->setObjectName(qstmp);
				li->setMinimumWidth(72);
				li->setMaximumWidth(180);
				li->setAlignment(Qt::AlignLeft);
				hbox->addWidget(li);
				sp = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
				hbox->addSpacerItem(sp);
				ucstringtoqstring(qstmp,it->getStringValue());
				li->setText(qstmp);
				m_settings[&*it] = li;
				if (vbl != nullptr) {
					m_groups[vbl][&*it] = li;
				}
				connect( li, SIGNAL( editingFinished() ), this, SLOT( enableApply() ) );
				break;
			case LDExporterSetting::TString:
				vbox = new QVBoxLayout();
				vbox->setSpacing(4);
				hbox->addLayout(vbox);
				label = new QLabel(qstmp);
				configureExportLabel(label);
				vbox->addWidget(label);
				hbox2 = new QHBoxLayout();
				hbox2->setSpacing(4);
				vbox->addLayout(hbox2);
				li = new QLineEdit(qstmp);
				hbox2->addWidget(li);
				ucstringtoqstring(qstmp,it->getStringValue());
				li->setText(qstmp);
				m_settings[&*it] = li;
				if (vbl != nullptr) {
					m_groups[vbl][&*it] = li;
				}
				if (it->isPath() ||
					  (label->text().contains(TCObject::ls("PovBottomInclude")) ||
					   label->text().contains(TCObject::ls("PovTopInclude")))) {
					QPushButton *but = new QPushButton();
					but->setText(TCObject::ls("LDXBrowse..."));
					hbox2->addWidget(but);
					connect( but, SIGNAL( clicked() ), this, SLOT( doBrowse()));
					m_button[but] = li;
				} else if (label->text() != TCObject::ls("PovLights")) {
					hbox2->addSpacerItem(sp);
				}
				if (label->text() != TCObject::ls("PovLights")) {
					connect( li, SIGNAL( editingFinished() ), this, SLOT( enableApply() ) );
				} else {
					label->hide();
					li->hide();
					li->setObjectName(label->text());
					hbox->setStretch(0, 1);
					povLightsLayout = vbl;

					m_PovLightList = qstmp.split(";");

					QGridLayout *grid = new QGridLayout();
					grid->setHorizontalSpacing(16);
					grid->setVerticalSpacing(4);
					grid->setContentsMargins(0, 4, 0, 8);
					for (int column = 0; column < 4; ++column)
					grid->setColumnStretch(column, 1);
					QWidget *lightForm = new QWidget(this);
					lightForm->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
					lightForm->setLayout(grid);
					vbox->addWidget(lightForm, 1);

					auto expandHorizontally = [](QWidget *widget) {
					QSizePolicy policy = widget->sizePolicy();
					policy.setHorizontalPolicy(QSizePolicy::Expanding);
					policy.setVerticalPolicy(QSizePolicy::Fixed);
					widget->setSizePolicy(policy);
					};
					auto addCaption = [&](const char *key, int captionRow, int captionColumn) {
					QLabel *caption = new QLabel(TCObject::ls(key), this);
					grid->addWidget(caption, captionRow, captionColumn, Qt::AlignLeft | Qt::AlignBottom);
					};
					auto addSection = [&](const char *key, int sectionRow) {
					QLabel *section = new QLabel(TCObject::ls(key), this);
					grid->addWidget(section, sectionRow, 0, 1, 4, Qt::AlignLeft | Qt::AlignVCenter);
					};

					QPalette readOnlyPalette = QApplication::palette();
					if (Preferences::darkTheme)
					readOnlyPalette.setColor(QPalette::Base,QColor(Preferences::themeColors[THEME_DARK_PALETTE_MIDLIGHT]));
					else
					readOnlyPalette.setColor(QPalette::Base,QColor(Preferences::themeColors[THEME_DEFAULT_PALETTE_LIGHT]));
					readOnlyPalette.setColor(QPalette::Text,QColor(LPUB3D_DISABLED_TEXT_COLOUR));

					int row = 0;
					addCaption("PovLightNum", row, 0);
					addCaption("PovLightOptLatitude", row, 1);
					addCaption("PovLightOptLongitude", row, 2);
					addCaption("PovLightOptIntensity", row, 3);

					row = 1;
					m_PovLightNumEdit = new QLineEdit(this);
					m_PovLightNumEdit->setPalette(readOnlyPalette);
					m_PovLightNumEdit->setReadOnly(true);
					expandHorizontally(m_PovLightNumEdit);
					grid->addWidget(m_PovLightNumEdit, row, 0);

					m_PovLightOptLatitudeDSpin = new QDoubleSpinBox(this);
					m_PovLightOptLatitudeDSpin->setRange(-360.0, 360.0);
					m_PovLightOptLatitudeDSpin->setDecimals(1);
					m_PovLightOptLatitudeDSpin->setSingleStep(1);
					m_PovLightOptLatitudeDSpin->setToolTip(TCObject::ls("PovLightLatitudeTT"));
					expandHorizontally(m_PovLightOptLatitudeDSpin);
					grid->addWidget(m_PovLightOptLatitudeDSpin, row, 1);

					m_PovLightOptLongitudeDSpin = new QDoubleSpinBox(this);
					m_PovLightOptLongitudeDSpin->setRange(-360.0, 360.0);
					m_PovLightOptLongitudeDSpin->setDecimals(1);
					m_PovLightOptLongitudeDSpin->setSingleStep(1);
					m_PovLightOptLongitudeDSpin->setToolTip(TCObject::ls("PovLightLongitudeTT"));
					expandHorizontally(m_PovLightOptLongitudeDSpin);
					grid->addWidget(m_PovLightOptLongitudeDSpin, row, 2);

					m_PovLightOptIntensityDSpin = new QDoubleSpinBox(this);
					m_PovLightOptIntensityDSpin->setRange(0.0,100);
					m_PovLightOptIntensityDSpin->setDecimals(2);
					m_PovLightOptIntensityDSpin->setSingleStep(0.1);
					m_PovLightOptIntensityDSpin->setToolTip(TCObject::ls("PovLightIntensityTT"));
					expandHorizontally(m_PovLightOptIntensityDSpin);
					grid->addWidget(m_PovLightOptIntensityDSpin, row, 3);

					row = 3;
					addCaption("PovLightOptType", row, 0);
					addCaption("PovLightOptTargetX", row, 1);
					addCaption("PovLightOptTargetY", row, 2);
					addCaption("PovLightOptTargetZ", row, 3);

					row = 4;
					m_PovLightOptTypeCombo = new QComboBox(this);
					m_PovLightOptTypeCombo->addItems({TCObject::ls("PovLightPoint"),TCObject::ls("PovLightArea"),TCObject::ls("PovLightSun"),TCObject::ls("PovLightSpot")});
					m_PovLightOptTypeCombo->setToolTip(TCObject::ls("PovLightTypeTT"));
					expandHorizontally(m_PovLightOptTypeCombo);
					grid->addWidget(m_PovLightOptTypeCombo, row, 0);

					m_PovLightOptTargetXDSpin = new QDoubleSpinBox(this);
					m_PovLightOptTargetXDSpin->setDecimals(1);
					m_PovLightOptTargetXDSpin->setSingleStep(1);
					m_PovLightOptTargetXDSpin->setToolTip(TCObject::ls("PovLightTargetXTT"));
					expandHorizontally(m_PovLightOptTargetXDSpin);
					grid->addWidget(m_PovLightOptTargetXDSpin, row, 1);

					m_PovLightOptTargetYDSpin = new QDoubleSpinBox(this);
					m_PovLightOptTargetYDSpin->setDecimals(1);
					m_PovLightOptTargetYDSpin->setSingleStep(1);
					m_PovLightOptTargetYDSpin->setToolTip(TCObject::ls("PovLightTargetYTT"));
					expandHorizontally(m_PovLightOptTargetYDSpin);
					grid->addWidget(m_PovLightOptTargetYDSpin, row, 2);

					m_PovLightOptTargetZDSpin = new QDoubleSpinBox(this);
					m_PovLightOptTargetZDSpin->setDecimals(1);
					m_PovLightOptTargetZDSpin->setSingleStep(1);
					m_PovLightOptTargetZDSpin->setToolTip(TCObject::ls("PovLightTargetZTT"));
					expandHorizontally(m_PovLightOptTargetZDSpin);
					grid->addWidget(m_PovLightOptTargetZDSpin, row, 3);

					row = 5;
					grid->setRowMinimumHeight(row, 12);
					row = 6;
					m_PovLightOptShadowlessChk = new QCheckBox(TCObject::ls("PovLightOptShadowless"), this);
					m_PovLightOptShadowlessChk->setToolTip(TCObject::ls("PovLightShadowlessTT"));
					grid->addWidget(m_PovLightOptShadowlessChk, row, 0, 1, 4, Qt::AlignLeft | Qt::AlignVCenter);

					row = 8;
					addCaption("PovLightOptColor", row, 0);
					addCaption("PovLightOptFadeDistance", row, 1);
					addCaption("PovLightOptFadePower", row, 2);

					row = 9;
					m_PovLightOptColorBtn = new QPushButton(this);
					m_PovLightOptColorBtn->setToolTip(TCObject::ls("PovLightColorTT"));
					expandHorizontally(m_PovLightOptColorBtn);
					grid->addWidget(m_PovLightOptColorBtn, row, 0);

					m_PovLightOptFadeDistanceDSpin = new QDoubleSpinBox(this);
					m_PovLightOptFadeDistanceDSpin->setRange(0.0, 1.0);
					m_PovLightOptFadeDistanceDSpin->setDecimals(1);
					m_PovLightOptFadeDistanceDSpin->setSingleStep(1);
					m_PovLightOptFadeDistanceDSpin->setToolTip(TCObject::ls("PovLightFadeDistanceTT"));
					expandHorizontally(m_PovLightOptFadeDistanceDSpin);
					grid->addWidget(m_PovLightOptFadeDistanceDSpin, row, 1);

					m_PovLightOptFadePowerDSpin = new QDoubleSpinBox(this);
					m_PovLightOptFadePowerDSpin->setRange(0.0, 1.0);
					m_PovLightOptFadePowerDSpin->setDecimals(1);
					m_PovLightOptFadePowerDSpin->setSingleStep(1);
					m_PovLightOptFadePowerDSpin->setToolTip(TCObject::ls("PovLightFadePowerTT"));
					expandHorizontally(m_PovLightOptFadePowerDSpin);
					grid->addWidget(m_PovLightOptFadePowerDSpin, row, 2);

					row = 10;
					grid->setRowMinimumHeight(row, 12);
					row = 11;
					addSection("PovLightSpotOptionsLbl", row);

					row = 12;
					addCaption("PovLightOptSpotRadius", row, 0);
					addCaption("PovLightOptSpotFalloff", row, 1);
					addCaption("PovLightOptSpotTightness", row, 2);

					row = 13;
					m_PovLightOptSpotRadiusDSpin = new QDoubleSpinBox(this);
					m_PovLightOptSpotRadiusDSpin->setDecimals(1);
					m_PovLightOptSpotRadiusDSpin->setSingleStep(1);
					m_PovLightOptSpotRadiusDSpin->setToolTip(TCObject::ls("PovLightSpotRadiusTT"));
					expandHorizontally(m_PovLightOptSpotRadiusDSpin);
					grid->addWidget(m_PovLightOptSpotRadiusDSpin, row, 0);

					m_PovLightOptSpotFalloffDSpin = new QDoubleSpinBox(this);
					m_PovLightOptSpotFalloffDSpin->setRange(-180.0, 180.0);
					m_PovLightOptSpotFalloffDSpin->setDecimals(1);
					m_PovLightOptSpotFalloffDSpin->setSingleStep(1);
					m_PovLightOptSpotFalloffDSpin->setToolTip(TCObject::ls("PovLightSpotFalloffTT"));
					expandHorizontally(m_PovLightOptSpotFalloffDSpin);
					grid->addWidget(m_PovLightOptSpotFalloffDSpin, row, 1);

					m_PovLightOptSpotTightnessDSpin = new QDoubleSpinBox(this);
					m_PovLightOptSpotTightnessDSpin->setDecimals(1);
					m_PovLightOptSpotTightnessDSpin->setSingleStep(1);
					m_PovLightOptSpotTightnessDSpin->setToolTip(TCObject::ls("PovLightSpotTightnessTT"));
					expandHorizontally(m_PovLightOptSpotTightnessDSpin);
					grid->addWidget(m_PovLightOptSpotTightnessDSpin, row, 2);

					row = 14;
					grid->setRowMinimumHeight(row, 12);
					row = 15;
					addSection("PovLightAreaOptionsLbl", row);

					row = 16;
					addCaption("PovLightOptAreaWidth", row, 0);
					addCaption("PovLightOptAreaHeight", row, 1);

					row = 17;
					m_PovLightOptAreaWidthSpin = new QSpinBox(this);
					m_PovLightOptAreaWidthSpin->setRange(0,10000);
					m_PovLightOptAreaWidthSpin->setSingleStep(1);
					m_PovLightOptAreaWidthSpin->setToolTip(TCObject::ls("PovLightAreaWidthTT"));
					expandHorizontally(m_PovLightOptAreaWidthSpin);
					grid->addWidget(m_PovLightOptAreaWidthSpin, row, 0);

					m_PovLightOptAreaHeightSpin = new QSpinBox(this);
					m_PovLightOptAreaHeightSpin->setRange(0,10000);
					m_PovLightOptAreaHeightSpin->setSingleStep(1);
					m_PovLightOptAreaHeightSpin->setToolTip(TCObject::ls("PovLightAreaHeightTT"));
					expandHorizontally(m_PovLightOptAreaHeightSpin);
					grid->addWidget(m_PovLightOptAreaHeightSpin, row, 1);

					m_PovLightOptAreaCircleChk = new QCheckBox(TCObject::ls("PovLightOptAreaCircle"), this);
					m_PovLightOptAreaCircleChk->setToolTip(TCObject::ls("PovLightAreaCircleTT"));
					grid->addWidget(m_PovLightOptAreaCircleChk, row, 2, Qt::AlignLeft | Qt::AlignVCenter);

					row = 18;
					grid->setRowMinimumHeight(row, 6);
					row = 19;
					addCaption("PovLightOptAreaRows", row, 0);
					addCaption("PovLightOptAreaColumns", row, 1);

					row = 20;
					m_PovLightOptAreaRowsSpin = new QSpinBox(this);
					m_PovLightOptAreaRowsSpin->setRange(0,1000);
					m_PovLightOptAreaRowsSpin->setSingleStep(1);
					m_PovLightOptAreaRowsSpin->setToolTip(TCObject::ls("PovLightAreaRowsTT"));
					expandHorizontally(m_PovLightOptAreaRowsSpin);
					grid->addWidget(m_PovLightOptAreaRowsSpin, row, 0);

					m_PovLightOptAreaColumnsSpin = new QSpinBox(this);
					m_PovLightOptAreaColumnsSpin->setRange(0,1000);
					m_PovLightOptAreaColumnsSpin->setSingleStep(1);
					m_PovLightOptAreaColumnsSpin->setToolTip(TCObject::ls("PovLightAreaColumnsTT"));
					expandHorizontally(m_PovLightOptAreaColumnsSpin);
					grid->addWidget(m_PovLightOptAreaColumnsSpin, row, 1);

					row = 21;
					grid->setRowMinimumHeight(row, 12);
					row = 22;
					m_PovLightCombo = new QComboBox(this);
					m_PovLightCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
					m_PovLightCombo->setMinimumContentsLength(24);
					m_PovLightCombo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
					grid->addWidget(m_PovLightCombo, row, 0, 1, 4);

					row = 23;
					m_messageLabel = new QLabel(this);
					m_messageLabel->setWordWrap(true);
					grid->addWidget(m_messageLabel, row, 0, 1, 4);

					m_addPovLightBtn = new QPushButton(this);
					m_addPovLightBtn->setText(TCObject::ls("PovLightAdd"));
					m_addPovLightBtn->setToolTip(TCObject::ls("PovLightAddTT"));
					m_addPovLightBtn->setEnabled(false);

					m_updatePovLightBtn = new QPushButton(this);
					m_updatePovLightBtn->setText(TCObject::ls("PovLightUpdate"));
					m_updatePovLightBtn->setToolTip(TCObject::ls("PovLightUpdateTT"));
					m_updatePovLightBtn->setEnabled(false);

					m_removePovLightBtn = new QPushButton(this);
					m_removePovLightBtn->setText(TCObject::ls("PovLightRemove"));
					m_removePovLightBtn->setToolTip(TCObject::ls("PovLightRemoveTT"));
					m_removePovLightBtn->setEnabled(false);
					connect( m_addPovLightBtn, SIGNAL( clicked() ), this, SLOT( addLight()));
					connect( m_updatePovLightBtn, SIGNAL( clicked() ), this, SLOT( updateLight()));
					connect( m_removePovLightBtn, SIGNAL( clicked() ), this, SLOT( removeLight()));

					setLights();

					connect( m_PovLightOptColorBtn, &QPushButton::clicked, this, &LDViewExportOption::colorButtonClicked);
					connect( m_PovLightCombo, SIGNAL( currentIndexChanged(int) ), this, SLOT( selectLight(int)));
					connect( m_PovLightOptIntensityDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptLatitudeDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptLongitudeDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptTypeCombo, SIGNAL( currentIndexChanged(int) ), this, SLOT( setLights(int)));
					connect( m_PovLightOptTargetXDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptTargetYDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptTargetZDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptShadowlessChk, SIGNAL( clicked(bool) ), this, SLOT( applyLights() ) );
					connect( m_PovLightOptFadeDistanceDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptFadePowerDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptSpotRadiusDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptSpotFalloffDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptSpotTightnessDSpin, SIGNAL( valueChanged(double) ), this, SLOT( setLights(double) ) );
					connect( m_PovLightOptAreaWidthSpin, SIGNAL( valueChanged(int) ), this, SLOT( setLights(int) ) );
					connect( m_PovLightOptAreaHeightSpin, SIGNAL( valueChanged(int) ), this, SLOT( setLights(int) ) );
					connect( m_PovLightOptAreaCircleChk, SIGNAL( clicked(bool) ), this, SLOT( applyLights() ) );
					connect( m_PovLightOptAreaRowsSpin, SIGNAL( valueChanged(int) ), this, SLOT( setLights(int)));
					connect( m_PovLightOptAreaColumnsSpin, SIGNAL( valueChanged(int) ), this, SLOT( setLights(int)));
				}
				break;
			case LDExporterSetting::TEnum:
				vbox = new QVBoxLayout();
				vbox->setSpacing(4);
				hbox->addLayout(vbox, 1);
				label = new QLabel(qstmp);
				configureExportLabel(label);
				vbox->addWidget(label);
				QComboBox *combo;
				combo = new QComboBox();
				combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
				combo->setMinimumContentsLength(12);
				combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
				vbox->addWidget(combo);
				for (size_t i = 0; i < it->getOptions().size(); i++) {
					ucstringtoqstring(qstmp,it->getOptions()[i]);
					combo->addItem(qstmp);
				}
				combo->setCurrentIndex(it->getSelectedOption());
				m_settings[&*it] = combo;
				if (vbl != nullptr) {
					m_groups[vbl][&*it] = combo;
				}
				connect( combo, SIGNAL( currentIndexChanged(int) ), this, SLOT( enableApply() ) );
				break;
			default:
				throw "not implemented";
			}
			if (it->getTooltip().size() > 0) {
				ucstringtoqstring(qstmp, it->getTooltip());
				//QToolTip::add(hbox, qstmp);
			}
			if (vbl) vbl->addLayout(hbox);
		}
	} // Settings

	if (vbl)
		addGroupResetRow(vbl);
}

void LDViewExportOption::enableApply(void)
{
	applyButton->setEnabled(true);
}

void LDViewExportOption::doApply(void)
{
	SettingsMap::const_iterator it;
	ucstring value;

	for (it = m_settings.begin(); it != m_settings.end(); it++) {
		LDExporterSetting *setting = it->first;

		switch (setting->getType())
		{
		case LDExporterSetting::TBool:
			if (strcmp(it->second->metaObject()->className(), "QGroupBox") == 0) {
				setting->setValue(((QGroupBox *)it->second)->isChecked(), true);
			} else {
				setting->setValue(((QCheckBox *)it->second)->isChecked(), true);
			}
			break;
		case LDExporterSetting::TLong:
		case LDExporterSetting::TFloat:
		case LDExporterSetting::TString:
			if (((QLineEdit *)it->second)->objectName() == TCObject::ls("PovLights")) {
				qstringtoucstring(value, m_PovLightList.join(";"));
			} else {
				qstringtoucstring(value, ((QLineEdit *)it->second)->text());
			}
			setting->setValue(value.c_str(), true);
			break;
		case LDExporterSetting::TEnum:
			setting->selectOption(((QComboBox *)it->second)->currentIndex(), true);
			break;
		default:
			// No default, but gets rid of warnings.
			break;
		}
	}
	applyExtraSearchDirs();
	TCUserDefaults::flush();
	applyButton->setEnabled(false);
}

void LDViewExportOption::doOk(void)
{
	doApply();
	QDialog::accept();
	QDialog::close();
}

void LDViewExportOption::doCancel(void)
{
	QDialog::reject();
	QDialog::close();
}

void LDViewExportOption::doReset(void)
{
	resetSettings(m_settings);
	captureExtraSearchDirs();
	enableApply();
}

void LDViewExportOption::doResetGroup()
{
	QPushButton *pb = qobject_cast<QPushButton *>(sender());
	resetSettings(m_groups[pb->parentWidget()->layout()]);
	enableApply();
}

void LDViewExportOption::resetSettings(SettingsMap &settings)
{
	SettingsMap::const_iterator it;
	QString value;

	for (it = settings.begin(); it != settings.end(); it++) {
		LDVExportSetting *setting = (LDVExportSetting*)it->first;
		bool povLights = ((QLineEdit *)it->second)->objectName() == TCObject::ls("PovLights");
		if (povLights) {
			setting->resetLights();
		} else {
			setting->reset();
		}

		switch (setting->getType())
		{
		case LDExporterSetting::TBool:
			if (strcmp(it->second->metaObject()->className(), "QGroupBox") == 0) {
				((QGroupBox *)it->second)->setChecked(setting->getBoolValue());
			} else {
				((QCheckBox *)it->second)->setChecked(setting->getBoolValue());
			}
			break;
		case LDExporterSetting::TLong:
		case LDExporterSetting::TFloat:
		case LDExporterSetting::TString:
			ucstringtoqstring(value, setting->getStringValue());
			if (povLights) {
				 m_PovLightList = value.split(";");
				 setLights();
			 } else {
				 ((QLineEdit *)it->second)->setText(value);
			 }
			break;
		case LDExporterSetting::TEnum:
			((QComboBox *)it->second)->setCurrentIndex(
				setting->getSelectedOption());
			break;
		default:
			// No default, but gets rid of warnings.
			break;
		}
	}
}

void LDViewExportOption::doBrowse()
{
	QPushButton *pb = qobject_cast<QPushButton *>(sender());
	QString dir;
	dir = QFileDialog::getOpenFileName(this,"",m_button[pb]->text());
	if (!dir.isEmpty()) {
		m_button[pb]->setText (dir);
		enableApply();
	}
}

void LDViewExportOption::applyExtraSearchDirs(void)
{
	int i;
	int count = extraSearchDirs->getCount();

	for (i = 0; i <= count; i++) {
		char key[128];
		char *extraDir;

		sprintf(key, "%s/Dir%03d", EXTRA_SEARCH_DIRS_KEY, i + 1);
		extraDir = extraSearchDirs->stringAtIndex(i);
		if (extraDir) {
			TCUserDefaults::setStringForKey(extraDir, key, false);
		} else {
			TCUserDefaults::removeValue(key, false);
		}
	}

	for (i=count; i<MAX_EXTRA_DIR;i++) {
		char key[128];
		char extraDir[]="";

		sprintf(key, "%s/Dir%03d", EXTRA_SEARCH_DIRS_KEY, i + 1);
		TCUserDefaults::setStringForKey(extraDir, key, false);
	}

	if (m_modelViewer) {
		m_modelViewer->setExtraSearchDirs(extraSearchDirs);
	}
}

void LDViewExportOption::captureExtraSearchDirs(void)
{
	int i;

	extraSearchDirs->removeAll();
	for (i = 1; true; i++) {
		char key[128];
		char *extraSearchDir;

		sprintf(key, "%s/Dir%03d", EXTRA_SEARCH_DIRS_KEY, i);
		extraSearchDir = TCUserDefaults::stringForKey(key, nullptr, false);
		if (extraSearchDir && extraSearchDir[0]) {
			extraSearchDirs->addString(extraSearchDir);
			delete extraSearchDir;
		} else {
			break;
		}
	}
}

// Lights

void LDViewExportOption::setLights(void)
{
	m_PovLightCombo->clear();
	m_PovLightMap.clear();
	int lightIndex = 0;

	for (; lightIndex < m_PovLightList.size(); lightIndex++) {
		PovLight light(m_PovLightList.at(lightIndex));
		if (light.isvalid) {
			m_PovLightCombo->addItem(QString("%1. %2").arg(lightIndex+1).arg(light.itemText));
			m_PovLightCombo->setItemData(lightIndex, QString("%1%2").arg(light.latitude).arg(light.longitude));
			m_PovLightMap.insert(lightIndex, light);
		}
	}

	if (m_PovLightMap.isEmpty()) {
		m_PovLightMap.insert(0, PovLight(TCObject::ls("PovLightInitializer")));
		m_PovLightCombo->addItem(TCObject::ls("PovLightNoneMsg"));
		m_PovLightCombo->setItemData(0, QString("0.00.0"));
	}

	m_PovLightCombo->setCurrentIndex(0);

	selectLight(0);

	m_addPovLightBtn->setEnabled(false);
	m_updatePovLightBtn->setEnabled(false);
	m_removePovLightBtn->setEnabled(true);
}

void LDViewExportOption::selectLight(int lightIndex)
{
	if (!m_PovLightMap.contains(lightIndex))
		return;

	auto setButtonPixmap = [&](float *color, QPushButton* button)
	{
		QPixmap pixmap(12, 12);
		QColor btnColor = QColor(color[0] * 255, color[1] * 255, color[2] * 255);
		button->setToolTip(tr("Name: %7 R(%1,%2) G(%3,%4) B(%5,%6)")
							   .arg(btnColor.red()).arg(btnColor.red() / 255.0f,0,'f',3)
							   .arg(btnColor.green()).arg(btnColor.green() / 255.0f,0,'f',3)
							   .arg(btnColor.blue()).arg(btnColor.blue() / 255.0f,0,'f',3)
							   .arg(btnColor.name().toUpper()));
		pixmap.fill(btnColor);
		button->setIcon(pixmap);
	};

	bool isArealight = m_PovLightMap[lightIndex].type == LightData::Area;
	bool isSpotlight = m_PovLightMap[lightIndex].type == LightData::Spot;
	int typeIndex = m_PovLightOptTypeCombo->findText(m_PovLightMap[lightIndex].typeText);

	m_PovLightNumEdit->setText(QString::number(lightIndex+1));
	m_PovLightOptTypeCombo->setCurrentIndex(typeIndex);
	m_PovLightOptShadowlessChk->setChecked(m_PovLightMap[lightIndex].shadowless);
	m_PovLightOptLatitudeDSpin->setValue(m_PovLightMap[lightIndex].latitude);
	m_PovLightOptLongitudeDSpin->setValue(m_PovLightMap[lightIndex].longitude);

	m_PovLightOptTargetXDSpin->setValue(m_PovLightMap[lightIndex].target[0]);
	m_PovLightOptTargetYDSpin->setValue(m_PovLightMap[lightIndex].target[1]);
	m_PovLightOptTargetZDSpin->setValue(m_PovLightMap[lightIndex].target[2]);

	setButtonPixmap(m_PovLightMap[lightIndex].color, m_PovLightOptColorBtn);

	m_PovLightOptIntensityDSpin->setValue(m_PovLightMap[lightIndex].intensity);
	m_PovLightOptFadeDistanceDSpin->setValue(m_PovLightMap[lightIndex].fadeDistance);
	m_PovLightOptFadePowerDSpin->setValue(m_PovLightMap[lightIndex].fadePower);
	m_PovLightOptSpotRadiusDSpin->setValue(m_PovLightMap[lightIndex].radius);
	m_PovLightOptSpotFalloffDSpin->setValue(m_PovLightMap[lightIndex].falloff);
	m_PovLightOptSpotTightnessDSpin->setValue(m_PovLightMap[lightIndex].tightness);
	m_PovLightOptAreaCircleChk->setChecked(m_PovLightMap[lightIndex].circle);
	m_PovLightOptAreaWidthSpin->setValue(m_PovLightMap[lightIndex].width);
	m_PovLightOptAreaHeightSpin->setValue(m_PovLightMap[lightIndex].height);
	m_PovLightOptAreaRowsSpin->setValue(m_PovLightMap[lightIndex].rows);
	m_PovLightOptAreaColumnsSpin->setValue(m_PovLightMap[lightIndex].columns);

	m_PovLightOptSpotRadiusDSpin->setEnabled(isSpotlight);
	m_PovLightOptSpotFalloffDSpin->setEnabled(isSpotlight);
	m_PovLightOptSpotTightnessDSpin->setEnabled(isSpotlight);
	m_PovLightOptAreaCircleChk->setEnabled(isArealight);
	m_PovLightOptAreaWidthSpin->setEnabled(isArealight);
	m_PovLightOptAreaHeightSpin->setEnabled(isArealight);
	m_PovLightOptAreaRowsSpin->setEnabled(isArealight);
	m_PovLightOptAreaColumnsSpin->setEnabled(isArealight);
}

PovLight LDViewExportOption::getLight() const
{
	PovLight light(TCObject::ls("PovLightInitializer"));
	QString target = QString("%1,%2,%3").arg(m_PovLightOptTargetXDSpin->value()).arg(m_PovLightOptTargetYDSpin->value()).arg(m_PovLightOptTargetZDSpin->value());
	int lightIndex = m_PovLightCombo->currentIndex();

	if (m_PovLightMap.contains(lightIndex) && m_PovLightMap[lightIndex].isvalid ) {
		QColor color = QColor(m_PovLightMap[lightIndex].color[0] * 255.0f,
							  m_PovLightMap[lightIndex].color[1] * 255.0f,
							  m_PovLightMap[lightIndex].color[2] * 255.0f);
		if (color.isValid()) {
			light.color[0] = double(color.red() / 255.0f);
			light.color[1] = double(color.green() / 255.0f);
			light.color[2] = double(color.blue() / 255.0f);
		}
	}

	light.typeText = m_PovLightOptTypeCombo->currentText();
	light.shadowless = m_PovLightOptShadowlessChk->isChecked();
	light.latitude = m_PovLightOptLatitudeDSpin->value();
	light.longitude = m_PovLightOptLongitudeDSpin->value();
	if (!light.parseString(target.toLatin1().constData(), light.target)) {
		light.target[0] = 0;
		light.target[1] = 0;
		light.target[2] = 0;
	}
	light.intensity = m_PovLightOptIntensityDSpin->value();
	light.fadeDistance = m_PovLightOptFadeDistanceDSpin->value();
	light.fadePower = m_PovLightOptFadePowerDSpin->value();
	light.radius = m_PovLightOptSpotRadiusDSpin->value();
	light.falloff = m_PovLightOptSpotFalloffDSpin->value();
	light.tightness = m_PovLightOptSpotTightnessDSpin->value();
	light.circle = m_PovLightOptAreaCircleChk->isChecked();
	light.width = m_PovLightOptAreaWidthSpin->value();
	light.height = m_PovLightOptAreaHeightSpin->value();
	light.rows = m_PovLightOptAreaRowsSpin->value();
	light.columns = m_PovLightOptAreaColumnsSpin->value();
	light.setTypeInt();
	light.setTypeText();
	light.setItemText();

	return light;
}

void LDViewExportOption::updateLight()
{
	PovLight light = getLight();
	if (light.isvalid) {
		int lightIndex = m_PovLightCombo->currentIndex();
		if (m_PovLightMap.contains(lightIndex)) {
			m_PovLightMap[lightIndex] = light;
			m_PovLightList.replace(lightIndex, light.getLightString());
			m_PovLightCombo->setItemText(lightIndex, QString("%1. %2").arg(lightIndex+1).arg(light.itemText));
			m_PovLightCombo->setItemData(lightIndex, QString("%1%2").arg(light.latitude).arg(light.longitude));
			enableApply();
		}
	}
}

void LDViewExportOption::addLight()
{
	const QString lightData = QString("%1%2").arg(m_PovLightOptLatitudeDSpin->value()).arg(m_PovLightOptLongitudeDSpin->value());

	if (m_PovLightCombo->findData(lightData) != -1) {
		m_messageLabel->setText(TCObject::ls("PovLightUniquePosMsg"));
		m_messageLabel->setStyleSheet("QLabel { color : red; }");
	} else {
		int lightIndex = m_PovLightCombo->count();
		PovLight light = getLight();
		if (light.isvalid) {
			m_PovLightCombo->addItem(QString("%1. %2").arg(lightIndex+1).arg(light.itemText));
			m_PovLightCombo->setItemData(lightIndex, QString("%1%2").arg(light.latitude).arg(light.longitude));
			m_PovLightMap.insert(lightIndex, light);
			m_PovLightList.append(QString("%1%2").arg(m_PovLightList.size() ? ";" : "").arg(light.getLightString()));
			selectLight(lightIndex);
			m_PovLightCombo->setCurrentIndex(lightIndex);
			m_addPovLightBtn->setEnabled(false);
			m_updatePovLightBtn->setEnabled(false);
			m_removePovLightBtn->setEnabled(m_PovLightCombo->count());
			enableApply();
		}
	}
}

void LDViewExportOption::removeLight(void)
{
	if (!m_PovLightList.size())
		return;

	bool addBtnEnabled = m_addPovLightBtn->isEnabled();
	bool updateBtnEnabled = m_updatePovLightBtn->isEnabled();
	bool removeBtnEnabled = m_removePovLightBtn->isEnabled();

	if (m_PovLightList.size()) {
		int lightIndex = m_PovLightCombo->currentIndex();
		if (m_PovLightCombo->count() > lightIndex)
			m_PovLightCombo->removeItem(lightIndex);
		if (m_PovLightList.size() > lightIndex )
			m_PovLightList.removeAt(lightIndex);
		for (auto &key : m_PovLightMap.keys()) {
			lightIndex = m_PovLightNumEdit->displayText().toInt()-1;
			if (key == lightIndex) {
				m_PovLightMap.remove(lightIndex);
				break;
			}
		}
		selectLight(lightIndex+1);
	}

	if (!m_PovLightList.size()) {
		m_PovLightCombo->addItem(TCObject::ls("PovLightNoneMsg"));
		m_PovLightCombo->setItemData(0, QString("0.00.0"));
		m_PovLightMap.insert(0, PovLight(TCObject::ls("PovLightInitializer")));
		selectLight(0);
		m_messageLabel->setText(TCObject::ls("PovLightRemoveMsg"));
		m_messageLabel->setStyleSheet("QLabel { color : red; }");
		addBtnEnabled = updateBtnEnabled = removeBtnEnabled = false;
	}

	m_addPovLightBtn->setEnabled(addBtnEnabled);
	m_updatePovLightBtn->setEnabled(updateBtnEnabled);
	m_removePovLightBtn->setEnabled(removeBtnEnabled);

	enableApply();
}

void LDViewExportOption::applyLights(void)
{
	m_messageLabel->clear();
	m_addPovLightBtn->setEnabled(true);
	m_updatePovLightBtn->setEnabled(true);
	m_removePovLightBtn->setEnabled(true);
}

void LDViewExportOption::applyLights(bool)
{
	applyLights();
}

void LDViewExportOption::setLights(double)
{
	applyLights();
}

void LDViewExportOption::setLights(int)
{
	applyLights();
}

void LDViewExportOption::colorButtonClicked()
{
	QObject* button = sender();
	QString title;
	QColorDialog::ColorDialogOptions DialogOptions;

	if (button != m_PovLightOptColorBtn)
		return;

	int lightIndex = m_PovLightCombo->currentIndex();
	if (!m_PovLightMap.contains(lightIndex) || !m_PovLightMap[lightIndex].isvalid )
		return;

	title = tr("Select Light Color");

	QColor oldColor = QColor(m_PovLightMap[lightIndex].color[0] * 255.0f,
							 m_PovLightMap[lightIndex].color[1] * 255.0f,
							 m_PovLightMap[lightIndex].color[2] * 255.0f);
	QColor newColor = QColorDialog::getColor(oldColor, this, title, DialogOptions);

	if (newColor == oldColor || !newColor.isValid())
		return;

	float color[3] = {color[0] = double(newColor.red() / 255.0f),
					  color[1] = double(newColor.green() / 255.0f),
					  color[2] = double(newColor.blue() / 255.0f)};

	m_PovLightMap[lightIndex].color[0] = double(color[0]);
	m_PovLightMap[lightIndex].color[1] = double(color[1]);
	m_PovLightMap[lightIndex].color[2] = double(color[2]);

	QPixmap pix(12, 12);
	newColor.setAlpha(255);
	pix.fill(newColor);
	((QPushButton*)button)->setIcon(pix);
	((QPushButton*)button)->setToolTip(tr("Name: %7 R(%1,%2) G(%3,%4) B(%5,%6)")
											 .arg(newColor.red()).arg(color[0],0,'f',3)
											 .arg(newColor.green()).arg(color[1],0,'f',3)
											 .arg(newColor.blue()).arg(color[2],0,'f',3)
											 .arg(newColor.name().toUpper()));
}

void LDVExportSetting::resetLights()
{
	std::string value = TCUserDefaults::defaultStringForKey(m_key.c_str());
	stringtowstring(m_string,value);
}

void PovLight::setTypeInt()
{
	if (typeText == TCObject::ls("PovLightPoint"))
		type = LightData::Point;
	else if (typeText == TCObject::ls("PovLightArea"))
		type = LightData::Area;
	else if (typeText == TCObject::ls("PovLightSun"))
		type = LightData::Sun;
	else if (typeText == TCObject::ls("PovLightSpot"))
		type = LightData::Spot;
}

void PovLight::setTypeText()
{
	if (type == LightData::Point)
		typeText = TCObject::ls("PovLightPoint");
	else if (type == LightData::Area)
		typeText = TCObject::ls("PovLightArea");
	else if (type == LightData::Sun)
		typeText = TCObject::ls("PovLightSun");
	else if (type == LightData::Spot)
		typeText = TCObject::ls("PovLightSpot");
}

void PovLight::setItemText()
{
	itemText = QString("%1 %2 %3 %4 %5 %6")
				   .arg(TCObject::ls("PovLightOptType")).arg(typeText)
				   .arg(TCObject::ls("PovLightOptLatitude")).arg(latitude)
				   .arg(TCObject::ls("PovLightOptLongitude")).arg(longitude);

}

bool PovLight::parseString(const char *in, float *out)
{
	QString tmp = QString(in).replace("<","").replace(">","");
	QStringList list = tmp.split(",");
	bool ok[3];
	if (list.size() == 3) {
		out[0] = list.at(0).toDouble(&ok[0]);
		out[1] = list.at(1).toDouble(&ok[1]);
		out[2] = list.at(2).toDouble(&ok[2]);
		isvalid &= (ok[0] && ok[1] && ok[2]);
	}
	return isvalid;
}

QString PovLight::getLightString()
{
	char buffer[512];
	if (isvalid)
		snprintf(
			buffer,
			sizeof(buffer),
			"%i %i %g %g <%g,%g,%g> <%g,%g,%g> %g %g %g %g %g %g %i %i %i %i %i",
			type, shadowless, latitude, longitude,target[0],target[1],target[2],color[0],color[1],color[2],
			intensity, fadeDistance, fadePower, radius, falloff, tightness, circle, width, height, rows, columns);
	//qDebug().noquote() << QString("Get Light String (%1)").arg(QString(buffer).trimmed());
	return QString(buffer).trimmed();
}

PovLight::PovLight(const QString &lightString)
	: isvalid(false)
{
	char _target[100];
	char _color[100];
	int items = sscanf(lightString.toLatin1().data(), "%d %d %f %f %s %s %f %f %f %f %f %f %d %d %d %d %d",
					  &type, &shadowless, &latitude, &longitude, _target, _color, &intensity, &fadeDistance,
					  &fadePower, &radius, &falloff, &tightness, &circle, &width, &height, &rows, &columns);
	if ((isvalid = items == 17)) {
		//qDebug().noquote() << QString("PovLight Light (%1)").arg(lightString);
		setTypeText();
		setItemText();
		parseString(_target, target);
		parseString(_color, color);
	}
}
