#include "mainwindow.h"
#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QScrollArea>
#include <QFrame>
#include <QFont>
#include <QApplication>
#include <QStyleHints>
#include <QPalette>
#include <QListWidget>
#include <QSvgRenderer>
#include <QUrl>
#include <QTabWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QGroupBox>

// ═════════════════════════════════════════════════════════════════════════
//  PRESETS — todos los temas base del sistema
// ═════════════════════════════════════════════════════════════════════════
AppTheme AppTheme::oscuro() {
    AppTheme t; t.name="Oscuro"; t.family="Clásico"; t.isDark=true;
    t.bgWindow=QColor("#121212"); t.bgTitleBar=QColor("#161616"); t.bgTitleBarHover=QColor("#1F1F1F");
    t.bgSysBar=QColor("#1E1E1E"); t.bgRibbon=QColor("#242424"); t.bgSidebar=QColor("#1E1E1E");
    t.bgBottomBar=QColor("#1E1E1E"); t.bgPanel=QColor("#262626"); t.bgInput=QColor("#2A2A2A");
    t.bgPreview=QColor("#121212"); t.bgHover=QColor("#333333"); t.bgSelected=QColor("#1E3A5F");
    t.textPrimary=QColor("#E8E8E8"); t.textSecondary=QColor("#B0B0B0"); t.textMuted=QColor("#888888");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#2E2E2E"); t.borderStrong=QColor("#4A4A4A");
    t.accent=QColor("#3B82F6"); t.accentHover=QColor("#5A95F5"); t.accentPressed=QColor("#2E6BC8");
    t.danger=QColor("#C05A55"); t.dangerHover=QColor("#D97070"); t.radius=4;
    return t;
}
AppTheme AppTheme::claro() {
    AppTheme t; t.name="Claro"; t.family="Clásico"; t.isDark=false;
    t.bgWindow=QColor("#E8E8E8"); t.bgTitleBar=QColor("#B8B8B8"); t.bgTitleBarHover=QColor("#ADADAD");
    t.bgSysBar=QColor("#F0F0F0"); t.bgRibbon=QColor("#F0F0F0"); t.bgSidebar=QColor("#F0F0F0");
    t.bgBottomBar=QColor("#F0F0F0"); t.bgPanel=QColor("#FAFAFA"); t.bgInput=QColor("#FAFAFA");
    t.bgPreview=QColor("#E8E8E8"); t.bgHover=QColor("#F0F0F0"); t.bgSelected=QColor("#D9E3EE");
    t.textPrimary=QColor("#1C1C1C"); t.textSecondary=QColor("#555555"); t.textMuted=QColor("#888888");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#DCDCDC"); t.borderStrong=QColor("#BBBBBB");
    t.accent=QColor("#2E6BC8"); t.accentHover=QColor("#3B82F6"); t.accentPressed=QColor("#1E4A8F");
    t.danger=QColor("#B04540"); t.dangerHover=QColor("#C05A55"); t.radius=4;
    return t;
}
AppTheme AppTheme::minimal() {
    AppTheme t; t.name="Minimal"; t.family="Minimal"; t.isDark=true;
    t.bgWindow=QColor("#0E0E10"); t.bgTitleBar=QColor("#131316"); t.bgTitleBarHover=QColor("#1A1A1E");
    t.bgSysBar=QColor("#131316"); t.bgRibbon=QColor("#16161A"); t.bgSidebar=QColor("#131316");
    t.bgBottomBar=QColor("#131316"); t.bgPanel=QColor("#1A1A1E"); t.bgInput=QColor("#202024");
    t.bgPreview=QColor("#0A0A0C"); t.bgHover=QColor("#24242A"); t.bgSelected=QColor("#2A2A32");
    t.textPrimary=QColor("#EDEDEF"); t.textSecondary=QColor("#A0A0A8"); t.textMuted=QColor("#6B6B75");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#25252B"); t.borderStrong=QColor("#3A3A42");
    t.accent=QColor("#7C8FFF"); t.accentHover=QColor("#9AA8FF"); t.accentPressed=QColor("#5A6FE0");
    t.danger=QColor("#E06C75"); t.dangerHover=QColor("#F08088"); t.radius=6;
    return t;
}
AppTheme AppTheme::minimalClaro() {
    AppTheme t; t.name="Minimal Claro"; t.family="Minimal"; t.isDark=false;
    t.bgWindow=QColor("#FAFAFA"); t.bgTitleBar=QColor("#F2F2F4"); t.bgTitleBarHover=QColor("#E8E8EC");
    t.bgSysBar=QColor("#FAFAFA"); t.bgRibbon=QColor("#F5F5F7"); t.bgSidebar=QColor("#FAFAFA");
    t.bgBottomBar=QColor("#FAFAFA"); t.bgPanel=QColor("#FFFFFF"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#F0F0F2"); t.bgHover=QColor("#EDEDF0"); t.bgSelected=QColor("#E4E4EA");
    t.textPrimary=QColor("#1A1A1F"); t.textSecondary=QColor("#55555F"); t.textMuted=QColor("#8A8A95");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#E5E5EA"); t.borderStrong=QColor("#D0D0D8");
    t.accent=QColor("#3D5AFE"); t.accentHover=QColor("#5A75FF"); t.accentPressed=QColor("#2A3FC0");
    t.danger=QColor("#D64545"); t.dangerHover=QColor("#E85A5A"); t.radius=6;
    return t;
}
AppTheme AppTheme::oneDark() {
    AppTheme t; t.name="One Dark"; t.family="One"; t.isDark=true;
    t.bgWindow=QColor("#282C34"); t.bgTitleBar=QColor("#21252B"); t.bgTitleBarHover=QColor("#2C313A");
    t.bgSysBar=QColor("#282C34"); t.bgRibbon=QColor("#2C313A"); t.bgSidebar=QColor("#282C34");
    t.bgBottomBar=QColor("#282C34"); t.bgPanel=QColor("#2C313A"); t.bgInput=QColor("#333842");
    t.bgPreview=QColor("#21252B"); t.bgHover=QColor("#353B45"); t.bgSelected=QColor("#3E4451");
    t.textPrimary=QColor("#ABB2BF"); t.textSecondary=QColor("#848B98"); t.textMuted=QColor("#5C6370");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#3E4451"); t.borderStrong=QColor("#545862");
    t.accent=QColor("#61AFEF"); t.accentHover=QColor("#7FC0F5"); t.accentPressed=QColor("#4A95D6");
    t.danger=QColor("#E06C75"); t.dangerHover=QColor("#F08088"); t.radius=4;
    return t;
}
AppTheme AppTheme::oneLight() {
    AppTheme t; t.name="One Light"; t.family="One"; t.isDark=false;
    t.bgWindow=QColor("#FAFAFA"); t.bgTitleBar=QColor("#EAEAEB"); t.bgTitleBarHover=QColor("#E0E0E2");
    t.bgSysBar=QColor("#FAFAFA"); t.bgRibbon=QColor("#EAEAEB"); t.bgSidebar=QColor("#FAFAFA");
    t.bgBottomBar=QColor("#FAFAFA"); t.bgPanel=QColor("#EAEAEB"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#E0E0E2"); t.bgHover=QColor("#DCDCDF"); t.bgSelected=QColor("#D0D0D4");
    t.textPrimary=QColor("#383A42"); t.textSecondary=QColor("#56586B"); t.textMuted=QColor("#A0A1A7");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#D0D0D4"); t.borderStrong=QColor("#B8B8BE");
    t.accent=QColor("#4078F2"); t.accentHover=QColor("#5A8CF5"); t.accentPressed=QColor("#2E5FC0");
    t.danger=QColor("#E45649"); t.dangerHover=QColor("#F06A5E"); t.radius=4;
    return t;
}
AppTheme AppTheme::github() {
    AppTheme t; t.name="GitHub"; t.family="GitHub"; t.isDark=true;
    t.bgWindow=QColor("#0D1117"); t.bgTitleBar=QColor("#161B22"); t.bgTitleBarHover=QColor("#1F242C");
    t.bgSysBar=QColor("#0D1117"); t.bgRibbon=QColor("#161B22"); t.bgSidebar=QColor("#0D1117");
    t.bgBottomBar=QColor("#0D1117"); t.bgPanel=QColor("#161B22"); t.bgInput=QColor("#21262D");
    t.bgPreview=QColor("#080B10"); t.bgHover=QColor("#262C36"); t.bgSelected=QColor("#2D333B");
    t.textPrimary=QColor("#E6EDF3"); t.textSecondary=QColor("#A8B2BF"); t.textMuted=QColor("#6E7681");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#21262D"); t.borderStrong=QColor("#30363D");
    t.accent=QColor("#2F81F7"); t.accentHover=QColor("#5898F9"); t.accentPressed=QColor("#1F6ADB");
    t.danger=QColor("#F85149"); t.dangerHover=QColor("#FA7068"); t.radius=6;
    return t;
}
AppTheme AppTheme::githubClaro() {
    AppTheme t; t.name="GitHub Claro"; t.family="GitHub"; t.isDark=false;
    t.bgWindow=QColor("#FFFFFF"); t.bgTitleBar=QColor("#F6F8FA"); t.bgTitleBarHover=QColor("#EAEEF2");
    t.bgSysBar=QColor("#FFFFFF"); t.bgRibbon=QColor("#F6F8FA"); t.bgSidebar=QColor("#FFFFFF");
    t.bgBottomBar=QColor("#FFFFFF"); t.bgPanel=QColor("#F6F8FA"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#EAEEF2"); t.bgHover=QColor("#E6EAEE"); t.bgSelected=QColor("#D8DEE4");
    t.textPrimary=QColor("#1F2328"); t.textSecondary=QColor("#475467"); t.textMuted=QColor("#818B98");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#D0D7DE"); t.borderStrong=QColor("#BCC4CD");
    t.accent=QColor("#0969DA"); t.accentHover=QColor("#2B7FE0"); t.accentPressed=QColor("#0550AE");
    t.danger=QColor("#CF222E"); t.dangerHover=QColor("#E04050"); t.radius=6;
    return t;
}
AppTheme AppTheme::nord() {
    AppTheme t; t.name="Nord"; t.family="Nord"; t.isDark=true;
    t.bgWindow=QColor("#2E3440"); t.bgTitleBar=QColor("#292E39"); t.bgTitleBarHover=QColor("#3B4252");
    t.bgSysBar=QColor("#2E3440"); t.bgRibbon=QColor("#3B4252"); t.bgSidebar=QColor("#2E3440");
    t.bgBottomBar=QColor("#2E3440"); t.bgPanel=QColor("#3B4252"); t.bgInput=QColor("#434C5E");
    t.bgPreview=QColor("#242933"); t.bgHover=QColor("#434C5E"); t.bgSelected=QColor("#4C566A");
    t.textPrimary=QColor("#ECEFF4"); t.textSecondary=QColor("#D8DEE9"); t.textMuted=QColor("#4C566A");
    t.textOnAccent=QColor("#2E3440"); t.border=QColor("#434C5E"); t.borderStrong=QColor("#4C566A");
    t.accent=QColor("#88C0D0"); t.accentHover=QColor("#9FD0DC"); t.accentPressed=QColor("#6DA8B8");
    t.danger=QColor("#BF616A"); t.dangerHover=QColor("#D47A82"); t.radius=4;
    return t;
}
AppTheme AppTheme::nordClaro() {
    AppTheme t; t.name="Nord Claro"; t.family="Nord"; t.isDark=false;
    t.bgWindow=QColor("#ECEFF4"); t.bgTitleBar=QColor("#E5E9F0"); t.bgTitleBarHover=QColor("#D8DEE9");
    t.bgSysBar=QColor("#ECEFF4"); t.bgRibbon=QColor("#E5E9F0"); t.bgSidebar=QColor("#ECEFF4");
    t.bgBottomBar=QColor("#ECEFF4"); t.bgPanel=QColor("#E5E9F0"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#D8DEE9"); t.bgHover=QColor("#D8DEE9"); t.bgSelected=QColor("#C8D0DC");
    t.textPrimary=QColor("#2E3440"); t.textSecondary=QColor("#3B4252"); t.textMuted=QColor("#7B88A0");
    t.textOnAccent=QColor("#ECEFF4"); t.border=QColor("#D8DEE9"); t.borderStrong=QColor("#C0C8D8");
    t.accent=QColor("#5E81AC"); t.accentHover=QColor("#81A1C1"); t.accentPressed=QColor("#4A6A8F");
    t.danger=QColor("#BF616A"); t.dangerHover=QColor("#D47A82"); t.radius=4;
    return t;
}
AppTheme AppTheme::catppuccin() {
    AppTheme t; t.name="Catppuccin"; t.family="Catppuccin"; t.isDark=true;
    t.bgWindow=QColor("#1E1E2E"); t.bgTitleBar=QColor("#181825"); t.bgTitleBarHover=QColor("#181825");
    t.bgSysBar=QColor("#1E1E2E"); t.bgRibbon=QColor("#313244"); t.bgSidebar=QColor("#1E1E2E");
    t.bgBottomBar=QColor("#1E1E2E"); t.bgPanel=QColor("#313244"); t.bgInput=QColor("#45475A");
    t.bgPreview=QColor("#11111B"); t.bgHover=QColor("#45475A"); t.bgSelected=QColor("#585B70");
    t.textPrimary=QColor("#CDD6F4"); t.textSecondary=QColor("#BAC2DE"); t.textMuted=QColor("#6C7086");
    t.textOnAccent=QColor("#1E1E2E"); t.border=QColor("#45475A"); t.borderStrong=QColor("#585B70");
    t.accent=QColor("#CBA6F7"); t.accentHover=QColor("#D9BFFF"); t.accentPressed=QColor("#B4BEFE");
    t.danger=QColor("#F38BA8"); t.dangerHover=QColor("#FF9EB5"); t.radius=6;
    return t;
}
AppTheme AppTheme::catppuccinClaro() {
    AppTheme t; t.name="Catppuccin Claro"; t.family="Catppuccin"; t.isDark=false;
    t.bgWindow=QColor("#EFF1F5"); t.bgTitleBar=QColor("#E6E9EF"); t.bgTitleBarHover=QColor("#DCE0E8");
    t.bgSysBar=QColor("#EFF1F5"); t.bgRibbon=QColor("#E6E9EF"); t.bgSidebar=QColor("#EFF1F5");
    t.bgBottomBar=QColor("#EFF1F5"); t.bgPanel=QColor("#E6E9EF"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#DCE0E8"); t.bgHover=QColor("#DCE0E8"); t.bgSelected=QColor("#CCD0DA");
    t.textPrimary=QColor("#4C4F69"); t.textSecondary=QColor("#5C5F77"); t.textMuted=QColor("#9CA0B0");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#CCD0DA"); t.borderStrong=QColor("#BCC0CC");
    t.accent=QColor("#8839EF"); t.accentHover=QColor("#A058F5"); t.accentPressed=QColor("#6B24C2");
    t.danger=QColor("#D20F39"); t.dangerHover=QColor("#E64553"); t.radius=6;
    return t;
}
AppTheme AppTheme::tokyoNight() {
    AppTheme t; t.name="Tokyo Night"; t.family="Tokyo Night"; t.isDark=true;
    t.bgWindow=QColor("#1A1B26"); t.bgTitleBar=QColor("#16161E"); t.bgTitleBarHover=QColor("#1F2335");
    t.bgSysBar=QColor("#1A1B26"); t.bgRibbon=QColor("#24283B"); t.bgSidebar=QColor("#1A1B26");
    t.bgBottomBar=QColor("#1A1B26"); t.bgPanel=QColor("#24283B"); t.bgInput=QColor("#292E42");
    t.bgPreview=QColor("#16161E"); t.bgHover=QColor("#292E42"); t.bgSelected=QColor("#343A55");
    t.textPrimary=QColor("#C0CAF5"); t.textSecondary=QColor("#A9B1D6"); t.textMuted=QColor("#565F89");
    t.textOnAccent=QColor("#1A1B26"); t.border=QColor("#292E42"); t.borderStrong=QColor("#3B4261");
    t.accent=QColor("#7AA2F7"); t.accentHover=QColor("#96B5F8"); t.accentPressed=QColor("#5A82D7");
    t.danger=QColor("#F7768E"); t.dangerHover=QColor("#FF8FA8"); t.radius=4;
    return t;
}
AppTheme AppTheme::tokyoNightDia() {
    AppTheme t; t.name="Tokyo Night Día"; t.family="Tokyo Night"; t.isDark=false;
    t.bgWindow=QColor("#E1E2E7"); t.bgTitleBar=QColor("#D5D6DB"); t.bgTitleBarHover=QColor("#C8C9CE");
    t.bgSysBar=QColor("#E1E2E7"); t.bgRibbon=QColor("#D5D6DB"); t.bgSidebar=QColor("#E1E2E7");
    t.bgBottomBar=QColor("#E1E2E7"); t.bgPanel=QColor("#D5D6DB"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#C8C9CE"); t.bgHover=QColor("#C8C9CE"); t.bgSelected=QColor("#B4B5BA");
    t.textPrimary=QColor("#3760BF"); t.textSecondary=QColor("#4C4F69"); t.textMuted=QColor("#848CB5");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#C4C8DA"); t.borderStrong=QColor("#A8AEC8");
    t.accent=QColor("#2E7DE9"); t.accentHover=QColor("#5090F0"); t.accentPressed=QColor("#1A5FC8");
    t.danger=QColor("#F52A65"); t.dangerHover=QColor("#FF4A80"); t.radius=4;
    return t;
}
AppTheme AppTheme::dracula() {
    AppTheme t; t.name="Dracula"; t.family="Dracula"; t.isDark=true;
    t.bgWindow=QColor("#282A36"); t.bgTitleBar=QColor("#21222C"); t.bgTitleBarHover=QColor("#2D2F3E");
    t.bgSysBar=QColor("#282A36"); t.bgRibbon=QColor("#2D2F3E"); t.bgSidebar=QColor("#282A36");
    t.bgBottomBar=QColor("#282A36"); t.bgPanel=QColor("#2D2F3E"); t.bgInput=QColor("#343746");
    t.bgPreview=QColor("#1E1F29"); t.bgHover=QColor("#44475A"); t.bgSelected=QColor("#44475A");
    t.textPrimary=QColor("#F8F8F2"); t.textSecondary=QColor("#BFBFBF"); t.textMuted=QColor("#6272A4");
    t.textOnAccent=QColor("#282A36"); t.border=QColor("#44475A"); t.borderStrong=QColor("#6272A4");
    t.accent=QColor("#BD93F9"); t.accentHover=QColor("#D0A8FF"); t.accentPressed=QColor("#A67FE0");
    t.danger=QColor("#FF5555"); t.dangerHover=QColor("#FF6E6E"); t.radius=4;
    return t;
}
AppTheme AppTheme::alucard() {
    AppTheme t; t.name="Alucard"; t.family="Dracula"; t.isDark=false;
    t.bgWindow=QColor("#FFFBEB"); t.bgTitleBar=QColor("#F5EFD8"); t.bgTitleBarHover=QColor("#EDE5C8");
    t.bgSysBar=QColor("#FFFBEB"); t.bgRibbon=QColor("#F5EFD8"); t.bgSidebar=QColor("#FFFBEB");
    t.bgBottomBar=QColor("#FFFBEB"); t.bgPanel=QColor("#F5EFD8"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#EDE5C8"); t.bgHover=QColor("#E8DFC0"); t.bgSelected=QColor("#CFCFDE");
    t.textPrimary=QColor("#1F1F1F"); t.textSecondary=QColor("#4A4A4A"); t.textMuted=QColor("#6C664B");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#D8CFB0"); t.borderStrong=QColor("#B0A888");
    t.accent=QColor("#6C3EB0"); t.accentHover=QColor("#8B5CC7"); t.accentPressed=QColor("#4E2A80");
    t.danger=QColor("#CB3A2A"); t.dangerHover=QColor("#E04A3A"); t.radius=4;
    return t;
}
AppTheme AppTheme::everforest() {
    AppTheme t; t.name="Everforest"; t.family="Everforest"; t.isDark=true;
    t.bgWindow=QColor("#2D3532"); t.bgTitleBar=QColor("#272E2B"); t.bgTitleBarHover=QColor("#343E3A");
    t.bgSysBar=QColor("#2D3532"); t.bgRibbon=QColor("#343E3A"); t.bgSidebar=QColor("#2D3532");
    t.bgBottomBar=QColor("#2D3532"); t.bgPanel=QColor("#343E3A"); t.bgInput=QColor("#3D4844");
    t.bgPreview=QColor("#232A27"); t.bgHover=QColor("#3D4844"); t.bgSelected=QColor("#47524D");
    t.textPrimary=QColor("#D3C6AA"); t.textSecondary=QColor("#B8A98E"); t.textMuted=QColor("#859289");
    t.textOnAccent=QColor("#2D3532"); t.border=QColor("#47524D"); t.borderStrong=QColor("#5A6660");
    t.accent=QColor("#A7C080"); t.accentHover=QColor("#BBD098"); t.accentPressed=QColor("#8DA668");
    t.danger=QColor("#E67E80"); t.dangerHover=QColor("#F09092"); t.radius=4;
    return t;
}
AppTheme AppTheme::everforestClaro() {
    AppTheme t; t.name="Everforest Claro"; t.family="Everforest"; t.isDark=false;
    t.bgWindow=QColor("#FDF6E3"); t.bgTitleBar=QColor("#F4ECD8"); t.bgTitleBarHover=QColor("#EBE3CF");
    t.bgSysBar=QColor("#FDF6E3"); t.bgRibbon=QColor("#F4ECD8"); t.bgSidebar=QColor("#FDF6E3");
    t.bgBottomBar=QColor("#FDF6E3"); t.bgPanel=QColor("#F4ECD8"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#EBE3CF"); t.bgHover=QColor("#E8DFC0"); t.bgSelected=QColor("#D8D0B0");
    t.textPrimary=QColor("#5C6A57"); t.textSecondary=QColor("#7A8872"); t.textMuted=QColor("#A6B0A0");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#D8D0B0"); t.borderStrong=QColor("#C4BC98");
    t.accent=QColor("#8DA101"); t.accentHover=QColor("#A5B818"); t.accentPressed=QColor("#6E8000");
    t.danger=QColor("#F85552"); t.dangerHover=QColor("#FF7070"); t.radius=4;
    return t;
}
AppTheme AppTheme::rosePine() {
    AppTheme t; t.name="Rosé Pine"; t.family="Rosé Pine"; t.isDark=true;
    t.bgWindow=QColor("#191724"); t.bgTitleBar=QColor("#1F1D2E"); t.bgTitleBarHover=QColor("#26233A");
    t.bgSysBar=QColor("#191724"); t.bgRibbon=QColor("#1F1D2E"); t.bgSidebar=QColor("#191724");
    t.bgBottomBar=QColor("#191724"); t.bgPanel=QColor("#1F1D2E"); t.bgInput=QColor("#26233A");
    t.bgPreview=QColor("#13111C"); t.bgHover=QColor("#2A273F"); t.bgSelected=QColor("#312E4A");
    t.textPrimary=QColor("#E0DEF4"); t.textSecondary=QColor("#B8B5D0"); t.textMuted=QColor("#6E6A86");
    t.textOnAccent=QColor("#191724"); t.border=QColor("#2A273F"); t.borderStrong=QColor("#403D52");
    t.accent=QColor("#C4A7E7"); t.accentHover=QColor("#D6BDF0"); t.accentPressed=QColor("#A98ED0");
    t.danger=QColor("#EB6F92"); t.dangerHover=QColor("#F588A8"); t.radius=6;
    return t;
}
AppTheme AppTheme::rosePineDawn() {
    AppTheme t; t.name="Rosé Pine Dawn"; t.family="Rosé Pine"; t.isDark=false;
    t.bgWindow=QColor("#FAF4ED"); t.bgTitleBar=QColor("#FFFBEF"); t.bgTitleBarHover=QColor("#F5EBDF");
    t.bgSysBar=QColor("#FAF4ED"); t.bgRibbon=QColor("#FFFBEF"); t.bgSidebar=QColor("#FAF4ED");
    t.bgBottomBar=QColor("#FAF4ED"); t.bgPanel=QColor("#FFFBEF"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#F5EBDF"); t.bgHover=QColor("#F2E9DE"); t.bgSelected=QColor("#E4DFD7");
    t.textPrimary=QColor("#575279"); t.textSecondary=QColor("#797593"); t.textMuted=QColor("#9893A5");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#E4DFD7"); t.borderStrong=QColor("#CECAC3");
    t.accent=QColor("#907AA9"); t.accentHover=QColor("#A892BD"); t.accentPressed=QColor("#75608E");
    t.danger=QColor("#B4637A"); t.dangerHover=QColor("#D07890"); t.radius=6;
    return t;
}
AppTheme AppTheme::ayu() {
    AppTheme t; t.name="Ayu"; t.family="Ayu"; t.isDark=true;
    t.bgWindow=QColor("#0B0E14"); t.bgTitleBar=QColor("#0D1016"); t.bgTitleBarHover=QColor("#141922");
    t.bgSysBar=QColor("#0B0E14"); t.bgRibbon=QColor("#0D1016"); t.bgSidebar=QColor("#0B0E14");
    t.bgBottomBar=QColor("#0B0E14"); t.bgPanel=QColor("#0D1016"); t.bgInput=QColor("#141922");
    t.bgPreview=QColor("#060810"); t.bgHover=QColor("#1A2030"); t.bgSelected=QColor("#222838");
    t.textPrimary=QColor("#BFBDB6"); t.textSecondary=QColor("#8A877F"); t.textMuted=QColor("#5C5A55");
    t.textOnAccent=QColor("#0B0E14"); t.border=QColor("#1A2030"); t.borderStrong=QColor("#2A3040");
    t.accent=QColor("#E6B450"); t.accentHover=QColor("#F0C468"); t.accentPressed=QColor("#C89A3A");
    t.danger=QColor("#F07178"); t.dangerHover=QColor("#FF8A90"); t.radius=4;
    return t;
}
AppTheme AppTheme::ayuClaro() {
    AppTheme t; t.name="Ayu Claro"; t.family="Ayu"; t.isDark=false;
    t.bgWindow=QColor("#FAFAFA"); t.bgTitleBar=QColor("#F3F3F3"); t.bgTitleBarHover=QColor("#EAEAEA");
    t.bgSysBar=QColor("#FAFAFA"); t.bgRibbon=QColor("#F3F3F3"); t.bgSidebar=QColor("#FAFAFA");
    t.bgBottomBar=QColor("#FAFAFA"); t.bgPanel=QColor("#F3F3F3"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#EAEAEA"); t.bgHover=QColor("#E5E5E5"); t.bgSelected=QColor("#D8D8D8");
    t.textPrimary=QColor("#5C6166"); t.textSecondary=QColor("#828C99"); t.textMuted=QColor("#ACB6BF");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#D8D8D8"); t.borderStrong=QColor("#C0C0C0");
    t.accent=QColor("#FF9940"); t.accentHover=QColor("#FFAD66"); t.accentPressed=QColor("#E07F20");
    t.danger=QColor("#E65050"); t.dangerHover=QColor("#F06A6A"); t.radius=4;
    return t;
}
AppTheme AppTheme::kanagawa() {
    AppTheme t; t.name="Kanagawa"; t.family="Kanagawa"; t.isDark=true;
    t.bgWindow=QColor("#1F1F28"); t.bgTitleBar=QColor("#16161D"); t.bgTitleBarHover=QColor("#252530");
    t.bgSysBar=QColor("#1F1F28"); t.bgRibbon=QColor("#252530"); t.bgSidebar=QColor("#1F1F28");
    t.bgBottomBar=QColor("#1F1F28"); t.bgPanel=QColor("#252530"); t.bgInput=QColor("#2A2A37");
    t.bgPreview=QColor("#18181F"); t.bgHover=QColor("#2F2F3C"); t.bgSelected=QColor("#363646");
    t.textPrimary=QColor("#DCD7BA"); t.textSecondary=QColor("#C8C0A3"); t.textMuted=QColor("#727169");
    t.textOnAccent=QColor("#1F1F28"); t.border=QColor("#2F2F3C"); t.borderStrong=QColor("#434356");
    t.accent=QColor("#7E9CD8"); t.accentHover=QColor("#96B0E0"); t.accentPressed=QColor("#6480B8");
    t.danger=QColor("#E82424"); t.dangerHover=QColor("#F04040"); t.radius=4;
    return t;
}
AppTheme AppTheme::kanagawaClaro() {
    AppTheme t; t.name="Kanagawa Claro"; t.family="Kanagawa"; t.isDark=false;
    t.bgWindow=QColor("#F2EBD9"); t.bgTitleBar=QColor("#E7DBBF"); t.bgTitleBarHover=QColor("#DED2B2");
    t.bgSysBar=QColor("#F2EBD9"); t.bgRibbon=QColor("#E7DBBF"); t.bgSidebar=QColor("#F2EBD9");
    t.bgBottomBar=QColor("#F2EBD9"); t.bgPanel=QColor("#E7DBBF"); t.bgInput=QColor("#FFF8E8");
    t.bgPreview=QColor("#DED2B2"); t.bgHover=QColor("#D8C8A0"); t.bgSelected=QColor("#C8B890");
    t.textPrimary=QColor("#54546D"); t.textSecondary=QColor("#6A6A80"); t.textMuted=QColor("#A0A0B0");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#C8B890"); t.borderStrong=QColor("#B0A078");
    t.accent=QColor("#4D699E"); t.accentHover=QColor("#6080B0"); t.accentPressed=QColor("#3A5080");
    t.danger=QColor("#C84040"); t.dangerHover=QColor("#E05050"); t.radius=4;
    return t;
}
AppTheme AppTheme::gruvbox() {
    AppTheme t; t.name="Gruvbox"; t.family="Gruvbox"; t.isDark=true;
    t.bgWindow=QColor("#282828"); t.bgTitleBar=QColor("#1D2021"); t.bgTitleBarHover=QColor("#32302F");
    t.bgSysBar=QColor("#282828"); t.bgRibbon=QColor("#3C3836"); t.bgSidebar=QColor("#282828");
    t.bgBottomBar=QColor("#282828"); t.bgPanel=QColor("#3C3836"); t.bgInput=QColor("#504945");
    t.bgPreview=QColor("#1D2021"); t.bgHover=QColor("#504945"); t.bgSelected=QColor("#665C54");
    t.textPrimary=QColor("#EBDBB2"); t.textSecondary=QColor("#D5C4A1"); t.textMuted=QColor("#928374");
    t.textOnAccent=QColor("#282828"); t.border=QColor("#504945"); t.borderStrong=QColor("#665C54");
    t.accent=QColor("#FABD2F"); t.accentHover=QColor("#FFD050"); t.accentPressed=QColor("#D79921");
    t.danger=QColor("#FB4934"); t.dangerHover=QColor("#FF6655"); t.radius=2;
    return t;
}
AppTheme AppTheme::gruvboxClaro() {
    AppTheme t; t.name="Gruvbox Claro"; t.family="Gruvbox"; t.isDark=false;
    t.bgWindow=QColor("#FBF1C7"); t.bgTitleBar=QColor("#F2E5BC"); t.bgTitleBarHover=QColor("#EBDBB2");
    t.bgSysBar=QColor("#FBF1C7"); t.bgRibbon=QColor("#F2E5BC"); t.bgSidebar=QColor("#FBF1C7");
    t.bgBottomBar=QColor("#FBF1C7"); t.bgPanel=QColor("#F2E5BC"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#EBDBB2"); t.bgHover=QColor("#EBDBB2"); t.bgSelected=QColor("#D5C4A1");
    t.textPrimary=QColor("#3C3836"); t.textSecondary=QColor("#504945"); t.textMuted=QColor("#928374");
    t.textOnAccent=QColor("#FBF1C7"); t.border=QColor("#D5C4A1"); t.borderStrong=QColor("#BDAE93");
    t.accent=QColor("#D65D0E"); t.accentHover=QColor("#E87020"); t.accentPressed=QColor("#AF3A03");
    t.danger=QColor("#CC241D"); t.dangerHover=QColor("#E04A3A"); t.radius=2;
    return t;
}
AppTheme AppTheme::solarized() {
    AppTheme t; t.name="Solarized"; t.family="Solarized"; t.isDark=true;
    t.bgWindow=QColor("#002B36"); t.bgTitleBar=QColor("#001F27"); t.bgTitleBarHover=QColor("#073642");
    t.bgSysBar=QColor("#002B36"); t.bgRibbon=QColor("#073642"); t.bgSidebar=QColor("#002B36");
    t.bgBottomBar=QColor("#002B36"); t.bgPanel=QColor("#073642"); t.bgInput=QColor("#0A4A5A");
    t.bgPreview=QColor("#001A22"); t.bgHover=QColor("#0A4A5A"); t.bgSelected=QColor("#0D5A6C");
    t.textPrimary=QColor("#93A1A1"); t.textSecondary=QColor("#839496"); t.textMuted=QColor("#586E75");
    t.textOnAccent=QColor("#002B36"); t.border=QColor("#0A4A5A"); t.borderStrong=QColor("#1A6A7A");
    t.accent=QColor("#268BD2"); t.accentHover=QColor("#40A0E0"); t.accentPressed=QColor("#1A6BA0");
    t.danger=QColor("#DC322F"); t.dangerHover=QColor("#E84A47"); t.radius=2;
    return t;
}
AppTheme AppTheme::solarizedClaro() {
    AppTheme t; t.name="Solarized Claro"; t.family="Solarized"; t.isDark=false;
    t.bgWindow=QColor("#FDF6E3"); t.bgTitleBar=QColor("#EEE8D5"); t.bgTitleBarHover=QColor("#E0D8C0");
    t.bgSysBar=QColor("#FDF6E3"); t.bgRibbon=QColor("#EEE8D5"); t.bgSidebar=QColor("#FDF6E3");
    t.bgBottomBar=QColor("#FDF6E3"); t.bgPanel=QColor("#EEE8D5"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#E0D8C0"); t.bgHover=QColor("#E0D8C0"); t.bgSelected=QColor("#D0C8B0");
    t.textPrimary=QColor("#657B83"); t.textSecondary=QColor("#586E75"); t.textMuted=QColor("#93A1A1");
    t.textOnAccent=QColor("#FDF6E3"); t.border=QColor("#D0C8B0"); t.borderStrong=QColor("#B8B090");
    t.accent=QColor("#268BD2"); t.accentHover=QColor("#40A0E0"); t.accentPressed=QColor("#1A6BA0");
    t.danger=QColor("#DC322F"); t.dangerHover=QColor("#E84A47"); t.radius=2;
    return t;
}
AppTheme AppTheme::monokai() {
    AppTheme t; t.name="Monokai"; t.family="Monokai"; t.isDark=true;
    t.bgWindow=QColor("#272822"); t.bgTitleBar=QColor("#1E1F1A"); t.bgTitleBarHover=QColor("#33342C");
    t.bgSysBar=QColor("#272822"); t.bgRibbon=QColor("#33342C"); t.bgSidebar=QColor("#272822");
    t.bgBottomBar=QColor("#272822"); t.bgPanel=QColor("#33342C"); t.bgInput=QColor("#3E3F36");
    t.bgPreview=QColor("#1E1F1A"); t.bgHover=QColor("#3E3F36"); t.bgSelected=QColor("#49483E");
    t.textPrimary=QColor("#F8F8F2"); t.textSecondary=QColor("#D0D0C8"); t.textMuted=QColor("#75715E");
    t.textOnAccent=QColor("#272822"); t.border=QColor("#3E3F36"); t.borderStrong=QColor("#4E4F46");
    t.accent=QColor("#A6E22E"); t.accentHover=QColor("#B8F040"); t.accentPressed=QColor("#86C21E");
    t.danger=QColor("#F92672"); t.dangerHover=QColor("#FF4682"); t.radius=2;
    return t;
}
AppTheme AppTheme::monokaiClaro() {
    AppTheme t; t.name="Monokai Claro"; t.family="Monokai"; t.isDark=false;
    t.bgWindow=QColor("#FAFAF5"); t.bgTitleBar=QColor("#EEEEE0"); t.bgTitleBarHover=QColor("#E0E0D0");
    t.bgSysBar=QColor("#FAFAF5"); t.bgRibbon=QColor("#EEEEE0"); t.bgSidebar=QColor("#FAFAF5");
    t.bgBottomBar=QColor("#FAFAF5"); t.bgPanel=QColor("#EEEEE0"); t.bgInput=QColor("#FFFFFF");
    t.bgPreview=QColor("#E0E0D0"); t.bgHover=QColor("#E0E0D0"); t.bgSelected=QColor("#D0D0C0");
    t.textPrimary=QColor("#3A3A35"); t.textSecondary=QColor("#55554E"); t.textMuted=QColor("#99998E");
    t.textOnAccent=QColor("#FFFFFF"); t.border=QColor("#D0D0C0"); t.borderStrong=QColor("#B8B8A8");
    t.accent=QColor("#6A9E1E"); t.accentHover=QColor("#80B830"); t.accentPressed=QColor("#4A7E0E");
    t.danger=QColor("#D91A5A"); t.dangerHover=QColor("#E93A6A"); t.radius=2;
    return t;
}

QList<AppTheme> AppTheme::presets() {
    return {
        oscuro(), claro(), minimal(), minimalClaro(),
        oneDark(), oneLight(), github(), githubClaro(),
        nord(), nordClaro(), catppuccin(), catppuccinClaro(),
        tokyoNight(), tokyoNightDia(), dracula(), alucard(),
        everforest(), everforestClaro(), rosePine(), rosePineDawn(),
        ayu(), ayuClaro(), kanagawa(), kanagawaClaro(),
        gruvbox(), gruvboxClaro(), solarized(), solarizedClaro(),
        monokai(), monokaiClaro(),
    };
}
AppTheme AppTheme::presetByName(const QString &n) {
    for (const AppTheme &t : presets())
        if (t.name.compare(n, Qt::CaseInsensitive) == 0) return t;
    return oscuro();
}
AppTheme AppTheme::findFamily(const QString &family, bool dark) {
    for (const AppTheme &t : presets())
        if (t.family == family && t.isDark == dark) return t;
    return dark ? oscuro() : claro();
}

// ═════════════════════════════════════════════════════════════════════════
//  ThemeEditorDialog — editor con pestañas: Colores / Skins
// ═════════════════════════════════════════════════════════════════════════
ThemeEditorDialog::ThemeEditorDialog(const AppTheme &initial, QWidget *parent)
    : QDialog(parent), m_theme(initial)
{
    setWindowTitle(tr("Editor de Apariencia"));
    setMinimumSize(760, 640);
    buildUI();
    refreshSwatches();
    refreshSkins();
}

void ThemeEditorDialog::buildUI() {
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(14);

    // Fila superior: preset + guardar
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(10);
    QLabel *lblPreset = new QLabel(tr("Tema base:"));
    m_presetCombo = new QComboBox();
    m_presetCombo->addItem(tr("— Personalizado —"), QString());
    for (const AppTheme &t : AppTheme::presets())
        m_presetCombo->addItem(t.name, t.name);
    QSettings s("Paintux", "PaintuxStudio");
    for (const QString &k : s.allKeys())
        if (k.startsWith("theme/custom_")) {
            const QString n = k.mid(QString("theme/custom_").length());
            m_presetCombo->addItem("★ " + n, n);
        }
    m_presetCombo->setMinimumWidth(220);
    topRow->addWidget(lblPreset);
    topRow->addWidget(m_presetCombo, 1);
    root->addLayout(topRow);

    m_tabs = new QTabWidget();
    root->addWidget(m_tabs, 1);

    buildColorsTab();
    buildSkinsTab();

    // Preview
    m_previewBox = new QWidget();
    m_previewBox->setFixedHeight(80);
    QHBoxLayout *prevLay = new QHBoxLayout(m_previewBox);
    prevLay->setContentsMargins(12, 8, 12, 8); prevLay->setSpacing(8);
    m_previewTitle = new QLabel(tr("Vista previa"));
    QFont pf = m_previewTitle->font(); pf.setBold(true);
    m_previewTitle->setFont(pf);
    prevLay->addWidget(m_previewTitle); prevLay->addStretch();
    QPushButton *pb1 = new QPushButton(tr("Botón")); pb1->setFixedHeight(26); prevLay->addWidget(pb1);
    QPushButton *pb2 = new QPushButton(tr("Acento")); pb2->setFixedHeight(26); pb2->setObjectName("previewAccent"); prevLay->addWidget(pb2);
    QPushButton *pb3 = new QPushButton(tr("Peligro")); pb3->setFixedHeight(26); pb3->setObjectName("previewDanger"); prevLay->addWidget(pb3);
    root->addWidget(m_previewBox);

    QDialogButtonBox *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset);
    QPushButton *btnSave = new QPushButton(tr("Guardar como preset..."));
    btnSave->setCursor(Qt::PointingHandCursor);
    btns->addButton(btnSave, QDialogButtonBox::ActionRole);
    root->addWidget(btns);

    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { applyPreset(); });
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(btns->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, [this]() {
        m_theme = AppTheme::oscuro();
        m_presetCombo->blockSignals(true);
        m_presetCombo->setCurrentIndex(0);
        m_presetCombo->blockSignals(false);
        m_theme.skins.clear();
        refreshSwatches(); refreshSkins(); updateTheme();
    });
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        QString name = QInputDialog::getText(this, tr("Guardar preset"),
            tr("Nombre del preset:"), QLineEdit::Normal, m_theme.name, &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        m_theme.name = name.trimmed();
        QSettings s("Paintux", "PaintuxStudio");
        QJsonObject obj = QJsonObject::fromVariantMap(m_theme.toVariantMap());
        s.setValue("theme/custom_" + name, QJsonDocument(obj).toJson(QJsonDocument::Compact));
        bool exists = false;
        for (int i = 0; i < m_presetCombo->count(); ++i)
            if (m_presetCombo->itemData(i).toString() == name) { exists = true; m_presetCombo->setCurrentIndex(i); break; }
        if (!exists) {
            m_presetCombo->addItem("★ " + name, name);
            m_presetCombo->setCurrentIndex(m_presetCombo->count() - 1);
        }
    });

    updatePreviewStyle();
    updateTheme();
}

void ThemeEditorDialog::buildColorsTab() {
    QWidget *colorsTab = new QWidget();
    QVBoxLayout *root = new QVBoxLayout(colorsTab);
    root->setContentsMargins(8, 12, 8, 12);
    root->setSpacing(10);

    QGridLayout *colorsGrid = new QGridLayout();
    colorsGrid->setSpacing(10);
    struct ColorToken { const char *key; const char *label; };
    const ColorToken tokens[] = {
        { "bgWindow",    "Fondo principal" },
        { "bgPanel",     "Paneles / tarjetas" },
        { "bgInput",     "Entradas / botones" },
        { "textPrimary", "Texto principal" },
        { "accent",      "Acento (selección, bordes)" },
        { "border",      "Bordes" },
    };
    for (int i = 0; i < 6; ++i) {
        QWidget *cell = new QWidget();
        QHBoxLayout *cellLay = new QHBoxLayout(cell);
        cellLay->setContentsMargins(2, 2, 2, 2); cellLay->setSpacing(8);
        QPushButton *swatch = new QPushButton();
        swatch->setFixedSize(40, 30);
        swatch->setCursor(Qt::PointingHandCursor);
        swatch->setToolTip(tr("Click para cambiar"));
        QLabel *lbl = new QLabel(tr(tokens[i].label));
        lbl->setStyleSheet("font-size: 12px;");
        cellLay->addWidget(swatch); cellLay->addWidget(lbl, 1);
        colorsGrid->addWidget(cell, i / 2, i % 2);
        m_colorButtons.insert(QString(tokens[i].key), swatch);
        const QString key = QString(tokens[i].key);
        connect(swatch, &QPushButton::clicked, this, [this, key]() { onColorClicked(key); });
    }
    root->addLayout(colorsGrid);

    QHBoxLayout *geomRow = new QHBoxLayout();
    geomRow->setSpacing(10);
    QLabel *lblRadius = new QLabel(tr("Radio:"));
    m_radiusSlider = new QSlider(Qt::Horizontal);
    m_radiusSlider->setRange(0, 16);
    m_radiusSlider->setValue(m_theme.radius);
    m_radiusSlider->setFixedWidth(160);
    m_radiusLabel = new QLabel(QString::number(m_theme.radius) + "px");
    m_radiusLabel->setFixedWidth(40);
    geomRow->addWidget(lblRadius); geomRow->addWidget(m_radiusSlider);
    geomRow->addWidget(m_radiusLabel); geomRow->addStretch();
    root->addLayout(geomRow);

    connect(m_radiusSlider, &QSlider::valueChanged, this, [this](int v) {
        m_theme.radius = v;
        m_radiusLabel->setText(QString::number(v) + "px");
        updateTheme();
    });

    root->addStretch();
    m_tabs->addTab(colorsTab, tr("Colores"));
}

void ThemeEditorDialog::buildSkinsTab() {
    QWidget *skinsTab = new QWidget();
    QVBoxLayout *root = new QVBoxLayout(skinsTab);
    root->setContentsMargins(8, 12, 8, 12);
    root->setSpacing(10);

    QLabel *info = new QLabel(tr(
        "Aplica una imagen PNG o SVG como fondo de cada componente de la interfaz.\n"
        "Si un componente no tiene skin activa, se usa su color sólido del tema."));
    info->setWordWrap(true);
    info->setStyleSheet("font-size: 11px; color: #999;");
    root->addWidget(info);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    QWidget *list = new QWidget();
    QVBoxLayout *listLay = new QVBoxLayout(list);
    listLay->setContentsMargins(0, 0, 0, 0);
    listLay->setSpacing(8);

    const QStringList keys = SkinKey::all();
    for (const QString &key : keys) {
        QFrame *row = new QFrame();
        row->setFrameShape(QFrame::StyledPanel);
        row->setStyleSheet("QFrame { border: 1px solid rgba(128,128,128,60); border-radius: 6px; padding: 4px; }");
        QGridLayout *gl = new QGridLayout(row);
        gl->setContentsMargins(10, 8, 10, 8);
        gl->setSpacing(8);

        SkinRow sr;
        sr.key = key;

        // Col 0: thumb + label
        sr.thumb = new QLabel();
        sr.thumb->setFixedSize(48, 48);
        sr.thumb->setAlignment(Qt::AlignCenter);
        sr.thumb->setStyleSheet("border: 1px dashed rgba(128,128,128,120); border-radius: 4px; background: rgba(0,0,0,30);");
        gl->addWidget(sr.thumb, 0, 0, 2, 1);

        // Col 1: nombre + ruta
        QLabel *nameLbl = new QLabel(SkinKey::label(key));
        nameLbl->setStyleSheet("font-weight: 600; font-size: 12px; border: none;");
        gl->addWidget(nameLbl, 0, 1);

        sr.pathLabel = new QLabel(tr("(sin skin)"));
        sr.pathLabel->setStyleSheet("font-size: 10px; color: #888; border: none;");
        sr.pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        gl->addWidget(sr.pathLabel, 1, 1);

        // Col 2: botones pick/clear
        QHBoxLayout *btnRow = new QHBoxLayout();
        btnRow->setSpacing(4);
        sr.btnPick = new QPushButton(tr("Elegir..."));
        sr.btnClear = new QPushButton(tr("Quitar"));
        sr.btnPick->setCursor(Qt::PointingHandCursor);
        sr.btnClear->setCursor(Qt::PointingHandCursor);
        sr.btnPick->setFixedHeight(24);
        sr.btnClear->setFixedHeight(24);
        btnRow->addWidget(sr.btnPick);
        btnRow->addWidget(sr.btnClear);
        gl->addLayout(btnRow, 0, 2, 1, 1);

        // Col 2 fila 1: enable + stretch
        QHBoxLayout *chkRow = new QHBoxLayout();
        chkRow->setSpacing(8);
        sr.chkEnabled = new QCheckBox(tr("Activa"));
        sr.chkStretch = new QCheckBox(tr("Estirar"));
        sr.chkEnabled->setStyleSheet("font-size: 11px; border: none;");
        sr.chkStretch->setStyleSheet("font-size: 11px; border: none;");
        chkRow->addWidget(sr.chkEnabled);
        chkRow->addWidget(sr.chkStretch);
        chkRow->addStretch();
        gl->addLayout(chkRow, 1, 2, 1, 1);

        // Col 3: opacidad + tinte
        QVBoxLayout *v = new QVBoxLayout();
        v->setSpacing(3);
        QLabel *opLbl = new QLabel(tr("Opacidad: 100%"));
        opLbl->setStyleSheet("font-size: 10px; border: none;");
        sr.opacity = new QSlider(Qt::Horizontal);
        sr.opacity->setRange(0, 100);
        sr.opacity->setValue(100);
        sr.opacity->setFixedWidth(110);
        v->addWidget(opLbl);
        v->addWidget(sr.opacity);

        sr.btnTint = new QPushButton(tr("Tinte..."));
        sr.btnTint->setCursor(Qt::PointingHandCursor);
        sr.btnTint->setFixedHeight(22);
        sr.btnTint->setStyleSheet("font-size: 10px;");
        v->addWidget(sr.btnTint);
        gl->addLayout(v, 0, 3, 2, 1);

        // Conexiones
        connect(sr.btnPick, &QPushButton::clicked, this, [this, key]() { pickSkinFile(key); });
        connect(sr.btnClear, &QPushButton::clicked, this, [this, key]() { clearSkin(key); });
        connect(sr.chkEnabled, &QCheckBox::toggled, this, [this, key](bool on) {
            if (m_theme.skins.contains(key)) {
                m_theme.skins[key].enabled = on;
                updateTheme();
            }
        });
        connect(sr.chkStretch, &QCheckBox::toggled, this, [this, key](bool on) {
            if (m_theme.skins.contains(key)) {
                m_theme.skins[key].stretch = on;
                updateTheme();
            }
        });
        connect(sr.opacity, &QSlider::valueChanged, this, [this, key, opLbl](int v) {
            opLbl->setText(tr("Opacidad: %1%").arg(v));
            if (m_theme.skins.contains(key)) {
                m_theme.skins[key].opacity = v;
                updateTheme();
            }
        });
        connect(sr.btnTint, &QPushButton::clicked, this, [this, key]() { onSkinTint(key); });

        m_skinRows.insert(key, sr);
        listLay->addWidget(row);
    }
    listLay->addStretch();
    scroll->setWidget(list);
    root->addWidget(scroll, 1);

    m_tabs->addTab(skinsTab, tr("Skins"));
}

void ThemeEditorDialog::refreshSwatches() {
    const QVariantMap vm = m_theme.toVariantMap();
    for (auto it = m_colorButtons.begin(); it != m_colorButtons.end(); ++it) {
        QColor c(vm.value(it.key()).toString());
        if (!c.isValid()) c = Qt::gray;
        it.value()->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: 1px solid rgba(128,128,128,0.5); border-radius: 5px; }"
            "QPushButton:hover { border: 2px solid #3b82f6; }").arg(c.name(QColor::HexArgb)));
    }
}

void ThemeEditorDialog::refreshSkins() {
    for (auto it = m_skinRows.begin(); it != m_skinRows.end(); ++it) {
        const QString &key = it.key();
        SkinRow &sr = it.value();
        const ComponentSkin skin = m_theme.skins.value(key, ComponentSkin());

        if (skin.isActive()) {
            QPixmap pm = SkinHelper::renderPixmap(skin, m_theme.isDark, QSize(40, 40));
            if (!pm.isNull()) {
                QPixmap scaled = pm.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                sr.thumb->setPixmap(scaled);
                sr.thumb->setStyleSheet("border: 1px solid rgba(80,180,80,180); border-radius: 4px; background: rgba(0,0,0,30);");
            }
            QFileInfo fi(skin.filePath);
            sr.pathLabel->setText(fi.fileName());
            sr.pathLabel->setToolTip(skin.filePath);
        } else if (!skin.filePath.isEmpty()) {
            QPixmap pm = SkinHelper::renderPixmap(skin, m_theme.isDark, QSize(40, 40));
            if (!pm.isNull()) sr.thumb->setPixmap(pm.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            sr.thumb->setStyleSheet("border: 1px dashed rgba(255,180,80,180); border-radius: 4px; background: rgba(0,0,0,30);");
            sr.pathLabel->setText(QFileInfo(skin.filePath).fileName() + tr(" (desactivada)"));
        } else {
            sr.thumb->clear();
            sr.thumb->setStyleSheet("border: 1px dashed rgba(128,128,128,120); border-radius: 4px; background: rgba(0,0,0,30);");
            sr.pathLabel->setText(tr("(sin skin)"));
        }
        sr.chkEnabled->blockSignals(true);
        sr.chkStretch->blockSignals(true);
        sr.opacity->blockSignals(true);
        sr.chkEnabled->setChecked(skin.enabled);
        sr.chkStretch->setChecked(skin.stretch);
        sr.opacity->setValue(skin.opacity);
        sr.chkEnabled->blockSignals(false);
        sr.chkStretch->blockSignals(false);
        sr.opacity->blockSignals(false);
    }
}

void ThemeEditorDialog::pickSkinFile(const QString &key) {
    QString path = QFileDialog::getOpenFileName(this,
        tr("Elegir imagen para skin"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        tr("Imágenes (*.png *.svg *.jpg *.jpeg *.webp *.bmp);;SVG (*.svg);;PNG (*.png);;Todos (*)"));
    if (path.isEmpty()) return;

    ComponentSkin skin = m_theme.skins.value(key, ComponentSkin());
    skin.filePath = path;
    skin.enabled  = true;
    if (skin.opacity == 0) skin.opacity = 100;
    m_theme.skins.insert(key, skin);
    m_theme.name = tr("Personalizado");
    m_theme.family = "";
    m_presetCombo->blockSignals(true);
    m_presetCombo->setCurrentIndex(0);
    m_presetCombo->blockSignals(false);
    refreshSkins();
    updateTheme();
}

void ThemeEditorDialog::clearSkin(const QString &key) {
    m_theme.skins.remove(key);
    m_theme.name = tr("Personalizado");
    m_theme.family = "";
    m_presetCombo->blockSignals(true);
    m_presetCombo->setCurrentIndex(0);
    m_presetCombo->blockSignals(false);
    refreshSkins();
    updateTheme();
}

void ThemeEditorDialog::onSkinTint(const QString &key) {
    ComponentSkin skin = m_theme.skins.value(key, ComponentSkin());
    QColor initial = skin.tintColor.alpha() > 0 ? skin.tintColor : Qt::white;
    QColor c = QColorDialog::getColor(initial, this, tr("Color de tinte"), QColorDialog::ShowAlphaChannel);
    if (!c.isValid()) return;
    skin.tintColor = c;
    if (!m_theme.skins.contains(key) && skin.filePath.isEmpty()) return;
    m_theme.skins.insert(key, skin);
    refreshSkins();
    updateTheme();
}

void ThemeEditorDialog::applyPreset() {
    const QString name = m_presetCombo->currentData().toString();
    if (name.isEmpty() || name == "sep") return;
    for (const AppTheme &t : AppTheme::presets())
        if (t.name == name) {
            m_theme = t;
            m_radiusSlider->setValue(m_theme.radius);
            refreshSwatches(); refreshSkins(); updateTheme(); return;
        }
    QSettings s("Paintux", "PaintuxStudio");
    const QString json = s.value("theme/custom_" + name).toString();
    if (!json.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        m_theme = AppTheme::fromVariantMap(doc.object().toVariantMap());
        m_radiusSlider->setValue(m_theme.radius);
        refreshSwatches(); refreshSkins(); updateTheme();
    }
}

void ThemeEditorDialog::onColorClicked(const QString &key) {
    const QVariantMap vm = m_theme.toVariantMap();
    QColor initial(vm.value(key).toString());
    if (!initial.isValid()) initial = Qt::gray;
    QColor c = QColorDialog::getColor(initial, this, tr("Elegir color"));
    if (!c.isValid()) return;
    if (key == "bgWindow") {
        m_theme.bgWindow = c; m_theme.bgTitleBar = c.darker(115); m_theme.bgSysBar = c;
        m_theme.bgRibbon = c.lighter(108); m_theme.bgSidebar = c; m_theme.bgBottomBar = c;
        m_theme.bgPreview = c.darker(120); m_theme.bgTitleBarHover = c.lighter(115);
    } else if (key == "bgPanel") { m_theme.bgPanel = c; m_theme.bgHover = c.lighter(115); m_theme.bgSelected = c.lighter(130); }
    else if (key == "bgInput") m_theme.bgInput = c;
    else if (key == "textPrimary") {
        m_theme.textPrimary = c; m_theme.textSecondary = c.darker(120); m_theme.textMuted = c.darker(160);
        int lum = (c.red()*299 + c.green()*587 + c.blue()*114) / 1000;
        m_theme.isDark = lum > 128;
        m_theme.textOnAccent = m_theme.isDark ? QColor("#1a1a1a") : QColor("#ffffff");
    } else if (key == "accent") {
        m_theme.accent = c; m_theme.accentHover = c.lighter(120); m_theme.accentPressed = c.darker(120);
    } else if (key == "border") {
        m_theme.border = c; m_theme.borderStrong = c.lighter(130);
    }
    m_theme.name = tr("Personalizado");
    m_theme.family = "";
    m_presetCombo->blockSignals(true); m_presetCombo->setCurrentIndex(0); m_presetCombo->blockSignals(false);
    refreshSwatches(); updateTheme();
}

void ThemeEditorDialog::updateTheme() { emit themeChanged(m_theme); updatePreviewStyle(); }

void ThemeEditorDialog::updatePreviewStyle() {
    const AppTheme &t = m_theme;
    QString css = QString(
        "QWidget { background-color: %1; color: %2; } "
        "QLabel { background: transparent; color: %2; } "
        "QPushButton { background-color: %3; color: %2; border: 1px solid %4; border-radius: %5px; padding: 4px 10px; } "
        "QPushButton:hover { background-color: %6; border-color: %7; } "
        "QPushButton#previewAccent { background-color: %7; color: %8; border: none; } "
        "QPushButton#previewDanger { background-color: %9; color: white; border: none; } ")
        .arg(t.bgPanel.name()).arg(t.textPrimary.name()).arg(t.bgInput.name()).arg(t.border.name())
        .arg(t.radius).arg(t.bgHover.name()).arg(t.accent.name()).arg(t.textOnAccent.name()).arg(t.danger.name());
    m_previewBox->setStyleSheet(css);
    QString dlgCss = QString(
        "QDialog { background-color: %1; }"
        "QLabel { color: %2; background: transparent; }"
        "QComboBox { background-color: %3; color: %2; border: 1px solid %4; border-radius: %5px; padding: 4px 8px; }"
        "QComboBox:hover { border-color: %6; }"
        "QComboBox QAbstractItemView { background-color: %3; color: %2; selection-background-color: %6; outline: none; }"
        "QSlider::groove:horizontal { height: 4px; background: %4; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: %6; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: %6; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; }"
        "QPushButton { background-color: %3; color: %2; border: 1px solid %4; border-radius: %5px; padding: 6px 14px; }"
        "QPushButton:hover { background-color: %7; border-color: %6; }"
        "QScrollArea { background: transparent; border: none; }"
        "QListWidget { background-color: %3; color: %2; border: 1px solid %4; border-radius: %5px; }"
        "QListWidget::item:selected { background-color: %6; }"
        "QTabWidget::pane { border: 1px solid %4; border-radius: %5px; background-color: %1; }"
        "QTabBar::tab { background-color: %3; color: %2; padding: 6px 14px; border: 1px solid %4; border-top-left-radius: 5px; border-top-right-radius: 5px; margin-right: 2px; }"
        "QTabBar::tab:hover { background-color: %7; }"
        "QTabBar::tab:selected { background-color: %1; border-bottom: 2px solid %6; font-weight: 600; }")
        .arg(t.bgWindow.name()).arg(t.textPrimary.name()).arg(t.bgInput.name()).arg(t.border.name())
        .arg(t.radius).arg(t.accent.name()).arg(t.bgHover.name());
    setStyleSheet(dlgCss);
}

// ═════════════════════════════════════════════════════════════════════════
//  buildQssFromTheme — ahora con soporte de SKINS por componente
// ═════════════════════════════════════════════════════════════════════════
QString mainwind::buildQssFromTheme(const AppTheme &t, int alpha) {
    const QColor wsA(t.bgWindow.red(), t.bgWindow.green(), t.bgWindow.blue(), alpha);
    const QString workspaceBg = wsA.name(QColor::HexArgb);
    const QString R  = QString::number(t.radius);
    const QString BW = QString::number(t.borderWidth);
    QString qss = QString::fromUtf8(R"QSS(
QMainWindow { background-color: transparent; font-family: '%fontFamily%', 'Noto Sans', sans-serif; }
QWidget#centralRootWidget { background-color: transparent; }
QWidget#titleDragBar { background-color: %bgTitleBar%; border-top-left-radius: %radiusPx%px; border-top-right-radius: %radiusPx%px; }
QWidget#titleDragBar:hover { background-color: %bgTitleBarHover%; }
QLabel#titleBarLabel { color: %textPrimary%; }
QWidget#leftSidebarWidget { background-color: %bgSidebar%; border-right: %borderW%px solid %border%; }
QWidget#sysBarContainer { background-color: %bgSysBar%; border-bottom: %borderW%px solid %border%; }
QWidget#ribbonContainer { background-color: %bgRibbon%; border-bottom: %borderW%px solid %border%; }
QWidget#bottomBarContainer { background-color: %bgBottomBar%; border-bottom-left-radius: %radiusPx%px; border-bottom-right-radius: %radiusPx%px; border-top: %borderW%px solid %border%; color: %textPrimary%; }
QScrollArea#framesScrollArea, QScrollArea#layersScrollArea { background-color: %bgPreview%; border: %borderW%px solid %border%; border-radius: %radiusPx%px; }
QWidget#framesContainerWidget, QWidget#layersContainerWidget { background-color: %bgPreview%; }
QFrame#layerItemFrame { background-color: %bgInput%; border: %borderW%px solid %border%; border-radius: %radiusPx%px; }
QFrame#layerItemFrame:hover { background-color: %bgHover%; border: %borderW%px solid %accent%; }
QFrame#maskThumbFrame { background-color: %bgPanel%; border: %borderW%px solid %border%; border-radius: %radiusPx%px; }
QFrame#colorMaskThumbFrame { background-color: %bgPanel%; border: %borderW%px solid %accent%; border-radius: %radiusPx%px; }
QGroupBox { font-size: 11px; color: %textMuted%; border: none; border-right: %borderW%px solid %border%; margin-top: 0px; padding: 2px; padding-bottom: 14px; }
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: bottom center; padding: 2px; }
QMenu { background-color: %bgPanel%; color: %textPrimary%; border: %borderW%px solid %borderStrong%; border-radius: %radiusPx%px; }
QMenu::item:selected { background-color: %bgSelected%; border-radius: %radiusPx%px; }
QMenu::separator { height: 1px; background-color: %borderStrong%; margin: 4px 8px; }
QPushButton, QToolButton { background-color: %bgInput%; color: %textPrimary%; border: %borderW%px solid %borderStrong%; border-radius: %radiusPx%px; padding: 3px; font-size: 11px; }
QPushButton:hover, QToolButton:hover { background-color: %bgHover%; border: %borderW%px solid %accent%; }
QPushButton:checked, QToolButton:checked { background-color: %bgSelected%; border: %borderW%px solid %accent%; color: %textPrimary%; }
QPushButton#btnWindowCtrl { background-color: transparent; border: none; border-radius: %radiusPx%px; font-size: 12px; font-weight: bold; color: %textPrimary%; padding: 0px; }
QPushButton#btnWindowCtrl:hover { background-color: %bgHover%; }
QPushButton#btnWindowCtrlClose { background-color: transparent; border: none; border-radius: %radiusPx%px; font-size: 12px; font-weight: bold; color: %textPrimary%; padding: 0px; }
QPushButton#btnWindowCtrlClose:hover { background-color: %danger%; color: white; }
QPushButton#btnArchivoMenu, QPushButton#btnViewMenu { background-color: %accent%; color: %textOnAccent%; font-weight: bold; padding: 2px 14px; border: none; border-radius: %radiusPx%px; }
QPushButton#btnArchivoMenu:hover, QPushButton#btnViewMenu:hover { background-color: %accentHover%; }
QPushButton#btnQuickIcon { background-color: transparent; border: none; padding: 2px; max-width: 26px; max-height: 26px; border-radius: %radiusPx%px; }
QPushButton#btnQuickIcon:hover { background-color: %bgHover%; border: %borderW%px solid %border%; }
QPushButton#btnQuickAction { background-color: transparent; border: none; padding: 2px 8px; min-width: 60px; color: %textPrimary%; border-radius: %radiusPx%px; }
QPushButton#btnQuickAction:hover { background-color: %bgHover%; border: %borderW%px solid %accent%; }
QPushButton#btnQuickAction::menu-indicator { image: none; width: 0px; height: 0px; }
QPushButton#btnSysToggle { background-color: transparent; border: %borderW%px solid %accent%; color: %accentHover%; font-size: 10px; padding: 2px 8px; border-radius: %radiusPx%px; }
QToolButton#btnBigPaste { font-size: 11px; min-height: 64px; min-width: 52px; border-radius: %radiusPx%px; }
QToolButton#btnCustomLong { min-width: 44px; min-height: 90px; border: %borderW%px solid %borderStrong%; border-radius: %radiusPx%px; background-color: %bgInput%; }
QToolButton#btnCustomLong:hover { background-color: %bgHover%; border: %borderW%px solid %accent%; }
QToolButton#btnCustomLong:checked { background-color: %bgSelected%; border: %borderW%px solid %accent%; }
QToolButton#btnVectorTool { background-color: %bgInput%; color: %textPrimary%; border: %borderW%px solid %borderStrong%; border-radius: %radiusPx%px; padding: 4px 8px; font-size: 11px; text-align: left; }
QToolButton#btnVectorTool:hover { background-color: %bgHover%; border: %borderW%px solid %accent%; }
QToolButton#btnVectorTool:checked { background-color: %bgSelected%; border: %borderW%px solid %accent%; color: %textPrimary%; }
QPushButton#btnSmallStacked { font-size: 10px; padding: 2px 4px; min-width: 75px; text-align: left; border-radius: %radiusPx%px; }
QPushButton#btnImgAction { font-size: 10px; padding: 3px; text-align: left; min-width: 85px; border-radius: %radiusPx%px; }
QPushButton#btnToolGrid { font-size: 12px; min-width: 25px; min-height: 25px; padding: 0px; border-radius: %radiusPx%px; }
QFrame#dividerTools { background-color: %border%; min-width: 1px; max-width: 1px; border: none; margin: 4px 2px; }
QToolButton#btnBrushUnit { min-width: 28px; min-height: 28px; padding: 2px; font-size: 9px; border-radius: %radiusPx%px; }
QFrame#shapesGridFrame { background-color: %bgInput%; border: %borderW%px solid %borderStrong%; border-radius: %radiusPx%px; }
QPushButton#btnShapeUnit { background-color: transparent; border: none; min-width: 24px; min-height: 24px; border-radius: %radiusPx%px; padding: 2px; }
QPushButton#btnShapeUnit:hover { background-color: %bgHover%; }
QPushButton#btnShapeUnit:checked { background-color: %bgSelected%; border: %borderW%px solid %accent%; }
QComboBox { background-color: %bgInput%; color: %textPrimary%; border: %borderW%px solid %borderStrong%; border-radius: %radiusPx%px; padding: 4px 8px; font-size: 11px; min-height: 22px; }
QComboBox:hover { border: %borderW%px solid %accent%; background-color: %bgHover%; }
QComboBox::drop-down { border: none; width: 18px; }
QComboBox::down-arrow { image: none; }
QComboBox QAbstractItemView { background-color: %bgInput%; color: %textPrimary%; border: %borderW%px solid %borderStrong%; border-radius: %radiusPx%px; selection-background-color: %bgSelected%; outline: none; }
QComboBox#comboZoomBottom { font-size: 8px; min-height: 18px; padding: 1px 4px; }
QToolButton#btnEditColorsWide { font-size: 10px; font-weight: normal; min-height: 32px; border-radius: %radiusPx%px; padding: 4px 6px; }
QLabel { color: %textSecondary%; }
QLabel#lblMini { font-size: 10px; color: %textMuted%; }
QLabel#lblZoomIndicator { font-size: 10px; color: %textSecondary%; }
QLabel#lblUndoIndicator { font-size: 10px; color: %textSecondary%; }
QFrame#frameSelectorsColor { border: %borderW%px solid %border%; background-color: %bgInput%; border-radius: %radiusPx%px; }
QSlider::groove:horizontal { height: 4px; background: %border%; border-radius: 2px; }
QSlider::handle:horizontal { background: %accent%; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; }
QSlider::sub-page:horizontal { background: %accent%; border-radius: 2px; }
QCheckBox { color: %textPrimary%; }
QCheckBox::indicator { width: 14px; height: 14px; border: %borderW%px solid %borderStrong%; border-radius: 3px; background-color: %bgInput%; }
QCheckBox::indicator:checked { background-color: %accent%; border-color: %accent%; }
QPushButton#btnZoomAction { background-color: %bgInput%; color: %textPrimary%; border: %borderW%px solid %borderStrong%; border-radius: %radiusPx%px; padding: 0px; font-weight: bold; }
QPushButton#btnZoomAction:hover { background-color: %bgHover%; border: %borderW%px solid %accent%; }
QFrame#separadorVertical { background-color: %borderStrong%; border: none; margin: 8px 10px; min-width: 1px; max-width: 1px; }
QWidget#workAreaContainer { background-color: %workspaceBg%; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
QScrollBar::handle:vertical { background: %borderStrong%; border-radius: 5px; min-height: 24px; }
QScrollBar::handle:vertical:hover { background: %accent%; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }
QScrollBar::handle:horizontal { background: %borderStrong%; border-radius: 5px; min-width: 24px; }
QScrollBar::handle:horizontal:hover { background: %accent%; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
QToolTip { background-color: %bgPanel%; color: %textPrimary%; border: %borderW%px solid %borderStrong%; padding: 4px; font-size: 11px; }
QSpinBox, QDoubleSpinBox { background-color: %bgInput%; color: %textPrimary%; border: %borderW%px solid %borderStrong%; border-radius: %radiusPx%px; padding: 3px 6px; }
QSpinBox:focus, QDoubleSpinBox:focus { border-color: %accent%; }
QPushButton#btnColorTarget { border: 2px solid %borderStrong%; border-radius: %radiusPx%px; }
)QSS");

    auto rep = [&](const char *token, const QString &val) { qss.replace(QString::fromLatin1(token), val); };
    rep("%fontFamily%", t.fontFamily);
    rep("%radiusPx%",   R);
    rep("%borderW%",    BW);
    rep("%workspaceBg%", workspaceBg);
    rep("%bgWindow%",        t.bgWindow.name());
    rep("%bgTitleBar%",      t.bgTitleBar.name());
    rep("%bgTitleBarHover%", t.bgTitleBarHover.name());
    rep("%bgSysBar%",        t.bgSysBar.name());
    rep("%bgRibbon%",        t.bgRibbon.name());
    rep("%bgSidebar%",       t.bgSidebar.name());
    rep("%bgBottomBar%",     t.bgBottomBar.name());
    rep("%bgPanel%",         t.bgPanel.name());
    rep("%bgInput%",         t.bgInput.name());
    rep("%bgPreview%",       t.bgPreview.name());
    rep("%bgHover%",         t.bgHover.name());
    rep("%bgSelected%",      t.bgSelected.name());
    rep("%textPrimary%",     t.textPrimary.name());
    rep("%textSecondary%",   t.textSecondary.name());
    rep("%textMuted%",       t.textMuted.name());
    rep("%textOnAccent%",    t.textOnAccent.name());
    rep("%border%",          t.border.name());
    rep("%borderStrong%",    t.borderStrong.name());
    rep("%accent%",          t.accent.name());
    rep("%accentHover%",     t.accentHover.name());
    rep("%accentPressed%",   t.accentPressed.name());
    rep("%danger%",          t.danger.name());
    rep("%dangerHover%",     t.dangerHover.name());

    // ═══════════════════════════════════════════════════════════════════
    //  Inyectar SKINS por componente
    // ═══════════════════════════════════════════════════════════════════
    auto addSkin = [&](const QString &key, const QString &selector) {
        const ComponentSkin s = t.skins.value(key);
        if (!s.isActive()) return;
        qss += SkinHelper::qssFor(s, selector, t.isDark);
    };

    addSkin(SkinKey::WorkArea,  "QWidget#workAreaContainer");
    addSkin(SkinKey::Window,    "QWidget#centralRootWidget");
    addSkin(SkinKey::TitleBar,  "QWidget#titleDragBar");
    addSkin(SkinKey::SysBar,    "QWidget#sysBarContainer");
    addSkin(SkinKey::Ribbon,    "QWidget#ribbonContainer");
    addSkin(SkinKey::Sidebar,   "QWidget#leftSidebarWidget");
    addSkin(SkinKey::BottomBar, "QWidget#bottomBarContainer");
    addSkin(SkinKey::Panel,     "QMenu, QGroupBox, QToolTip, QFrame#maskThumbFrame, QFrame#colorMaskThumbFrame");
    addSkin(SkinKey::Input,     "QPushButton, QToolButton, QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit, QFrame#shapesGridFrame, QFrame#layerItemFrame, QFrame#frameSelectorsColor");
    addSkin(SkinKey::Preview,   "QScrollArea#framesScrollArea, QScrollArea#layersScrollArea");
    addSkin(SkinKey::Hover,     "QPushButton:hover, QToolButton:hover, QFrame#layerItemFrame:hover");
    addSkin(SkinKey::Accent,    "QPushButton#btnArchivoMenu, QPushButton#btnViewMenu, QPushButton:checked, QToolButton:checked");

    return qss;
}

// ═════════════════════════════════════════════════════════════════════════
//  applyTheme / loadThemeFromSettings / saveThemeToSettings / toggleTheme
// ═════════════════════════════════════════════════════════════════════════
void mainwind::applyTheme(const AppTheme &t, bool persist) {
    currentTheme = t;
    darkMode = t.isDark;
    if (btnLayerMask)  btnLayerMask->setIcon(crearIconoMascara(darkMode));
    if (btnDeformTool) btnDeformTool->setIcon(crearIconoDeformacion(darkMode));
    if (paintArea) {
        paintArea->setDarkMode(darkMode);
        paintArea->setAccentColor(t.accent);
    }
    refrescarAccentEnCapas();
    if (btnThemeToggle) btnThemeToggle->setText(t.isDark ? tr("Modo Claro") : tr("Modo Oscuro"));
    compilarHojasDeEstiloGlobales();
    actualizarEstilosDePrevisualizacion();
    if (persist) saveThemeToSettings();
    if (statusBar()) statusBar()->showMessage(tr("Tema: %1").arg(t.name), 1500);
}
void mainwind::refrescarAccentEnCapas() {
    for (DraggableLayerItem *item : draggableLayerItems)
        if (item) item->setAccentColor(currentTheme.accent);
}
void mainwind::saveThemeToSettings() {
    QSettings s("Paintux", "PaintuxStudio");
    s.setValue("theme/current", currentTheme.name);
    QJsonObject obj = QJsonObject::fromVariantMap(currentTheme.toVariantMap());
    s.setValue("theme/active", QJsonDocument(obj).toJson(QJsonDocument::Compact));
}
void mainwind::loadThemeFromSettings() {
    QSettings s("Paintux", "PaintuxStudio");
    const QString name = s.value("theme/current", "Oscuro").toString();
    for (const AppTheme &t : AppTheme::presets())
        if (t.name == name) { currentTheme = t; darkMode = t.isDark; return; }
    const QString json = s.value("theme/custom_" + name).toString();
    if (!json.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        currentTheme = AppTheme::fromVariantMap(doc.object().toVariantMap());
        darkMode = currentTheme.isDark;
        return;
    }
    currentTheme = AppTheme::oscuro();
    darkMode = true;
}
void mainwind::toggleTheme() {
    const QString family = currentTheme.family.isEmpty() ? "Clásico" : currentTheme.family;
    const bool nextDark = !currentTheme.isDark;
    AppTheme next = AppTheme::findFamily(family, nextDark);
    // Conservar skins al alternar claro/oscuro del mismo family
    next.skins = currentTheme.skins;
    applyTheme(next, true);
    actualizarPanelCapas();
    actualizarBotonSelectFree();
}
void mainwind::openThemeEditor() {
    ThemeEditorDialog dlg(currentTheme, this);
    connect(&dlg, &ThemeEditorDialog::themeChanged, this, [this](const AppTheme &t) {
        currentTheme = t; darkMode = t.isDark; compilarHojasDeEstiloGlobales();
    });
    if (dlg.exec() == QDialog::Accepted) applyTheme(dlg.currentTheme(), true);
    else { loadThemeFromSettings(); compilarHojasDeEstiloGlobales(); }
}

void mainwind::compilarHojasDeEstiloGlobales() {
    const QJsonObject themeObj = QJsonObject::fromVariantMap(currentTheme.toVariantMap());
    const QByteArray themeJson = QJsonDocument(themeObj).toJson(QJsonDocument::Compact);
    const QString sig = QString::number(qHash(themeJson)) + ":" + QString::number(currentWorkspaceAlpha);
    if (sig == m_lastThemeSignature && !m_lastAppliedQss.isEmpty()) return;
    m_lastThemeSignature = sig;
    const QString qss = buildQssFromTheme(currentTheme, currentWorkspaceAlpha);
    if (qss == m_lastAppliedQss) return;
    m_lastAppliedQss = qss;
    setStyleSheet(qss);
}
void mainwind::compilarHojasDeEstilosGlobales() { compilarHojasDeEstiloGlobales(); }