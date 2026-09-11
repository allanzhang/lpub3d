#pragma once

#include <QtWidgets>

class lcAutomateEdgeColorDialog : public QDialog
{
	Q_OBJECT
public:
	lcAutomateEdgeColorDialog(QWidget *Parent, bool ShowHighContrastDialog);
	bool mStudCylinderColorEnabled;
	quint32 mStudCylinderColor;
	bool mPartEdgeColorEnabled;
	quint32 mPartEdgeColor;
	bool mBlackEdgeColorEnabled;
	quint32 mBlackEdgeColor;
	bool mDarkEdgeColorEnabled;
	quint32 mDarkEdgeColor;
	float mPartEdgeContrast;
	float mPartColorValueLDIndex;

protected slots:
	void SliderValueChanged(int);
	void ResetSliderButtonClicked();
	void ColorButtonClicked();
	void ColorCheckBoxClicked();
	void ResetColorButtonClicked();

protected:
	QSlider* PartColorValueLDIndexSlider;
	QSlider* PartEdgeContrastSlider;

	QLabel* PartEdgeContrast;
	QLabel* PartColorValueLDIndex;

	QPushButton* ResetPartEdgeContrastButton;
	QPushButton* ResetPartColorValueLDIndexButton;

	QCheckBox* StudCylinderColorEnabledBox;
	QCheckBox* PartEdgeColorEnabledBox;
	QCheckBox* BlackEdgeColorEnabledBox;
	QCheckBox* DarkEdgeColorEnabledBox;

	QPushButton* StudCylinderColorButton;
	QPushButton* PartEdgeColorButton;
	QPushButton* BlackEdgeColorButton;
	QPushButton* DarkEdgeColorButton;

	QPushButton* ResetStudCylinderColorButton;
	QPushButton* ResetPartEdgeColorButton;
	QPushButton* ResetBlackEdgeColorButton;
	QPushButton* ResetDarkEdgeColorButton;
};

