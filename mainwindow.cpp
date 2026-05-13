#include "mainwindow.h"
#include "startpage.h"
#include "charactergridpage.h"
#include "characterdetailpage.h"
#include "opponentgridpage.h"
#include "mapselectionpage.h"
#include "arenadrawpage.h"
#include "gamepage.h"
#include "bossselectionpage.h"
#include "customcharpage.h"
#include "bosspage.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    m_playerCharId(-1), m_opponentCharId(-1), m_selectedBossId(-1)
{
    setWindowTitle("简易擂台");
    resize(800, 600);

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // 创建所有页面
    m_startPage = new StartPage(this);
    m_gridPage = new CharacterGridPage(this);
    m_detailPage = new CharacterDetailPage(this);
    m_opponentPage = new OpponentGridPage(this);
    m_mapPage = new MapSelectionPage(this);
    m_arenaDrawPage = new ArenaDrawPage(this);
    m_gamePage = new GamePage(this);
    m_bossSelPage = new BossSelectionPage(this);
    m_customPage = new CustomCharPage(this);
    m_bossPage = new BossPage(this);

    // 添加到stacked widget
    m_stackedWidget->addWidget(m_startPage);        // 0
    m_stackedWidget->addWidget(m_gridPage);         // 1
    m_stackedWidget->addWidget(m_detailPage);       // 2
    m_stackedWidget->addWidget(m_opponentPage);     // 3
    m_stackedWidget->addWidget(m_mapPage);          // 4
    m_stackedWidget->addWidget(m_arenaDrawPage);    // 5
    m_stackedWidget->addWidget(m_gamePage);         // 6
    m_stackedWidget->addWidget(m_bossSelPage);      // 7
    m_stackedWidget->addWidget(m_customPage);       // 8
    m_stackedWidget->addWidget(m_bossPage);         // 9

    // ========== 模拟对战流程信号 ==========
    connect(m_startPage, &StartPage::simulationClicked, this, &MainWindow::goToCharacterGrid);

    connect(m_gridPage, &CharacterGridPage::characterClicked, this, &MainWindow::showCharacterDetail);
    connect(m_gridPage, &CharacterGridPage::backToStart, this, [this]() {
        m_stackedWidget->setCurrentIndex(0);
    });

    connect(m_detailPage, &CharacterDetailPage::characterSelected, this, &MainWindow::goToOpponentSelection);
    connect(m_detailPage, &CharacterDetailPage::backClicked, this, &MainWindow::goBackToGrid);

    connect(m_opponentPage, &OpponentGridPage::opponentClicked, this, &MainWindow::goToMapSelection);
    connect(m_opponentPage, &OpponentGridPage::backClicked, this, [this]() {
        m_stackedWidget->setCurrentIndex(2);
    });

    connect(m_mapPage, &MapSelectionPage::mapTypeSelected, this, &MainWindow::goToArenaDraw);
    connect(m_mapPage, &MapSelectionPage::backClicked, this, [this]() {
        m_stackedWidget->setCurrentIndex(3);
    });

    connect(m_arenaDrawPage, &ArenaDrawPage::arenaConfirmed, this, &MainWindow::startGameWithArena);
    connect(m_arenaDrawPage, &ArenaDrawPage::backClicked, this, [this]() {
        m_stackedWidget->setCurrentIndex(4);
    });

    connect(m_gamePage, &GamePage::windowSizeNeeded, this, [this](QSize size) {
        resize(size.width() + 20, size.height() + 40);
    });
    connect(m_gamePage, &GamePage::restartRequested, this, &MainWindow::restartToStart);

    // ========== Boss挑战流程信号 ==========
    connect(m_startPage, &StartPage::bossClicked, this, &MainWindow::goToBossSelection);

    connect(m_bossSelPage, &BossSelectionPage::bossSelected, this, &MainWindow::goToCustomChar);
    connect(m_bossSelPage, &BossSelectionPage::backClicked, this, [this]() {
        m_stackedWidget->setCurrentIndex(0);
    });

    connect(m_customPage, &CustomCharPage::characterCustomized, this, &MainWindow::startBossGame);
    connect(m_customPage, &CustomCharPage::backClicked, this, [this]() {
        m_stackedWidget->setCurrentIndex(7); // 返回Boss选择页
    });

    connect(m_bossPage, &BossPage::backToMenu, this, &MainWindow::restartToStart);

    m_stackedWidget->setCurrentIndex(0);
}

// 模拟对战槽函数（与之前相同）
void MainWindow::goToCharacterGrid() { m_stackedWidget->setCurrentIndex(1); }
void MainWindow::showCharacterDetail(int id) {
    if (id == 2) return;
    m_detailPage->setCharacterId(id);
    m_stackedWidget->setCurrentIndex(2);
}
void MainWindow::goBackToGrid() { m_stackedWidget->setCurrentIndex(1); }
void MainWindow::goToOpponentSelection(int playerId) {
    m_playerCharId = playerId;
    m_stackedWidget->setCurrentIndex(3);
}
void MainWindow::goToMapSelection(int opponentId) {
    m_opponentCharId = opponentId;
    m_stackedWidget->setCurrentIndex(4);
}
void MainWindow::goToArenaDraw(int mapType) {
    m_arenaDrawPage->setMapType(mapType);
    m_stackedWidget->setCurrentIndex(5);
}
void MainWindow::startGameWithArena(int mapType, QRect arenaRect) {
    m_gamePage->setCharacters(m_playerCharId, m_opponentCharId);
    m_gamePage->setMap(mapType, arenaRect);
    m_stackedWidget->setCurrentIndex(6);
    m_gamePage->startBattle();
}

// Boss挑战槽函数
void MainWindow::goToBossSelection() {
    m_stackedWidget->setCurrentIndex(7);
}
void MainWindow::goToCustomChar(int bossId) {
    m_selectedBossId = bossId;
    m_stackedWidget->setCurrentIndex(8);
}
void MainWindow::startBossGame(const QPixmap &playerImg) {
    m_bossPage->setBossId(m_selectedBossId);
    if (!playerImg.isNull()) {
        m_bossPage->setPlayerImage(playerImg);
    }
    m_stackedWidget->setCurrentIndex(9);
    m_bossPage->startBattle();
}

void MainWindow::restartToStart() {
    m_stackedWidget->setCurrentIndex(0);
}