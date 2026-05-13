#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>

class StartPage;
class CharacterGridPage;
class CharacterDetailPage;
class OpponentGridPage;
class MapSelectionPage;
class ArenaDrawPage;
class GamePage;
class ModeSelectionPage;   // 新增
class BossSelectionPage;   // 新增
class CustomCharPage;      // 新增
class BossPage;            // 新增

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    // 模拟对战流程
    void goToCharacterGrid();
    void showCharacterDetail(int id);
    void goBackToGrid();
    void goToOpponentSelection(int playerId);
    void goToMapSelection(int opponentId);
    void goToArenaDraw(int mapType);
    void startGameWithArena(int mapType, QRect arenaRect);

    // Boss挑战流程
    void goToBossSelection();
    void goToCustomChar(int bossId);
    void startBossGame(const QPixmap &playerImg);

    // 通用
    void restartToStart();

private:
    QStackedWidget *m_stackedWidget;

    // 页面指针
    StartPage           *m_startPage;
    // 模拟对战相关
    CharacterGridPage   *m_gridPage;
    CharacterDetailPage *m_detailPage;
    OpponentGridPage    *m_opponentPage;
    MapSelectionPage    *m_mapPage;
    ArenaDrawPage       *m_arenaDrawPage;
    GamePage            *m_gamePage;
    // Boss挑战相关
    ModeSelectionPage   *m_modePage;      // 实际上开始页直接分两路，不需要模式选择页了，但保留可选
    BossSelectionPage   *m_bossSelPage;
    CustomCharPage      *m_customPage;
    BossPage            *m_bossPage;

    // 临时数据
    int m_playerCharId;
    int m_opponentCharId;
    int m_selectedBossId;
};

#endif // MAINWINDOW_H