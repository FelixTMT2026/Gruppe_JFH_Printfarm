// Farbwelt (Stylesheet + Palette) fuer Light- und Darkmode aus den Layout-Entwuerfen.
#pragma once


#include <QString>
#include <QPalette>
#include <QColor>


inline QString dunklesStylesheet()
{
    return QStringLiteral(
        "QMainWindow, QDialog { background: #202B39; color: #E6ECF3; }"
        "QWidget { color: #E6ECF3; }"
        "QMenuBar { background: #212832; color: #E6ECF3; }"
        "QMenuBar::item:selected { background: #31445A; }"
        "QMenu { background: #26333F; color: #E6ECF3; border: 1px solid #31445A; }"
        "QMenu::item:selected { background: #31445A; }"
        "QTabWidget::pane { border: none; background: #202B39; }"
        "QTabBar::tab { background: #1F2D3B; color: #AEB9C6; padding: 12px 22px; border: none; }"
        "QTabBar::tab:selected { background: #202B39; color: #6FA0FF; font-weight: bold; border-bottom: 3px solid #4A7DFF; }"
        "QTabBar::tab:hover { background: #26333F; }"
        "QTableView { background: #26333F; alternate-background-color: #2B3947; border: 1px solid #31445A; gridline-color: #31445A; color: #E6ECF3; selection-background-color: #4A7DFF; }"
        "QHeaderView::section { background: #212832; color: #E6ECF3; border: none; padding: 6px; font-weight: bold; }"
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { background: #1C2630; border: 1px solid #31445A; color: #E6ECF3; padding: 5px; border-radius: 3px; }"
        "QLineEdit:focus, QComboBox:focus { border: 1px solid #4A7DFF; }"
        "QComboBox QAbstractItemView { background: #26333F; color: #E6ECF3; border: 1px solid #31445A; selection-background-color: #4A7DFF; selection-color: #FFFFFF; outline: none; }"
        "QScrollBar:vertical { background: #1F2D3B; width: 10px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #31445A; min-height: 24px; border-radius: 4px; }"
        "QScrollBar::handle:vertical:hover { background: #4A7DFF; }"
        "QScrollBar:horizontal { background: #1F2D3B; height: 10px; margin: 0; }"
        "QScrollBar::handle:horizontal { background: #31445A; min-width: 24px; border-radius: 4px; }"
        "QScrollBar::handle:horizontal:hover { background: #4A7DFF; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }"
        "QCheckBox, QRadioButton { color: #E6ECF3; spacing: 6px; }"
        "QChartView { background: transparent; }"
        "QPushButton { background: #4A7DFF; color: white; padding: 6px 16px; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #6FA0FF; }"
        "QPushButton:disabled { background: #31445A; color: #7C8A99; }"
        "QGroupBox { border: 1px solid #31445A; border-radius: 4px; margin-top: 14px; }"
        "QGroupBox::title { color: #6FA0FF; subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QStatusBar { background: #1C2228; color: #AEB9C6; }"
        "QSplitter::handle { background: #31445A; }"
        "QLabel { color: #E6ECF3; }"
        "QToolTip { background: #26333F; color: #E6ECF3; border: 1px solid #31445A; }"
    );
}


inline QString hellesStylesheet()
{
    return QStringLiteral(
        "QMainWindow, QDialog { background: #D9DFE7; color: #1C2733; }"
        "QWidget { color: #1C2733; }"
        "QMenuBar { background: #B9C1CD; color: #1C2733; }"
        "QMenuBar::item:selected { background: #9AA6B8; }"
        "QMenu { background: #FFFFFF; color: #1C2733; border: 1px solid #B9C1CD; }"
        "QMenu::item:selected { background: #D9DFE7; color: #001065; }"
        "QTabWidget::pane { border: none; background: #D9DFE7; }"
        "QTabBar::tab { background: #DAE0E8; color: #4A5568; padding: 12px 22px; border: none; }"
        "QTabBar::tab:selected { background: #D9DFE7; color: #001065; font-weight: bold; border-bottom: 3px solid #001065; }"
        "QTabBar::tab:hover { background: #CBD3DE; }"
        "QTableView { background: #FFFFFF; alternate-background-color: #EEF1F5; border: 1px solid #C4CCD8; gridline-color: #C4CCD8; color: #1C2733; selection-background-color: #001065; selection-color: #FFFFFF; }"
        "QHeaderView::section { background: #B9C1CD; color: #1C2733; border: none; padding: 6px; font-weight: bold; }"
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { background: #FFFFFF; border: 1px solid #B9C1CD; color: #1C2733; padding: 5px; border-radius: 3px; }"
        "QLineEdit:focus, QComboBox:focus { border: 1px solid #001065; }"
        "QComboBox QAbstractItemView { background: #FFFFFF; color: #1C2733; border: 1px solid #B9C1CD; selection-background-color: #001065; selection-color: #FFFFFF; outline: none; }"
        "QScrollBar:vertical { background: #D9DFE7; width: 10px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #B9C1CD; min-height: 24px; border-radius: 4px; }"
        "QScrollBar::handle:vertical:hover { background: #001065; }"
        "QScrollBar:horizontal { background: #D9DFE7; height: 10px; margin: 0; }"
        "QScrollBar::handle:horizontal { background: #B9C1CD; min-width: 24px; border-radius: 4px; }"
        "QScrollBar::handle:horizontal:hover { background: #001065; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }"
        "QCheckBox, QRadioButton { color: #1C2733; spacing: 6px; }"
        "QChartView { background: transparent; }"
        "QPushButton { background: #001065; color: white; padding: 6px 16px; border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #1A2A8C; }"
        "QPushButton:disabled { background: #9AA6B8; color: #D9DFE7; }"
        "QGroupBox { border: 1px solid #B9C1CD; border-radius: 4px; margin-top: 14px; }"
        "QGroupBox::title { color: #001065; subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QStatusBar { background: #DDE2E7; color: #4A5568; }"
        "QSplitter::handle { background: #C4CCD8; }"
        "QToolTip { background: #FFFFFF; color: #1C2733; border: 1px solid #B9C1CD; }"
    );
}


inline QPalette dunklePalette()
{
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(32, 43, 57));
    pal.setColor(QPalette::WindowText, QColor(230, 236, 243));
    pal.setColor(QPalette::Base, QColor(38, 51, 63));
    pal.setColor(QPalette::AlternateBase, QColor(43, 57, 71));
    pal.setColor(QPalette::Text, QColor(230, 236, 243));
    pal.setColor(QPalette::Button, QColor(31, 45, 59));
    pal.setColor(QPalette::ButtonText, QColor(230, 236, 243));
    pal.setColor(QPalette::Highlight, QColor(74, 125, 255));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    return pal;
}


inline QPalette hellePalette()
{
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(217, 223, 231));
    pal.setColor(QPalette::WindowText, QColor(28, 39, 51));
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::AlternateBase, QColor(238, 241, 245));
    pal.setColor(QPalette::Text, QColor(28, 39, 51));
    pal.setColor(QPalette::Button, QColor(185, 193, 205));
    pal.setColor(QPalette::ButtonText, QColor(28, 39, 51));
    pal.setColor(QPalette::Highlight, QColor(0, 16, 101));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    return pal;
}
